# elish run35 passive UFS queue snapshot, 2026-11-26

The run35 UEFI loader probe extends the run33 read-only path. After validating
MMIO descriptors, it requires the UFS core reset to be deasserted, the PHY
GDSC powered, and the core and interface clock gates enabled. It then samples
UFSHCI status and the transfer/task request-list base, run/stop, and doorbell
registers. The probe does not dereference list addresses or submit commands.
It logs the existing IORT and SMMU snapshots and requests a warm reboot into
fastboot before DarwinBoot. The normal firmware excludes the probe.

The stock elish Android R clock provider at `52a2f180865f8368828985d6384130acd093fa93`
maps `GCC_UFS_PHY_BCR` to `0x77000` with the default bit index 0. Its
`qcom_reset_assert()` sets `BIT(map->bit)` and its deassert operation clears
it. The run31/run32 raw value `0` therefore indicates that this reset line was
deasserted at capture time. The HCI CAP `0x1587031f` has the 64-bit addressing
capability bit set. The run33 SMMU ID2 `0x00005111` encodes 36-bit IAS, OAS,
and UBS using the matching Lineage kernel's `arm_smmu_id_size_to_bits()`.
These are controller and SMMU capabilities. Live UFS routing, queue ownership,
and the mapping of every DMA buffer remain unverified.

## Host build

The `build-loader-probe.py` pipeline completed the board preflight, DEBUG
firmware build, and Android boot-image packaging. An earlier reader upload
after run34 stalled and the bootloader stopped returning bulk IN responses.
The fresh bootloader session restored control.
[Transport evidence](elish-run34-android-kernel-20261126.md#reader-attempt-after-manual-fastboot-entry)

Artifacts and `probe.json` are under
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run35-ufs-queues-readonly/`.

| Input or output | Identity |
| --- | --- |
| Device base | `f0b9d57f5088f80f7055e80599cb80a0d3877db3`, with local changes |
| Device build helper SHA-256 | `6e875cb5e32173ab663211e426729535c2e3f3e8eed8b3de230bc8341e1e4595` |
| UniXNU core base | `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, with existing local changes |
| UEFI base | `d489a050255f90b9c011d25b95c15c1b12f382f0`, with local probe changes |
| `ElishLoaderProbeDxe.c` SHA-256 | `791b8ffabf1a589c35cc3ba11fbaa85b3b8012e59e8a2f0cf103bff6b626417e` |
| Reused run28 payload SHA-256 | `f06570608c20cca5a47c826b1f813ced5efe87d816b9b0429af68b22c2dc523f` |
| DTB SHA-256 | `0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48` |
| `SILICIUM_UEFI.fd` | 3,145,728 bytes, SHA-256 `870340e558ef1a8cd7d3ef202180e89fb0025cca2b45405cffd7104a3fc33472` |
| `boot.img` | 235,528,192 bytes, SHA-256 `bfca4d3e00af3ace7808d6f9f2be5213c90aac745983a92ba322086ca24880a7` |

```sh
python3 scripts/build-loader-probe.py \
  --unixnu /root/repo/UniXNU --uefi /root/repo/uefi-elish \
  --payload /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img \
  --dtb /tmp/elish-uefi-probe.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run35-ufs-queues-readonly \
  --feeds /root/elish-build-20261123/UniXNU/out/feeds \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-clocks-readonly
```

## Device run and log recovery

Bootloader USB `18d1:d00d`, serial `4bed07e4`, and `getvar product=elish`
were verified before the RAM-only boot. The first attempt returned both
`Sending OKAY` and `Booting OKAY` in 5.888 seconds and re-enumerated as
fastboot (USB device 23 to 24). `fastboot reboot recovery` succeeded, but
Recovery root ADB found an empty pstore directory.

A repeat using the same verified probe image returned both `OKAY` results
in 5.893 seconds and re-enumerated as fastboot (USB device 27 to 28).
The verified RAM-only Lineage reader then booted in 1.579 seconds. ADB first
returned UID 2000 with no read access to pstore. `adb root` restarted adbd
with UID 0 and exposed the 54,782-byte console record. Its SHA-256 is
`789508cbfad15b69e8836c1f0b4a7182142ca50855ad87b713d7ef008494a17c`.
[Raw console](elish-run35-ufs-queues-console-ramoops-20261126.txt)

```sh
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run35-ufs-queues-readonly/boot.img
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img
adb -s 4bed07e4 root
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0
```

## Register results

Core reset raw `0x00000000`, PHY GDSC `0xF822F000` and the clock gates
satisfied the HCI read checks. `CAP=0x1587031F`, `VER=0x00000300`,
`IS=0x00000400`, `HCS=0x0000010F` and `HCE=0x00000001`.

| Request list | Register base value | Run/stop | Doorbell |
| --- | --- | --- | --- |
| Transfer | `0x000000009B147000` | `1` | `0` |
| Task management | `0x000000009B148000` | `0` | `0` |

The transfer list was running with no active doorbell bits in this sample.
Both address values fall numerically inside the UEFI DXE heap range
`0x98900000–0x9BBFFFFF`. The values alone establish neither a physical
address nor ownership. The probe left the list memory, HCI, and SMMU
untouched and logged its warm restart before DarwinBoot.

Active ACPI had no IORT. `sCR0=0x00230406` had `CLIENTPD=0`, `USFCFG=1`.
The only valid non-secure stream match was group 4, SID `0x820`, mask `0x402`,
CB4, attributed to MDSS SDE. CB6 and CB73 again returned upper bits in SCTLR,
with different values from run33. Their access and ownership remain open.

The normal Kona SEC stage calls `ArmSmmuDetach`, preserving matches for the
display SIDs and clearing the valid bit on other visible stream entries.
Run35 observes the later DXE state. Sampling before that SEC step is needed
to test whether bootloader firmware leaves a visible UFS route. Native UFS
commands and DMA remain gated on the live route, context ownership and DMA
addressability.
