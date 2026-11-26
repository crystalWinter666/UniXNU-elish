# elish run47 UFS root-window guard

Run47 booted a diagnostic payload with `elish-ufs-root=1 elish-ram-only=45` and no ramdisk. The payload's `/chosen/root-matching` selects IOMedia UUID `04769D26-2CA0-4825-B443-E2E5F88300F2`. The XNU transport passed both run40 GPT capture CRC checks and read the first block of `userdata`. The screen showed `elish UFS root: userdata has no SHNGROOT manifest`, `stopped=1` and `faults-same=1` at 20 and 57 seconds. The manifest guard runs before block publication. The expected root image remains on the host. The original write authorization gate applies.

The host did not regain a responding bootloader endpoint during the automatic reset observation. The user returned the tablet to fastboot manually. After a RAM-only Lineage reader boot, root ADB listed an empty `/sys/fs/pstore`. This build attempted the firmware-style Qualcomm reset sequence from XNU after the panic screen and ramoops append. The follow-on build and machine-code diagnosis are in the [run48 record](elish-run48-xnu-psci-root-20261126.md).

## Build and device evidence

Device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, and UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139` with local changes. XNU source `d8b80295118ef25ac3a784134bcf95cd8e88109f`. `source.sha256` pins the changed source inputs. Kernel prelink, both 190-slot storage ABI checks, FAT readback and `fsck.fat -vn` passed. UEFI was built in `--ufs-run-kernel` mode with route retention and no UFS commands before DarwinBoot.

Output directory `/tmp/elish-run47-xnu-ufs-root-20261126`.

| Artifact | SHA-256 |
| --- | --- |
| `boot/boot.img` | `d8a60194325719a9d4ee853a6d16d84275b2f6d260d9761e622116151675b6ac` |
| `boot/SILICIUM_UEFI.fd` | `cf72ac5beb8917c7973963f327db00e4800b92dabe3ee730f84d19526049af28` |
| `payload.img` | `ca4b0c04b13cda0a2877a535d6983a8cd1bf2ef175b25101c482754d966cdd19` |
| `kernel.macho` | `cbd57a6faa6438f140ce92ca6b0434495dd6f3713fe4c92fcc7ee61baa6b6dfa` |
| `kernel.macho.adt` | `3a25b1cdac689fe7e576facf4feddb7f8ca9231582adf80f75b2d86437ab7da6` |
| `screen-20s.jpg` | `a3d54cd54ee362f1276ca605ee3c338afa110939c926bd06f54811f9f2981069` |
| `screen-57s.jpg` | `406d973f12672f2c40ecf72cc1ede2a8b2ca8842e19140d7b846766c6ae56d71` |

```sh
CORE=/root/elish-build-20261123/UniXNU
OUT=/tmp/elish-run47-xnu-ufs-root-20261126
FB=/tmp/elish-platform-tools/platform-tools/fastboot
S=4bed07e4
export UNIXNU_UEFI=/root/repo/uefi-elish
export PATH=/usr/lib/llvm-21/bin:$PATH
python3 "$CORE/unixnu" build kernel -D "$PWD" --os ipados-17.7.11-21H461 -j 12
python3 "$CORE/unixnu" build payload -D "$PWD" --os ipados-17.7.11-21H461 \
  --uefi "$UNIXNU_UEFI" --kernel-arg elish-ufs-root=1 --kernel-arg elish-ram-only=45
cp "$CORE/out/elish/ipados-17.7.11-21H461/elish/payload/payload.img" "$OUT/payload.img"
python3 scripts/build-loader-probe.py --unixnu "$CORE" --uefi "$UNIXNU_UEFI" \
  --dtb /tmp/elish-uefi-probe.dtb --out "$OUT/boot" --feeds "$CORE/out/feeds" \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-run-kernel --payload "$OUT/payload.img"
lsusb -d 18d1:d00d
"$FB" devices -l
"$FB" -s "$S" getvar product
"$FB" -s "$S" getvar is-userspace
"$FB" -s "$S" boot "$OUT/boot/boot.img"
```

At 04:06:06 CST the USB device reported `4bed07e4`, `elish`, and bootloader fastboot. Both `OKAY` results arrived within 1.769 seconds. This run only submitted read commands to UFS.
