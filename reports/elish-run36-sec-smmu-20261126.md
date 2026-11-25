# elish run36 SMMU snapshot before SEC detach

A firmware-only RAM boot captured five valid stream matches before Kona's
`PlatformInitialize()` call. The new observations are the two unmasked UFS
matches and their inherited context banks. The ARID column comes from the
stock elish XBL IORT and HALIOMMU call-site audit. The SID and context columns
come from run36. [Stock XBL audit](elish-run33-smmu-context-20261125.md#stock-xbl-iort-reference)

| UFS ARID | SID | SMR group | S2CR | Context bank |
| --- | --- | --- | --- | --- |
| `0x0` | `0x4E0` | 2 | `0x00000002` | 2 |
| `0x10` | `0xE0` | 3 | `0x00000003` | 3 |

Both entries have S2CR type 0 with `CBAR=0x0001F000` (stage-1 context,
stage-2 bypass), `CBA2R=1`, `SCTLR=0xE0` (`M=0`), `TTBR0=0`, and `TCR=0`.
The other valid matches were SID `0x4A0` on CB0, SID `0xC0` on CB1, and the
MDSS match on CB4. The two UFS banks were distinct and had no other visible
valid matches. `sCR0=0x00230406` and `ID2=0x00005111` continued to report
unmatched-stream fault policy and 36-bit input/output/upstream widths.

Kona SEC runs `ArmSmmuDetach` with a display-only skip list. It invalidates
non-display stream entries and clears unretained context state. This accounts
for the missing UFS routes in run35 after SEC. Preserve and recheck the two
qualified routes through DXE before allocating and validating a bounded DMA
arena. The inherited queue bases from run35 remain firmware state. Run36 issued
no UFS command and stopped before SEC detach. [Run35](elish-run35-ufs-queues-20261126.md)

## Build and device sequence

| Artifact | Identity |
| --- | --- |
| Device repository | `f0b9d57f5088f80f7055e80599cb80a0d3877db3` plus local changes |
| UniXNU build checkout | `71c5c6569bae6369c0e0f634df8b9bd5afc19139` plus local changes |
| UEFI checkout | `d489a050255f90b9c011d25b95c15c1b12f382f0` plus local changes |
| `Sec.c` SHA-256 | `84e6f878c2c0eb99a91760692ef36c874890286ff7d21d51baba010c4bbb0139` |
| Build helper SHA-256 | `ee2020783a225414b40037532ebfc0b1f729313c075320831a23074aa5be258b` |
| `/tmp/elish-uefi-probe.dtb` SHA-256 | `0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48` |
| `SILICIUM_UEFI.fd` | 3,145,728 bytes. SHA-256 `1557f53b167a40fd8872dd7aa0764a5fa92f52d8be02d87400eb59520ca82d07` |
| `boot.img` | 638,976 bytes. SHA-256 `ce7e574da09dd0e60d68f927df74110dad2764cf17b36ebb1b6ee64e0435562c` |
| Console ramoops | 1,305 bytes. SHA-256 `d1e826a1c2d80bbc4b858ae5b0483816ecd777ab599665d1d96bdb72d5b6cb8b` |

The DEBUG firmware and boot-image packer passed. The Android v3 header had
`ramdisk_size=0`. After verifying `4bed07e4` and `product=elish`, fastboot
accepted the image in 0.258 seconds. The warm restart returned to fastboot
(USB device 31 to 32). The previously verified RAM-only reader booted in
1.578 seconds and `adb root` exposed the saved SEC console. No device
partition write command was issued. [Raw console](elish-run36-sec-smmu-console-ramoops-20261126.txt)

```sh
CORE=/root/elish-build-20261123/UniXNU
OUT="$CORE/out/elish-darwinboot-20261125/run36-sec-smmu-readonly"
python3 scripts/build-loader-probe.py --unixnu "$CORE" --uefi /root/repo/uefi-elish \
  --dtb /tmp/elish-uefi-probe.dtb --out "$OUT" --feeds "$CORE/out/feeds" \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --sec-smmu-readonly
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$OUT/boot.img"
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$CORE/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img"
adb -s 4bed07e4 root
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0
```
