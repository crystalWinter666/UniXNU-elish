# elish run56 System APFS boot and Apple launchd

Run56 booted iPadOS 17.7.11 (`21H461`) from the installed UFS System APFS volume on `elish`. The camera recorded `elish System root confirmed` and `PID 1 main: launchd` at 84 seconds. The image carried no Restore ramdisk or Data seed. [Framebuffer capture](elish-run56-system-launchd-20261126.jpg)

| Mounted volume | APFS UUID | BSD device |
| --- | --- | --- |
| System root | `04769D26-2CA0-4825-B443-E2E5F88300F2` | `/dev/disk3s1` |
| SystemOS union over `/` | `8846FE78-2910-4297-AF97-D59BB6F8FC16` | `/dev/disk1s1` |

The checkpoint fires on PID 1's successful write of `kern.initproc_spawned=1`. It follows the live SystemOS union mount to its covered System vnode and checks both mounted devices against read-only APFS media backed by the authenticated elish UFS windows. The camera crops the end of the UUID line. The `confirmed` verdict requires an exact match to the complete System UUID in the table. The checkpoint deliberately stops startup to display the result and then warm-restarts into bootloader fastboot.

## Build and device evidence

`OUT` is `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run56-verified-systemos-union`. Source baselines are device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139` and XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`. `source.sha256` records the local build inputs.

Use the [run55 build inputs](elish-run55-verified-system-handoff-20261126.md), including the same firmware, Apple trust cache and `RootPolicies` bundle. Refresh `elish_defconfig` with `unixnu defconfig elish_defconfig -D . --os ipados-17.7.11-21H461` to enable `CONFIG_XNU_OWNER_UNION_SYSTEMOS`. Build with `unixnu build kernel -D . --os ipados-17.7.11-21H461 -j 12`. Set the payload override `--kernel-arg elish-ram-only=150` and retain `elish-ufs-root=1`, `elish-warm-reset=1` and `elish-launchd-checkpoint=1`. XNU compilation, prelink, both 190-slot storage ABI checks, digest bundle and trust cache readback, FAT validation and Android v3 packaging passed.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `11919eb7908ddec029480e248851041cfe9dc148d8438a470e98eca0cd61466e` |
| `payload.img` | `dcae397483e4e3b54d0a4ba48c510e647816a061b7523a83db8f3e932349bbd6` |
| `kernel.macho` from FAT | `b36c7dee87142011f4da63f5c5e83d9ccde69fdd543d7f3ed4dff79b45a0e1f4` |
| `kernel.macho.adt` | `211260cb10b90ea68623f36a1d443028547822aa058221ce08905d400eecc0a0` |
| `root-digests.bin` | `d33ea5be6ec15c3acb1c88940687f8d21859b1903f2d9705e956be164b460066` |
| `SILICIUM_UEFI.fd` | `fda4d664b23a58f28da7f567ff3b74fe6a54619c6649457fc29e47f4d867f55d` |
| `device-attempt.2y9oDv/camera.mkv` | `0db7baad2358480f9d6c8fd68ef9ac05e3b9cdebf4e8943bd2e7de15659c2570` |
| `elish-run56-system-launchd-20261126.jpg` | `bbcf5d2e187ff9aabcc0d12c46efa1d69f93692ae91a04f6053aa18530d067b4` |

`device-attempt.2y9oDv/device.log` records USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no`. The preflight verified the image hash. `fastboot -s 4bed07e4 boot "$OUT/boot.img"` returned both `OKAY` results in 1.811 s. The tablet re-enumerated in bootloader fastboot 88 seconds after the command completed. The boot was RAM-only and issued no persistent partition writes.

## Setup and Data

| Check | Result | Prerequisite |
| --- | --- | --- |
| Setup Assistant | Untested | An unconfigured writable target Data seed and a boot that continues past the PID 1 checkpoint. |
| Data persistence | Untested | A full verified backup and rollback preflight for the candidate UFS Data window, write authorization for the exact range, and the elish write path. [Run54 boundary reads and backup plan](elish-run54-ufs-data-readonly-20261126.md) |
