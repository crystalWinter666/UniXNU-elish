# elish active UFS firmware readback, 2026-11-25

## Scope and safety

After the user authorized ADB, this pass tried bootloader readback, then exported
the active firmware from Recovery. It used no flash, erase, or `fastboot continue`.
Recovery root ADB exported the images with ordinary Linux block reads through
the running UFS host; no native UniXNU UFS diagnostic command, manually
programmed DMA, SMMU write, or persistent partition write was issued.
The initial readback from fastboot was unavailable because the bootloader does not
support `fetch` (`Unable to get max-fetch-size`). `fastboot reboot recovery`
was accepted; Recovery ADB then reported root.

Identity in Recovery: model `M2105K81AC`, device `elish`, serial `4bed07e4`,
slot `_b`; Android build `Xiaomi/elish/elish:13/RKQ1.211001.001/V816.0.2.0.TKYCNXM:user/release-keys`.
After export, `adb reboot` returned to Android; the same identity returned,
`sys.boot_completed=1`, and ADB was back as `uid=2000(shell)`.

## Active-slot readback

The full partition images are host-only at
`/tmp/elish-recovery-firmware-20261125/`. Each active partition is larger than
the corresponding supplied ROM file. The ROM file is an exact prefix of each
active partition; the full-partition hashes therefore must not be compared
directly to the smaller ROM-file hashes.

| Active partition | Bytes read | Full partition SHA-256 | Supplied ROM payload bytes | ROM/prefix SHA-256 | Prefix match |
| --- | ---: | --- | ---: | --- | --- |
| `xbl_b` | 8,388,608 | `1f83e3e89a7b119702802bb78a113a53e02e81b76639b88839ab0007c3e01c28` | 3,579,904 | `9a3a3bb92e6cf478b227f70dd1bd140fde32982994a6c34a07bd314ea0acb72f` | exact |
| `xbl_config_b` | 8,343,552 | `3d470f4d5584dd7b3ced8d2b9b639e5d5bc10deca648a9d8c96c2bc14cdb87fa` | 102,400 | `f290260d3c38c9c93db5790c6550846871f5c3dbc1607433af52c1cd19e184bd` | exact |
| `abl_b` | 2,097,152 | `4f35f73e066c573513b93aefefb403140d2351dd563c77112c3674253c3f5077` | 208,896 | `53e95850b306178b46f96abe68ca0f64952a3f43abffffc3218425a1527fbe8c` | exact |
| `imagefv_b` | 50,331,648 | `3384ad4ec503dff20e4dcca091d978206c2c57edbc8597ebf1146d15be459960` | 2,097,152 | `4b354ee876cb2518da70f94cafad6deff5ceeddf1891199860c3669fd6e7cf0e` | exact |

The XBL readback contains literal strings `QC_IMAGE_VERSION_STRING=BOOT.XF.3.2-00304-SM8250-2`,
`OEM_IMAGE_VERSION_STRING=pangu-build-bp-34186-pgqjx-vj3dj-j3wcr`,
`EnableUfsIOC = 0` and `UfsSmmuConfigForOtherBootDev = 1` are literal firmware
configuration strings. Their semantics do not establish a live UFS stream,
context-bank ownership, coherency mode, or DMA addressability.

## Public-source cross-check

The directly inspected upstream Linux
[`sm8250.dtsi`](https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sm8250.dtsi#L2510-L2524)
defines the generic UFS host at `0x1d84000` with IOMMU SIDs `0xE0` and
`0x4E0`. The upstream
[elish common DTSI](https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sm8250-xiaomi-elish-common.dtsi)
inherits that SoC node and adds UFS supply configuration. This supports those
values as Linux/stock-XBL candidates, not as the active UEFI SMMU route. The
installed Android FDT still omits UFS `iommus`, `iommu-map`, `dma-ranges`, and
`dma-coherent`; run33's active ACPI has no IORT. The checked upstream mapping
does not support a UFS SID of `0x300`.

The run33 stock-XBL IORT extraction maps UFS0 input IDs `0` and `0x10` to
candidate SIDs `0x4E0` and `0xE0`; the exact active-slot B XBL payload prefix
match now confirms that this supplied XBL payload is installed in the active
slot. The table still has an invalid ACPI checksum and was not present in
run33's active ACPI, so the extracted mappings remain unvalidated runtime
routing evidence. Run33's only valid observed SMMU group remains SID `0x820`,
mask `0x402`, CB4, attributed by live FDT to MDSS SDE.

Recovery's same Android kernel still exposes no `/sys/kernel/debug`, `/dev/mem`,
SMMU `resource*` files, or UFS `iommu_group`. The Recovery query filtered for
UFS/MDSS and found no UFS group; its only matches were MDSS secure/nonsecure.
Recovery `dmesg` independently reports
UFS reinitialization from Gear 1 / one lane / SLOWAUTO to Gear 4 / two lanes /
FAST, but does not disclose the UFS SMMU route.

## Reproduction

```sh
fastboot -s 4bed07e4 getvar current-slot
fastboot -s 4bed07e4 fetch xbl_b /tmp/xbl_b.img  # unsupported by this bootloader
fastboot -s 4bed07e4 reboot recovery
adb -s 4bed07e4 get-state                       # recovery
adb -s 4bed07e4 shell 'blockdev --getsize64 /dev/block/by-name/xbl_b'
adb -s 4bed07e4 exec-out sh -c 'dd if=/dev/block/by-name/xbl_b bs=1048576 2>/dev/null' > /tmp/elish-recovery-firmware-20261125/xbl_b.img
cmp -n 3579904 /tmp/elish-recovery-firmware-20261125/xbl_b.img /root/repo/elish_os1.0.2.0/xbl.img
adb -s 4bed07e4 reboot
```

The same readback and prefix comparison were done for `xbl_config_b`, `abl_b`,
and `imagefv_b`. Run33 state and its stock-IORT checksum findings are recorded
in [the SMMU report](elish-run33-smmu-context-20261125.md).

## Android kernel-log cross-check · 2026-11-26

ADB read-only identity check on serial `4bed07e4` reported model `M2105K81AC`,
device `elish`, `sys.boot_completed=1`, and kernel
`4.19.325-cip136-st20-perf-g6b34d2476532`. `dmesg` was denied to the
unprivileged Android shell (`klogctl: Permission denied`); `logcat -b kernel`
was readable.

The complete kernel-buffer stream was SHA-256
`d784febd81b27f4e89cfa9f57b1162add20ea9232fb3bca9b48e9dbf457801bb` before
filtering. The filtered capture shows `15000000.apps-smmu` with 87 stream-match
groups; `ufshcd-qcom` binds `1d84000.ufshc`, registers SCSI host0 and reaches
Gear 4 / two-lane FAST mode. Other clients have explicit `Linked as a consumer`
and `Adding device ... to group` log entries. No UFS SMMU-consumer or IOMMU-group
entry appears in the complete filtered buffer. This confirms no Linux IOMMU
attachment is visible for UFS; it does not prove physical bypass or identify
the UEFI ARID-to-SID route/context.

Collection used no reboot, UFS diagnostic command, SMMU MMIO access/write, or
persistent partition write. The exact command and capture hash are preserved
below for reproduction.

```sh
adb devices -l
adb -s 4bed07e4 shell 'getprop ro.product.model; getprop ro.product.device; getprop sys.boot_completed; uname -r; dmesg'
adb -s 4bed07e4 exec-out logcat -b kernel -d -v brief | tee >(wc -c >&2) >(sha256sum >&2) | grep -iE '(ufshcd|apps-smmu|arm-smmu|iommu|context fault|smmu fault)'
```

The UFS ARID-to-SID mapping, SMMU context ownership and complete DMA range
remain unresolved; keep UniXNU UFS DMA disabled.

## Kona TBU topology cross-check · 2026-11-26

The captured elish Recovery FDT (`61ebe37bf537d8302abd5b82d22615ea0e5c92e6766eb1c75c034f8fd5d774f3`)
and merged stock elish DTB (`3ec7c535406bfd27e791180c69807e0c3c90542aac90a6733c847fd29cb61827`)
declare the same `qcom,stream-id-range` properties below `/soc/apps-smmu@15000000`.
The matching Lineage kernel at `6b34d24765327d746074c8e53b5ad0d24a51acc7`
decodes these properties as a start and count in `qsmmuv500_find_tbu()`.

| SID | TBU whose declared range contains the SID | TBU base | Status register |
| --- | --- | --- | --- |
| UFS candidate `0xE0` | ANOC1 `[0x0, 0x400)` | `0x15185000` | `0x15182200` |
| UFS candidate `0x4E0` | ANOC2 `[0x400, 0x800)` | `0x15189000` | `0x15182208` |
| MDSS SDE `0x820` | MNOC HF0 `[0x800, 0xC00)` | `0x1518D000` | `0x15182210` |

Both DTBs also show the UFS bandwidth vote sourced by `MSM_BUS_MASTER_UFS_MEM`
(`0x7B`). Its master node `mas-xm-ufs-mem` connects to `slv-qns-a1noc-snoc`
in the Kona fabric description. This confirms the described ANOC1 bandwidth
path. The live selection of UFS streams and any context-bank assignments
remain open. The ANOC TBU and status ranges appear in the Recovery
`/proc/iomem` capture and lie outside the current UEFI `SMMU` MMIO descriptor
(`0x15000000+0xD0000`). Any TBU readback needs a separately validated window
and a documented, read-only register definition.

```sh
for dtb in reports/elish-recovery-fdt-20261124.dtb /tmp/elish-stock-elish-merged.dtb; do
  for tbu in anoc_1_tbu@15185000 anoc_2_tbu@15189000 mnoc_hf_0_tbu@1518d000; do
    fdtget -t x "$dtb" "/soc/apps-smmu@15000000/$tbu" qcom,stream-id-range
  done
  dtc -q -I dtb -O dts "$dtb" | rg -A12 '^\s*mas-xm-ufs-mem \{'
done
```

## Decision and next step

Do not enable UFS DMA or issue UFS commands from these candidates. The current
unresolved inputs are the live UFS ARID-to-SID route, SMMU context ownership,
and complete DMA addressability under the actual UEFI handoff. Continue only
with read-only firmware analysis or a correctly scoped UEFI/HALIOMMU probe;
after those are proven, proceed through bounded NOP, capacity, known-block,
GPT and OS/SystemOS-window reads, then verify the System APFS UUID/mount and
Apple PID 1. Full System-root boot, Setup and persistent Data remain
unverified. No persistent partition write was performed.
