# elish run20 and run21 boot evidence

Run21 reached BSD root device discovery and panicked at `IOKitBSDInit.cpp:984`. The photo records `Failed to mount root device`. The four frame addresses map to `_IOFindBSDRoot`, `_bsd_init`, `_kernel_bootstrap_thread`, and `_Call_continuation` in the kernel extracted from the boot image. The VM initialization and console setup have returned by this point. [Device photo](elish-run21-root-panic-20261125.jpg)

## Inputs and commands

- Device M2105K81AC, bootloader fastboot serial `4bed07e4`. The host ran `fastboot -s 4bed07e4 getvar product` and `fastboot -s 4bed07e4 getvar is-userspace`, which returned `elish` and `no`.
- Device source baseline `24de4f60e84762378ce9b1e906b86827b1b2121c`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, and elish firmware `d489a050255f90b9c011d25b95c15c1b12f382f0`, with local build changes. OS profile `ipados-17.7.11-21H461`.
- Host command `fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run21-4g-xnu/boot.img`. `Sending OKAY` and `Booting OKAY` completed in 1.760 s. After USB disconnected, the host did not observe either protocol return during the 22.5 s polling window. The tablet later reappeared in bootloader fastboot.
- Boot image SHA-256 `1257f0924fdae4425bcbffbf855fcf75e7e6bf330effafbc335adf2f7bcbdd9a`. Firmware SHA-256 `ef645c8c18e0befd00f975ef31d168da20c2720ee6eea598d82e3478aad6ebdb`. Photo SHA-256 `ebad5804236f2283706d8bf4e52a7a55fd6375232ec44239c573e88c3c10b8c2`.
- The host unpacked the Android v3 ramdisk, extracted `EFI/UniXNU/Payloads/kernel.macho` with `mcopy`, and resolved the photo frames with `llvm-nm-21 -n`. FAT SHA-256 `a954fc5950428d593cd5c3d1991e964c6649ee76667bad24f6424c4e85d01bd6` matches run20. Kernel SHA-256 `65bd7cad957dccf6db024e8aad0614e41ef6823e9da187ff218aecbb12f7f603`. The FAT menu has no `initrd` entry.

## Memory and root observations

Run20's firmware stopped after `ExitBootServices()` and recovered the accepted memory map through ramoops. DarwinBoot selected `0xc007c000+0x100000000` (4 GiB), with `0x8040000` bytes of loader code and no excluded ranges. Map SHA-256 `62147b36f53db9a488142584e0467f14a7516312e4213d5a22e4e493b538245a`. Logs and image manifests are in the run20 and run21 build directories alongside `boot.img`.

In IOKitBSDInit, the panic covers a root media match timeout or the tenth mount attempt. The image matches the target APFS System volume UUID but has no root ramdisk or native UFS transport. APFS root mounting, PID 1, and the full iPadOS System volume remain unverified. The test was RAM-only. The device's advertised fastboot download ceiling is 805,306,368 bytes.

The 21H461 IPSW includes `044-75545-038.dmg`, whose Image4 payload is a 176 MiB raw APFS container with volume UUID `9071148B-B025-402D-AD7A-48464D8B5FFF`. The DarwinBoot sparse packer verified full reconstruction of its 137,445,888-byte output. The paired restore trust cache is version 1. This image can exercise Apple restore user space through RamBlock while full System-root transport is developed.
