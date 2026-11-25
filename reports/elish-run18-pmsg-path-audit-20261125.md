# elish run18 pmsg physical-write path audit, 2026-11-25

This is a source and evidence audit only. No tablet was enumerated, no image
was started, and no device partition was changed.

## Physical range and write path

The run06/run11 DarwinBoot log reports the XNU window at physical
`0xc007c000` with size `0x10000000` (256 MiB). The 4 MiB ramoops reservation
is `0xb0000000..0xb0400000`; the pmsg area is `0xb0200000..0xb0400000`, and
run18's marker is at `0xb03ff000`. Thus the marker is below the XNU-managed
window, not an address that can use its normal `phystokv` mapping.

Pinned XNU feed revision `d8b80295118ef25ac3a784134bcf95cd8e88109f`
(`xnu-10063.141.1`) supports the intended fallback in source:

- `arm_vm_init()` initializes `gPhysBase` from `boot_args.physBase` and its
  physical span from `boot_args.memSize` (`osfmk/arm64/arm_vm_init.c:1833-1842`).
- `vm_first_phys` and `vm_last_phys` delimit kernel-managed pages;
  `pa_valid()` rejects addresses outside that interval
  (`osfmk/arm/pmap/pmap_data.h:58-78`).
- `_ml_phys_write` defaults to the CPU copy-window path and only uses
  `phystokv()` when `pmap_valid_address()` accepts the address
  (`osfmk/arm64/loose_ends.c:521-564`).
- For an unmanaged page, `pmap_cache_attributes()` returns a matching I/O
  range's attributes or `VM_WIMG_IO` when none is registered
  (`osfmk/arm/pmap/pmap.c:10428-10453`). The bootloader map records ramoops as
  reserved and uncached (run06 log, map entry 72 at `0xb0000000`, 4 MiB).

This establishes a plausible non-cacheable copy-window path for the marker;
it does not establish that the platform permits the runtime mapping or that
the write reaches retained RAM. The existing UEFI readback image remains the
test for that. A valid marker after warm reset would directly establish that
the diagnostic reached the code after `_arm_vm_init()` and that pmsg retained
the write; an absent marker alone would not distinguish an earlier kernel
fault, mapping failure, or reset-retention behavior.

## Binary verification

The diagnostic patch was regenerated to `/tmp/elish-run18-post-vm-pmsg-reproduced.macho`
from the run12 entry-bars input. Its SHA-256 exactly matches the existing
run18 kernel: `8f0fcd60069e14672bb71ec633c74b9b02f53870daafc5c8188c507bae146ee5`.
`llvm-objdump` confirms the `_arm_init` call at `0xfffffff0079e49a0` branches to
the cave at `0xfffffff00741ab20`; the cave calls `_arm_vm_init` at
`0xfffffff007411c94`, then `_ml_phys_write` twice at `0xfffffff0074155f4`,
followed by `dsb sy`, a 288,000,000-tick hold (15 seconds at the configured
19.2 MHz), and the PSCI system-reset request. The disassembly verifies the
probe layout and branch targets, not hardware execution.

## Readback does not overwrite pmsg

The UEFI ramoops logger owns only the 2 MiB console zone at
`0xb0000000..0xb0200000`. Its `ResetHeader()` and ring writes stay within that
zone (`uefi-elish/Platforms/Xiaomi/elishPkg/Library/RamoopsLogLib/RamoopsLogLib.c:20-24,81-114`).
The pmsg probe reads `0xb0200000..0xb0400000` and the marker at `0xb03ff000`,
then emits its text with `RamLogWrite()` to the console zone
(`uefi-elish/Platforms/Xiaomi/elishPkg/Drivers/ElishLoaderProbeDxe/ElishLoaderProbeDxe.c:309-344`).
Thus the diagnostic readback path itself does not overwrite the marker. This
is a source-level range check; actual retention across the warm reset remains
unverified.

## 2026-11-25 live result

The run18 post-VM marker image and pmsg-readback image were both RAM-booted on
elish. The readback log reports marker magic `0x00004000`, complement
`0x00000000`, and `valid=0`; it does not prove that `_arm_vm_init()` returned.
The pmsg readback console log is archived at
`reports/elish-run18-pmsg-readback-console-ramoops-20261125.txt`, SHA-256
`a2d7e0c084aa13572dc9dc9ff51faf1593f829f1ddab8716426154137b3c3627`.

The SEC entry's `RamLogReset()` calls `ResetHeader()` for the console ring at
`0xb0000000` only. Its writes remain within `0xb0000000..0xb0200000`; it does
not clear the pmsg area at `0xb0200000..0xb0400000`. This removes UEFI console
initialization as an explanation for the absent marker. The remaining
possibilities include XNU stopping before the write site or the physical write
path not reaching retained pmsg memory. The full run18/run19 device record is
in `reports/elish-run18-run19-device-20261125.md`.

## Current test inputs and evidence

- Standard iPadOS 17.7.11 candidate: `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`, SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`. Not started this round.
- run18 post-VM marker image: `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image/boot.img`, SHA-256 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`. Not started this round.
- run18 UEFI pmsg readback image: `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image/boot.img`, SHA-256 `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`. Not started this round.
- Initial USB snapshot had no `18d1:d00d` tablet, no fastboot device and no ADB device. No fastboot or ADB protocol request was sent. The V4L2 screen frames at `/tmp/elish-live-state-current.jpg` (SHA-256 `114095894845ac0b12cd0859755511626bba61f5510434449cdb2b1933c5be00`) and `/tmp/elish-live-state-brightness-max.jpg` (SHA-256 `0ac008dfeb4bb5d4ac0833cb7c07962e2c69745b354a2181e6cc15dde03c6ade`) are too dark to identify the screen. Camera brightness was restored to 128; no device state is inferred from these frames.
- Follow-up USB snapshot still had no tablet in fastboot or ADB. A third maximum-brightness V4L2 frame, `/tmp/elish-live-state-1033.jpg` (SHA-256 `aa2c8ca152154b33962aa935e58b7e7ba9ca8b20b3348fee8d17538991cb30ed`), is also unreadable; camera brightness was restored to 128.
- Latest direct XNU stage remains the run09 close photo showing entry and pre-MMU bands. The archived run06/run11 console logs directly confirm FAT payload access, DarwinBoot, the 256 MiB window and successful `ExitBootServices()`; run11 explicitly stops before XNU.
- `scripts/patch-xnu-post-mmu-checkpoint.py` reproduced the run18 kernel byte-for-byte in `/tmp`; disassembly confirmed the hook branch, `_arm_vm_init` call, both `_ml_phys_write` calls, 15-second timer and PSCI reset. This is host verification only.

No new boot stage or device log was produced. `_arm_vm_init` completion, UFS/SMMU handoff, APFS root mounting and iPadOS userspace remain unverified. Once the tablet enumerates and fastboot responds, run the existing RAM-only post-VM image, then the readback image, and recover the UEFI console from the root-ADB reader. Do not issue a poweroff command or choose the UniXNU menu's return-to-system entry.
