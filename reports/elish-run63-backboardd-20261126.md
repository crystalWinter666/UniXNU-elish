# elish run63 backboardd process

The 185-second snapshot shows `Data APFS rw=1`, `launchd=1 main=1`, `backboardd=57` and `SpringBoard=0 Setup=0`. The screen crop cuts off the full device name and Data UUID. The kernel uses the byte-identical run62 check for the writable RamBlock Data volume `7D443C28-95AC-49F8-AF25-4FC12FBC93E5`. The camera captured the loader marks before the snapshot and showed no native UI. Touch, Setup completion and Data persistence remain untested. [Snapshot photo](elish-run63-backboardd-20261126.jpg)

## Build

The [run61 source hashes and revisions](elish-run61-userspace-snapshot-20261126.md#build-and-readback) apply. Starting with `run62-earlier-snapshot/payload.img`, replace `EFI/UniXNU/unixnu.toml` with the same file after removing `-v` and setting `elish-userspace-snapshot=185`. Verify the config and byte equality of the kernel, ADT, RootPolicies, Apple trust cache and `scratch-data.img` by FAT readback. `fsck.fat -vn`, `scripts/check-elish.py --static-only` and `porting-record.py check --device .` passed at build time.

```sh
CORE=/root/elish-build-20261123/UniXNU
OUT=$CORE/out/elish-darwinboot-20261125/run63-graphics-console
UEFI=/root/repo/uefi-elish
PATH=/usr/lib/llvm-21/bin:$PATH python3 "$CORE/unixnu" bootimg -D "$PWD" --os ipados-17.7.11-21H461 \
  --uefi "$UEFI" --fd "$UEFI/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd" \
  --dtb reports/elish-recovery-fdt-20261124.dtb --ramdisk "$OUT/payload.img" --output "$OUT/boot.img"
```

## Tablet and camera

The host verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` before `fastboot -s 4bed07e4 boot "$OUT/boot.img"`. The command completed in 1.805 s, with all identity checks passing on fresh USB enumeration 207.84 s later. This run used RAM-only boot and issued no persistent partition writes.

The transcript and 22 H.264 segments are in `$OUT/device-attempt.CqKJ8T/`. `camera.sha256` records the whole set. The snapshot is visible around 4 s into `camera-020.mkv`.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `810e53eceb4de2220ac93650edd289efc386784d84ae2d09bff346e0f599479d` |
| `payload.img` | `0949c1866cfdae5e5f23e934de457d9cc28156cb3e223bd85e0d7884b0405382` |
| `camera-020.mkv` | `891039c0c74ca3b0d216cffe6918d867621fc59054fb78c2c4a35056822a9605` |
| `elish-run63-backboardd-20261126.jpg` | `223a7fa1a8804d6d5a4b43c58a7984b031e891be85fc6f4ee11358cfca20e7ef` |
