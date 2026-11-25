# elish run12 post-MMU console checkpoint

Device: Xiaomi Pad 5 Pro Wi-Fi (M2105K81AC / elish), target iPadOS 17.7.11 (21H461).

## Image and expected checkpoint

The RAM-only diagnostic image is `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run12-postmmu-console/boot.img`, 67,756,032 bytes, SHA-256 `49460e8b7aabf22dc6e921e3435ab47811c9dbd4cfe27b1880a5858d786f324a`. Its FAT payload is SHA-256 `b35392da63b9448093c497cfa7395704dfbcdf8fe554a5dc79c1a9c1a6d180f6`; the patched kernel is SHA-256 `5678c2cce3e27df5308ea34936583c5d76ea78c4c84fdd3a6d7c30b6e6a20f42`. The package manifest records FD SHA-256 `2f23213f3e5f3ebc3706c58de58339558a74b61a48a84e8d7f047dddb62755d2` and DTB SHA-256 `c1479901a76f6fc8c6f6cb39d0a5b57f0ce39598880dcbfe34956145ec368ce8`.

The kernel patch replaces the `_PE_init_printf` call in `_arm_init`. It calls the original initializer, prints `ELISH_POST_MMU_VM_INIT_CONSOLE_READY`, waits 15 seconds using `cntpct_el0`, and requests PSCI SYSTEM_RESET. The hook is after the `_arm_vm_init` call, console creation, and printf initialization. Build and packaging hashes are recorded in `run12-postmmu-console/boot.json` and `kernel-post-mmu.macho.post-mmu.json`.

## Device result

The user photo `/root/repo/86D644D8053EE3CF1BB0D8414E2EBB4D.png` (SHA-256 `51420af2d0b488f94776c6e19711e238af87f798a7ddeac91d927f53739aa2d4`) shows the green handoff, blue XNU-entry, and magenta pre-MMU bands. The visible lower text remains loader/capsule firmware output; the post-MMU marker is not visible. This directly confirms execution through the existing pre-MMU marker, not MMU initialization completion.

For a timed repeat, Android was sent to bootloader and fastboot 37.0.3 confirmed `product=elish`, `is-userspace=no`, and active slot `b`. `fastboot boot` accepted run12 with `Sending OKAY` and `Booting OKAY` in 1.760 seconds. The host's fastboot polling detected the device again about 6.9 seconds after the boot command returned. The reader Android reports `arch_timer` at 19.20 MHz, matching the checkpoint patch's 19,200,000 Hz basis. The observed return was earlier than the patch's 15-second hold, so this repeat does not support execution of the post-MMU checkpoint. Timing cannot identify the exact failing instruction or rule out a fault in the checkpoint path itself.

A 25.021-second, 640x480, 5 fps overhead-camera capture is `/tmp/elish-run12-retest-20261125.mkv`, SHA-256 `c01c3c7582fbe7a7e48b4d152ddd3bc07864590e97b5d6f4c8467bd9baffd52c`. It shows only a dark screen outline and cannot resolve text or color markers.

## Readout and current state

The hash-verified LineageOS reader image `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img` (SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`) was then started RAM-only. Fastboot reported `Sending OKAY` and `Booting OKAY`; root ADB returned as `lineage_elish` / `elish`. The reader's `sys.boot.reason` is `bootloader`.

`/sys/fs/pstore` is empty. The kernel log reports ramoops at `0xb0000000+0x400000`, `record_size=0`, and `Invalid compression size for deflate: 0`. No checkpoint was recovered. The empty directory is not proof that the XNU checkpoint failed to execute because the ramoops/pstore readout path remains unresolved.

The device is currently powered on in root ADB. No device partition was written and `fastboot continue` was not used. XNU execution to the pre-MMU marker is confirmed; `_arm_vm_init` completion, UFS/SMMU operation, APFS root mounting, and iPadOS userspace remain unconfirmed.
