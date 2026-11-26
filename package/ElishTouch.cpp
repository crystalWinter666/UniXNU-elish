// SPDX-License-Identifier: BSD-2-Clause-Patent
// elish K81 NT36523 on Kona QUP0 SE4. No panel power/reset, GPI DMA, or CPU IRQ.
#include "ElishTouchClock.h"
#include "ElishTouchFrame.h"
#include <IOKit/IOLib.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <kern/debug.h>
#include <kern/thread.h>
#include <libkern/c++/OSSharedPtr.h>
#include <pexpert/pexpert.h>

extern "C" {
#include "GeniSpi.h"
}

void elish_touch_log();
#if ELISH_TOUCH_HID
bool elish_touch_hid_start();
bool elish_touch_hid_submit(const ElishTouchFrame &);
void elish_touch_hid_log();
void elish_touch_hid_snapshot(char *, size_t);
#endif
#if ELISH_TOUCH_FW
#include "ElishTouchFirmware.inc"
#endif

namespace {
struct Mmio {
    OSSharedPtr<IOMemoryMap> mapping;
    uintptr_t base = 0;
    bool map(uint64_t pa, size_t length)
    {
        auto memory = IOMemoryDescriptor::withPhysicalAddress(pa, length, kIODirectionInOut);
        if (!memory) return false;
        mapping = OSSharedPtr<IOMemoryMap>(memory->map(kIOMapAnywhere | kIOMapInhibitCache), OSNoRetain);
        memory->release();
        if (mapping) base = mapping->getVirtualAddress();
        return base != 0;
    }
    uint32_t read(unsigned offset) const { return *reinterpret_cast<volatile uint32_t *>(base + offset); }
    void write(unsigned offset, uint32_t value)
    {
        *reinterpret_cast<volatile uint32_t *>(base + offset) = value;
        asm volatile("dsb sy" ::: "memory");
    }
};

uint32_t ioRead(void *ctx, uint32_t reg) { return static_cast<Mmio *>(ctx)->read(reg); }
void ioWrite(void *ctx, uint32_t reg, uint32_t value) { static_cast<Mmio *>(ctx)->write(reg, value); }
void delay(void *, unsigned us) { IODelay(us); }

// GCC Kona offsets from MiCode gcc-kona.c. The vote register is shared; add
// only this controller's clock votes and never reset the whole wrapper.
constexpr unsigned kVote = 0x8;
constexpr unsigned kMasterAhb = 0x004, kSlaveAhb = 0x008;
constexpr unsigned kSe4Branch = 0x4cc, kRcg = 0x4d0;
constexpr uint32_t kXoDivMask = 0x371f; // mode, source selector, half-integer divider
constexpr unsigned kXo = 19200000;
constexpr unsigned kEvent = 0x2fe00;
// Kona TLMM GPIO39: GPIO input, rising edge latched in raw status.
constexpr unsigned kIrqControl = 0, kIrqConfig = 8, kIrqStatus = 0xc;
constexpr unsigned kIrqGpioInputMask = (0xfu << 2) | (1u << 9);
constexpr unsigned kIrqConfigMask = 0x1f, kIrqRawRising = 0x16;

unsigned stage, reads, bad, down, moves, lifts, hidReports, transferErrors;
unsigned clkVotes, clkMaster, clkSlave, clkEngine;
unsigned seFirmware, seInterface, seStatus, seFifo;
unsigned irqEdges, badHead, badTail, badKind;
unsigned started;
void setStage(unsigned value) { __atomic_store_n(&stage, value, __ATOMIC_RELAXED); }
void store(unsigned &variable, unsigned value) { __atomic_store_n(&variable, value, __ATOMIC_RELAXED); }
void increment(unsigned &value) { __atomic_add_fetch(&value, 1, __ATOMIC_RELAXED); }
unsigned get(const unsigned &value) { return __atomic_load_n(&value, __ATOMIC_RELAXED); }

struct Controller {
    Mmio se, wrapper, clocks, vote, pins, irq;
    geni_spi spi{};
    uint32_t oldPins[4]{};
    uint32_t oldRcg = 0;
    bool pinsChanged = false, rcgChanged = false;

    void restorePins()
    {
        if (pinsChanged) for (unsigned i = 0; i < 4; ++i) pins.write(i * 0x1000, oldPins[i]);
    }

    bool enableClocks()
    {
        vote.write(kVote, vote.read(kVote) | kElishTouchClockVotes);
        uint32_t votes, master, slave, engine;
        bool ready;
        for (unsigned i = 0;; ++i) {
            votes = vote.read(kVote);
            master = clocks.read(kMasterAhb);
            slave = clocks.read(kSlaveAhb);
            engine = clocks.read(kSe4Branch);
            ready = elish_touch_clocks_ready(votes, master, slave, engine);
            if (ready || i == 999) break;
            IODelay(1);
        }
        store(clkVotes, votes);
        store(clkMaster, master);
        store(clkSlave, slave);
        store(clkEngine, engine);
        return ready;
    }

    bool configureClock(unsigned &level)
    {
        const auto dfs = clocks.read(kRcg + 0x14);
        const auto cfg = clocks.read(kRcg + 4);
        IOLog("ELISH_TOUCH: S4 cfg=%x dfs=%x\n", cfg, dfs);
        if (dfs & 1) {
            for (level = 0; level < 8; ++level)
                if ((clocks.read(kRcg + 0x1c + 4 * level) & kXoDivMask) == 1) return true;
            return false; // no verified 19.2 MHz DFS level; don't guess a parent/rate
        }
        level = 0;
        if ((cfg & kXoDivMask) == 1) return true;
        if (clocks.read(kRcg) & 1) return false;
        oldRcg = cfg;
        rcgChanged = true;
        clocks.write(kRcg + 4, (cfg & ~kXoDivMask) | 1);
        clocks.write(kRcg, clocks.read(kRcg) | 1);
        for (unsigned i = 0; i < 1000; ++i) {
            if (!(clocks.read(kRcg) & 1)) return (clocks.read(kRcg + 4) & kXoDivMask) == 1;
            IODelay(1);
        }
        return false;
    }

    bool initialize()
    {
        if (!clocks.map(0x117000, 0x1000) || !vote.map(0x152000, 0x1000) ||
            !se.map(0x990000, 0x4000) || !wrapper.map(0x9c0000, 0x2000) ||
            !pins.map(0x0f908000, 0x4000) || !irq.map(0x0f527000, 0x1000)) return false;
        setStage(2);
        if (!enableClocks()) return false;
        setStage(3);
        spi.se = {&se, ioRead, ioWrite, delay};
        spi.wrapper = {&wrapper, ioRead, nullptr, nullptr};
        spi.poll_limit = 2000;
        spi.poll_only = 1;
        if (geni_spi_read_info(&spi)) return false;
        store(seFirmware, spi.info.fw_revision);
        store(seInterface, spi.info.if_disable);
        store(seStatus, spi.info.geni_status);
        store(seFifo, spi.info.hw_param0);
        IOLog("ELISH_TOUCH: S4 fw=%x protocol=%u if=%x status=%x fifo=%ux%u wrapper=%x irq39=%x/%x\n",
              spi.info.fw_revision, spi.info.protocol, spi.info.if_disable, spi.info.geni_status,
              spi.info.tx_fifo_depth, spi.info.fifo_width_bits, spi.info.qup_hw_version, irq.read(0), irq.read(4));
        if (spi.info.protocol != GENI_PROTO_SPI || spi.info.fifo_disabled ||
            spi.info.tx_fifo_depth < 4 || spi.info.fifo_width_bits != 32 ||
            (spi.info.geni_status & GENI_STATUS_M_ACTIVE)) return false;
        unsigned level;
        if (!configureClock(level)) return false;
        // TLMM GPIO8-11: north tile, qup4 mux (1), 6 mA (2), no bias or GPIO OE.
        pinsChanged = true;
        for (unsigned i = 0; i < 4; ++i) {
            const unsigned offset = i * 0x1000;
            oldPins[i] = pins.read(offset);
            pins.write(offset, (oldPins[i] & ~0x3ffu) | 0x84);
        }
        if (geni_spi_init(&spi) || geni_spi_setup(&spi, 0, 0) || geni_spi_set_clock(&spi, kXo, kXo, level)) return false;
        IOLog("ELISH_TOUCH: S4 PIO ready mode=0 cs=0 XO level=%u oversampling=%u\n", level, spi.oversampling);
        setStage(4);
        return true;
    }

    bool transfer(const uint8_t *tx, uint8_t *rx, size_t bytes)
    {
        const int result = geni_spi_transfer(&spi, tx, rx, bytes, 0);
        if (!result) return true;
        increment(transferErrors);
        if (get(transferErrors) <= 4)
            IOLog("ELISH_TOUCH: SPI error=%d bytes=%zu irq=%x status=%x tx=%x rx=%x\n", result, bytes,
                  spi.snap_irq, spi.snap_status, spi.snap_txfs, spi.snap_rxfs);
        return false;
    }

    bool page(unsigned address)
    {
        const uint8_t tx[] = {0xff, static_cast<uint8_t>(address >> 15), static_cast<uint8_t>(address >> 7)};
        return transfer(tx, nullptr, sizeof(tx));
    }

    bool readPage(unsigned offset, uint8_t *out, size_t bytes)
    {
        uint8_t tx[kElishTouchEventBytes + 2]{}, rx[sizeof(tx)];
        if (bytes > sizeof(tx) - 2) return false;
        tx[0] = offset & 0x7f;
        if (!transfer(tx, rx, bytes + 2)) return false;
        memcpy(out, rx + 2, bytes);
        return true;
    }

    bool read(unsigned address, uint8_t *out, size_t bytes)
    {
        return page(address) && readPage(address, out, bytes);
    }

    bool writeBlock(unsigned address, const uint8_t *data, size_t bytes)
    {
        uint8_t tx[513];
        if (!bytes || bytes >= sizeof(tx)) return false;
        tx[0] = static_cast<uint8_t>(0x80 | (address & 0x7f));
        memcpy(tx + 1, data, bytes);
        return page(address) && transfer(tx, nullptr, bytes + 1);
    }

    bool write(unsigned address, uint8_t value) { return writeBlock(address, &value, 1); }

    bool bootloaderReset()
    {
        if (!write(0x3f0fe, 0x69)) return false;
        IOSleep(5);
        return write(0x3f310, 0);
    }

#if ELISH_TOUCH_FW
    bool writeLe(unsigned address, uint32_t value, unsigned bytes)
    {
        const uint8_t data[] = {static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8),
                                static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 24)};
        return writeBlock(address, data, bytes);
    }

    bool crcBank(unsigned dest, unsigned length, unsigned crc, const TouchPartition &part)
    {
        return writeLe(dest, part.sram, 3) && writeLe(length, part.last, 3) && writeLe(crc, part.crc, 4);
    }

    bool download()
    {
        if (!bootloaderReset() || !crcBank(0x3f128, 0x3f118, 0x3f100, kTouchPartitions[0]) ||
            !crcBank(0x3f12c, 0x3f130, 0x3f104, kTouchPartitions[1]) || !write(0x3f7e8, 0x69)) return false;
        for (const auto &part : kTouchPartitions) {
            for (unsigned done = 0; done <= part.last;) {
                const unsigned remaining = part.last + 1 - done;
                const unsigned chunk = remaining < 512 ? remaining : 512;
                if (!writeBlock(part.sram + done, elish_touch_fw01 + part.offset + done, chunk)) return false;
                done += chunk;
            }
        }
        uint8_t copy = 0xff, crc = 0;
        for (unsigned i = 0; i < 200; ++i) {
            if (!read(0x3f7f1, &copy, 1)) return false;
            if (copy == 0) break;
            IOSleep(1);
        }
        IOLog("ELISH_TOUCH: cascade SRAM upload complete copy=%x\n", copy);
        if (copy || !read(0x3f30e, &crc, 1) || !write(0x3f30e, crc | 0x80) ||
            !write(kEvent + 0x60, 0) || !write(kEvent + 0x50, 0xae) || !write(0x3f10d, 1)) return false;
        const unsigned resetTimeoutMs = 300;
        for (unsigned t = 0; t < resetTimeoutMs; t += 10) {
            IOSleep(10);
            uint8_t state = 0;
            if (!read(kEvent + 0x60, &state, 1)) return false;
            if (state >= 0xa0 && state <= 0xaf && firmwareRunning()) {
                uint8_t flags = 0;
                const bool ok = read(0x3f133, &flags, 1);
                IOLog("ELISH_TOUCH: fw boot complete CRC flags=%x valid=%d\n", flags, ok);
                return true;
            }
        }
        uint8_t flags = 0;
        const bool ok = read(0x3f133, &flags, 1);
        IOLog("ELISH_TOUCH: fw boot timeout CRC flags=%x valid=%d\n", flags, ok);
        return false;
    }
#endif

    bool firmwareRunning()
    {
        uint8_t state[1]{}, info[8]{}, pid[2]{};
        if (!read(kEvent + 0x60, state, sizeof(state)) || !read(kEvent + 0x78, info, sizeof(info)) ||
            !read(kEvent + 0x9a, pid, sizeof(pid))) return false;
        const unsigned product = (pid[1] << 8) | pid[0];
        const unsigned x = (info[4] << 8) | info[5], y = (info[6] << 8) | info[7];
        IOLog("ELISH_TOUCH: firmware state=%x PID=%x version=%x/%x matrix=%ux%u bounds=%ux%u\n",
              state[0], product, info[0], info[1], info[2], info[3], x, y);
        return state[0] >= 0xa0 && state[0] <= 0xaf && product == 0x5961 &&
               info[0] == 0x1a && info[0] + info[1] == 0xff && x == kElishTouchWidth && y == kElishTouchHeight;
    }

    bool identify()
    {
        // Try an undisturbed trim read first. If necessary reset the controller
        // into its bootloader, not the panel. This can lose resident SRAM code.
        const unsigned trims[] = {0x3f004, 0x1f64e};
        for (unsigned attempt = 0; attempt < 2; ++attempt) {
            if (attempt) {
                if (!write(0x7fff80, 0x5a)) return false;
                IOSleep(11);
                if (!bootloaderReset()) return false;
            }
            for (const auto addr : trims) {
                uint8_t id[6]{};
                const bool ok = read(addr, id, sizeof(id));
                IOLog("ELISH_TOUCH: trim addr=%x reset=%u ok=%d %02x%02x%02x%02x%02x%02x\n",
                      addr, attempt, ok, id[0], id[1], id[2], id[3], id[4], id[5]);
                if (ok && id[3] == 0x23 && id[4] == 0x65 && id[5] == 0x03) return true;
            }
        }
        return false;
    }

    void scan()
    {
#if ELISH_TOUCH_HID
        const bool hidReady = elish_touch_hid_start();
        IOLog("ELISH_TOUCH: HID publication=%d\n", hidReady);
#endif
        if (!page(kEvent)) return;
        irq.write(kIrqControl, irq.read(kIrqControl) & ~kIrqGpioInputMask);
        irq.write(kIrqConfig, (irq.read(kIrqConfig) & ~kIrqConfigMask) | kIrqRawRising);
        irq.write(kIrqStatus, 0);
        setStage(7);
        ElishTouchFrame last{};
        unsigned failureRun = 0;
        for (;;) {
            if (!(irq.read(kIrqStatus) & 1)) { IOSleep(2); continue; }
            irq.write(kIrqStatus, 0);
            increment(irqEdges);

            uint8_t data[kElishTouchEventBytes];
            ElishTouchFrame frame{};
            bool valid = false;
            if (readPage(kEvent, data, sizeof(data))) {
                increment(reads);
                const auto result = elish_decode_touch(data, frame);
                valid = result == ElishTouchPacketResult::Valid;
                if (!valid) {
                    if (get(bad) == 0) {
                        auto pack = [](const uint8_t *p) {
                            return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
                        };
                        store(badHead, pack(data));
                        store(badTail, pack(data + kElishTouchFingerBytes - 4));
                        store(badKind, static_cast<unsigned>(result));
                    }
                    increment(bad);
                }
            }
            if (!valid) {
                if (failureRun < 40 && ++failureRun == 40) {
#if ELISH_TOUCH_HID
                    if (hidReady) elish_touch_hid_submit(ElishTouchFrame{});
#endif
                    last = {};
                }
                IOSleep(2);
                continue;
            }
            failureRun = 0;
            bool changed = false, active = false;
            for (unsigned i = 0; i < 10; ++i) {
                const auto &old = last.points[i], &point = frame.points[i];
                active |= point.active;
                if (!old.active && point.active) { increment(down); changed = true; }
                if (old.active && !point.active) { increment(lifts); changed = true; }
                if (old.active && point.active && (point.x != old.x || point.y != old.y)) {
                    increment(moves); changed = true;
                }
                if (old.active != point.active && get(down) + get(lifts) <= 30)
                    IOLog("ELISH_TOUCH: id=%u %s x=%u y=%u\n", i + 1, point.active ? "down" : "up",
                          point.active ? point.x : old.x, point.active ? point.y : old.y);
            }
            last = frame;
#if ELISH_TOUCH_HID
            // Submit each fresh active frame while the matching driver attaches.
            if (hidReady && (changed || active) && elish_touch_hid_submit(frame)) increment(hidReports);
#else
            (void)changed;
            (void)active;
#endif
        }
    }
};

void touchThread(void *, wait_result_t)
{
    IOSleep(500);
    Controller touch;
    if (touch.initialize()) {
        bool ready = touch.firmwareRunning();
        if (!ready && touch.identify()) {
            setStage(5);
#if ELISH_TOUCH_FW
            for (unsigned attempt = 1; attempt <= 2 && !ready; ++attempt) {
                ready = touch.download();
                IOLog("ELISH_TOUCH: fw01 download attempt=%u ready=%d\n", attempt, ready);
            }
#else
            IOLog("ELISH_TOUCH: NT36523 identified; no verified resident fw01 or configured download\n");
#endif
        }
        if (ready) {
            setStage(6);
            touch.scan();
        }
    }
    touch.restorePins();
    if (touch.rcgChanged && !(touch.clocks.read(kRcg) & 1)) {
        touch.clocks.write(kRcg + 4, touch.oldRcg);
        touch.clocks.write(kRcg, touch.clocks.read(kRcg) | 1);
    }
    elish_touch_log();
    thread_terminate(current_thread());
}
} // namespace

void elish_touch_start()
{
    unsigned enabled = 0;
    if (!PE_parse_boot_argn("elish-touch", &enabled, sizeof(enabled)) || enabled != 1 ||
        __atomic_exchange_n(&started, 1, __ATOMIC_RELAXED)) return;
    setStage(1);
    thread_t thread;
    if (kernel_thread_start(touchThread, nullptr, &thread) == KERN_SUCCESS) thread_deallocate(thread);
    else IOLog("ELISH_TOUCH: could not start probe thread\n");
}

void elish_touch_log()
{
    const unsigned currentStage = get(stage);
    if (currentStage >= 2)
        IOLog("ELISH_TOUCH: clk V=%08x M=%08x S=%08x SE=%08x\n",
              get(clkVotes), get(clkMaster), get(clkSlave), get(clkEngine));
    if (currentStage) IOLog("ELISH_TOUCH: stage=%u reads=%u SPI-errors=%u bad=%u down=%u move=%u up=%u HID=%u\n",
                            currentStage, get(reads), get(transferErrors), get(bad), get(down), get(moves), get(lifts), get(hidReports));
    if (currentStage >= 7)
        IOLog("ELISH_TOUCH: IRQ edges=%u first bad=%u %08x ... %08x\n", get(irqEdges), get(badKind), get(badHead), get(badTail));
#if ELISH_TOUCH_HID
    if (currentStage >= 7) elish_touch_hid_log();
#endif
}

// The RAM-only deadline can expose this snapshot directly on the diagnostic
// panic screen, even when a reader cannot recover the warm-reset console ring.
void elish_touch_deadline_snapshot()
{
    unsigned enabled = 0, snapshot = 0;
    if (!PE_parse_boot_argn("elish-touch", &enabled, sizeof(enabled)) || enabled != 1 ||
        !PE_parse_boot_argn("elish-touch-snapshot", &snapshot, sizeof(snapshot)) || snapshot != 1) return;

    char hid[192] = "HID bridge not linked";
#if ELISH_TOUCH_HID
    elish_touch_hid_snapshot(hid, sizeof(hid));
#endif
    panic("\n\n\n\n\n\n\n\n"
          "ELISH TOUCH SNAPSHOT\n"
          "Clock vote=%08x SE4=%08x\n"
          "AHB master=%08x slave=%08x\n"
          "Stage=%u SE fw=%08x if=%08x\n"
          "SE status=%08x FIFO param=%08x\n"
          "Reads=%u SPI errors=%u Bad=%u\n"
          "IRQ=%u First bad=%u %08x ... %08x\n"
          "Down=%u Move=%u Up=%u HID=%u\n%s",
          get(clkVotes), get(clkEngine), get(clkMaster), get(clkSlave),
          get(stage), get(seFirmware), get(seInterface), get(seStatus), get(seFifo),
          get(reads), get(transferErrors), get(bad), get(irqEdges), get(badKind), get(badHead), get(badTail),
          get(down), get(moves), get(lifts), get(hidReports), hid);
}
