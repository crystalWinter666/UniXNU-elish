# elish UEFI host build, 2026-11-24

## Inputs

- Device repository worktree: `/root/repo/UniXNU-elish`, based on device HEAD `208b47d20e87d3876fd862bb1f0ee0c17467b0c2` with local changes.
- UniXNU core: `/root/elish-build-20261123/UniXNU`, revision `71c5c6569bae6369c0e0f634df8b9bd5afc19139` with local changes.
- Mu-Silicium source: `/root/repo/uefi-elish`, revision `66421cb7bb5694843aec8de1ca3d1de4c20cca38` with the elish package uncommitted.
- Mu_Basecore: revision `cfb93724a06ff86ff5ba971413368a7809ee42a5`.
- Device-Binaries submodule: `a1501e0c2e642d6ac0d8f52c85af2a67b75a7799`.
- Host: Debian Linux, EDK II toolchain `CLANGPDB` using clang 21.1.8.

Mu_Basecore nested submodules were initialized at the gitlinks recorded by its pinned revision. This included Brotli (`e230f474b87134e8c6c85b630084c612057f253e`), libfdt (`cfff805481bdea27f900c32698171286542b8d3c`), Oniguruma (`4ef89209a239c1aea328cf13c05a2807e5c146d1`), MipiSysT (`370b5944c046bab043dd8b133727b2135af7747a`) and libspdm (`f55cf6d48ec69b4ac60a63903e9c6a2cb0fd155d`). The pinned Cmocka, GoogleTest and Subhook submodules were also initialized.

## Build

BaseTools initially stopped because the Brotli submodule was absent. After initializing the pinned dependencies, this completed successfully:

```sh
make -C BaseTools/Source/C -j8
```

The elish firmware build completed with:

```sh
./unixnu build firmware -D /root/repo/UniXNU-elish \
  --uefi /root/repo/uefi-elish \
  --edk2 /root/repo/uefi-elish/Mu_Basecore -j 8
```

The output is `/root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd`, 3,145,728 bytes, SHA-256 `f8f662313c0ffee611a2e55f687486b1625c9e80a983d9280f26f46f5dbdbf74`. The EDK build compiled UniXNU, UniXNUDxe, DarwinBoot, UniXNURamDiskDxe, DiskIoDxe and FAT. The build emitted one warning that `ArmPsciResetSystemLib.inf` does not support the `HwResetSystemLib` class. The complete log is in the core output at `out/elish/ipados-17.7.11-21H461/elish/logs/firmware.log`.

The boot-image packer then produced an Android header-v3 firmware-only probe with the device configuration's gzip kernel compression and no appended DTB. It has an empty ramdisk, passed the builder's unpack-and-compare verification, and is not an iPadOS image. Output `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/elish-uefi-probe.img`, 618,496 bytes, SHA-256 `8b08c449b4af3a4ab13c07389a789c170a83d6c373b68dfe76dc959224fef349`. Its report records `hardware_tested=false`.

## Remaining evidence

The diagnostic image SHA-256 was rechecked as `b2ccb5968d7cb30a462bf0f1c6deca84167ccb10c7f7a36782e07559c0fbf6d6`, then sent with `fastboot -s ELISH_SERIAL boot`. Fastboot returned `Sending 'boot.img' ... OKAY` and `Booting OKAY`. The target then disappeared from ADB, fastboot and USB enumeration and remained absent after the 30-second display hold. The host cannot observe whether `ElishMemoryMapProbeDxe` reached its callback or read its framebuffer output, so no RAM Partition entries or UEFI descriptors have yet been captured. No partition-write command was run. A screen capture or transcription is needed to interpret the probe output.

The Android kernel does not list debugfs in `/proc/filesystems` and exports no UEFI memory map or RAM Partition table. The final UEFI `GetMemoryMap` and XNU ownership therefore remain unverified. The iPadOS kernel candidate is separately marked `loadable=false`; no payload or iPadOS boot image has been produced.

## Instrumented memory-map probe

The elish package now includes an optional `ElishMemoryMapProbeDxe`, enabled
for this diagnostic build by `[firmware.defines].ELISH_MEMORY_MAP_PROBE`. At
`ReadyToBoot`, it reads the Qualcomm RAM Partition protocol and the current UEFI
memory descriptors overlapping the two DRAM banks measured through root ADB.
It prints both tables through the framebuffer debug logger. The first build
held the screen for 30 seconds. The extended build now holds for 120 seconds.
The snapshot is taken before UniXNU starts and does not represent the later
`ExitBootServices` map.

The build command was:

```sh
./unixnu build firmware -D /root/repo/UniXNU-elish \
  --uefi /root/repo/uefi-elish \
  --edk2 /root/repo/uefi-elish/Mu_Basecore -j 8
```

The instrumented FD remains 3,145,728 bytes and has SHA-256
`8f797816d1b3ac59338c248a7ed17442d4f3878e5abc2fae80314811d6967548`. The EDK
log confirms that `ElishMemoryMapProbeDxe.inf` compiled into the firmware. The
second DRAM bank endpoint is `0x280000000`, as measured in the live FDT. An
earlier diagnostic build used an incorrect endpoint calculation, so its image
hashes are superseded. The same non-fatal `ArmPsciResetSystemLib.inf` warning remains.

The Android v3 diagnostic image is
`/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/elish-memory-map-probe.img`.
It is 622,592 bytes with SHA-256
`b2ccb5968d7cb30a462bf0f1c6deca84167ccb10c7f7a36782e07559c0fbf6d6`. Its
header is version 3, the ramdisk is empty, and `hardware_tested` is false. The
boot-image round-trip tests and the porting-record and static scaffold checks
pass. The 30-second image was accepted by `fastboot boot`, but the target
disappeared from USB enumeration and no framebuffer output was captured. A new
image with a 120-second hold has not yet been booted. It is 622,592 bytes with
SHA-256 `f312ddfa2adacd2a48ef3d09e313765b668b1affe7e710426b3e3411fcd1e650`;
its UEFI FD is 3,145,728 bytes with SHA-256
`063614fdc3de0d2e7ab7a26bb7856c684fa4d4c45e95dfe83e8065fe31d1b2b1`. The new
image retains Android header v3 and an empty ramdisk. `hardware_tested` remains
false, and no RAM Partition or runtime memory-map result has been recorded.
The 120-second image is saved separately as
`/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/elish-memory-map-probe-120s.img`;
its current on-disk SHA-256 was rechecked as
`f312ddfa2adacd2a48ef3d09e313765b668b1affe7e710426b3e3411fcd1e650`.

## Host check after crash recovery

The user indicated root/recovery ADB access. At the next host check,
`adb devices -l`, `fastboot devices -l` and `lsusb` showed no elish target, so no
new device read or boot command was run. After temporarily creating the
configured sibling symlink `/root/repo/UniXNU ->
/root/elish-build-20261123/UniXNU`, this completed successfully:

```sh
./scripts/package.sh --check --os ipados-17.7.11-21H461
```

It resolved dependencies, the pinned IPSW inputs, `elish_defconfig` and all 62
patches. It did not build a payload or final image. These checks also passed:

```sh
python3 scripts/check-elish.py --static-only
git diff --check
python3 -m py_compile scripts/check-elish.py
python3 scripts/porting-record.py check --device /root/repo/UniXNU-elish
```

The static scaffold check still reports that measured DRAM and reserved ranges
are not reconciled with UEFI/XNU ownership. The generated, ignored
`unixnu.local.toml` records the local device, firmware, EDK II, IPSW,
kernelcache, FDT, candidate-kernel and FD paths. It has no platform seed. The
kernel candidate remains marked `loadable=false`, and the UFS/SMMU and APFS
device-side path remain unverified.
