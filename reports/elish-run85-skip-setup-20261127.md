# Run85 boots past Setup from UFS

On the Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC`, `elish`, `4bed07e4`), Run85 booted iPadOS 17.7.11 (`21H461`) from `boot_b` with Setup pre-completed in the APFS RAM Data seed. The internal panel showed the lock screen at 365 seconds and the SpringBoard home screen at 410 seconds. System and SystemOS retain Run84's verified read-only UFS path. Data is volatile.

## Prepare the Data seed on Linux

Use a fresh copy of the matching raw APFS Data seed. Run85 used `run76-mobile-tmp/data-mobile-tmp.img`, a 1 GiB seed with volume UUID `7D443C28-95AC-49F8-AF25-4FC12FBC93E5`. The guest writes preferences for UID/GID `501` and creates the runtime container directories. It unmounts and remounts the copy read-only to verify the changes before packing it.

The commands below use an x86-64 Linux initramfs tree in `GUEST_ROOT` with `/bin/busybox`, `/usr/bin/python3.13` with its loader and standard library, `/dev/console` (character device `5:1`), empty `/data` and `/proc` directories, and the `virtio_blk.ko`, `crc32c_generic.ko`, `libcrc32c.ko` and `apfs.ko` modules at its root. Run85 used Debian `6.12.117+deb13-amd64` with `linux-apfs-rw` `0.3.13`. Set `GUEST_KERNEL` to the matched kernel and `BASE_DATA` to the original raw seed. Run from the elish repository with `bash`.

```sh
set -euo pipefail
out=$(mktemp -d)
cp -a "$GUEST_ROOT" "$out/root"
install -m 755 scripts/elish-skip-setup-init "$out/root/init"
install -m 644 scripts/elish-skip-setup.py "$out/root/elish-skip-setup.py"
(cd "$out/root" && find . -print0 | cpio --null -o -H newc --quiet | gzip -1) > "$out/initrd"
cp --sparse=always "$BASE_DATA" "$out/data.raw"
timeout 60s qemu-system-x86_64 -machine q35,accel=kvm:tcg -m 512 \
    -nodefaults -no-reboot -display none -serial stdio -monitor none -nic none \
    -kernel "$GUEST_KERNEL" -initrd "$out/initrd" \
    -append 'console=ttyS0 quiet panic=1 rdinit=/init' \
    -drive "if=none,id=seed,format=raw,file=$out/data.raw" \
    -device virtio-blk-pci,drive=seed | tee "$out/guest.log"
tr -d '\r' < "$out/guest.log" | grep -Fx ELISH_SKIP_SETUP_VERIFIED
python3 ../UniXNU/drivers/DarwinBoot/Test/packed_ramdisk.py "$out/data.raw" "$out/data.dbr"
```

Pass `--data-ramdisk "$out/data.dbr"` to the normal payload build with the selected kernel, trust cache, platform identity and verified UFS root inputs. For the Run85 repack, copy the Run84 FAT payload and replace `EFI/UniXNU/Payloads/scratch-data.img` with the new seed using `mcopy -o`. Remove `elish-ram-only=420` and `elish-touch-snapshot=1` from the FAT's `EFI/UniXNU/unixnu.toml` for the installed variant. Keep `elish-touch=1`. Verify the seed and configuration by FAT readback, then use `unixnu bootimg` with the Run84 FD, DTB and updated `--ramdisk`. Run85 retains Run84's kernel, ADT, RootPolicies and trust cache.

## Installed boot evidence

Device source baseline `b4d33c52cb746d6d7589a3cc569f99f239edf641`. Run85 reuses the [Run84 touch kernel and firmware inputs](elish-run81-touch-20261126.md#event-acquisition) and replaces the Data seed and boot arguments. The Linux guest reported `ELISH_SKIP_SETUP_VERIFIED` after read-only remount. FAT validation passed. The installed and RAM-test payloads have identical kernel, ADT, RootPolicies, trust cache and Data seed bytes.

| Artifact | SHA-256 |
| --- | --- |
| Packed Data seed | `6dc059e86e688c2ad6649a4b47fbb81e2182f24355657667f17179e323fa7660` |
| `payload.img` | `861c64e346c1af8113b4a80eff072c1f4be9346871e7ea834e90bd3be001434a` |
| `boot.img` | `2fd92014b12964d5f4b3336bef0e12468ec70de0981701cac8c8449e290e1605` |
| `boot_b-install.img` (192 MiB, zero-padded) | `ab95e9cd4523d0b8767e97c81c8cc48482f25f6e15ab96acccb6f938d56f9e6b` |
| [Home screen capture at 410 seconds](elish-run85-home-20261127.jpg) | `fe38de3ba31efb6f97147d231db755ea226a9953c83c82ae8771bf86a4fbab64` |

Bootloader fastboot reported `product: elish`, `is-userspace: no`, `current-slot: b` and `partition-size:boot_b: 0xC000000`. The destination was the [previously mapped UFS boot slot](elish-run80-installed-hello-20261126.md#build-and-boot). The device booted the verified install artifact after `fastboot -s 4bed07e4 reboot`.

```text
$ fastboot -s 4bed07e4 flash boot_b boot_b-install.img
Sending 'boot_b' (196608 KB)                       OKAY [  5.189s]
Writing 'boot_b'                                   OKAY [  0.418s]
Finished. Total time: 5.651s
```
