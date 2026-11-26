# elish run54 read-only Data window probe

`elish-data-probe-blocks=786457` selects a candidate LUN 0 range for a 3 GiB logical Data disk. RamBlock's format adds one header block and 24 bitmap blocks to 786,432 data blocks. The probe places the range below UniXNU's 1 GiB log reserve and above the installed 21H461 prefix. Each native block is 4 KiB.

| Region | Inclusive LBA range |
| --- | --- |
| Installed 21H461 prefix | 6,619,136–8,909,823 |
| Candidate Data store | 60,821,714–61,608,170 |
| Log reserve | 61,608,171–61,870,314 |

XNU checks live capacity, both run40 GPT CRCs and the installed SHNGROOT manifest and APFS headers before sampling the first and last Data blocks. It reports their CRC32 values, request-list stop and SMMU fault state, then stops at the panic checkpoint. Compare both CRCs with independent read-only Recovery reads of the same LBAs. The command path uses NOP, READ CAPACITY(10) and READ(10). This candidate contains no Data seed. The window size is a probe input and the range has no write authorization.

## Backup and rollback preflight

1. After the run54 checkpoint, use authorized Recovery ADB to confirm serial `4bed07e4`, product `elish`, a 4 KiB LUN 0 block size and the GPT mapping of `userdata` to `/dev/block/sda34`. Confirm `/data` and `/metadata` are unmounted and the partition has no holders, using the [run50 checks](elish-run50-xnu-system-checkpoint-20261126.md#offline-installation-and-rollback-preflight).
2. Export exactly 786,457 blocks (3,221,327,872 bytes) to `OUT/data-window-before.img`. With `bs=4096`, the `dd` input skip is `60821714` on `/dev/block/sda` or `54202578` on `/dev/block/sda34`. Require the file length to match and verify its SHA-256 against an independent read through the other device node. Compute the first and last 4 KiB CRC32s from that backup and compare them to run54. Save both GPT captures and adjacent blocks at LBA `60821713` and `61608171` as guards.
3. Present the exact candidate range, backup path and verified hashes for approval before enabling writes. After an approved test, compare the guards and verify the same Data volume UUID and synced marker across restart and Recovery. If rollback is needed, return to the same offline Recovery state, restore the exact range from the verified backup and flush. Require a whole-range readback matching the original SHA-256 and verify both GPT captures and guards again.

Run54 completed the boundary read. The full candidate-window backup and restore preflight remain pending. The existing run51 backup covers the installed System prefix. Current diagnostics use bootloader fastboot and camera capture. Recovery requires manually enabling ADB.

## Build and device status

`OUT` is `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run54-ufs-data-readonly`. Its `source.sha256` pins local inputs over device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139` and XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`. Kernel build and prelink, both 190-slot storage ABI checks, FAT validation and Android v3 packaging passed.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `b5e5c8df09ef6e5aa5093249bf5cbe9805d96f95d24131a6f920f9033d3d5be2` |
| `kernel.macho` from FAT | `3087490862e899525a54d4917cab81f0fca263d77d10ec1fe5e88258104b7f5f` |
| `kernel.macho.adt` | `e1350b5bdf75ed95a6c1032351201ae79a45b8a7ca2e552ba4a8f1a6a17632e7` |
| `payload.img` | `0fff23a406547f1669a8d0d4e2768e1c5679f10f8202762b61c02bd1216edd5c` |

```sh
CORE=/root/elish-build-20261123/UniXNU
WORK=$CORE/out/elish/ipados-17.7.11-21H461/elish
OUT=$CORE/out/elish-darwinboot-20261125/run54-ufs-data-readonly
export UNIXNU_UEFI=/root/repo/uefi-elish PATH=/usr/lib/llvm-21/bin:$PATH
unset UNIXNU_ELISH_ROOT_IMAGE
python3 "$CORE/unixnu" build kernel -D "$PWD" --os ipados-17.7.11-21H461 -j 12
python3 "$WORK/tree/scripts/bind-elish-ram-block.py" --check "$WORK/kernel/kernel.macho"
python3 "$CORE/unixnu" build payload -D "$PWD" --os ipados-17.7.11-21H461 --uefi "$UNIXNU_UEFI" \
  --kernel-arg elish-data-probe-blocks=786457 --kernel-arg elish-ram-only=75 --kernel-arg elish-warm-reset=1
cp "$WORK/payload/payload.img" "$OUT/payload.img"
fsck.fat -vn "$OUT/payload.img"
python3 "$CORE/unixnu" bootimg -D "$PWD" --os ipados-17.7.11-21H461 --uefi "$UNIXNU_UEFI" \
  --fd "$UNIXNU_UEFI/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd" \
  --dtb reports/elish-recovery-fdt-20261124.dtb --ramdisk "$OUT/payload.img" --output "$OUT/boot.img"
```

At 05:52 CST the host verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no`. `fastboot -s 4bed07e4 boot "$OUT/boot.img"` reported `Sending OKAY` and `Booting OKAY` in 1.804 s. The camera recorded `boundary reads complete`, LBA `60821714–61608170`, `blocks=786457`, and `first CRC=c71c0011 last CRC=c71c0011`. The queue stopped and the SMMU fault state matched its initial value (`stopped=1 faults-same=1`). USB returned to bootloader fastboot.

The files under `device-attempt.lAwUwf/` include `device.log` and `camera.mkv` (SHA-256 `68ec60b818220c1d2e80c1dbaee0639061e0601d2cb5614480be8fdfd6e30d0d`). The 18-second source frame has SHA-256 `7be1a4abab2b634215e81ee81ffa000d5fec574868c93e19f4ee7e81976db77b`. The two sampled CRCs qualify the endpoint reads. Whole-window backup, Setup and Data persistence remain untested. The run issued no partition writes.
