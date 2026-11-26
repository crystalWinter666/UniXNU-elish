# elish run42 UFS root-window address check

Run42 read six native 4 KiB LBAs after reproducing the run40 capacity, known block and primary/backup GPT CRC checks. Android root ADB independently read the same six blocks through `/dev/block/sda` and `/dev/block/sda34` before the probe. Each pair from Android matched byte for byte. Run42 matched all six Android CRC32 values. The diagnostic logged CRCs and signature matches only.

| Sample | Relative 4 KiB LBA | LUN 0 LBA | Android and UEFI CRC32 | 21H461 image CRC32 |
| --- | ---: | ---: | --- | --- |
| Manifest | 0 | 6,619,136 | `A37C5576` | `EE82E4D7` |
| OS first block | 256 | 6,619,392 | `E26A269B` | `8762762E` |
| OS APFS container | 260 | 6,619,396 | `DF21F342` | `79B3D53B` |
| OS last block | 1,484,040 | 8,103,176 | `C71C0011` | `11354F1C` |
| SystemOS first block | 1,484,288 | 8,103,424 | `C71C0011` | `C034A6F0` |
| SystemOS last block | 2,290,687 | 8,909,823 | `C71C0011` | `C71C0011` |

The OS disk image's inner GPT uses 512-byte LBAs. Its APFS container starts at inner LBA 34. The `NXSB` magic lands at byte `0x420` in native LBA 6,619,396. SystemOS is a raw APFS container with `NXSB` at byte `0x20` of LBA 8,103,424. The device samples lack those signatures, the OS GPT header, and the `SHNGROOT` manifest. The matching SystemOS tail is an all-zero block. The full image remains on the host. [Image build](elish-userdata-build-20261124.md)

The request list stopped with doorbell `0`. GFSR/FSR2/FSR3 stayed `0/0x400/0x400`. The probe warm-restarted before DarwinBoot. Neither the firmware probe nor the host sent a partition write command. The Lineage reader was loaded in RAM to recover ramoops and resumes its normal Android startup. [Console ramoops](elish-run42-ufs-root-windows-console-ramoops-20261126.txt)

## Inputs and commands

Device `2561162a380f3570f5e9cb8360a74603e23bf691` plus the helper change. Firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30` plus the probe changes. UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139` with the existing local build support changes. XNU profile source `d8b80295118ef25ac3a784134bcf95cd8e88109f`. The probe uses the run28 FAT payload to reach the loader hook.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` (235,532,288 bytes) | `5d0be74cc6744aa36543d782404a26f1deef6acf6f770404372b2f5b0aca8192` |
| `SILICIUM_UEFI.fd` | `f4057dab52cbfe3f392650ea0a017f48811ca481a3f31efad18dec9874cbda64` |
| `build-loader-probe.py` | `52a1d1be09687a3fca24e8b3bc6bbd0cd79f4e43fdacbb3765c05162aa107d3f` |
| `elish.dsc` | `3c907c5e6acdce750e426a53d69900ef04d4058bc78d754de315025a6f040f6c` |
| `ElishLoaderProbeDxe.c` | `fec21eb923d126750f73982ba9ec49dcf9d1478c2febdff8392e7b4a1fcb6200` |
| Console ramoops (57,984 bytes) | `457b90c15fc307fa1cb7e63b44c363554a734d6139d5312de47300a21a391886` |

Android ADB identified `4bed07e4`, `elish`, `M2105K81AC`, slot `_b`. Fastboot 37.0.3 reported `is-userspace: no` and `product: elish` before each RAM boot. The run42 transfer and boot took 5.893 s. USB re-enumerated from device 054 to 055. The Lineage reader boot took 1.577 s. Root ADB recovered the console.

```sh
CORE=/root/elish-build-20261123/UniXNU
OUT=/tmp/elish-run42-ufs-root-windows-rebuilt-20261126
S=4bed07e4
FB=/tmp/elish-platform-tools/platform-tools/fastboot
python3 scripts/build-loader-probe.py --unixnu "$CORE" --uefi ../uefi-elish \
  --dtb /tmp/elish-uefi-probe.dtb --out "$OUT" --feeds "$CORE/out/feeds" \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-root-windows \
  --payload "$CORE/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img"
# Repeat these two read-only commands for each relative and absolute LBA above.
adb -s "$S" exec-out dd if=/dev/block/sda34 bs=4096 skip=256 count=1 status=none
adb -s "$S" exec-out dd if=/dev/block/sda bs=4096 skip=6619392 count=1 status=none
adb -s "$S" reboot bootloader
lsusb -d 18d1:d00d
"$FB" devices -l
"$FB" -s "$S" getvar product
"$FB" -s "$S" getvar is-userspace
"$FB" -s "$S" boot "$OUT/boot.img"
# After USB re-enumerates, repeat USB, serial, product and mode checks.
"$FB" -s "$S" boot "$CORE/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img"
adb -s "$S" root
adb -s "$S" exec-out cat /sys/fs/pstore/console-ramoops-0 > "$OUT/console-ramoops-0.txt"
```

## Persistent-write gate

The measured image would overwrite exactly 9,382,658,048 bytes at the start of `sda34` if written through a bounded block writer. The inclusive LUN 0 range is 6,619,136–8,909,823. Android mounts this partition through `dm-43` as writable F2FS. The overwrite would make Android `/data` unavailable until the original prefix is restored. A whole-partition fastboot implementation may discard beyond the image, so the bounded prefix plan requires a block writer and out-of-range preservation checks.

Recovery must leave `/data` unmounted and the userdata block device quiescent. Keep the recovery path bootable with an invalid `/data` filesystem. Persistent iPadOS Data will require a separate range and recovery review.

## Offline prefix backup

The current installation uses the newer offline backup and guard checks from [run50](elish-run50-xnu-system-checkpoint-20261126.md#offline-installation-and-rollback-preflight).

Bootloader `fastboot reboot recovery` on `4bed07e4` reached Lineage Recovery on slot `_b`. Recovery ADB ran as root. The PID 1 and ADB mount tables showed neither `/data` nor `/metadata` mounted. The `sda34` holder directory was empty and the `userdata` symlink resolved to `/dev/block/sda34`. The kernel reported a 4,096-byte LUN block and a partition start at sector 52,953,088 in 512-byte units.

The first read attempt used Toybox `dd iflag=direct`, which returned `EINVAL` in Recovery. Buffered reads completed. The host synced the 9,382,658,048-byte backup and matched its SHA-256 against a separate Recovery read through the whole LUN. Both GPT captures matched run40 CRC32. Both GPT captures and the first block beyond the write range matched byte for byte before and after the backup.

```sh
S=4bed07e4
BACKUP=/root/elish-userdata-prefix-backup.KGeZtgQ9
adb -s "$S" shell -T 'dd if=/dev/block/sda34 bs=1048576 count=8948 status=none' > "$BACKUP/userdata-prefix.img"
sha256sum "$BACKUP/userdata-prefix.img"
adb -s "$S" shell -T 'set -o pipefail; dd if=/dev/block/sda bs=1048576 skip=25856 count=8948 status=none | sha256sum'
```

| Local artifact | SHA-256 |
| --- | --- |
| `userdata-prefix.img` | `3daa1b0b46f70543e23f25e7a3066a2bc60c9b98c6d357d4add7a8538f7b1581` |
| `gpt-primary.before.bin` | `b8b416c188f9ee5cca9146ad19068b975c0eba949dfde5e950166577189ff08a` |
| `gpt-backup.before.bin` | `f72f278000f453775369dcad13cdfe458e5e08ee445907b07af1e149b99c2845` |
| `after-prefix.before.bin` | `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7` |

Before writing, recheck the complete prefix hash and guards in Recovery against this backup. Then install the pinned image using the partition node and a writer limited to 8,948 × 1 MiB. Read back the complete written range and the guards. To roll back, boot Recovery directly, repeat the identity and offline checks, restore the same range from the local backup, and verify the full hash and guards before checking Android `/data`. Recovery boot and readback were exercised with Android userdata intact. The write and rollback remain gated by authorization.
