// SPDX-License-Identifier: BSD-2-Clause-Patent
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOService.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <kern/clock.h>
#include <kern/task.h>
#include <kern/thread_call.h>
#include <libkern/c++/OSIterator.h>
#include <libkern/crypto/sha2.h>
#include <mach/mach_time.h>
#include <arm/machine_routines.h>
#include <pexpert/boot.h>
#include <pexpert/pexpert.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/param.h>
#include <sys/vnode.h>

#if ELISH_VERIFIED_ROOT
#include "elish-root-digests.h"
#endif

extern "C" struct msgbuf *msgbufp;
extern "C" void elish_snapshot_processes(int sample[10], char *springboard, char *chronod, size_t stackSize);
extern "C" void elish_format_user_breakpoint(char *out, size_t size);
extern "C" void elish_key_store_start();
void elish_touch_start();
void elish_touch_log();
void elish_touch_deadline_snapshot();
extern "C" void elish_key_store_snapshot(char *out, size_t size);
#if ELISH_LEGACY_DISPLAY
extern "C" void elish_snapshot_display(unsigned values[8]);
#endif

namespace {
// elish stock DT and run40. Firmware owns link, PHY, clocks, power and SMMU.
constexpr uint32_t kHostBase = 0x01d84000;
constexpr uint32_t kGccUfsBase = 0x00177000;
constexpr uint32_t kSmmuBase = 0x15000000;
constexpr uint32_t kBlockBytes = 4096;
constexpr uint32_t kLastLba = 61880319;
constexpr uint32_t kUserdataLba = 6619136;
constexpr uint32_t kUserdataLast = 61870314;
constexpr uint32_t kPublishedBlockBytes = 512;
constexpr uint32_t kMaxRead = 5;
constexpr uint32_t kDmaBytes = (2 + kMaxRead) * kBlockBytes;
constexpr uint32_t kUcd = kBlockBytes;
constexpr uint32_t kData = 2 * kBlockBytes;
constexpr uint32_t kScsiResponse = 512;
constexpr uint32_t kPrdt = 1024;
// UTP command/response and NOP UPIU header byte offsets.
constexpr unsigned kUpiuType = 0;
constexpr unsigned kUpiuFlags = 1;
constexpr unsigned kUpiuLun = 2;
constexpr unsigned kUpiuTag = 3;
constexpr unsigned kUpiuResponse = 6;
constexpr unsigned kUpiuStatus = 7;
constexpr uint32_t kCacheLine = 64;
constexpr uint32_t kRunStop = 0x60;
constexpr uint32_t kDoorbell = 0x58;
constexpr uint32_t kInterruptStatus = 0x20;
constexpr uint32_t kContext2 = 0x82000;
constexpr uint32_t kContext3 = 0x83000;
constexpr uint32_t kSmmuControl = 0x00230406;
// Run36 SEC and run44 XNU differ from run40 by MemAttr[2] only.
// IHI 0062D.b §9.6.3: MTCFG (bit 20) is clear in both states.
constexpr uint32_t kSmmuControlVariable = 1U << 18;

// Native 4 KiB LBA and CRC32 from the independent Android and run42 reads.
constexpr struct {
    uint32_t lba;
    uint32_t crc;
} kKnownReads[] = {
    {1,       0xfa250023},
    {kUserdataLba, 0xa37c5576},
    {6619392, 0xe26a269b},
    {6619396, 0xdf21f342},
    {8103176, 0xc71c0011},
    {8103424, 0xc71c0011},
    {8909823, 0xc71c0011},
};

// Exact SHNGROOT v1 layout from the 21H461 build in the porting record.
// A whole-block manifest digest pins the sizes, offsets and window hashes.
struct RootWindow {
    char label[9];
    uint32_t firstLba;
    uint64_t bytes;
};
constexpr RootWindow kSystemOS = {"SystemOS", kUserdataLba + 1484288, 3303014400ULL};
constexpr RootWindow kOS = {"OS", kUserdataLba + 256, 6077580800ULL};
constexpr char kSystemUuid[] = "04769D26-2CA0-4825-B443-E2E5F88300F2";
constexpr char kSystemOSUuid[] = "8846FE78-2910-4297-AF97-D59BB6F8FC16";
#if ELISH_VERIFIED_ROOT
static_assert(kVerifiedChunk % kBlockBytes == 0 && kVerifiedChunk > kMaxRead * kBlockBytes);
constexpr uint64_t digestTableBytes(const RootWindow &window)
{
    return ((window.bytes + kVerifiedChunk - 1) / kVerifiedChunk) * SHA256_DIGEST_LENGTH;
}
constexpr size_t kOsDigestsOffset = 64;
constexpr size_t kSystemOSDigestsOffset = kOsDigestsOffset + digestTableBytes(kOS);
constexpr size_t kRootDigestsBytes = kSystemOSDigestsOffset + digestTableBytes(kSystemOS);

const uint8_t *loadRootDigests()
{
    auto map = OSSharedPtr<IORegistryEntry>(IORegistryEntry::fromPath("/chosen/memory-map", gIODTPlane), OSNoRetain);
    if (!map) return nullptr;
    const auto *range = OSDynamicCast(OSData, map->getProperty("RootPolicies"));
    uint64_t extent[2];
    if (!range || range->getLength() != sizeof(extent)) return nullptr;
    memcpy(extent, range->getBytesNoCopy(), sizeof(extent));
    const auto *args = static_cast<const boot_args *>(PE_state.bootArgs);
    const uint64_t pa = extent[0];
    if (!args || extent[1] != kRootDigestsBytes || (pa & (kBlockBytes - 1)) || pa < args->physBase ||
        pa >= args->topOfKernelData || kRootDigestsBytes > args->topOfKernelData - pa) return nullptr;
    const auto *bundle = reinterpret_cast<const uint8_t *>(ml_static_ptovirt(pa));
    if (!bundle) return nullptr;
    SHA256_CTX context;
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256_Init(&context);
    SHA256_Update(&context, bundle, kRootDigestsBytes);
    SHA256_Final(digest, &context);
    return memcmp(digest, kRootDigestsSha256, sizeof(digest)) == 0 ? bundle : nullptr;
}
#endif
constexpr uint32_t lastLba(RootWindow window)
{
    return window.firstLba + (window.bytes - 1) / kBlockBytes;
}
static_assert(lastLba(kOS) < kSystemOS.firstLba && lastLba(kSystemOS) == 8909823 &&
              lastLba(kSystemOS) <= kUserdataLast);
// UniXNU's userdata layout reserves the last GiB for logs.
constexpr uint32_t kDataLast = kUserdataLast - (1U << 30) / kBlockBytes;
constexpr uint32_t dataFirstLba(uint32_t blocks)
{
    return blocks && blocks <= kDataLast - lastLba(kSystemOS) ? kDataLast + 1 - blocks : 0;
}
static_assert(dataFirstLba(786457) == 60821714 && kDataLast == 61608170);
static_assert(dataFirstLba(0) == 0 && dataFirstLba(kDataLast - lastLba(kSystemOS) + 1) == 0);
constexpr uint8_t kManifestSha256[] = {
    0xbc, 0x90, 0x8a, 0x6c, 0xf6, 0x64, 0x91, 0x05, 0x50, 0x77, 0x79, 0x85, 0x50, 0x66, 0x0f, 0x40,
    0x12, 0x8b, 0xfe, 0x6d, 0xce, 0x2f, 0xdc, 0x0d, 0xb5, 0x6e, 0x6a, 0xf9, 0x65, 0x11, 0x20, 0x8d,
};

uint32_t read32(uintptr_t base, unsigned offset)
{
    return *reinterpret_cast<volatile uint32_t *>(base + offset);
}

uint64_t read64(uintptr_t base, unsigned offset)
{
    return *reinterpret_cast<volatile uint64_t *>(base + offset);
}

void write32(uintptr_t base, unsigned offset, uint32_t value)
{
    *reinterpret_cast<volatile uint32_t *>(base + offset) = value;
    asm volatile("dsb sy" ::: "memory");
}

uintptr_t map(uint64_t physical, size_t bytes, OSSharedPtr<IOMemoryMap> &mapping)
{
    auto memory = IOMemoryDescriptor::withPhysicalAddress(physical, bytes, kIODirectionInOut);
    if (!memory) return 0;
    mapping = OSSharedPtr<IOMemoryMap>(memory->map(kIOMapAnywhere | kIOMapInhibitCache), OSNoRetain);
    return mapping ? mapping->getVirtualAddress() : 0;
}

// The DEBUG firmware prepares PM8150 warm reset during RamLogReset(). The
// Android ramoops console has no ECC and stays reserved across the handoff.
constexpr uint32_t kRamoopsSignature = 0x43474244;
constexpr uint32_t kRamoopsBytes = 0x200000;
constexpr uint32_t kRamoopsDataBytes = kRamoopsBytes - 12;
constexpr uint32_t kMsgbufLimit = 64 * 1024;
OSSharedPtr<IOMemoryMap> logMapping, imemMapping, powerMapping;
uintptr_t logBase, imemBase, powerBase;
unsigned resetReady, resetting;
bool launchdCheckpoint;
unsigned launchdReachedMain, dataMountConfirmed;
char dataMountDevice[64], dataMountUuid[37];
char matchedRootDevice[32], matchedRootUuid[37];

struct ConsoleRing {
    uint32_t cursor = read32(logBase, 4);
    uint32_t size = read32(logBase, 8);
    bool valid = read32(logBase, 0) == kRamoopsSignature && cursor < kRamoopsDataBytes &&
                 size <= kRamoopsDataBytes && (size == kRamoopsDataBytes || cursor == size);

    void put(char byte)
    {
        if (!byte) return;
        *reinterpret_cast<volatile char *>(logBase + 12 + cursor) = byte;
        if (++cursor == kRamoopsDataBytes) cursor = 0;
        if (size < kRamoopsDataBytes) ++size;
    }

    void text(const char *message)
    {
        for (unsigned i = 0; i < 2048 && message[i]; ++i) put(message[i]);
    }
};

void resetMonitorCall(uint64_t fn, uint64_t info, uint64_t arg)
{
    register uint64_t x0 asm("x0") = fn;
    register uint64_t x1 asm("x1") = info;
    register uint64_t x2 asm("x2") = arg;
    register uint64_t x3 asm("x3") = 0;
    asm volatile("smc #0" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3) ::
                 "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13",
                 "x14", "x15", "x16", "x17", "memory", "cc");
}

void waitResetSeconds(unsigned seconds)
{
    uint64_t now, freq;
    asm volatile("isb\n\tmrs %0, cntpct_el0\n\tmrs %1, cntfrq_el0" : "=r"(now), "=r"(freq));
    const uint64_t deadline = now + seconds * freq;
    do { asm volatile("isb\n\tmrs %0, cntpct_el0" : "=r"(now)); } while (now < deadline);
}

void resetToFastboot(const char *message, bool hold)
{
    if (!__atomic_load_n(&resetReady, __ATOMIC_ACQUIRE) || __atomic_exchange_n(&resetting, 1, __ATOMIC_ACQ_REL)) return;
    if (logBase) {
        ConsoleRing ring;
        if (ring.valid) {
            ring.text("\nELISH_XNU: recent kernel messages\n");
            const auto *buffer = msgbufp;
            if (buffer && buffer->msg_magic == MSG_MAGIC) {
                const unsigned capacity = buffer->msg_size;
                const unsigned end = buffer->msg_bufx;
                if (capacity && capacity <= MAX_MSG_BSIZE && end < capacity && buffer->msg_bufc) {
                    const char *data = buffer->msg_bufc;
                    const unsigned count = capacity < kMsgbufLimit ? capacity : kMsgbufLimit;
                    unsigned offset = (end + capacity - count) % capacity;
                    for (unsigned i = 0; i < count; ++i) {
                        ring.put(data[offset]);
                        if (++offset == capacity) offset = 0;
                    }
                }
            }
            ring.text("\nELISH_XNU: ");
            ring.text(message);
            ring.text(powerBase ? "\nELISH_XNU: requesting prepared warm reset\n" :
                                  "\nELISH_XNU: requesting PSCI SYSTEM_RESET with bootloader reason\n");
            asm volatile("dsb sy" ::: "memory");
            write32(logBase, 4, ring.cursor);
            write32(logBase, 8, ring.size);
        }
    }
    if (hold) waitResetSeconds(5);
    write32(imemBase, 0x65c, 0x77665500);
    if (powerBase) {
        // RamoopsLogLib's warm path assumes the DEBUG SEC prepared PM8150.
        resetMonitorCall(0xc2000109, 2, 1);
        resetMonitorCall(0xc2000901, 1, 0);
        write32(powerBase, 0, 0);
        waitResetSeconds(1);
    }
    resetMonitorCall(0x84000009, 0, 0);
}

void resetDeadline(thread_call_param_t, thread_call_param_t)
{
    elish_touch_log();
    elish_touch_deadline_snapshot();
    resetToFastboot("RAM-only deadline reached", false);
}

void userspaceSnapshot(thread_call_param_t, thread_call_param_t)
{
    int sample[10] = {};
    char springboard[400] = "absent";
    char chronod[400] = "absent";
    char breakpoint[112] = "BRK not sampled";
    char keyStore[72];
    elish_snapshot_processes(sample, springboard, chronod, sizeof(springboard));
    elish_format_user_breakpoint(breakpoint, sizeof(breakpoint));
    elish_key_store_snapshot(keyStore, sizeof(keyStore));
    const bool data = __atomic_load_n(&dataMountConfirmed, __ATOMIC_ACQUIRE);
    const unsigned main = __atomic_load_n(&launchdReachedMain, __ATOMIC_ACQUIRE);
#if ELISH_LEGACY_DISPLAY
    unsigned display[8];
    elish_snapshot_display(display);
    // Start below the panel area clipped by the diagnostic camera.
    panic("\n\n\n\n\n\n\n\n"
          "Data %d Launchd %d Main %u Setup %d\n"
          "Copies %u Ends %u Rejects %u\n"
          "backboardd %d lsd %d pkd %d\nLiveActivities pid %d exits %d status %x\n%s\n%s\nSpringBoard %d %s\nchronod %d %s",
          data, sample[0], main, sample[3], display[2], display[4], display[6],
          sample[1], sample[5], sample[7], sample[4], sample[8], static_cast<unsigned>(sample[9]),
          keyStore, breakpoint, sample[2], springboard, sample[6], chronod);
#else
    panic("elish userspace snapshot\nData APFS rw=%d %s\n%s\nlaunchd=%d main=%u\nSetup=%d\nbackboardd %d lsd %d pkd %d\nLiveActivities pid %d exits %d status %x\n%s\n%s\nSpringBoard %d %s\nchronod %d %s",
          data, data ? dataMountDevice : "unconfirmed", data ? dataMountUuid : "",
          sample[0], main, sample[3], sample[1], sample[5], sample[7],
          sample[4], sample[8], static_cast<unsigned>(sample[9]), keyStore, breakpoint, sample[2], springboard, sample[6], chronod);
#endif
}

bool armRamOnlyReset(unsigned seconds)
{
    logBase = map(0xb0000000, kRamoopsBytes, logMapping);
    imemBase = map(0x146bf000, kBlockBytes, imemMapping);
    if (!imemBase) return false;
    unsigned warm = 0;
    if (PE_parse_boot_argn("elish-warm-reset", &warm, sizeof(warm)) && warm == 1) {
        powerBase = map(0x0c264000, kBlockBytes, powerMapping);
        if (!powerBase) return false;
    }
    const auto call = thread_call_allocate(resetDeadline, nullptr);
    if (!call) return false;
    __atomic_store_n(&resetReady, 1, __ATOMIC_RELEASE);
    uint64_t deadline;
    clock_interval_to_deadline(seconds, NSEC_PER_SEC, &deadline);
    thread_call_enter_delayed(call, deadline);
    unsigned snapshot = 0;
    if (PE_parse_boot_argn("elish-userspace-snapshot", &snapshot, sizeof(snapshot)) && snapshot) {
        if (snapshot >= seconds) panic("elish snapshot must precede RAM-only deadline");
        const auto sample = thread_call_allocate_with_options(userspaceSnapshot, nullptr,
                                                               THREAD_CALL_PRIORITY_KERNEL, 0);
        if (!sample) panic("elish userspace snapshot setup failed");
        clock_interval_to_deadline(snapshot, NSEC_PER_SEC, &deadline);
        thread_call_enter_delayed(sample, deadline);
    }
    return true;
}

bool wait32(uintptr_t base, unsigned offset, uint32_t expected)
{
    uint64_t deadline;
    clock_interval_to_deadline(1, 1000000000, &deadline);
    do {
        if (read32(base, offset) == expected) return true;
        IODelay(10);
    } while (mach_absolute_time() < deadline);
    return read32(base, offset) == expected;
}

// Full, dedicated cache lines for all descriptor and data DMA. All three
// regions start on 4 KiB boundaries. There is exactly one active slot.
void syncDma(uint8_t *start, size_t bytes, bool before)
{
    asm volatile("dsb sy" ::: "memory");
    for (size_t off = 0; off < bytes; off += kCacheLine) {
        if (before) asm volatile("dc civac, %0" :: "r"(start + off) : "memory");
        else asm volatile("dc ivac, %0" :: "r"(start + off) : "memory");
    }
    asm volatile("dsb sy" ::: "memory");
}

uint32_t crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1)));
    }
    return ~crc;
}

class Transport {
    OSSharedPtr<IOMemoryMap> hostMapping, smmuMapping;
    OSSharedPtr<IOBufferMemoryDescriptor> arena;
    uintptr_t host = 0, smmu = 0;
    uint8_t *memory = nullptr;
    uint32_t physical = 0;
    uint32_t faults[3] = {};
    bool mayDma = false;

    const char *checkRoutes();
    bool expect32(uintptr_t base, uint32_t physicalBase, unsigned offset, uint32_t expected);
    bool sameFaults() const;
    bool submit(uint8_t opcode, uint32_t lba, uint16_t blocks);

public:
    struct RegisterMismatch {
        uint32_t address = 0;
        uint64_t observed = 0;
        uint64_t expected = 0;
    } mismatch;
    struct Completion {
        uint32_t doorbell = 0;
        uint32_t interrupts = 0;
        uint32_t ocs = 0xf;
        uint32_t header = 0;
        uint32_t status = 0;
        uint32_t residual = 0;
    } completion;

    ~Transport();
    const char *open();
    bool stop();
    bool read(uint32_t lba, uint16_t blocks);
    bool faultsUnchanged() const { return physical && sameFaults(); }
    const uint8_t *data() const { return memory + kData; }
    uint32_t address() const { return physical; }
};

bool Transport::expect32(uintptr_t base, uint32_t physicalBase, unsigned offset, uint32_t expected)
{
    const uint32_t observed = read32(base, offset);
    if (observed == expected) return true;
    mismatch = {physicalBase + offset, observed, expected};
    return false;
}

const char *Transport::checkRoutes()
{
    const uint32_t control = read32(smmu, 0);
    if ((control & ~kSmmuControlVariable) != kSmmuControl) {
        mismatch = {kSmmuBase, control, kSmmuControl};
        return "SMMU global control";
    }
    if (!expect32(smmu, kSmmuBase, 0x20, 0x4c017e57) ||
        !expect32(smmu, kSmmuBase, 0x24, 0x6000004a) || !expect32(smmu, kSmmuBase, 0x28, 0x00005111))
        return "SMMU global state";

    unsigned seen = 0;
    for (unsigned group = 0; group < 87; ++group) {
        const unsigned offset = 0x800 + 4 * group;
        const uint32_t smr = read32(smmu, offset);
        if (!(smr & 0x80000000U)) continue;
        const uint32_t s2cr = read32(smmu, 0xc00 + 4 * group);
        const unsigned sid = smr & 0x7fff;
        const unsigned mask = (smr >> 16) & 0x7fff;
        const unsigned bank = s2cr & 0xff;
        if (smr == UINT32_MAX || s2cr == UINT32_MAX) {
            mismatch = {kSmmuBase + offset, (uint64_t{smr} << 32) | s2cr, 0};
            return "SMMU invalid route (SMR:S2CR)";
        }
        if (mask == 0 && ((sid == 0x4e0 && s2cr == 2) || (sid == 0xe0 && s2cr == 3))) {
            if (seen & (1U << bank)) {
                mismatch = {kSmmuBase + offset, (uint64_t{smr} << 32) | s2cr, 0};
                return "SMMU duplicate route (SMR:S2CR)";
            }
            seen |= 1U << bank;
        } else if (((sid ^ 0x4e0) & ~mask) == 0 || ((sid ^ 0xe0) & ~mask) == 0 ||
                   (((s2cr >> 16) & 3) == 0 && (bank == 2 || bank == 3))) {
            mismatch = {kSmmuBase + offset, (uint64_t{smr} << 32) | s2cr, 0};
            return "SMMU conflicting route (SMR:S2CR)";
        }
    }
    if (seen != 0xc) {
        mismatch = {0, seen, 0xc};
        return "SMMU missing routes (bank mask)";
    }
    for (unsigned bank = 2; bank <= 3; ++bank) {
        const unsigned context = (128 + bank) * kBlockBytes;
        if (!expect32(smmu, kSmmuBase, 0x1000 + 4 * bank, 0x1f000) ||
            !expect32(smmu, kSmmuBase, 0x1800 + 4 * bank, 1) ||
            !expect32(smmu, kSmmuBase, context, 0xe0) ||
            !expect32(smmu, kSmmuBase, context + 0x30, 0)) return "SMMU context state";
        const uint64_t ttbr0 = read64(smmu, context + 0x20);
        if (ttbr0 != 0) {
            mismatch = {kSmmuBase + context + 0x20, ttbr0, 0};
            return "SMMU TTBR0";
        }
    }
    if (!expect32(smmu, kSmmuBase, 0x48, 0) || !expect32(smmu, kSmmuBase, kContext2 + 0x58, 0x400) ||
        !expect32(smmu, kSmmuBase, kContext3 + 0x58, 0x400)) return "SMMU initial fault state";
    faults[1] = faults[2] = 0x400;
    return nullptr;
}

bool Transport::sameFaults() const
{
    return read32(smmu, 0x48) == faults[0] && read32(smmu, kContext2 + 0x58) == faults[1] &&
           read32(smmu, kContext3 + 0x58) == faults[2];
}

const char *Transport::open()
{
    OSSharedPtr<IOMemoryMap> gccMapping;
    const uintptr_t gcc = map(kGccUfsBase, kBlockBytes, gccMapping);
    if (!gcc) return "GCC MMIO map";
    if (!expect32(gcc, kGccUfsBase, 0, 0) || !expect32(gcc, kGccUfsBase, 4, 0xf822f000) ||
        !expect32(gcc, kGccUfsBase, 0x10, 0x221) || !expect32(gcc, kGccUfsBase, 0x18, 1))
        return "GCC clock, GDSC or reset";
    smmu = map(kSmmuBase, 0xd0000, smmuMapping);
    if (!smmu) return "SMMU MMIO map";
    if (const char *stage = checkRoutes()) return stage;
    host = map(kHostBase, 0x3000, hostMapping);
    if (!host) return "UFSHCI MMIO map";
    if (!expect32(host, kHostBase, 0, 0x1587031f) || !expect32(host, kHostBase, 0x08, 0x300) ||
        !expect32(host, kHostBase, 0x30, 0x10f) || !expect32(host, kHostBase, 0x34, 1) ||
        !expect32(host, kHostBase, kRunStop, 1) || !expect32(host, kHostBase, kDoorbell, 0) ||
        !expect32(host, kHostBase, 0x78, 0) || !expect32(host, kHostBase, 0x80, 0))
        return "inherited UFSHCI state";

    const IOOptionBits options = kIODirectionInOut | static_cast<IOOptionBits>(kIOMemoryPhysicallyContiguous) |
                                 static_cast<IOOptionBits>(kIOMemoryMapperNone);
    arena = OSSharedPtr<IOBufferMemoryDescriptor>(
        IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, options, kDmaBytes, 0xfffff000ULL), OSNoRetain);
    if (!arena) return "low DMA allocation";
    memory = static_cast<uint8_t *>(arena->getBytesNoCopy());
    IOByteCount contiguous = 0;
    const auto pa = arena->getPhysicalSegment(0, &contiguous, kIOMemoryMapperNone);
    if (!memory || !pa || (pa & (kBlockBytes - 1)) || contiguous < kDmaBytes ||
        pa > 0x100000000ULL - kDmaBytes) return "low DMA geometry";
    physical = static_cast<uint32_t>(pa);
    bzero(memory, kDmaBytes);
    syncDma(memory, kDmaBytes, true);

    write32(host, 0x24, 0);
    write32(host, kRunStop, 0);
    if (!wait32(host, kRunStop, 0) || read32(host, kDoorbell) != 0) return "inherited queue stop";
    write32(host, 0x50, physical);
    write32(host, 0x54, 0);
    if (read32(host, 0x50) != physical || read32(host, 0x54) != 0) return "queue base readback";
    mayDma = true;
    write32(host, kRunStop, 1);
    if (!wait32(host, kRunStop, 1)) return "XNU queue start";
    if (!submit(0, 0, 0)) return "NOP";
    if (!submit(0x25, 0, 0)) return "READ CAPACITY(10)";
    const auto capacity = reinterpret_cast<const uint32_t *>(data());
    if (__builtin_bswap32(capacity[0]) != kLastLba || __builtin_bswap32(capacity[1]) != kBlockBytes)
        return "LUN 0 capacity";
    return nullptr;
}

// This transport can construct only NOP, READ CAPACITY(10) and READ(10).
bool Transport::submit(uint8_t opcode, uint32_t lba, uint16_t blocks)
{
    completion = {};
    if (!mayDma || read32(host, kRunStop) != 1 || read32(host, kDoorbell) != 0 || !sameFaults()) return false;
    const bool scsi = opcode != 0;
    const uint8_t tag = scsi ? 0x38 : 0x37;
    const uint32_t transfer = blocks ? blocks * kBlockBytes : 8;
    const uint32_t responseOffset = scsi ? kScsiResponse : 32;
    const uint32_t dataSync = blocks ? transfer : kBlockBytes;

    auto trd = reinterpret_cast<uint32_t *>(memory);
    auto command = memory + kUcd;
    auto response = command + responseOffset;
    auto prdt = reinterpret_cast<uint32_t *>(command + kPrdt);
    bzero(command, kBlockBytes);
    command[kUpiuTag] = tag;
    trd[0] = (1U << 28) | (1U << 24) | (scsi ? 2U << 25 : 0);
    trd[1] = 0;
    trd[2] = 0xf;
    trd[3] = 0;
    trd[4] = physical + kUcd;
    trd[5] = 0;
    trd[6] = (responseOffset / 4) << 16 | (scsi ? 512 : 32) / 4;
    trd[7] = scsi ? ((kPrdt / 4) << 16) | 1 : 0;
    if (scsi) {
        command[kUpiuType] = 1;
        command[kUpiuFlags] = 0x40;
        command[12] = transfer >> 24;
        command[13] = transfer >> 16;
        command[14] = transfer >> 8;
        command[15] = transfer;
        command[16] = opcode;
        command[18] = lba >> 24;
        command[19] = lba >> 16;
        command[20] = lba >> 8;
        command[21] = lba;
        command[23] = blocks >> 8;
        command[24] = blocks;
        prdt[0] = physical + kData;
        prdt[3] = transfer - 1;
        syncDma(memory + kData, dataSync, true);
    }
    syncDma(memory, kCacheLine, true);
    syncDma(command, kBlockBytes, true);
    write32(host, kInterruptStatus, 1);
    write32(host, kDoorbell, 1);
    const bool completed = wait32(host, kDoorbell, 0);
    completion.doorbell = read32(host, kDoorbell);
    completion.interrupts = read32(host, kInterruptStatus);
    if (!completed || completion.doorbell) return false;
    syncDma(memory, kCacheLine, false);
    syncDma(command, kBlockBytes, false);
    if (scsi) syncDma(memory + kData, dataSync, false);
    auto responseWords = reinterpret_cast<const uint32_t *>(response);
    completion.ocs = trd[2] & 0xff;
    completion.header = __builtin_bswap32(responseWords[0]);
    completion.status = __builtin_bswap32(responseWords[1]);
    completion.residual = __builtin_bswap32(responseWords[3]);
    return sameFaults() && completion.ocs == 0 &&
           (response[kUpiuType] & 0x3f) == (scsi ? 0x21 : 0x20) &&
           response[kUpiuTag] == tag && response[kUpiuResponse] == 0 &&
           (!scsi || (response[kUpiuLun] == 0 && response[kUpiuStatus] == 0 &&
                      (response[kUpiuFlags] & 0x60) == 0 && completion.residual == 0 &&
                      (completion.interrupts & 1)));
}

bool Transport::read(uint32_t lba, uint16_t blocks)
{
    return blocks && blocks <= kMaxRead && lba <= kLastLba && blocks - 1U <= kLastLba - lba &&
           submit(0x28, lba, blocks);
}

bool Transport::stop()
{
    if (!mayDma) return true;
    write32(host, kRunStop, 0);
    if (!wait32(host, kRunStop, 0) || read32(host, kDoorbell) != 0) return false;
    mayDma = false;
    return true;
}

Transport::~Transport()
{
    // A timed-out controller may still own these pages. Keep them wired.
    if (!stop()) arena->retain();
}

const char *checkRootLayout(Transport &ufs)
{
    // Both capture CRCs pin the GPT geometry whose header and entries were
    // validated on this device in run40. No external LBA enters the transport.
    if (!ufs.read(1, 5) || crc32(ufs.data(), 5 * kBlockBytes) != 0x4f1522ce)
        return "primary GPT differs from run40";
    if (!ufs.read(kLastLba - 4, 5) || crc32(ufs.data(), 5 * kBlockBytes) != 0x294cad87)
        return "backup GPT differs from run40";
    if (!ufs.read(kUserdataLba, 1)) return "userdata manifest read";
    if (memcmp(ufs.data(), "SHNGROOT", 8) != 0) return "userdata has no SHNGROOT manifest";
    SHA256_CTX context;
    uint8_t digest[32];
    static_assert(sizeof(digest) == sizeof(kManifestSha256));
    SHA256_Init(&context);
    SHA256_Update(&context, ufs.data(), kBlockBytes);
    SHA256_Final(digest, &context);
    if (memcmp(digest, kManifestSha256, sizeof(digest)) != 0) return "userdata manifest differs from 21H461";
    if (!ufs.read(kSystemOS.firstLba, 1) || crc32(ufs.data(), kBlockBytes) != 0xc034a6f0)
        return "SystemOS APFS header";
    if (!ufs.read(kOS.firstLba, 1) || crc32(ufs.data(), kBlockBytes) != 0x8762762e ||
        !ufs.read(kOS.firstLba + 4, 1) || crc32(ufs.data(), kBlockBytes) != 0x79b3d53b)
        return "OS GPT or APFS header";
    return nullptr;
}
} // namespace

extern "C" __attribute__((used)) void elish_ram_only_panic(const char *message)
{
    resetToFastboot(message, true);
}

extern "C" void elish_record_root_media(const char *device, const char *uuid)
{
    if (!launchdCheckpoint) return;
    strlcpy(matchedRootDevice, device, sizeof(matchedRootDevice));
    strlcpy(matchedRootUuid, uuid, sizeof(matchedRootUuid));
}

class ElishUfsProbe : public IOService {
    OSDeclareDefaultStructors(ElishUfsProbe)
    Transport ufs;
    IOLock *gate = nullptr;
    bool online = false;
#if ELISH_VERIFIED_ROOT
    // Never DMA into this cache. Hash and deliver the same CPU-owned bytes.
    uint8_t *verified = nullptr;
    const uint8_t *rootDigests = nullptr;
    uint32_t verifiedLba = 0;
#endif
    void runProbe();
    void runDataProbe(uint32_t blocks);
    const char *prepareRoot();
public:
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
    void free() override;
    bool isOnline();
    IOReturn readWindow(const RootWindow &, IOMemoryDescriptor *, uint64_t offset, uint64_t bytes, uint64_t &done);
};

OSDefineMetaClassAndStructors(ElishUfsProbe, IOService)

class ElishUfsBlock : public IOBlockStorageDevice {
    OSDeclareDefaultStructors(ElishUfsBlock)
    ElishUfsProbe *host = nullptr;
    const RootWindow *window = nullptr;
public:
    void configure(ElishUfsProbe *owner, const RootWindow &layout) { host = owner; window = &layout; }
    bool authenticates(const char *uuid) const {
        return !isInactive() && window && host->isOnline() &&
               strcasecmp(uuid, window == &kOS ? kSystemUuid : kSystemOSUuid) == 0;
    }
    bool start(IOService *provider) override;
    IOReturn doEjectMedia() override { return kIOReturnUnsupported; }
    IOReturn doFormatMedia(UInt64) override { return kIOReturnNotWritable; }
    UInt32 doGetFormatCapacities(UInt64 *, UInt32) const override { return 0; }
    char *getVendorString() override { return const_cast<char *>("elish"); }
    char *getProductString() override { return const_cast<char *>(window->label); }
    char *getRevisionString() override { return const_cast<char *>("1.1"); }
    char *getAdditionalDeviceInfoString() override { return const_cast<char *>("Read-only UFS userdata window"); }
    IOReturn reportBlockSize(UInt64 *value) override { *value = kPublishedBlockBytes; return kIOReturnSuccess; }
    IOReturn reportEjectability(bool *value) override { *value = false; return kIOReturnSuccess; }
    IOReturn reportMaxValidBlock(UInt64 *value) override {
        *value = window->bytes / kPublishedBlockBytes - 1;
        return kIOReturnSuccess;
    }
    IOReturn reportMediaState(bool *present, bool *changed) override {
        *present = !isInactive();
        if (changed) *changed = false;
        return kIOReturnSuccess;
    }
    IOReturn reportRemovability(bool *value) override { *value = false; return kIOReturnSuccess; }
    IOReturn reportWriteProtection(bool *value) override { *value = true; return kIOReturnSuccess; }
    IOReturn doSynchronizeCache() override { return kIOReturnSuccess; }
    IOReturn doSynchronize(UInt64, UInt64, IOOptionBits) override { return kIOReturnSuccess; }
    IOReturn getWriteCacheState(bool *value) override { *value = false; return kIOReturnSuccess; }
    IOReturn setWriteCacheState(bool) override { return kIOReturnUnsupported; }
    IOReturn doAsyncReadWrite(IOMemoryDescriptor *, UInt64, UInt64, IOStorageAttributes *, IOStorageCompletion *) override;
};

OSDefineMetaClassAndStructors(ElishUfsBlock, IOBlockStorageDevice)

namespace {
dev_t verifiedVolume(const char *uuid, dev_t device, char *path, size_t size)
{
    if (strcasecmp(uuid, kSystemUuid) && strcasecmp(uuid, kSystemOSUuid)) return NODEV;
    auto matching = OSSharedPtr<OSDictionary>(IOService::serviceMatching("IOMedia"), OSNoRetain);
    auto properties = OSSharedPtr<OSDictionary>(OSDictionary::withCapacity(1), OSNoRetain);
    auto wanted = OSSharedPtr<OSString>(OSString::withCString(uuid), OSNoRetain);
    if (!matching || !properties || !wanted || !properties->setObject("UUID", wanted.get()) ||
        !matching->setObject("IOPropertyMatch", properties.get())) return NODEV;
    auto iterator = OSSharedPtr<OSIterator>(IOService::getMatchingServices(matching.get()), OSNoRetain);
    if (!iterator) return NODEV;
    auto *media = OSDynamicCast(IOService, iterator->getNextObject());
    if (!media || iterator->getNextObject() || media->isInactive()) return NODEV;
    const auto *mjr = OSDynamicCast(OSNumber, media->getProperty(kIOBSDMajorKey));
    const auto *mnr = OSDynamicCast(OSNumber, media->getProperty(kIOBSDMinorKey));
    if (!mjr || !mnr || mjr->unsigned64BitValue() > 0xff || mnr->unsigned64BitValue() > 0xffffff) return NODEV;
    const dev_t actual = makedev(mjr->unsigned32BitValue(), mnr->unsigned32BitValue());
    if (actual == NODEV || (device != NODEV && device != actual)) return NODEV;
    unsigned depth = 0;
    for (auto *provider = media; provider && depth++ < 64; provider = provider->getProvider()) {
        const auto *block = OSDynamicCast(ElishUfsBlock, provider);
        if (!block) continue;
        if (!block->authenticates(uuid)) return NODEV;
        if (path) {
            const auto *name = OSDynamicCast(OSString, media->getProperty(kIOBSDNameKey));
            if (!name || !name->getLength() || name->getLength() + sizeof("/dev/") > size) return NODEV;
            snprintf(path, size, "/dev/%s", name->getCStringNoCopy());
        }
        return actual;
    }
    return NODEV;
}

bool matchesReadOnlyApfs(mount_t mount, const char *uuid, char *device, size_t size)
{
    if (!mount) return false;
    const auto *stat = vfs_statfs(mount);
    strlcpy(device, stat->f_mntfromname, size);
    if (!(vfs_flags(mount) & MNT_RDONLY) || strcmp(stat->f_fstypename, "apfs")) return false;
    vnode_t vnode = vfs_devvp(mount);
    if (!vnode) return false;
    const dev_t actual = vnode_specrdev(vnode);
    vnode_put(vnode);
    return actual != NODEV && verifiedVolume(uuid, actual, nullptr, 0) == actual;
}
} // namespace

extern "C" void elish_report_launchd(vnode_t root, const char *process)
{
    const bool launchd = strcmp(process, "launchd") == 0;
    if (launchd) __atomic_store_n(&launchdReachedMain, 1, __ATOMIC_RELEASE);
    if (!launchdCheckpoint) return;
    mount_t upper = vnode_mount(root);
    vnode_t covered = (vfs_flags(upper) & MNT_UNION) ? vfs_vnodecovered(upper) : nullptr;
    char systemDevice[64] = {};
    char cacheDevice[64] = {};
    const bool system = covered && matchesReadOnlyApfs(vnode_mount(covered), kSystemUuid, systemDevice, sizeof(systemDevice)) &&
        strncmp(systemDevice, "/dev/", 5) == 0 && strcmp(systemDevice + 5, matchedRootDevice) == 0 &&
        strcasecmp(matchedRootUuid, kSystemUuid) == 0;
    if (covered) vnode_put(covered);
    const bool cache = matchesReadOnlyApfs(upper, kSystemOSUuid, cacheDevice, sizeof(cacheDevice));
    const bool confirmed = system && cache && launchd;
    panic("elish System root %s\nAPFS UUID=%s\nSystem=%s verified=%d\nSystemOS=%s verified=%d\nPID 1 main: %s",
          confirmed ? "confirmed" : "checkpoint mismatch", matchedRootUuid,
          systemDevice, system, cacheDevice, cache, process);
}

extern "C" void elish_report_data_mount(const char *device, const char *uuid)
{
    strlcpy(dataMountDevice, device, sizeof(dataMountDevice));
    strlcpy(dataMountUuid, uuid, sizeof(dataMountUuid));
    __atomic_store_n(&dataMountConfirmed, 1, __ATOMIC_RELEASE);
    elish_key_store_start();
    elish_touch_start();
}

extern "C" int elish_system_root_authorized(dev_t device)
{
    const bool authorized = device != NODEV && verifiedVolume(kSystemUuid, device, nullptr, 0) == device;
    printf("ELISH_XNU: System root %s dev=%#x UUID=%s\n", authorized ? "verified" : "refused", device, kSystemUuid);
    return authorized;
}

extern "C" int native_verified_block_root_authorized(dev_t device)
{
    return device != NODEV && (verifiedVolume(kSystemUuid, device, nullptr, 0) == device ||
                              verifiedVolume(kSystemOSUuid, device, nullptr, 0) == device);
}

extern "C" int native_verified_block_manifest_sha384(dev_t device, const char *uuid, uint8_t digest[48])
{
    if (!digest) return 0;
    bzero(digest, SHA384_DIGEST_LENGTH);
#if ELISH_VERIFIED_ROOT
    if (!uuid || device == NODEV || verifiedVolume(uuid, device, nullptr, 0) != device) return 0;
    // The archive generates these digests from the same pinned windows as RootPolicies.
    const auto *manifest = strcasecmp(uuid, kSystemUuid) == 0 ? kOsManifestSha384 : kSystemOsManifestSha384;
    memcpy(digest, manifest, SHA384_DIGEST_LENGTH);
    return 1;
#else
    return 0;
#endif
}

extern "C" int native_verified_block_volume_device(const char *uuid, dev_t *device, char *path, size_t size)
{
    *device = verifiedVolume(uuid, NODEV, path, size);
    return *device != NODEV;
}

bool ElishUfsBlock::start(IOService *provider)
{
    auto protocol = OSSharedPtr<OSDictionary>(OSDictionary::withCapacity(2), OSNoRetain);
    auto location = OSSharedPtr<OSString>(OSString::withCString(kIOPropertyInternalKey), OSNoRetain);
    auto interconnect = OSSharedPtr<OSString>(OSString::withCString(kIOPropertyPhysicalInterconnectTypeVirtual), OSNoRetain);
    auto device = OSSharedPtr<OSDictionary>(OSDictionary::withCapacity(1), OSNoRetain);
    auto medium = OSSharedPtr<OSString>(OSString::withCString(kIOPropertyMediumTypeSolidStateKey), OSNoRetain);
    return protocol && location && interconnect && device && medium &&
           protocol->setObject(kIOPropertyPhysicalInterconnectLocationKey, location.get()) &&
           protocol->setObject(kIOPropertyPhysicalInterconnectTypeKey, interconnect.get()) &&
           device->setObject(kIOPropertyMediumTypeKey, medium.get()) &&
           setProperty(kIOPropertyProtocolCharacteristicsKey, protocol.get()) &&
           setProperty(kIOPropertyDeviceCharacteristicsKey, device.get()) && IOBlockStorageDevice::start(provider);
}

IOReturn ElishUfsBlock::doAsyncReadWrite(IOMemoryDescriptor *buffer, UInt64 block, UInt64 count,
                                        IOStorageAttributes *, IOStorageCompletion *completion)
{
    const uint64_t capacity = window->bytes / kPublishedBlockBytes;
    if (!buffer || !completion || !completion->action || !count || block >= capacity || count > capacity - block ||
        buffer->getLength() < count * kPublishedBlockBytes) return kIOReturnBadArgument;
    if (buffer->getDirection() != kIODirectionIn) return kIOReturnNotWritable;
    if (isInactive()) return kIOReturnNotReady;
    uint64_t done = 0;
    const IOReturn result = host->readWindow(*window, buffer, block * kPublishedBlockBytes,
                                             count * kPublishedBlockBytes, done);
    completion->action(completion->target, completion->parameter, result, done);
    return kIOReturnSuccess;
}

bool ElishUfsProbe::isOnline()
{
    IOLockLock(gate);
    const bool active = online;
    IOLockUnlock(gate);
    return active;
}

IOReturn ElishUfsProbe::readWindow(const RootWindow &window, IOMemoryDescriptor *buffer, uint64_t offset,
                                   uint64_t bytes, uint64_t &done)
{
#if ELISH_VERIFIED_ROOT
    const auto *digests = rootDigests + (&window == &kOS ? kOsDigestsOffset : kSystemOSDigestsOffset);
    IOLockLock(gate);
    IOReturn result = online ? kIOReturnSuccess : kIOReturnNotReady;
    while (result == kIOReturnSuccess && done < bytes) {
        const uint64_t cursor = offset + done;
        const uint64_t chunk = cursor / kVerifiedChunk;
        const uint64_t start = chunk * kVerifiedChunk;
        const uint32_t lba = window.firstLba + start / kBlockBytes;
        const uint64_t tail = window.bytes - start;
        const uint32_t span = tail < kVerifiedChunk ? tail : kVerifiedChunk;
        if (verifiedLba != lba) {
            for (uint32_t copied = 0; copied < span;) {
                const uint32_t remaining = span - copied;
                const uint32_t needed = (remaining + kBlockBytes - 1) / kBlockBytes;
                const uint16_t blocks = needed < kMaxRead ? needed : kMaxRead;
                if (!ufs.read(lba + copied / kBlockBytes, blocks)) {
                    result = kIOReturnIOError;
                    break;
                }
                const uint32_t available = blocks * kBlockBytes;
                const uint32_t count = remaining < available ? remaining : available;
                memcpy(verified + copied, ufs.data(), count);
                copied += count;
            }
            if (result == kIOReturnSuccess) {
                SHA256_CTX context;
                uint8_t digest[SHA256_DIGEST_LENGTH];
                SHA256_Init(&context);
                SHA256_Update(&context, verified, span);
                SHA256_Final(digest, &context);
                if (memcmp(digest, digests + chunk * sizeof(digest), sizeof(digest))) result = kIOReturnNotPrivileged;
            }
            if (result != kIOReturnSuccess) {
                online = false;
                const bool stopped = ufs.stop();
                printf("ELISH_XNU: %s verified read failed LBA=%u status=%#x stopped=%d\n",
                       window.label, lba, result, stopped);
                break;
            }
            verifiedLba = lba;
        }
        const uint32_t skip = cursor - start;
        const uint64_t remaining = bytes - done;
        const IOByteCount length = remaining < span - skip ? remaining : span - skip;
        const IOByteCount copied = buffer->writeBytes(done, verified + skip, length);
        done += copied;
        if (copied != length) result = kIOReturnIOError;
    }
    IOLockUnlock(gate);
    return result;
#else
    return kIOReturnNotPrivileged;
#endif
}

const char *ElishUfsProbe::prepareRoot()
{
    if (const char *stage = ufs.open()) return stage;
    if (const char *stage = checkRootLayout(ufs)) return stage;
#if ELISH_VERIFIED_ROOT
    rootDigests = loadRootDigests();
    if (!rootDigests) return "21H461 root digest handoff missing or invalid";
    verified = static_cast<uint8_t *>(IOMalloc(kVerifiedChunk));
    if (!verified) return "verified read cache allocation";
    gate = IOLockAlloc();
    if (!gate) return "storage lock allocation";

    auto system = OSSharedPtr<ElishUfsBlock>(new ElishUfsBlock, OSNoRetain);
    auto os = OSSharedPtr<ElishUfsBlock>(new ElishUfsBlock, OSNoRetain);
    if (!system || !os || !system->init(nullptr) || !os->init(nullptr)) return "storage nub initialization";
    system->configure(this, kSystemOS);
    os->configure(this, kOS);
    if (!system->attach(this)) return "SystemOS attach";
    if (!os->attach(this)) {
        system->detach(this);
        return "OS attach";
    }
    if (!system->start(this)) {
        os->detach(this);
        system->detach(this);
        return "SystemOS start";
    }
    if (!os->start(this)) {
        system->stop(this);
        os->detach(this);
        system->detach(this);
        return "OS start";
    }
    online = true;
    system->registerService();
    os->registerService();
    return nullptr;
#else
    return "21H461 root digests missing from boot kernel";
#endif
}

bool ElishUfsProbe::start(IOService *provider)
{
    unsigned probe = 0, root = 0, dataBlocks = 0;
    const bool diagnostic = PE_parse_boot_argn("elish-ufs-probe", &probe, sizeof(probe)) && probe == 1;
    const bool dataProbe = PE_parse_boot_argn("elish-data-probe-blocks", &dataBlocks, sizeof(dataBlocks));
    const bool publish = PE_parse_boot_argn("elish-ufs-root", &root, sizeof(root)) && root == 1;
    if ((!diagnostic && !dataProbe && !publish) || !IOService::start(provider)) return false;
    unsigned checkpoint = 0;
    launchdCheckpoint = publish && PE_parse_boot_argn("elish-launchd-checkpoint", &checkpoint, sizeof(checkpoint)) && checkpoint == 1;
    unsigned seconds = 0;
    if (PE_parse_boot_argn("elish-ram-only", &seconds, sizeof(seconds)) &&
        (!seconds || seconds > 600 || !armRamOnlyReset(seconds))) panic("elish RAM-only reset setup failed");
    if (diagnostic) runProbe();
    if (dataProbe) runDataProbe(dataBlocks);
    if (const char *stage = prepareRoot()) {
        const bool stopped = ufs.stop();
        panic("elish UFS root: %s\nDMA=%08x stopped=%d faults-same=%d", stage, ufs.address(), stopped,
              ufs.faultsUnchanged());
    }
    return true;
}

void ElishUfsProbe::stop(IOService *provider)
{
    if (gate) {
        IOLockLock(gate);
        online = false;
        ufs.stop();
        IOLockUnlock(gate);
    }
    IOService::stop(provider);
}

void ElishUfsProbe::free()
{
#if ELISH_VERIFIED_ROOT
    if (verified) IOFree(verified, kVerifiedChunk);
#endif
    if (gate) IOLockFree(gate);
    IOService::free();
}

void ElishUfsProbe::runDataProbe(uint32_t blocks)
{
    const uint32_t first = dataFirstLba(blocks);
    const char *stage = first ? ufs.open() : "window size";
    if (!stage) stage = checkRootLayout(ufs);
    uint32_t firstCrc = 0, lastCrc = 0;
    if (!stage) {
        if (!ufs.read(first, 1)) stage = "first block READ(10)";
        else firstCrc = crc32(ufs.data(), kBlockBytes);
    }
    if (!stage) {
        if (!ufs.read(kDataLast, 1)) stage = "last block READ(10)";
        else lastCrc = crc32(ufs.data(), kBlockBytes);
    }
    const bool stopped = ufs.stop();
    const bool faultsUnchanged = ufs.faultsUnchanged();
    if (!stage && !stopped) stage = "queue stop";
    if (!stage && !faultsUnchanged) stage = "final SMMU fault state";
    panic("elish Data probe: %s\nLUN 0 LBA=%u-%u blocks=%u\nfirst CRC=%08x last CRC=%08x\n"
          "DMA=%08x stopped=%d faults-same=%d", stage ? stage : "boundary reads complete",
          first, kDataLast, blocks, firstCrc, lastCrc, ufs.address(), stopped, faultsUnchanged);
}

void ElishUfsProbe::runProbe()
{
    const char *stage = ufs.open();
    uint32_t lba = 0, crc = 0, want = 0;
    unsigned verified = 0;
    if (!stage) {
        for (const auto &sample : kKnownReads) {
            lba = sample.lba;
            want = sample.crc;
            crc = 0;
            if (!ufs.read(lba, 1)) {
                stage = "known-block READ(10)";
                break;
            }
            crc = crc32(ufs.data(), kBlockBytes);
            if (crc != want) {
                stage = "known-block CRC32";
                break;
            }
            ++verified;
        }
    }
    const bool stopped = ufs.stop();
    const bool faultsUnchanged = ufs.faultsUnchanged();
    if (!stage && !stopped) stage = "queue stop";
    if (!stage && !faultsUnchanged) stage = "final SMMU fault state";
    panic("elish UFS %s\nreg=%08x got=%08x%08x want=%08x%08x\n"
          "DMA=%08x reads=%u stopped=%d faults-same=%d\nLBA=%u CRC=%08x want=%08x\n"
          "DBR=%x IS=%x OCS=%x UPIU=%08x/%08x res=%x",
          stage ? stage : "known reads complete", ufs.mismatch.address,
          static_cast<uint32_t>(ufs.mismatch.observed >> 32), static_cast<uint32_t>(ufs.mismatch.observed),
          static_cast<uint32_t>(ufs.mismatch.expected >> 32), static_cast<uint32_t>(ufs.mismatch.expected),
          ufs.address(), verified, stopped, faultsUnchanged, lba, crc, want,
          ufs.completion.doorbell, ufs.completion.interrupts, ufs.completion.ocs,
          ufs.completion.header, ufs.completion.status, ufs.completion.residual);
}
