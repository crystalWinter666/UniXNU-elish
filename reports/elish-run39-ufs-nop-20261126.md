# elish run39 UFS descriptor DMA

Run39 confirmed descriptor DMA through the inherited UFS routes qualified by [run38](elish-run38-ufs-handoff-20261126.md). The probe checked the HCI, idle doorbells and both SMMU routes, reserved two 4 KiB pages below 4 GiB, installed the diagnostic queue and issued one no-data NOP. The controller read UTRL at `0x9A8D3000` and UCD at `0x9A8D4000` and returned `NOP IN` with tag `0x37`, OCS `0`, and response `0`. The transfer queue stopped with doorbell `0`. Global and CB2/CB3 fault status stayed `0/0x400/0x400`. The probe warm-restarted before DarwinBoot. The NOP used no data PRD.

The first fastboot upload returned `unknown command` at `Sending`. `getvar product` then returned `GetVar Variable Not found`. The identified USB bootloader accepted `reboot bootloader` and the tablet returned to Android ADB. After verifying model and product over ADB, the second upload from a fresh bootloader session completed in 5.885 s. Fastboot re-enumerated from USB device 46 to 47. The RAM reader boot took 1.576 s. Root ADB recovered the console. All diagnostic images were loaded by `fastboot boot`.

Root ADB also re-exported the primary and backup GPT from Android `/dev/block/sda`, respectively from LBA 1 and 61,880,315, each five 4 KiB blocks. Both SHA-256 values matched [the existing Android reference](elish-live-hardware-20261123.md#ufs-and-userdata-gpt). The IEEE CRC32 comparison values for native reads are `fa250023` for LBA 1, `4f1522ce` for the complete primary capture, and `294cad87` for the complete backup capture.

## Artifacts

Path `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run39-ufs-nop/`. Source base revisions match [run36](elish-run36-sec-smmu-20261126.md#build-and-device-sequence). The SEC and Kona local changes match [run37](elish-run37-ufs-handoff-20261126.md). The build and pack commands completed with the run28 FAT payload and run38 DTB hashes. The output `probe.json` pins both inputs.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` (235,532,288 bytes) | `9407681e33418a2a63c5d73c1225b5ca63295da151c14cbf3734acea60263e1b` |
| `SILICIUM_UEFI.fd` | `40f8f2168096593074669831224793508db8f8ac529feacad90c6d6b0a9a7cbe` |
| `elish.dsc` | `7bf9ee378169b098c4f151f6acddbf2043d7f40a30645e7cf7de0af0aee532db` |
| `ElishLoaderProbeDxe.c` | `a972e6ecd623c8c9dcd5f9246674cc7355b9f2b0abffc908a61d5d13ea9dc4bf` |
| `ElishLoaderProbeDxe.inf` | `5c9508c0810d8bd377afb2a7a0e213a7939b2205c2881450aa67f4f3d119fd3d` |
| `build-loader-probe.py` | `7b9a03b428cb7bb5fffedd4242bfea901667fd474b1026a6ea990ddcc1880e61` |
| [Console ramoops](elish-run39-ufs-nop-console-ramoops-20261126.txt) (56,822 bytes) | `3c337f83d24533a4b39db3cb72df2e95730f04d73d47fc083c91b51fc0f3f9f5` |

```sh
CORE=/root/elish-build-20261123/UniXNU
BASE="$CORE/out/elish-darwinboot-20261125"
OUT="$BASE/run39-ufs-nop"
python3 scripts/build-loader-probe.py --unixnu "$CORE" --uefi /root/repo/uefi-elish \
  --dtb /tmp/elish-uefi-probe.dtb --out "$OUT" --feeds "$CORE/out/feeds" \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-nop \
  --payload "$BASE/run28-restore-launchd-cyan/payload.img"
adb -s 4bed07e4 reboot bootloader
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$OUT/boot.img"
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$CORE/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img"
adb -s 4bed07e4 root
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0
```
