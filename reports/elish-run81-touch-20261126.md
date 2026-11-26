# elish run81: NT36523 → 21H461 digitizer

## Baseline and source inputs

Run80 on `boot_b` reaches original iPadOS 17.7.11 (`21H461`) Setup Assistant Hello, but ships no touchscreen driver. This experiment preserves the installed image and does not enable persistent Data. Its candidate copies the run80 FAT and replaces only the patched Mach-O and boot arguments. The CPU ADT, RootPolicies, Apple trust cache and initial 1 GiB RAM APFS Data seed (`7D443C28-95AC-49F8-AF25-4FC12FBC93E5`) are read back byte-for-byte. The payload builder can generate a new ADT random seed; the final FAT deliberately keeps the original ADT. No terminal userspace snapshot; the existing kernel warm-return timer is set to its maximum 600 seconds. GOP CPU-copy is unchanged.

- Source baselines: device `9c24ae8f3acf74ea77aa0eaf47c3a5216d726ed2` plus this working tree; UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139` with the run79 local changes; XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`; firmware `226fbcc677568edf9a04ce7cd95459c664821a03`; DarwinBoot `5243c68ec332a227fe3680cd5da6d5a0e127035c`; XnuPrelink `440589b0c31625f17edf363e24b3c82c51c70bb9`; SM8250 `166235ec2480ff0992627b7aafa15c65ce6b4efa` with the run79 CPU fixes; GopFramebuffer `b0d812d578efa2acb005b66f73168891719af09a`; AppleKeyStoreState `8c70daf814aafdf4a3d35e40dde0322655fd0a83`; QcomGeniSpi `eac47baefe6a5d2368a70351e6ad1b6361bedbe2`. `source.sha256` in each build directory records the changed inputs; unchanged inputs are checked against run79 before building.
- HW references: [Recovery FDT](elish-recovery-fdt-20261124.dtb) and [Android runtime](elish-live-hardware-20261123.md#display-and-input); MiCode `elish-r-oss` [`52a2f180865f8368828985d6384130acd093fa93`](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/52a2f180865f8368828985d6384130acd093fa93) (`drivers/input/touchscreen/nt36523`, `drivers/clk/qcom/gcc-kona.c`, `drivers/pinctrl/qcom/pinctrl-kona.c`, `drivers/irqchip/qcom-pdc.c`).
- Protocol and HID references: [NabuTouch `54b4f984`](https://github.com/zzweiqing/UniXNU-nabu/blob/54b4f98457f8e9e232cc0a077c20d08d0b2f0ac9/platform/NabuTouch.cpp) uses NT36523 on Snapdragon 860, verified on 21H16, not elish wiring or 21H461 bindings. [ShengHidInput `71a40e76`](https://github.com/UniXNU/ShengHidInput/tree/71a40e76daeae1bb4127546f96ffa7339c7a06ff) is the 21H461 reference; ShengNovatekTouch targets NT36532/NT36532E and is not selected. The 21H461 run79 binary retains executable IOHIDFamily and codeless event personalities. The build checks the original IOHIDDevice layouts/methods and defers the board class initializer until the original family has loaded.

## Board path and limits

| Item | elish evidence and implementation |
| --- | --- |
| Bus | QUP0 SE4 `0x990000+0x4000`, wrapper `0x9c0000+0x2000`, CS0 mode 0, 19.2 MHz, GPIO8–11 North TLMM (`0x0f908000` etc.), qup4 mux 1, 6 mA no pull. GCC source `0x1174d0`, halt registers `0x1174cc/0x117004/0x117008`; adds three clock votes at `0x152008` bits 14/6/7 and checks AHB/branch. If DFS is enabled it accepts only a matching XO level; otherwise uses the Kona XO RCG tuple. No wrapper reset; gated on SPI protocol and verified FIFO geometry. |
| IRQ | GPIO39 South TLMM `0x0f527000`, stock rising IRQ. PDC line 73/GIC INTID 585 is distinct from the TLMM summary. Run84 polls the GPIO edge latch with CPU IRQ forwarding disabled. SPI uses FIFO PIO. |
| TDDI | Recovery names the K81 `42_02_0a` panel with GPIO75 RESX and display power rails; the touch node has no dedicated reset or supply. No panel rail/reset writes. Retained display power is a runtime dependency, not an independently measured rail state. |
| Firmware | `panel_vendor=S`, `panel_display=B`, `touch_vendor=4`; Android identifies PID `5961`, v`0x1a`. Original `boot.img` SHA-256 `336f36fea4b386af81be542f6573098d493036f1ed9bd3bd7d59f41bef2487df`; its built-in firmware table ties `novatek_nt36523_k81a_fw01.bin` to `0x22c2650`, size `0x30000`; extracted SHA-256 `33d2e0a9217df7e898a96728631f3b6decd85ddb7d4112b38b9d7d6d162d6938`. Proprietary firmware bytes are not committed. The driver first tests firmware already running; if absent, reads trim `23 65 03` and loads the pinned cascade firmware into NT36523 SRAM, not flash. Separate QUP SPI engine firmware is in local `qupfw.img` at `0xbe20+0x84c`; it is not loaded by this probe. |
| Frames | Ten IDs, packet checksum, watchdog/ESD rejection, valid positions and releases. X 0–1599, Y 0–2559 mapped directly to the portrait display axes pending physical validation; pen excluded. Transport, firmware, frame, HID registration and reports log separately. |

`ELISH_TOUCH: stage=` reports the last stage entered. Stage `1` requests the worker, `2` starts clock votes after mapping the registers, `3` checks engine protocol and FIFO configuration, `4` has initialized SPI PIO, `5` enters the fw01 path after trim identification, `6` has verified running firmware, and `7` begins scanning. Run83 and earlier reported `8` when scanning stopped after consecutive bad reads. A resident verified fw01 skips stage `5`. The accompanying counters and `elish-touch-hid:` lines distinguish bus progress, real contacts and HID delivery.

## Commands

```sh
BASE=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125
OUT="$BASE/run81e-touch-hid"
export UNIXNU_ELISH_TOUCH_STOCK_BOOT=/root/repo/elish_os1.0.2.0/boot.img
# Run81e used 600 seconds; the build script now defaults to 420 for later rounds.
UNIXNU_ELISH_TOUCH_DEADLINE=600 bash scripts/build-elish-touch-candidate.sh "$BASE" "$OUT" hid
bash scripts/capture-elish-touch.sh "$OUT"
python3 scripts/check-elish.py --static-only
python3 /root/UniXNU/scripts/porting-record.py check --device .
```

Offline validation so far: the packet harness passed checksum, stable IDs, bounds, movement and release tests; exact fw01 extraction passed and retains four non-empty partition writes. `run81-touch-spi` failed initially due to a missing XNU smart-pointer include; `run81b-touch-hid` linked and both 190-slot storage ABI checks passed but the repack stopped on a regenerated ADT `random-seed`. The repack now retains and independently checks the original ADT. `run81c-touch-hid` overlapped intermediate HID source edits and stopped on two unused-symbol build errors. None of these attempts ran on the tablet.

## Selected image: run81e

Run81d linked and packaged but was not booted: its 192 KiB fw01 blob in `__TEXT` moved `__TEXT_EXEC` to `0xfffffff007270000`. In run81e, the blob is hash-verified in `__LASTDATA_CONST,__touch_fw`. The executable start is `0xfffffff007244000` instead of `0xfffffff007240000` in run79, a single 16 KiB page shift. The actual entry moves by the same page to `0xfffffff007427488`, keeping the same offset within executable code. DarwinBoot reads the entry and segment bounds from the Mach-O. `layout-readback.log` verifies that layout in both kernels and compares the Android v3 carrier and extracted FAT. The carrier is byte-identical to run80. FAT readback checks the new kernel and args, matches the unaffected assets to run80, and passes `fsck.fat -vn`. `hid-linked-check.log` verifies the retained family, 52 private bindings, 207 linked slots and deferred initialization. These are host checks, not proof of device execution.

| Artifact under `OUT` | SHA-256 |
| --- | --- |
| `boot.img` | `716bfacf71c166bcc89e5515e9b133fd53f11c93c873de22670cabab5298a35e` |
| `payload.img` | `e99201c00e9fb8ec19eaf25cc1bc2ddb93135b7bed99b34bbb8102feb030627a` |
| `kernel.macho` (pristine) | `d609a87881d8aac20a37abe02e1cb1a95ddaa537716d878193598a961279a5f4` |
| `packed-kernel.macho` | `3c3041ad19baa20d2b92bafe3cce6cc26633e9d6a2f6151f3d9be30be11a6214` |
| `source.sha256` | `aee4ae84c847b717c4f8a69927cd668a9c85cb224762f6dea6ff046e88da51ca` |

## Hardware acceptance

At 19:20:39 CST, `capture-elish-touch.sh` verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no`, then ran `fastboot -s 4bed07e4 boot "$OUT/boot.img"`. Fastboot reported `Sending OKAY` (1.758 s) and `Booting OKAY` (0.238 s). `device-attempt.LumUbu/device.log` records the command; `device-attempt.LumUbu/camera.mkv` is the continuous capture. No `fastboot continue` or partition write was issued.

At camera timestamp `01:45` the screen still shows the green, blue and magenta bands (`frame-105s.jpg`). At `02:30` the internal panel shows the Apple logo (`frame-150s.jpg`, SHA-256 `d4ca1926e1273ca8a1937aba2ec26813f30de6df9688fd0e687bcce42a0ac75d`). At `06:13` the recording shows original Setup Assistant (`olá` and the bottom swipe prompt; `frame-373s.jpg`, SHA-256 `3d60ab549ab5ea6b6bff0d9b5c9045dd35b7949cca5dd522e1148c6ab55b5102`). The continuous recording shows physical bottom-up attempts around `05:46`–`05:56` (`gesture-detail-345-357.jpg`, SHA-256 `2fd7a8af5bb9f4155f632b1c9f65a9f5328e724a70f4183f3b0e80d2137ad244`). The greeting keeps animating; no language list opens. The owner confirmed there was no response. At `09:20` the panel still shows the original greeting (`frame-560s.jpg`). No language choice or tap feedback was observed. SPI, the touch firmware, physical report delivery, and HID matching remain unverified.

The tablet re-enumerated as verified bootloader fastboot (USB device 005) at 19:31:00 CST, 619.02 seconds after `Booting OKAY`. The single MJPEG-in-MKV recording is 623.015 seconds and 3,771,338,113 bytes, SHA-256 `c3537ab69a249d081a57fb9a1dd42178d98001cef54077d99d1e38589d2e69e7`. The historical Lineage reader is not a read-only fallback: its initramfs fstab defines a first-stage read/write metadata mount with `check,formattable`, and lists writable userdata and persist. It was not booted. A separate firmware-only viewer is being prepared to read the preserved ramoops instead.

At the owner's request subsequent touch candidates default to `elish-ram-only=420` (seven minutes measured from the UFS service, about 7:20 after `Booting OKAY` if startup repeats this run). The existing run81e artifact is unchanged. The capture script reads the deadline from the verified FAT and allows 40 extra seconds for loader/XNU startup and USB enumeration; 420 and 600 parsed correctly, and missing, duplicate, or out-of-range values were rejected. The decoder and GENI register-model host tests, `check-elish.py --static-only`, the workflow's 14-area record check with working-tree coverage, and `git diff --check` passed.

## Deadline snapshot and ramoops reader

The first firmware-only viewer was accepted by `fastboot -s 4bed07e4 boot` after checking the elish USB identity. The camera shows three text lines during the 35-second viewing interval. They cannot be transcribed from the captured frames. USB did not return to fastboot after PSCI reset. The camera subsequently showed the installed image's XNU entry markers and Apple logo. The capture directory is `run81-viewer.LlxPi0/image/` under `BASE`. Its Android v3 boot image had zero ramdisk bytes and SHA-256 `6465c1e2ba8d8159913cf29e9a30e0dc4a9ce422c751ea7745c295ddfd3e06e8`.

The revised viewer runs in DXE with `ELISH_RAMOOPS_LOG=FALSE`, reads the `0xb0000000` console ring, handles XNU's timestamp prefixes, and requests the PMIC warm return. It does not enter BDS or write the ring. The build uses the local `uefi-elish` changes based on `226fbcc677568edf9a04ce7cd95459c664821a03`. The 634,880-byte image in `run81-viewer-warm.799AxE/image/` has SHA-256 `e9f831fddf70091ded2137293feeb5bbf00a54c9b0c99eb2250a03ad276b9abd`. The build and module inclusion check passed. Hardware return and log recovery are unverified.

```sh
python3 scripts/build-loader-probe.py --unixnu "$BASE/../.." --uefi ../uefi-elish \
  --dtb reports/elish-recovery-fdt-20261124.dtb --out "$BASE/touch-viewer" \
  --feeds "$BASE/../feeds" --clang-bin /usr/lib/llvm-21/bin --ramoops-viewer
```

The touch candidate builder now extracts the exact run80 FD from the verified baseline boot image. The new `elish-touch-snapshot=1` argument shows SE4 register values, SPI and packet errors, and touch/HID counters on the diagnostic panic screen at the RAM-only deadline. The existing panic renderer invokes the warm-return handler after holding the frame for capture. Use the snapshot to identify which stage to adjust before attempting the acceptance swipe and language tap again.

The run82 snapshot image is `run82-touch-snapshot.lywvYX/image/boot.img` under `BASE` (67,756,032 bytes, SHA-256 `e512ff04491963cfaedd0603d74dc489909076292c17254f4ed827b9e5f7f09d`). `build.log` and `image/hid-linked-check.log` record successful kernel compilation, prelink, boot-image packaging, FAT and asset readback, and verification of the retained IOHIDFamily, private bindings and 207 linked slots. The final FAT contains `elish-touch=1 elish-touch-snapshot=1 elish-ram-only=420`. The host did not enumerate the tablet in fastboot or ADB after this build. Run82 has no device result yet.

```sh
OUT="$BASE/run82-touch-snapshot.lywvYX/image"
bash scripts/capture-elish-touch.sh "$OUT"
```

## QUP0 clock admission

An independent firmware-only readback captured `Warm deadline marker present` and `ELISH_TOUCH: stage=2` in `run82-ramoops-readback/device-attempt.tyOHLs/viewer.jpg` under `BASE`. Stage 2 follows successful MMIO mapping and precedes the first QUP serial-engine access. The recovered line does not identify the branch that rejected startup or the exact source image.

Kona `gcc_qupv3_wrap_0_s_ahb_clk` declares hardware clock gating at CBCR bit 1. The vendor `clk_branch_wait` skips halt polling in hardware-gated mode. The original elish code could time out on the idle slave AHB branch even with all three votes present. The driver now accepts that branch with hardware gating enabled and continues to require the master AHB and SE4 to be on. It checks vote readback and records the vote and all three CBCRs in the warm log and deadline screen. [Kona GCC definitions](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/blob/52a2f180865f8368828985d6384130acd093fa93/drivers/clk/qcom/gcc-kona.c#L3119-L3145) · [Vendor branch wait](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/blob/52a2f180865f8368828985d6384130acd093fa93/drivers/clk/qcom/clk-branch.c#L19-L101)

```sh
OUT="$BASE/run83-touch-clock.zvLfo4/image"
bash scripts/build-elish-touch-candidate.sh "$BASE" "$OUT" hid
c++ -std=c++17 -Wall -Wextra -Werror scripts/test-elish-touch.cpp -o /tmp/test-elish-touch
/tmp/test-elish-touch
```

The run83 image is 67,756,032 bytes with SHA-256 `1a778e73299b39d63dd51790d8931705601c79891419b684f1edbf269f622110`. The source manifest records the worktree files relative to the revision above. The host tests cover the hardware-gated slave branch, halted master and SE4, missing vote bits and contact decoding. Packaging and FAT readback passed with the 420-second snapshot enabled. The linked audit verified the original HID family and 207 vtable slots.

Run83 RAM-booted at 20:32:27 CST, reached Hello and returned to verified bootloader fastboot at 20:39:50 CST. Its deadline snapshot shows `stage=8`, `Reads=40 SPI errors=0 Bad=40`, and `HID stage=5` with `match=1`. The idle run established clock, SPI and HID startup. The transport captured no valid frames. See `run83-touch-clock.zvLfo4/image/device-attempt.Mv6Vs4/camera.mkv` around 263 seconds of video time.

## Event acquisition

The Recovery FDT enables `novatek,pen-support`. MiCode's `nvt_ts_work_func` reads 79 payload bytes per GPIO39 rising interrupt and checks the first 65 for finger integrity. The prior XNU loop read 65 bytes at fixed intervals and stopped after 40 rejects, even if every read happened before a fresh event. The revised driver selects the event page once and polls the GPIO39 rising-edge latch, with the CPU interrupt disabled. It consumes the full stock wire frame and keeps the finger checksum enabled. A run of invalid packets releases any held contact and the reader continues waiting for events. Diagnostics include the latch count and the first rejected finger frame's reason and boundary bytes. [Stock event reader](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/blob/52a2f180865f8368828985d6384130acd093fa93/drivers/input/touchscreen/nt36523/nt36xxx.c#L1564-L1701)

Run84 uses the same build command with `OUT="$BASE/run84-touch-event.3eD31w/image"`. The 67,756,032-byte `boot.img` has SHA-256 `030a0b15352438c11c7345d3bdf8e2b82fe0a4e016e7dc36df7acecbadb6e777`. The host touch harness passed clock, pen-tail isolation, finger checksum, ID, bounds, movement and release checks. The GENI register-model test, FAT readback and linked HID audit passed.

Run84 RAM-booted at 20:42:59 CST, reached Hello and returned to verified bootloader fastboot at 20:50:24 CST. No gesture was performed during the run. The recording and USB identity checks are in `run84-touch-event.3eD31w/image/device-attempt.lv6Z06/` under `BASE`.

The enlarged firmware viewer recovered `stage=7 reads=0 SPI-errors=0 bad=0 down=0 move=0 up=0 HID=0`, `IRQ edges=0`, and `elish-touch-hid: stage=5` with `match=1`. The viewer showed those counters in `run84-viewer-large.Scq70d/viewer-10.jpg` and returned to verified bootloader fastboot. Its 634,880-byte image at `run84-viewer-large.Scq70d/image/boot.img` has SHA-256 `8e1d63c439522323bc725d8c9e3d994c2b05cb82d7cab2496a8e323e66e95492` and zero ramdisk bytes. The idle run verifies startup and diagnostic readback. GPIO39 and frame handling under physical contact still require a gesture run with the same image.
