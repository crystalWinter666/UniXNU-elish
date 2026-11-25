# elish run38 UFS routes retained through DXE

[Run37](elish-run37-ufs-handoff-20261126.md) identified a build flag that reached SEC but was absent from Kona `PlatformSecLib`. Run38 scopes the flag to the DEBUG handoff build without a module type filter. EDK II autogen showed `-DELISH_UFS_HANDOFF_PROBE=1` in the Kona library makefile. Board preflight, firmware compilation, and Android v3 packaging passed.

DXE read the same two unshared SMMU routes that SEC observed before detach. `0x4E0 → CB2` and `0xE0 → CB3` both had S2CR type 0, CBAR `0x0001F000`, SCTLR `0xE0`, zero TTBR0/TCR, and FSR `0x400`. Stage 1 was disabled and stage 2 was bypassed. The MDSS route remained valid. The SMMU reported `sCR0=0x00270406`, with client lookups enabled and unmatched streams configured to fault, and ID2 `0x00005111` with 36-bit address widths.

UFSHCI reported CAP `0x1587031F`, version `0x300`, HCE `1`, HCS `0x10F`. The transfer list was running but idle at `0x9B145000`. The task list at `0x9B146000` was stopped and idle. DXE reserved and released `[0x9A8B8000,0x9A8BB000)` below 4 GiB. The probe issued zero commands. This qualifies the inherited route and a bounded address range for a NOP trial. Actual descriptor and data reachability require command completion.

## Artifacts and execution

Path `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run38-ufs-handoff-library-flag/`. Source base revisions match [run36](elish-run36-sec-smmu-20261126.md#build-and-device-sequence). The SEC, Kona and DXE local changes are identified in [run37](elish-run37-ufs-handoff-20261126.md).

| Artifact | SHA-256 |
| --- | --- |
| `elish.dsc` | `75d311c9f11b5f16c4b1207863b2e70ddf2c1dae66be8658c1dbcd2829e20488` |
| `boot.img` (235,528,192 bytes) | `19ada4cf0f80cbd9c2a1068a732f94a82f6dde85c278d4159454ab7081d38c9a` |
| `SILICIUM_UEFI.fd` | `2484c19085e61969f7205a1851d6ecf5ab808aa9b71f9fe39ca65818cd157082` |
| [Console ramoops](elish-run38-ufs-handoff-console-ramoops-20261126.txt) (56,531 bytes) | `8a606d9aaa5eb0f48df9a5ee008875d21d5c2673fa4ac736e9206fe39846ab65` |

Verified USB VID/PID `18d1:d00d` and fastboot `product=elish` on serial `4bed07e4`. The probe boot took 5.894 seconds. The warm restart returned to fastboot with USB device number 39 changed to 40. The RAM-only reader boot took 1.577 seconds and root ADB exported the console. No device partition write command ran.

```sh
CORE=/root/elish-build-20261123/UniXNU
BASE="$CORE/out/elish-darwinboot-20261125"
OUT="$BASE/run38-ufs-handoff-library-flag"
python3 scripts/build-loader-probe.py --unixnu "$CORE" --uefi /root/repo/uefi-elish \
  --dtb /tmp/elish-uefi-probe.dtb --out "$OUT" --feeds "$CORE/out/feeds" \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-handoff-readonly \
  --payload "$BASE/run28-restore-launchd-cyan/payload.img"
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$OUT/boot.img"
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$CORE/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img"
adb -s 4bed07e4 root
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0
```
