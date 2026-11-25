# elish UFS clock/IORT/SMMU probe result, 2026-11-25

## Scope and result

Added an opt-in UEFI probe that validates the board's UFS/PHY/GCC MMIO descriptors, reads Kona GCC clock registers, reports ACPI IORT presence, scans SMMUv2 stream state, and warm-restarts before DarwinBoot. It performs no UFS/SMMU writes and leaves the normal firmware configuration disabled.

The firmware and RAM-only boot image built successfully. The first device attempt timed out during bulk transfer, but a later RAM-only retry did execute the image: its recovered ramoops log reaches the DarwinBoot hook, samples the GCC clock branches, scans all 87 SMMU groups, and warm-restarts before DarwinBoot. The original timeout was not evidence that the image never ran. The last confirmed XNU/userspace milestone remains run28's Apple `launchd` PID 1 checkpoint on the Restore APFS ramdisk; the complete System root remains untested.

Run30 reports the UFS host, PHY and ICE windows as mapped. Kona GCC branches read enabled, including core/interface, UniPro, ICE, lane-sync and aggregate clocks; four RCG command registers read zero. This does not establish rates or active PHY/link state. No UFS HCI, PHY, ICE, reset, GDSC or rail register was read. The UEFI ACPI configuration has no IORT table. SMMU group 4 is the only valid group: SID `0x820`, mask `0x402`, S2CR type 0/context bank 4; CBAR `0x1f000`, SCTLR `0xe0` with `M=0`, TTBR0/TCR zero. The live FDT identifies SID `0x820` as MDSS SDE, not UFS. No UFS stream or DMA path is established.

No persistent partition was written; DMA remains disabled. Full decoded output: [run30 ramoops log](elish-run30-ufs-clocks-console-ramoops-20261125.txt).

## Inputs and build

| Input | Identity |
| --- | --- |
| Device repository | `f0b9d57f50` baseline; local porting-record changes retained |
| UniXNU core | `71c5c6569bae6369c0e0f634df8b9bd5afc19139`; existing local changes used |
| UEFI base | `d489a050`; local read-only probe changes used |
| Payload | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img`, SHA-256 `f06570608c20cca5a47c826b1f813ced5efe87d816b9b0429af68b22c2dc523f` |
| DTB | `/tmp/elish-uefi-probe.dtb`, SHA-256 `0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48` |
| Build output | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run30-ufs-iort-smmu-readonly/` |
| Firmware FD | 3,145,728 bytes, SHA-256 `51f1b71ea06c45290d21c479b10836789ad8fe5383253a4dcd1a49ab61a24966` |
| `boot.img` | 235,528,192 bytes, SHA-256 `31a5e9dc5b23e5e89670de34c2536a0bd84638f46d1f30da0ed1fc5dfd89d138` |

Build command:

```sh
python3 scripts/build-loader-probe.py \
  --unixnu /root/repo/UniXNU \
  --uefi /root/repo/uefi-elish \
  --payload /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img \
  --dtb /tmp/elish-uefi-probe.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run30-ufs-iort-smmu-readonly \
  --feeds /root/elish-build-20261123/UniXNU/out/feeds \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-clocks-readonly
```

The resulting manifest describes validation of UFS/GCC windows, read-only Kona clock sampling, IORT/SMMUv2 capture, and warm restart before DarwinBoot.

## Device attempt

Before boot, `fastboot devices -l` listed `4bed07e4` on USB `1-2`; `getvar product` returned `elish` and `getvar serialno` returned `4bed07e4`. Device USB ID was `18d1:d00d` at 480 Mbit/s.

```sh
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run30-ufs-iort-smmu-readonly/boot.img
```

The first invocation stalled at `Sending 'boot.img' (230008 KB)` and timed out after 120 seconds. After fastboot became responsive, the same RAM-only image was retried and accepted; the recovered log shows the loader probe's clock/GCC/SMMU output and planned warm restart. This direct device log proves execution despite the earlier timeout. It records no UFS host/PHY/ICE register reads and no storage command or DMA.

The recovered log is `reports/elish-run30-ufs-clocks-console-ramoops-20261125.txt`; the verified reader image used for ramoops recovery remains `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`, SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`.

## Next step and limits

The read-only Android snapshot reports UFS runtime `suspended`, `ufs_phy_gdsc=disabled`, and `ufs_card_gdsc=disabled`; these are not UEFI handoff values. The later gated run31 read the UFS PHY GDSC (`0xF822F000`, powered) and HCI status registers only after core/interface clocks and GDSC were on; the raw reset register was `0`, without interpreting its polarity. ACPI IORT remained absent and the only valid SMMU group is the display SDE stream, so no UFS command/DMA followed. See [run31 evidence](elish-run31-ufs-hci-20261125.md).

Setup and persistent Data are also unverified. No full-System-root or Data partition write has been authorized or performed.
