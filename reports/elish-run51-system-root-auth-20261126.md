# elish run51 installed System root authentication

Recovery on slot `_b` verified `elish`, serial `4bed07e4`, and that `/data` and `/metadata` were offline. The authorized install wrote the 9,382,658,048-byte 21H461 image to LUN 0 LBA 6,619,136–8,909,823. Partition-node readback and an independent cache-cleared whole-LUN readback both returned SHA-256 `fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093`. Recovery also verified LBA 1–5, 61,880,315–61,880,319, and 8,909,824 against the pre-write captures. The original prefix backup and its rollback procedure are in the [run50 record](elish-run50-xnu-system-checkpoint-20261126.md#offline-installation-and-rollback-preflight).

The host verified USB `18d1:d00d`, serial `4bed07e4`, product `elish`, and `is-userspace=no`. `fastboot -s 4bed07e4 boot "$OUT/boot.img"` accepted the RAM-only image in 1.813 seconds. The image is byte-identical to run50 and selected IOMedia UUID `04769D26-2CA0-4825-B443-E2E5F88300F2` with no Restore ramdisk. At 16 seconds the display showed `rootvp not authenticated after mounting @bsd_init.c:1100`. That line executes after the root mount and `VFS_ROOT` succeed. The running mount's device and UUID did not appear on screen. Apple `launchd` PID 1 had not reached its checkpoint.

USB returned to bootloader fastboot about 20 seconds after the boot command. `fastboot -s 4bed07e4 reboot recovery` returned `OKAY`. Root Recovery ADB identified `lineage_elish` and showed an empty `/sys/fs/pstore`. Setup and Data persistence were untested.

`OUT` is `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run51-xnu-system-installed`. It contains `device.log`, `recovery.log`, `repack.log`, `boot.json`, and `camera.mkv`. Source baselines and local build inputs match [run50](elish-run50-xnu-system-checkpoint-20261126.md#build-and-artifacts).

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `a97b762d32dc5da6fc55c325536432503ffa7967f647db7aff284dc448cbbedd` |
| `SILICIUM_UEFI.fd` | `fda4d664b23a58f28da7f567ff3b74fe6a54619c6649457fc29e47f4d867f55d` |
| `camera.mkv` | `5d2da1f9f1aee61ff0a857fc7b308a236b34646936740284ca094e290f25ad9b` |
| [16-second photo](elish-run51-root-auth-panic-20261126.jpg) | `e7ee39cad59da5e3775e1b8de2c3bae1354f7c7467fbbcf3102294bd63f4036a` |
