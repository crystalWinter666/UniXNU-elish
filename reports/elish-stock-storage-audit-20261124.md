# elish stock UFS device-tree audit

## Inputs and extraction

The audit used the supplied stock ROM at `/root/repo/elish_os1.0.2.0`.

| Input | SHA-256 |
| --- | --- |
| `vendor_boot.img` | `63dbb687d39b4d3c12aae6828fdd414d15e41dbec74f48ec56e2b23195a3a467` |
| `dtbo.img` | `4d797a4d3ea9ea16168128a9c189d7f32bf21bd1b049205fe39959a90046b9d7` |
| Extracted `kona v2.1` DTB | `324049d746aaa16ada3b425ea5eed30f6d49a90c9d63d5e57ea5a632c27becb8` |
| elish DTBO entry 12 | `8d7c8c6666e10e8633d883f7bc4cd3032a262ce4c95959392948270109d13d3b` |
| Merged elish DTB | `3ec7c535406bfd27e791180c69807e0c3c90542aac90a6733c847fd29cb61827` |

`vendor_boot.img` is Android vendor_boot v3. Its DTB field contains four concatenated FDTs. The first is the Kona v2.1 DTB with `qcom,msm-id = <0x164 0x20001>`. The stock `dtbo.img` contains 29 entries. Entry 12 has model `Qualcomm Technologies, Inc. xiaomi elish` and `qcom,board-id = <0x2f 0x00>`. `fdtoverlay` applied that entry to the matching Kona v2.1 DTB.

The relevant read-only commands were a Python parse of the vendor_boot v3 header and Android DT table, `fdtoverlay -i /tmp/elish-stock-dtb-0.dtb -o /tmp/elish-stock-elish-merged.dtb /tmp/elish-stock-overlay-12.dtbo`, and `dtc -I dtb -O dts` on the merged FDT. The DTB-to-DTS conversion emitted warnings for unrelated inherited Linux nodes. The merged FDT was produced and parsed successfully.

## UFS and PHY facts in the merged stock tree

- UFS host `/soc/ufshc@1d84000` uses `compatible = "qcom,ufshc"`, MMIO `0x1d84000+0x3000`, and ICE MMIO `0x1d90000+0x8000`.
- Its interrupt is `<0 0x109 4>`, the PHY has two lanes, and the host status is `ok`.
- The PHY `/soc/ufsphy_mem@1d87000` uses `compatible = "qcom,ufs-phy-qmp-v4"`, registers `0x1d87000+0xe00` and `0x1d90000+0x8000`, and status `ok`.
- The host names nine clocks: `core_clk`, `bus_aggr_clk`, `iface_clk`, `core_clk_unipro`, `core_clk_ice`, `ref_clk`, `tx_lane0_sync_clk`, `rx_lane0_sync_clk`, and `rx_lane1_sync_clk`. The node also names `core_reset`.
- The stock UFS host node has no `iommus` property. Its tree therefore supplies no UFS stream ID or DarwinBoot SMMU ownership evidence.

These addresses, interrupt, PHY and clock names corroborate the live Android FDT observations in `reports/elish-live-hardware-20261123.md`. They describe the stock Android device tree. They do not establish which ranges or SMMU contexts remain available after UEFI handoff.

## Driver scope review

`ShengUfs` catalog revision `2b6638be3e5630969e316dd1755065542fcee505` identifies itself as an SM8550 driver. Its `NativeUfs.cpp` personality matches `qcom,sm8550-ufshc`, and its implementation uses SM8550 GCC registers, SMMU base `0x15000000`, and stream ID `0x60`. The stock elish UFS node advertises `qcom,ufshc` and has no SMMU stream assignment.

The community reference `FFT-infra/NativeUfs-alioth` at revision `336680c353e6c3f25d6b29284532c4fd16104f68` adds the generic `qcom,ufshc` match and documents SM8250 stream IDs `0xe0` and `0x4e0`. Its `GccUfs.h` still labels its GCC register inputs as SM8550-derived, and the stream values are recorded for alioth. This is a useful SM8250 adaptation candidate. The source and its README provide no elish runtime result, and the elish stock DT does not encode those stream IDs.

The current board DTS keeps UFS and its PHY disabled. Enabling them requires an SM8250-specific controller and SMMU ownership implementation, plus validation of firmware handoff and known-block reads. GPT geometry and the host-built APFS root container remain independently verified.

No ADB, fastboot, USB enumeration, image write, or device-side operation was run for this audit. Extracted DTB files were kept under `/tmp` only.
