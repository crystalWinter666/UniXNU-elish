# elish run58 RAM Data and pre-MMU checkpoint

Run58 booted the 21H461 candidate with a 1 GiB writable Data seed. The magenta XNU pre-MMU entry marker remained on the screen at 220 seconds into the continuation video. USB re-enumerated in bootloader fastboot 257 seconds after `Booting OKAY`, consistent with the 240-second XNU deadline plus startup. `/private/var`, sustained PID 1, backboardd, Setup Assistant, SpringBoard and touch remain unverified on run58. The device test used only `fastboot boot`. The elish UFS service exposes read-only System and SystemOS windows. Data persistence remains untested. [Late camera frame](elish-run58-pre-mmu-20261126.jpg)

## Data seed

Under `BASE=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125`, `data-seed-tools/build-seed.sh` booted the Debian `6.12.117+deb13-amd64` kernel in QEMU with `linux-apfs-rw` 0.3.13 and `apfsprogs`. It attached only a read-only raw view of the 21H461 `userdata-root.img` OS window and a temporary 1 GiB APFS image. `seed-guest-init` copied `/private/var` with numeric owners and modes, omitted `staged_system_apps`, and created the standard runtime directories. It preserved the existing global preferences and verified absence of root and mobile purplebuddy preferences and `db/.AppleSetupDone`. The guest unmounted and remounted Data read-only. Host `apfsck` passed. `fsapfsinfo` reported Data APFS UUID `7D443C28-95AC-49F8-AF25-4FC12FBC93E5`, distinct from container UUID `0E992A31-39E1-4799-A164-FA65F79E4AEB`.

```sh
CORE=/root/elish-build-20261123/UniXNU
WORK=$CORE/out/elish/ipados-17.7.11-21H461/elish
BASE=$CORE/out/elish-darwinboot-20261125
OUT=$BASE/run58-writable-ram-data
REF=$BASE/run56-verified-systemos-union
bash "$BASE/data-seed-tools/build-seed.sh" "$WORK/userdata/userdata-root.img"
python3 "$CORE/drivers/DarwinBoot/Test/packed_ramdisk.py" \
  "$BASE/data-seed-tools/data-21H461.JqQbNB.img" "$OUT/data-21H461.dbr"
```

`data-21H461.JqQbNB.img` is the original verified image. Generating a new image assigns a new volume UUID and requires updating the active kernel config. Packed reconstruction verified the full 1,073,741,824-byte digest. The packed file is 1,011,840 bytes.

## Kernel and payload

Device baseline `16cedc7b8a9a417e3994e37f420dc6ec485a7da1`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`, firmware checkout `226fbcc677568edf9a04ce7cd95459c664821a03`. The firmware FD bytes match run56, whose firmware build provenance is recorded separately. `source.sha256` pins the local kernel changes, the seed scripts and `kernel.config`.

The active `.config` includes `CONFIG_XNU_OWNER_BOOT_MANIFEST=y`, `CONFIG_XNU_OWNER_DATA_MOUNT=y` and the observed Data UUID in `CONFIG_XNU_OWNER_DATA_VOLUME_UUID`. The elish provider returns a SHA-384 digest of the pinned SHNGROOT entry only after matching the queried BSD device and UUID to the verified UFS media chain. The existing authenticated chunk tables and RootPolicies transport are byte-identical to run56. The Data mount logs the matched device and UUID after a successful read-write APFS mount at `/private/var`, and includes stage and errno on failure.

```sh
export PATH=/usr/lib/llvm-21/bin:$PATH UNIXNU_UEFI=/root/repo/uefi-elish
export UNIXNU_ELISH_ROOT_IMAGE="$WORK/userdata/userdata-root.img"
cp "$OUT/kernel.config" "$WORK/.config"
python3 "$CORE/unixnu" build kernel -D "$PWD" --os ipados-17.7.11-21H461 -j 12
python3 "$WORK/tree/scripts/bind-elish-ram-block.py" --check "$WORK/kernel/kernel.macho"
python3 "$CORE/unixnu" build payload -D "$PWD" --os ipados-17.7.11-21H461 --uefi "$UNIXNU_UEFI" \
  --kernel "$WORK/kernel/kernel.macho" --trust-cache "$REF/apple.trustcache" \
  --root-policies "$REF/kernel.macho.vroot" --data-ramdisk "$OUT/data-21H461.dbr" \
  --kernel-arg elish-ufs-root=1 --kernel-arg elish-ram-only=240 \
  --kernel-arg elish-warm-reset=1 --kernel-arg elish-launchd-checkpoint=0
cp "$WORK/payload/payload.img" "$OUT/payload.img"
fsck.fat -vn "$OUT/payload.img"
python3 "$CORE/unixnu" bootimg -D "$PWD" --os ipados-17.7.11-21H461 --uefi "$UNIXNU_UEFI" \
  --fd "$UNIXNU_UEFI/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd" \
  --dtb reports/elish-recovery-fdt-20261124.dtb --ramdisk "$OUT/payload.img" --output "$OUT/boot.img"
```

Prelink and the 190-slot RamBlock and UFS ABI checks passed. FAT readback matched the Data seed, RootPolicies and Apple trust cache. Run56 and run58 have the same `__TEXT_EXEC` VM address (`0xfffffff007240000`) and size (`0x7bc000`).

## Tablet and controls

`device-attempt.7L4WrQ/device.log` records the verified bootloader identity (`18d1:d00d`, serial `4bed07e4`, `product=elish`, `is-userspace=no`) and both `OKAY` replies from `fastboot -s 4bed07e4 boot "$OUT/boot.img"`. The capture script initially matched the original USB device before it disconnected and ended the first recording. `device-attempt.7L4WrQ/camera-continuation.mkv` captures the following screen state. The updated `capture-boot.sh` requires a new USB enumeration and repeats the identity checks on return.

Both controls passed FAT validation and byte-for-byte readback. Run59 replaces only `EFI/UniXNU/Payloads/kernel.macho` in the run58 FAT with the run56 FAT's extracted kernel. The screen displayed `mount[3] exited` at 86 seconds of `run59-packed-handoff-control/device-attempt.bwaCuT/camera.mkv`. Fresh bootloader fastboot returned 90.4 seconds after `Booting OKAY`. Run60 replaces only that file in the unseeded run57 FAT with the run58 FAT's extracted kernel. Both use the same `bootimg` command above with their own output directories.

| Artifact under `BASE`, except the report photo | SHA-256 |
| --- | --- |
| `data-seed-tools/data-21H461.JqQbNB.img` | `a1f77a9f5f197591057562e0e5487d6c2daec7616bf421d75b03bdeccbad723a` |
| `run58-writable-ram-data/data-21H461.dbr` | `7745816edb50b8671a9d80ce33bd9afa9b82bef1607e4af885e087eaf2d7d83a` |
| `run58-writable-ram-data/payload.img` | `81c9c8dd43fa3dbec6475697b99e6956afa76dcc2fd9bdcdf0ddbc0334eb76d8` |
| `run58-writable-ram-data/boot.img` | `4230b261d836393079f9585341435a9b421a785c6be3e4f9a0e3df1642938177` |
| `run58-writable-ram-data/device-attempt.7L4WrQ/camera-continuation.mkv` | `a3dd364282a4c7f7389a9a021a304afc04ea161f0ed0f2a2f07503ca704cb2c9` |
| `run59-packed-handoff-control/boot.img` | `7f291e1a60a4d138dbb8f35f05dc4b0dc0ae60d27d17c412734c793f3d33a304` |
| `run59-packed-handoff-control/device-attempt.bwaCuT/camera.mkv` | `4198fb7f9ef11a8d17b754b377ccb644b55e2d1bbc52627adc8311a8b8818f03` |
| `run60-seeded-kernel-no-seed-control/boot.img` | `eaf50b75004e9d204c194f14028f62f553e19e2e96b1f589a3d5af88e1a8cf14` |
| `elish-run58-pre-mmu-20261126.jpg` | `fbba7aa6a5295e09b794a59c47ed14332b2d5d18724b8b0e532e67b008ba5835` |
