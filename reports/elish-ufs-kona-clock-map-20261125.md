# elish Kona UFS clock and reset map, 2026-11-25

## Inputs and method

This is a host-only comparison of the stock Android ROM device tree and
Xiaomi's open-source Android R kernel. It does not describe active clock state
after UEFI handoff.

| Input | SHA-256 |
| --- | --- |
| `vendor_boot.img` | `63dbb687d39b4d3c12aae6828fdd414d15e41dbec74f48ec56e2b23195a3a467` |
| `dtbo.img` | `4d797a4d3ea9ea16168128a9c189d7f32bf21bd1b049205fe39959a90046b9d7` |
| Kona v2.1 base DTB | `324049d746aaa16ada3b425ea5eed30f6d49a90c9d63d5e57ea5a632c27becb8` |
| elish overlay entry 12 | `8d7c8c6666e10e8633d883f7bc4cd3032a262ce4c95959392948270109d13d3b` |
| merged elish DTB | `3ec7c535406bfd27e791180c69807e0c3c90542aac90a6733c847fd29cb61827` |

The stock `vendor_boot.img` contains four concatenated DTBs. The first is Kona
v2.1 (`qcom,msm-id = <0x164 0x20001>`). Entry 12 of the 29-entry `dtbo.img`
matches model `Qualcomm Technologies, Inc. xiaomi elish` and board ID
`<0x2f 0x00>`. Applying it to the base DTB reproduces the hashes in
`reports/elish-stock-storage-audit-20261124.md`.

The clock provider is the stock DT node `qcom,gcc@100000`, compatible with
`qcom,gcc-kona`; its register offsets below are relative to GCC base `0x100000`.
The clock names and specifiers come from the merged stock DT. IDs and register
offsets come from Xiaomi kernel branch `elish-r-oss`, commit
`52a2f180865f8368828985d6384130acd093fa93`, in
`include/dt-bindings/clock/qcom,gcc-kona.h` and `drivers/clk/qcom/gcc-kona.c`.

## Controller clocks

| Stock DT name | Specifier | Kona clock ID | Branch register | Source RCG register |
| --- | --- | --- | --- | --- |
| `core_clk` | GCC `0xa7` | `GCC_UFS_PHY_AXI_CLK` (167) | `0x77010` | `0x77024` |
| `bus_aggr_clk` | GCC `0x06` | `GCC_AGGRE_UFS_PHY_AXI_CLK` (6) | `0x770cc` | parent is UFS PHY AXI source |
| `iface_clk` | GCC `0xa6` | `GCC_UFS_PHY_AHB_CLK` (166) | `0x77018` | none |
| `core_clk_unipro` | GCC `0xb3` | `GCC_UFS_PHY_UNIPRO_CORE_CLK` (179) | `0x7705c` | `0x77084` |
| `core_clk_ice` | GCC `0xaa` | `GCC_UFS_PHY_ICE_CORE_CLK` (170) | `0x77064` | `0x7706c` |
| `tx_lane0_sync_clk` | GCC `0xb2` | `GCC_UFS_PHY_TX_SYMBOL_0_CLK` (178) | `0x7701c` | none |
| `rx_lane0_sync_clk` | GCC `0xb0` | `GCC_UFS_PHY_RX_SYMBOL_0_CLK` (176) | `0x77020` | none |
| `rx_lane1_sync_clk` | GCC `0xb1` | `GCC_UFS_PHY_RX_SYMBOL_1_CLK` (177) | `0x770b8` | none |

`ref_clk` is not a GCC clock. Its specifier uses provider phandle `0x15`,
whose compatible is `qcom,kona-rpmh-clk`, with clock index 0. In particular,
the GCC binding's numeric ID 21 is `GCC_CPUSS_RBCPR_CLK`; interpreting the
stock `ref_clk` cell as GCC ID 21 would be incorrect.

The stock PHY node names `ref_clk_src` and `ref_aux_clk`. `ref_clk_src` also
uses RPMh provider index 0. `ref_aux_clk` uses GCC ID `0xad` (173),
`GCC_UFS_PHY_PHY_AUX_CLK`, with branch register `0x7709c` and source RCG
register `0x770a0`.

The controller's `core_reset` uses GCC reset ID `0x21` (33),
`GCC_UFS_PHY_BCR`, mapped by the Kona provider to block-reset register
`0x77000`.

## Android R UFS/SMMU cross-check

At kernel revision `52a2f180865f8368828985d6384130acd093fa93`, the local
source tree contains the generic Kona MTP UFS supply setup in
`arch/arm64/boot/dts/qcom/kona-mtp.dtsi`; it enables the PHY/controller and
lists regulator inputs, but carries no `iommus` property or stream ID. The
generic `qcom,ufshc` match in `drivers/scsi/ufs/ufs-qcom.c` probes through
`ufshcd_pltfrm_init()` and adds no board-specific stream assignment. This is
not an elish DTS implementation. Together with the separately parsed stock
elish DT, which also has no UFS `iommus` property, the Android R sources do not
identify run11's SID `0x0820` as UFS; its owner remains unknown.

## `web1n/android_device_xiaomi_elish` cross-check

The user-linked device repository was reviewed at HEAD
`23e652dde3a1b1a2f6f6ff3f7e088d3f6c814772`. Its README identifies the Xiaomi
Pad 5 Pro (`elish`) LineageOS 21 device tree. `BoardConfig.mk` inherits
`device/xiaomi/sm8250-common/BoardConfigCommon.mk`, and `lineage.dependencies`
fetches that common device repository. The elish repository itself contains no
DTS/DTSI or UFS controller/IOMMU mapping.

The common device repository at
`ae56501ed9cedbd1a66047e0d3aea9f41a830aec` supplies
`rootdir/etc/fstab.qcom`; its userdata entry names
`/sys/devices/platform/soc/1d84000.ufshc` as `sysfs_path`. This identifies the
Android UFS host path used by fstab, not a DMA stream ID. The separate
LineageOS Android kernel source is revision
`6b34d24765327d746074c8e53b5ad0d24a51acc7`; Xiaomi Android R kernel source is
revision `52a2f180865f8368828985d6384130acd093fa93` and provides Kona clock
definitions, but neither the linked device repository nor its fstab assigns
the runtime SID `0x820` to UFS.

At the 2026-11-25 15:38 CST root ADB snapshot, the live node
`/sys/firmware/devicetree/base/soc/ufshc@1d84000` exposed register, IRQ, PHY,
clock, reset and regulator properties, but no `iommus`; the platform device
also had no `iommu_group` link. These observations corroborate the source
review. The repository is useful for board naming and Android storage routing,
but it does not provide the UFS-to-SMMU mapping.

## Evidence limits

- The stock controller is at `0x1d84000+0x3000`, ICE at `0x1d90000+0x8000`,
  and the two-lane QMP v4 PHY at `0x1d87000+0xe00`. Its UFS node has no
  `iommus` property.
- The source establishes clock IDs and MMIO register layout, not the live
  enable bits, selected rates, RPMh votes, power-rail state, reset sequence,
  or QMP PHY programming needed by XNU.
- The kernel branch is Android R source. It is useful for decoding the stock
  DT and Kona GCC provider, but it is not evidence that current LineageOS,
  XBL/UEFI, or the iPadOS boot path leaves those resources in a usable state.
- The run19 valid apps-SMMU context has SID `0x0820`, mask `0x0402`, and
  context bank 4. The Android live FDT's MDSS child `qcom,smmu_sde_unsec_cb`
  declares `iommus = <0x46 0x820 0x402>`; phandle `0x46` resolves to apps-SMMU
  with `#iommu-cells = <2>`. This identifies the observed tuple as the SDE
  display stream. The UFS node has no `iommus` property, so this mapping does
  not identify UFS's stream or DMA mode.

No UFS component was enabled. The run09 photo directly shows XNU through the
pre-MMU checkpoint; its stage-3 timed reset and USB return meet UniXNU's
diagnostic criterion for MMU enable. UFS DMA, device APFS mount and iPadOS
userspace remain unverified.

## Reproduction

```sh
sha256sum /root/repo/elish_os1.0.2.0/vendor_boot.img /root/repo/elish_os1.0.2.0/dtbo.img
fdtoverlay -i /tmp/elish-stock-dtb-0.dtb -o /tmp/elish-stock-elish-merged.dtb /tmp/elish-stock-overlay-12.dtbo
fdtget -t s /tmp/elish-stock-elish-merged.dtb /soc/ufshc@1d84000 clock-names
fdtget -t x /tmp/elish-stock-elish-merged.dtb /soc/ufshc@1d84000 clocks
fdtget -t x /tmp/elish-stock-elish-merged.dtb /soc/ufshc@1d84000 resets
git -C /tmp/elish-linux-kernel-elish-r-oss rev-parse HEAD
```
