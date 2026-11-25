# elish run40 UFS capacity and GPT readback

Run40 requalified the run38 SMMU routes, passed the run39 NOP and read native LUN 0 through the data PRD at `0x9A71B000–0x9A720000`. `READ CAPACITY(10)` returned last LBA `61880319` and logical block size `4096`. Capacity is 253,461,790,720 bytes.

| Check | On-device result |
| --- | --- |
| Known LBA 1 | CRC32 `FA250023`, matches Android |
| Primary GPT | Capture CRC32 `4F1522CE`, header `7C8FE76B`, entries `62C2DBF6` |
| Backup GPT | Capture CRC32 `294CAD87`, header `3E1BA72E`, entries `62C2DBF6` |
| `userdata` | Entry 34 in both copies, inclusive LBA `6619136–61870314` |
| Queue and SMMU | Stopped, doorbell `0`, GFSR/FSR2/FSR3 unchanged at `0/0x400/0x400` |

The capture CRCs match the independent [Android GPT export](elish-run39-ufs-nop-20261126.md). Each GPT header and entry-array CRC also matched the computed value. The probe stopped before DarwinBoot and issued only NOP, READ CAPACITY and READ(10).

Outputs are under `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run40-ufs-gpt/`. Base revisions match [run36](elish-run36-sec-smmu-20261126.md#build-and-device-sequence). The SEC and Kona source inputs match [run37](elish-run37-ufs-handoff-20261126.md). The build ran board preflight, compiled DEBUG firmware and verified Android v3 packaging. `probe.json` pins the run28 FAT payload and DTB inputs.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` (235,532,288 bytes) | `f218105c06724e2ef8cd4c0cb1e597edd1abaf4152c80a4be2bc827fa387c59d` |
| `SILICIUM_UEFI.fd` | `574174a5b4721dd2ded155037b58a22fdd26bf870720f42f6ab1f6306724b3bb` |
| `elish.dsc` | `df1f76cfa453772b3047a92c4c2678081c23627f69d35c8b65fe83e6ca265b37` |
| `ElishLoaderProbeDxe.c` | `1ea1e13f0f562c25a23da6edccdeb664f9d27dd5138d830db1633c6e5510ea52` |
| `build-loader-probe.py` | `f57cfa9c4135c6fd736ff38768e31dc2847afaa4e7ec4d3003e7654eb62d8a95` |
| [Console ramoops](elish-run40-ufs-gpt-console-ramoops-20261126.txt) (57,337 bytes) | `40256a2af809e9ca1e6addbfecfd42ae7441b46bb4d3d94ba50042c91a58cd9a` |

USB serial `4bed07e4` and fastboot product `elish` matched before both RAM boots. The probe boot completed in 5.914 s. Its warm restart changed USB device 50 to 51. The verified reader booted in 1.577 s and root ADB recovered the console.

```sh
CORE=/root/elish-build-20261123/UniXNU
BASE="$CORE/out/elish-darwinboot-20261125"
OUT="$BASE/run40-ufs-gpt"
python3 scripts/build-loader-probe.py --unixnu "$CORE" --uefi /root/repo/uefi-elish \
  --dtb /tmp/elish-uefi-probe.dtb --out "$OUT" --feeds "$CORE/out/feeds" \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-gpt \
  --payload "$BASE/run28-restore-launchd-cyan/payload.img"
adb -s 4bed07e4 reboot bootloader
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$OUT/boot.img"
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$CORE/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img"
adb -s 4bed07e4 root
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0
```
