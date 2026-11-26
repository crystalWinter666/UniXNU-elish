# elish run80 installed boot to Setup Assistant

The Xiaomi Pad 5 Pro Wi-Fi (`elish`, `M2105K81AC`, `4bed07e4`) booted from its installed UFS `boot_b` slot into the original iPadOS 17.7.11 (`21H461`) Setup Assistant. The owner confirmed Hello and the camera recorded the animated greeting on the internal panel. [Hello photo](elish-run80-installed-hello-20261126.jpg)

## Build and boot

```sh
OUT=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run80-installed-hello
bash "$OUT/repack.sh"
adb -s 4bed07e4 reboot
```

Run the repack script from the device repository. It verifies the firmware and FDT against run79, removes `elish-userspace-snapshot=360` and `elish-ram-only=420` from the FAT configuration, and compares the other files in both FAT volumes. FAT validation and Android v3 unpack-and-compare passed. The kernel, ADT, `RootPolicies`, Apple trust cache and 1 GiB RAM Data seed with volume UUID `7D443C28-95AC-49F8-AF25-4FC12FBC93E5` carry forward from run79. Source revisions and the run79 working-tree input manifest are described in the [run79 report](elish-run79-key-store-20261126.md).

| Artifact under `OUT` | SHA-256 |
| --- | --- |
| `boot.img` | `7e45c4a8daa4c069898d3db073fc9c45921dfda81b94a278de2d2319abc43c1c` |
| `payload.img` | `bac30dde7b9e143998e1ca64d71f7b77509f9f13379b1445a863349b545abdef` |
| `boot_b-install.img` (192 MiB, zero-padded) | `d4dceb7aa1ec16a51e828ca6c983efe513fba1e1e152808545f55f09da505b2c` |
| `boot_b-before.img` (192 MiB, full original slot) | `fce50614e0f90a4fd42b1eef26b913712883e4133723b802b4ac65a8dd1dd890` |
| `device-attempt.gF3Lbt/camera-035.mkv` | `0938400dddf4b71406408c378106235dce107dce267dfd9d15e7433ee29a11bf` |
| `device-attempt.gF3Lbt/camera-045.mkv` | `3ea840c21f6f50343d20d5b712862263ee8fe8712f17178ac0edb46d33e75265` |
| [Hello photo in the device repository](elish-run80-installed-hello-20261126.jpg) | `8e9f0cdadae23899c26f8309b50ea4dc72fcf5ebdf9683704e7a5fc036e0e63f` |

Recovery ADB identified `elish` in active slot `_b` with root access. `boot_b` mapped to `/dev/block/sde37`, GPT entry 37 on UFS LUN 4. The primary and backup GPT headers and arrays passed CRC checks and agreed on the inclusive LBA range `393654–442805` at 4 KiB per block. The full backup matched an independent read of that range through `/dev/block/sde`.

After authorization, Recovery wrote the padded image, flushed it and matched the full partition and independent LUN readbacks to the padded image hash. Both GPT captures and the adjacent blocks at `393653` and `442806` matched the pre-write copies. `installation.log`, `gpt-verification.log`, `backup-verification.log` and `install.sha256` are under `OUT`. A rollback would restore `boot_b-before.img` from Recovery after checking the device and range, then compare the full original hash and the guard blocks before rebooting. Rollback was unnecessary on this run.

## Hardware result

The normal reboot was requested at 18:24:53 CST. `camera-035.mkv` shows Setup Assistant about 352 seconds later. `camera-045.mkv` shows animated greetings including English `hello` and `Swipe up to open` about 457 seconds after reboot, past run79's 382.55-second return to fastboot. The camera ran for 460 seconds after reboot. `device.log` and `camera.sha256` are in `device-attempt.gF3Lbt`.

Touch, language selection, Setup completion and the SpringBoard home screen remain untested. The installed boot image still carries volatile Data. The proposed persistent Data window was not written.
