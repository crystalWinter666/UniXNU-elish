# elish run18 post-VM pmsg checkpoint, 2026-11-25

Device: Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC` / `elish`), target iPadOS 17.7.11
(`21H461`). Both images below are host-built RAM-only diagnostics. Neither was
booted in this session, and no persistent device partition was written.

## Post-VM image

The input is the run12 kernel with the blue XNU-entry and magenta pre-MMU
bands. A separate 128-byte code cave replaces the `_arm_vm_init` call in
`_arm_init`, calls the original function, then writes two words with exported
`_ml_phys_write` to `0xb03ff000`: magic `0x4d564c45` and complement
`0xb2a9b3ba`. The marker is in the final page of the reserved 2 MiB pmsg area.
The probe waits 15 seconds and requests PSCI `SYSTEM_RESET`. Its code cave is
at `0xfffffff00741ab20`, 0x500 bytes after the stage-4 cave base; the older
entry-pmsg probe uses the non-overlapping +0x100 offset.

The pinned XNU source checks `pa_valid()` before using `phystokv()`.
`0xb03ff000` is outside the run06 XNU physical window beginning at
`0xc007c000`, so `_ml_phys_write` uses the CPU copy-window path. For
non-managed pages, `pmap_cache_attributes()` selects the I/O attribute when no
more specific range exists; the UEFI memory map independently classifies the
4 MiB RAMOOPS reservation as uncached. This source audit supports readback
across warm reset but is not runtime proof that the marker was written.

The patched kernel SHA-256 is
`8f0fcd60069e14672bb71ec633c74b9b02f53870daafc5c8188c507bae146ee5`.
The 64 MiB FAT payload SHA-256 is
`d6ff20e31c4b375ea15192c7c718c595d3de2bb8310596b6647ccc5acf27c0ce`;
the kernel copied back out of that payload compared byte-for-byte with the
patched input. `fsck.fat -vn` found no filesystem errors.

The Android v3 diagnostic image is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image/boot.img`,
67,751,936 bytes, SHA-256
`842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`.
Its 3 MiB UEFI FD SHA-256 is
`ef645c8c18e0befd00f975ef31d168da20c2720ee6eea598d82e3478aad6ebdb`.

## Pmsg readback image

The opt-in DEBUG-only `ElishLoaderProbeDxe` readback mode logs the pmsg
signature/start/size, bytes 0-47, and the two marker words before entering
DarwinBoot. The existing EBS interposer then records the accepted map and
restarts without entering XNU. The mode defaults off in the normal board
configuration.

The readback image reuses the standard 21H461 payload (SHA-256
`9aaca2b53648d2dd8bb9ae7f362b8069c951e1f6760c048f8b8b18e45b5f3821`) and DTB
(SHA-256 `c1479901a76f6fc8c6f6cb39d0a5b57f0ce39598880dcbfe34956145ec368ce8`).
Its path is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image/boot.img`,
67,751,936 bytes, SHA-256
`d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`.
Its 3 MiB UEFI FD SHA-256 is
`e8f34ecc4758ba38d820965ca6e6e3ca526645986f90c01d05c7229bd64e3ae8`.
The compiled driver contains all three `ELISH_PMSG_READBACK` format strings.

## Build and verification

Source HEADs used were device repository
`4b3da68ea0aca815a80e8d1bad802dfec8d5c905`, UEFI repository
`f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`, and UniXNU core
`71c5c6569bae6369c0e0f634df8b9bd5afc19139`. The diagnostic changes were
uncommitted working-tree changes in the device and UEFI repositories.

```sh
python3 scripts/patch-xnu-post-mmu-checkpoint.py \
  /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run12-postmmu-console/kernel-entry-bars.macho \
  /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/kernel-post-vm-pmsg.macho \
  --after-vm-init --pmsg-return-marker --hold-seconds 15

cp /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run12-postmmu-console/payload.img \
  /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/payload.img
mcopy -o -i /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/payload.img \
  /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/kernel-post-vm-pmsg.macho \
  ::/EFI/UniXNU/Payloads/kernel.macho
mcopy -i /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/payload.img \
  ::/EFI/UniXNU/Payloads/kernel.macho \
  /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/kernel-readback.macho
cmp /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/kernel-post-vm-pmsg.macho \
  /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/kernel-readback.macho

python3 scripts/build-loader-probe.py \
  --unixnu /root/repo/UniXNU --uefi /root/repo/uefi-elish \
  --payload /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg/payload.img \
  --dtb /root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/device-tree/hardware.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image \
  --feeds /root/elish-build-20261123/UniXNU/out/elish-resume/feeds \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --run-kernel

python3 scripts/build-loader-probe.py \
  --unixnu /root/repo/UniXNU --uefi /root/repo/uefi-elish \
  --payload /root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/payload/payload.img \
  --dtb /root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/device-tree/hardware.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image \
  --feeds /root/elish-build-20261123/UniXNU/out/elish-resume/feeds \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --pmsg-readback
```

Python syntax checks, both firmware builds, `test_bootimg.py` (1 test),
`test_boot_template.py` (13 tests), `check-elish.py --static-only`, the
14-area porting-record check, and `git diff --check` passed. These are host
results only.

## Device status and next test

At the start of this session `lsusb -d 18d1:d00d`, fastboot `devices -l`, and
ADB listed no tablet. Per the device-probing instruction, no fastboot request,
boot, restart, or further USB probe was sent. The latest direct XNU stage
remains the run09 photograph showing the entry and pre-MMU bands; run06/run11
remain the direct FAT/DarwinBoot/256 MiB/EBS log evidence. No new stage was
confirmed. `_arm_vm_init` return, UFS/SMMU ownership, device APFS root mount,
and iPadOS userspace remain unverified.

When the device enumerates again, verify fastboot responds and reports
`product=elish`, RAM-only boot run18, then RAM-only boot the pmsg-readback
image and export `console-ramoops-0` with the verified root-ADB reader. Do not
select the UniXNU menu's return-to-system action. The readback is diagnostic
and reads the marker on the final pmsg page without writing to pmsg; its log
output is confined to the separate console ring. It does not modify a
persistent device partition.
