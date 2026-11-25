# elish run33 SMMU context-state read-only probe, 2026-11-25

## Result

Run33 used a RAM-only `fastboot boot` image on serial `4bed07e4`; preflight identified `M2105K81AC`, Android device `elish`, kernel `4.19.325-cip136-st20-perf-g6b34d2476532`, and fastboot product `elish`. Fastboot accepted the image (`Sending OKAY`, `Booting OKAY`). The probe warm-restarted before DarwinBoot. A separate RAM-only Lineage reader recovered the console ramoops. No UFS command, DMA, SMMU write, or persistent partition write was issued.

The active ACPI configuration had no IORT. The SMMU read-only probe reported region `0x15000000+0xD0000`, `sCR0=0x00270406` (`CLIENTPD=0`, `USFCFG=1`), `ID0=0x4C017E57`, `ID1=0x6000004A`, and `ID2=0x00005111`. ID1 reports 74 context banks. Only stream-match group 4 was valid: `SMR=0x84020820`, SID `0x820`, mask `0x402`, S2CR type 0, context bank 4. This remains the MDSS SDE route identified by the live FDT, not a UFS route.

The all-context read-only scan returned `CBAR=0x000100FF` and `SCTLR=0xE0` for banks 0–3 and 5; bank 4 returned `CBAR=0x0001F000` and `SCTLR=0xE0`. `TTBR0` and `TCR` were zero for these banks. Banks 6–73 returned `CBAR=0`, `TTBR0=0`, `TCR=0`, but nonzero, high-bit `SCTLR` values (for example CB6=`0x20CDA71A`, CB73=`0x0775A314`). Those readings are not sufficient to label the banks free, accessible, or owned; the context-register view/offset must be resolved before any bank can be reused. No context bank was selected.

## Stock XBL IORT reference

A host-only extraction of the supplied stock `xbl.img` found an embedded IORT table. The source image SHA-256 is `9a3a3bb92e6cf478b227f70dd1bd140fde32982994a6c34a07bd314ea0acb72f`; the extracted AML and disassembly are `/tmp/elish-stock-xbl-iort-20261125.aml` (SHA-256 `74ef2245bc177ce64b10131642ec444fe9ab1382cf562b680f8ec8580ae550b7`) and `/tmp/elish-stock-xbl-iort-20261125.dsl` (SHA-256 `ae0860d4aecb6140b02d460b68495bc4985e8821ee48a5056201b84f328e4ae4`). The IORT header identifies OEM `QCOM`, table ID `2KDEMOCQ`, 25 nodes and length `0x1174`; its checksum byte is zero and `iasl` reports the expected checksum as `0xB8`. Treat it as a stock-firmware reference, not an active/validated runtime table.

In that table, SMMUv2 node offset `0x30` has base `0x15000000` and span `0xE0000`. The `\\_SB_.UFS0` Named Component at offset `0x760` has two mappings to node `0x30`: input ID `0` → output ID `0x4E0`, and input ID `0x10` → output ID `0xE0`; the raw byte for its `Device memory address size limit` field is `0x24` (36 decimal). The stock HALIOMMU audit confirms `UFSDxe` calls attach for `\\_SB_.UFS0` with ARIDs `0` and `0x10` and resolves them through IORT. This therefore provides **candidate stock-XBL stream IDs** `0x4E0` and `0xE0`, not proof of the live UEFI mapping or current SMMU ownership. The current valid group-4 match does not match either candidate, and active ACPI has no IORT. The UEFI `MemoryMapLib` descriptor is `0x15000000+0xD0000`, 64 KiB shorter than the stock table's SMMU span; no descriptor change was made.

ARM DEN 0049E, Table 13 ([IORT specification](https://documentation-service.arm.com/static/5f8eb325f86e16515cdbe428)), defines this byte as the number of address bits the device can generate when accessing memory. The stock value `0x24` therefore describes a 36-bit (64 GiB) device-generated address space; the separately measured DRAM upper endpoint `0x280000000` is below that limit. This does not establish DMA reachability or SMMU ownership. The extracted table has an invalid ACPI checksum and is not active in run33, so the value is reference evidence only.

Recovery ADB's read-only `/proc/iomem` capture separately reports SMMU range `0x15000000-0x150FFFFF` (1 MiB); see `reports/elish-recovery-proc-iomem-20261124.txt` and `reports/elish-recovery-memory-map-20261124.md`. The run33 `ID1` decode gives 128 4-KiB global pages, so the context-bank window begins at offset `0x80000`; CB73 occupies `0xC9000-0xCA000`, inside the UEFI probe's `0xD0000` descriptor. The three sources therefore disagree on aperture length (UEFI `0xD0000`, stock IORT `0xE0000`, Recovery Linux `0x100000`), but the UEFI descriptor includes all 74 ID1-reported context-bank pages. The length mismatch alone does not explain the high-bit CB6–73 reads; their access/ownership semantics remain unresolved.

The live Android snapshot separately records `arm_smmu` `disable_bypass=Y`; that policy and run33 `USFCFG=1` mean an unmatched stream is configured to fault, but neither observation identifies the runtime UFS route. Do not enable UFS DMA from these candidate IDs alone.

### Android live-kernel cross-check

After the RAM-only reader returned to Android, read-only ADB reconfirmed model `M2105K81AC`, device `elish`, kernel `4.19.325-cip136-st20-perf-g6b34d2476532`, and root UID. The UFS platform device still linked to `ufshcd-qcom` and had no `iommu_group` link. Its live FDT node `/sys/firmware/devicetree/base/soc/ufshc@1d84000` reports `compatible=qcom,ufshc`, `status=okay`, and `reg=<0x1d84000 0x3000 0x1d90000 0x8000>`; `iommus`, `iommu-map`, `dma-ranges`, and `dma-coherent` are absent. The apps-SMMU node has `qcom,skip-init`. In matching Linux source revision `6b34d24765327d746074c8e53b5ad0d24a51acc7`, `drivers/iommu/arm-smmu.c` resets SMRs/S2CRs and context banks only when this option is absent; it still sets `sCR0.USFCFG` when `disable_bypass=Y`. Thus retained preexisting SMMU state is a plausible Android UFS route, not proof of a particular SID/context or of UEFI ownership. `/proc/config.gz` reports `CONFIG_DEBUG_FS` unset, `/sys/kernel/debug` and `/dev/mem` are absent, and the apps-SMMU platform node has no sysfs `resource`, `resource0`, or `resource1`; the standard user-space interfaces checked do not expose retained SMMU registers. Android does not expose `/sys/firmware/acpi`. `/sys/kernel/iommu_groups/27` links to `ae00000.qcom,mdss_mdp:qcom,smmu_sde_unsec_cb`; no group device path matched UFS. The run33 capture remains the evidence for pre-DarwinBoot UEFI ACPI and SMMU state.

The device query and matching-source inspection were read-only:

```sh
adb -s 4bed07e4 shell 'for p in /sys/devices/platform/soc/1d84000.ufshc/driver /sys/devices/platform/soc/1d84000.ufshc/iommu_group; do if [ -L "$p" ]; then readlink "$p"; else echo "$p absent"; fi; done'
adb -s 4bed07e4 shell 'for g in /sys/kernel/iommu_groups/*; do for d in "$g"/devices/*; do [ -e "$d" ] || continue; case "$d" in *mdss*|*ufshc*) echo "$g: $(readlink -f "$d")";; esac; done; done'
adb -s 4bed07e4 shell 'grep " /sys/kernel/debug " /proc/mounts; ls -l /dev/mem; ls -ld /sys/firmware/acpi'
adb -s 4bed07e4 shell 'p=/sys/devices/platform/soc/1d84000.ufshc/of_node; echo "node=$(readlink -f "$p")"; for f in compatible status reg iommus iommu-map dma-ranges dma-coherent; do if [ -e "$p/$f" ]; then printf "%s=" "$f"; od -An -tx1 "$p/$f" | tr -d "\n"; echo; else echo "$f=absent"; fi; done; echo "DMA-related sysfs:"; ls -1 /sys/devices/platform/soc/1d84000.ufshc | grep -E "dma|iommu|driver|of_node" || true; echo "platform driver=$(readlink -f /sys/devices/platform/soc/1d84000.ufshc/driver)"'
adb -s 4bed07e4 shell 'p=/sys/firmware/devicetree/base/soc/apps-smmu@15000000; [ -e "$p/qcom,skip-init" ] && echo qcom,skip-init=present || echo qcom,skip-init=absent; zcat /proc/config.gz | grep -E "^(# )?CONFIG_DEBUG_FS"; [ -e /sys/kernel/debug ] && echo debugfs-path=present || echo debugfs-path=absent; [ -e /dev/mem ] && echo devmem=present || echo devmem=absent'
git -C /tmp/android_kernel_xiaomi_sm8250-lineage-24.0 show 6b34d24765327d746074c8e53b5ad0d24a51acc7:drivers/iommu/arm-smmu.c | sed -n '4898,4928p'
adb -s 4bed07e4 shell 'p=/sys/bus/platform/devices/15000000.apps-smmu; for f in resource resource0 resource1; do if [ -e "$p/$f" ]; then printf "%s=" "$f"; ls -l "$p/$f"; else echo "$f=absent"; fi; done'
```

The group query printed only UFS- and SDE-related paths; unrelated IOMMU clients are omitted. The FDT query also made no changes. No debugfs enable/mount, MMIO access, kernel setting change, reboot, or partition write was performed.
## Inputs and artifacts

| Item | Identity |
| --- | --- |
| Device repository | `4b3da68ea0aca815a80e8d1bad802dfec8d5c905`, existing local changes retained |
| UniXNU build checkout | `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, existing local build changes retained |
| UEFI checkout | `d489a050` base plus local read-only probe changes |
| Reused run28 payload | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img`, SHA-256 `f06570608c20cca5a47c826b1f813ced5efe87d816b9b0429af68b22c2dc523f` |
| DTB | `/tmp/elish-uefi-probe.dtb`, SHA-256 `0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48` |
| Firmware FD | `/root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd`, SHA-256 `3a9ca3cd64619d9775a5e91905635a74c5e4909b57bc2367c134940b941a2cd5` |
| RAM-only boot image | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run33-smmu-context-state-readonly/boot.img`, 235,528,192 bytes, SHA-256 `c650c13b6bba566b959d32ba2ee0d867300e902069ce2aa725faf0a457673f7c` |
| Recovered log | `reports/elish-run33-smmu-context-console-ramoops-20261125.txt`, 53,966 bytes, SHA-256 `4a3d2bf2ce1fd4ee98e12ab2ce4d5c1b425a72e70f74dd5ba07a5228344e048b` |

The run33 firmware was rebuilt from the run32 device directory with the opt-in SMMU probe; the boot image reused the run28 payload:

```sh
BASE=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125
OUT="$BASE/run33-smmu-context-state-readonly"
mkdir -p "$OUT"
cp -a "$BASE/run32-smmu-control-readonly/device" "$OUT/device"
/root/elish-build-20261123/UniXNU/unixnu build firmware \
  -D "$OUT/device" --os ipados-17.7.11-21H461 --out "$OUT/build" \
  --uefi /root/repo/uefi-elish --edk2 /root/repo/uefi-elish/Mu_Basecore \
  --clang-bin /usr/lib/llvm-21/bin -j 8 --target DEBUG
/root/elish-build-20261123/UniXNU/unixnu bootimg \
  -D "$OUT/device" --os ipados-17.7.11-21H461 --out "$OUT/build" \
  --uefi /root/repo/uefi-elish --edk2 /root/repo/uefi-elish/Mu_Basecore \
  --fd /root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
  --dtb /tmp/elish-uefi-probe.dtb \
  --ramdisk "$BASE/run28-restore-launchd-cyan/payload.img" \
  --output "$OUT/boot.img"

adb -s 4bed07e4 shell 'getprop ro.product.model; getprop ro.product.device; uname -r'
adb -s 4bed07e4 reboot bootloader
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot "$OUT/boot.img"
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img
adb -s 4bed07e4 root
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0
```

## Next step and limit

The latest confirmed XNU/userspace checkpoint remains run28: Apple `launchd` reached `main()` as PID 1 from the 21H461 Restore APFS root through RamBlock. Run33 stopped before DarwinBoot/XNU and says nothing new about root mounting. The full System root, Setup, and persistent Data remain unverified.

Before any UFS command or DMA, validate the stock-XBL IORT provenance/checksum and determine the live mapping for both UFS ARIDs, the SMMU context-bank visibility/ownership, and the full DMA address range. Keep DMA disabled. After those facts are proven, proceed in order with bounded NOP, capacity, known-block readback, GPT, and OS/SystemOS window checks; then verify the System UUID, APFS mount, and PID 1 checkpoint. No partition write is authorized without a named target, backup, and recovery plan.
