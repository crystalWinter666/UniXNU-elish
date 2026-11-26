# elish run57 after the PID 1 checkpoint

The 21H461 System boot continued past the run56 PID 1 checkpoint. The overhead camera shows `mount[3] exited -- no exit reason available` at 83–87 seconds. The frame clips the signal and exit status. The XNU panic path names a process other than PID 1 in this format. The tablet re-enumerated in bootloader fastboot 88.33 seconds after `Booting OKAY`, before the 240-second deadline. Run57 carried no Data seed. `/private/var`, backboardd, Setup Assistant, SpringBoard, touch and Data persistence remain unverified. [Panic frame](elish-run57-mount-panic-20261126.jpg)

## Inputs and reproduction

`OUT=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run57-pid1-continues`. The board checkout was `16cedc7b8a9a417e3994e37f420dc6ec485a7da1` and the firmware checkout was `226fbcc677568edf9a04ce7cd95459c664821a03`. The actual kernel and firmware bytes came from the [run56 build](elish-run56-verified-systemos-union-20261126.md) with its recorded source pins and local inputs. Running `build payload` directly on the extracted kernel failed because the entry probe had already been applied. Run57 copies the original FAT volume and replaces only `EFI/UniXNU/unixnu.toml` with `mcopy -o -i "$OUT/payload.img" "$OUT/unixnu.toml" ::/EFI/UniXNU/unixnu.toml`. This changes `elish-launchd-checkpoint=1` to `0` and `elish-ram-only=150` to `240`.

```sh
CORE=/root/elish-build-20261123/UniXNU
UNIXNU_UEFI=/root/repo/uefi-elish
python3 "$CORE/unixnu" bootimg -D "$PWD" --os ipados-17.7.11-21H461 --uefi "$UNIXNU_UEFI" \
  --fd "$UNIXNU_UEFI/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd" \
  --dtb reports/elish-recovery-fdt-20261124.dtb --ramdisk "$OUT/payload.img" --output "$OUT/boot.img"
fastboot -s 4bed07e4 boot "$OUT/boot.img"
```

`fsck.fat -vn` passed. FAT readback matched run56 for the kernel, ADT, RootPolicies and Apple trust cache. The Android v3 boot header and FD digest matched the baseline. `device-attempt.Rhlq1S/device.log` records USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no` before RAM-only boot. The test performed zero partition writes.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `3ea8adeec3d7a4b870851175cf925adc8bce5310bef241211cd70dca549d74a5` |
| `payload.img` | `8a509826d88207b78b41e46a368c2dd2ad7cfda63f6ad98304eb9ee4ede6bf83` |
| `device-attempt.Rhlq1S/camera.mkv` | `8a526b66bace44f57278fa4c52b5e3bbb6f8a00280fb935cb1efdd74f555cfce` |
| `device-attempt.Rhlq1S/screen-85s.jpg` | `f39593042759fa182d0d8ba05ac9c9f9a855e4c100bb15e433f37f1385639203` |
| `elish-run57-mount-panic-20261126.jpg` | `d607adf37aef8ec491e94de0f0ed90fe70f1790dccf57db2b28d7cda21da2b94` |
