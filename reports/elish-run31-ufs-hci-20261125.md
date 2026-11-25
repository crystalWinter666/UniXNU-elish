# elish run31 UFS power-gated status probe, 2026-11-25

## Result

The first RAM-only upload stalled, but the probe ran after a host-side USB reauthorization and a second RAM-only `fastboot boot`. The recovered `console-ramoops-0` log proves the probe reached the DarwinBoot loader hook and returned to fastboot before DarwinBoot. No XNU kernel was entered in this diagnostic run.

Run31 validated the UFS/GCC/PHY/ICE MMIO descriptors. Kona GCC core and interface gates were enabled, and the UFS PHY GDSC was powered (`0xF822F000`, bit 31 set). Only then did the probe read UFSHCI status registers. Values: CAP `0x1587031F`, VER `0x00000300`, IS `0x00000400`, HCS `0x0000010F`, HCE `0x00000001`. The UFS core block-reset register at GCC offset `0x77000` read `0x00000000`; the probe did not write it and does not infer reset polarity from this raw value. Four clock RCG command registers read zero; these values do not establish clock rates. The PHY and ICE register blocks themselves were not read.

The active ACPI configuration has no IORT table. The only valid apps-SMMU stream-match group remains group 4, SID `0x820`/mask `0x402`, S2CR type 0/context bank 4; CBAR `0x1F000`, SCTLR `0xE0` (`M=0`), TTBR0/TCR zero. The live FDT attributes that tuple to MDSS SDE, not UFS. Run31 performed no UFS command, DMA, SMMU write, or persistent partition write. The UFS SID/context and DMA addressability remain unknown, so no NOP, capacity, block, GPT or root-volume access was attempted.

The last confirmed XNU/userspace milestone remains run28: Apple `launchd` reached `main()` as PID 1 from the Restore APFS root through RamBlock. The full System root, Setup and persistent Data remain unverified.

## Inputs and artifacts

| Input/output | Identity |
| --- | --- |
| Device repository | `f0b9d57f50` baseline; local porting-record and probe documentation changes retained |
| UniXNU build checkout | `71c5c6569bae6369c0e0f634df8b9bd5afc19139`; existing local changes retained |
| UEFI checkout | `d489a050` base with local read-only probe changes |
| Payload | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img`, SHA-256 `f06570608c20cca5a47c826b1f813ced5efe87d816b9b0429af68b22c2dc523f` |
| DTB | `/tmp/elish-uefi-probe.dtb`, SHA-256 `0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48` |
| Output directory | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run31-ufs-host-status-gated/` |
| Firmware FD | 3,145,728 bytes, SHA-256 `838964f207748cc42db6e1129f81e9e2cb9454a68c8ba128840e7830379aba4c` |
| `boot.img` | 235,528,192 bytes, SHA-256 `fb7d0ea8f68b8716eacc9e2e3eeff3d98ebd584be9ba74ea7dda412f75ce8f5c` |
| Recovered UEFI log | `reports/elish-run31-ufs-hci-console-ramoops-20261125.txt`, 45,518 bytes, SHA-256 `b1d4e692456c4f83d3dd59a328d0e5602cf4ce74a06e072f1c6b55c00ee0d20d` |
| Ramoops reader | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`, SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4` |

## Build

```sh
python3 scripts/build-loader-probe.py \
  --unixnu /root/repo/UniXNU \
  --uefi /root/repo/uefi-elish \
  --payload /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img \
  --dtb /tmp/elish-uefi-probe.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run31-ufs-host-status-gated \
  --feeds /root/elish-build-20261123/UniXNU/out/feeds \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-clocks-readonly
```

Firmware compilation and Android v3 image packaging succeeded. The manifest records the payload/DTB hashes and output hashes above. The probe remains opt-in; routine board configuration keeps it disabled.

## Device sequence

ADB preflight confirmed serial `4bed07e4`, product `elish`, model `M2105K81AC`, and kernel `4.19.325-cip136-st20-perf-g6b34d2476532`. `adb reboot bootloader` entered fastboot; `fastboot getvar product` returned `elish` (the bootloader does not implement `getvar serialno`).

First upload attempt:

```sh
timeout 240 fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run31-ufs-host-status-gated/boot.img
```

It timed out at `Sending 'boot.img' (230008 KB)` after 240 seconds. The device still enumerated as USB `18d1:d00d` at 480 Mbit/s and `fastboot devices -l` listed `4bed07e4`; bounded `getvar product` timed out and ADB was absent. No fastboot process remained. The host USB authorization for `/sys/bus/usb/devices/1-3` was toggled off/on; the device re-enumerated and `getvar product` returned `elish`.

The same RAM-only boot command then returned `Sending OKAY` in 5.787 seconds and `Booting OKAY` in 0.237 seconds. After the probe's warm restart, fastboot again reported `product=elish`. The verified reader was also booted with `fastboot boot`; its upload and boot both returned `OKAY`. Root ADB identified the reader environment as `elish` / `M2105K81AC`, kernel `4.19.325-cip136-st20-perf-g6b34d2476532`; `/sys/fs/pstore/console-ramoops-0` held the 45,518-byte log saved above.

## Next step and limits

Use the stock XBL/HALIOMMU evidence to resolve the `\\_SB_.UFS0` ARID-to-IORT mapping and reconcile it with the live SMMU state. Run31 proves the HCI window is readable when its observed GDSC and core/interface gates are on, but it does not prove which DMA path UFS uses or that XNU-owned buffers are reachable. Keep DMA and UFS commands disabled until the UFS stream/context and address range are established. Then continue with bounded NOP, capacity, known-block, GPT, OS/SystemOS window, System-volume UUID, APFS mount, and Apple PID 1 checks in order. Do not write persistent partitions without the specified target, backup, restoration plan, and explicit authorization.
