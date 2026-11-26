# elish run50 System-root probe and ramoops recovery

The 21H461 RAM-only payload enables `elish-ufs-root=1 elish-ram-only=75 elish-warm-reset=1 elish-launchd-checkpoint=1`. The ADT matches IOMedia UUID `04769D26-2CA0-4825-B443-E2E5F88300F2`. The kernel can report the matched UUID, mounted APFS device and Apple `launchd` reaching `main()` after the successful PID 1 sysctl write. This run stopped at the expected missing manifest before OS/SystemOS publication. The two GPT capture CRC checks and the `userdata` manifest-block read passed.

The host verified USB `18d1:d00d`, serial `4bed07e4`, product `elish` and `is-userspace=no` before each RAM boot. `fastboot -s 4bed07e4 boot "$OUT/boot/boot.img"` returned both `OKAY` results in 1.765 seconds. USB re-enumerated from device 080 to 081 and the bootloader answered both `getvar` checks approximately 20 seconds after the boot command completed. The RAM-only Lineage reader boot completed in 1.477 seconds. Root ADB exported 80,997 bytes from `/sys/fs/pstore/console-ramoops-0`.

```text
RamLog: 0xB0000000, resets warm into fastboot, PON 0x00000000
ELISH_LOADER_PROBE: EBS returned Success accepted-snapshot=1
ELISH_XNU: recent kernel messages
ELISH_XNU: elish UFS root: userdata has no SHNGROOT manifest DMA=c7fb5000 stopped=1 faults-same=1 @ElishUfs.cpp:715 frames fffffff0079e3bdc fffffff007904c7c fffffff007904330 fffffff0078fdfa8 fffffff007906974 fffffff0074244a4
ELISH_XNU: requesting prepared warm reset
```

| Acceptance item | Result |
| --- | --- |
| Full System APFS mount | Pending installation of the target image and verification on elish |
| Apple `launchd` as PID 1 on that root | Pending the full System-root boot |
| Setup | Untested |
| Data persistence | Untested |

## Build and artifacts

Device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, and UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139` with local changes. XNU source `d8b80295118ef25ac3a784134bcf95cd8e88109f`. The output directory `/tmp/elish-run50-xnu-system-checkpoint-20261126` holds the input fingerprints in `source.sha256` and the USB transcript in `device.log`. Kernel prelink, both 190-slot storage ABI checks, FAT trust-cache readback, `fsck.fat -vn` and DEBUG firmware packaging passed.

The Apple static and System cryptex trust modules came from the 21H461 `j120ap` IPSW. Their IM4P SHA-384 values matched both `j120ap` build identities. UniXNU `scripts/bundle-ipsw-trust.py` packaged the unchanged modules, with 2,478 and 76 entries respectively, in `/tmp/elish-21H461-apple-trust.ItGgiK8R/original-static-system.bundle`.

| Artifact | SHA-256 |
| --- | --- |
| `boot/boot.img` | `a97b762d32dc5da6fc55c325536432503ffa7967f647db7aff284dc448cbbedd` |
| `boot/SILICIUM_UEFI.fd` | `fda4d664b23a58f28da7f567ff3b74fe6a54619c6649457fc29e47f4d867f55d` |
| `payload.img` | `074851bb6102a0d67a7d046ed5c9f0aab6b2177c96f4c5faeb3054eff8598952` |
| `kernel.macho` | `15f53d7ef85843df3bf32bbfde28bd9333f552aa49603d2e1ada960d11c3b48a` |
| `kernel.macho.adt` | `7eef22b8e7d8462c25f6473e29451dcb5a7fc107a975f12a02d80fb9975b6877` |
| `original-static-system.bundle` | `7416ca4f8377146626bb284e2133c76bee6cb4e3ee0af5d648b186235a7323a7` |
| `console-ramoops-0.txt` | `ddf3c12899088cacfac98433e9be05fc8f1160c04a927531192a9339f164c571` |

Use the run47 command sequence with the run50 output directory and the following payload command. The full artifact hashes and runtime logs apply to the run50 source fingerprints above. [Run47 commands](elish-run47-xnu-ufs-root-20261126.md#build-and-device-evidence)

```sh
TRUST=/tmp/elish-21H461-apple-trust.ItGgiK8R/original-static-system.bundle
python3 "$CORE/unixnu" build payload -D "$PWD" --os ipados-17.7.11-21H461 \
  --uefi "$UNIXNU_UEFI" --trust-cache "$TRUST" \
  --kernel-arg elish-ufs-root=1 --kernel-arg elish-ram-only=75 \
  --kernel-arg elish-warm-reset=1 --kernel-arg elish-launchd-checkpoint=1
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0 > "$OUT/console-ramoops-0.txt"
```

## Offline installation and rollback preflight

USB `18d1:4ee0` identified fastbootd with serial `4bed07e4`, `product=elish` and `is-userspace=yes`. `fastboot -s 4bed07e4 reboot recovery` returned `OKAY`. After the user authorized Recovery ADB, the root shell verified slot `_b`, the `/dev/block/by-name/userdata` mapping to `/dev/block/sda34`, and the partition start, length and 4 KiB LUN block size. `/data` and `/metadata` were unmounted and `sda34` had no holders.

The complete prefix no longer matched the run42 backup hash. The new full backup at `/root/elish-userdata-prefix-backup.MV0PYNbU/userdata-prefix.img` is 9,382,658,048 bytes and SHA-256 `fcc2fc25384f6e69d6deb2f33a5c3c30acf47e5b46ae0e102aea7922703e2b78`. The export resumed from the first missing 1 MiB block after a timeout. Its complete hash matches an independent read of LUN 0 through `/dev/block/sda`. The primary GPT capture at LBA 1–5, backup GPT capture at LBA 61,880,315–61,880,319, and first block after the prefix at LBA 8,909,824 match the run42 hashes. The first primary GPT comparison used LBA 0 and was corrected to LBA 1. The run directory contains `recovery-prefix.log`, `recovery-guards.log` and `recovery-backup-final.log`.

Installation requires explicit approval to overwrite LUN 0 LBA 6,619,136–8,909,823 with `userdata-root.img` (SHA-256 `fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093`). Overwriting those blocks makes Android `/data` unavailable until the original prefix is restored. A stdout-only test verified that Recovery Toybox `dd` honors `iflag=count_bytes` across short ADB input reads. Use that byte limit for the partition write, flush it, then compare the complete readback SHA-256 and all three guard captures. To roll back, return directly to Recovery, recheck the offline state, restore the same byte range from the verified backup, and check the full original hash and guards before booting Android to check `/data`. Reuse this backup while the pre-write prefix and guards still match. [Source image](elish-userdata-build-20261124.md)
