# elish live hardware measurements, 2026-11-23

## Device and collection

Root ADB identified the connected device as `elish` / `M2105K81AC`. `adb shell id` returned UID 0. The Android build reports version 17, build ID `CP2A.260605.016`, and vendor fingerprint `Xiaomi/elish/elish:13/RKQ1.211001.001/V816.0.2.0.TKYCNXM:user/release-keys`.

All commands below read device properties, kernel-exported memory data, or block-device sectors. The block-device exports were written to host files under `/tmp` for parsing.

## Live FDT and memory

`/sys/firmware/fdt` is 845,892 bytes with SHA-256 `90cf1d5333e758f001a8e21ed4e78fc000ebf3b5502338f010435a2da292a663`.

The live FDT memory node reports two banks.

| Base | Size | End, exclusive |
| --- | --- | --- |
| `0x80000000` | `0x3bb00000` | `0xbbb00000` |
| `0xc0000000` | `0x1c0000000` | `0x280000000` |

The banks total `0x1fbb00000` bytes. The gap is `0x4500000` bytes, and the full address span is 8 GiB. Core revision `71c5c6569bae6369c0e0f634df8b9bd5afc19139` requires one contiguous memory range in `scripts/unixnu/devicetree.py` and encodes its base and length as `chosen/dram-base` and `chosen/dram-size`. The selected XNU source defines `is_dram_addr(addr)` as membership in `[gDramBase, gDramBase+gDramSize)`, which is used in pmap memory cache-attribute selection. A range spanning both banks would also classify the gap as DRAM. The live FDT's two ranges therefore do not yet establish a safe ADT/XNU handoff range.

The live `/reserved-memory` node has 22 fixed children and 12 dynamic children.

| Node | Range or requested allocation | FDT attributes |
| --- | --- | --- |
| `hyp_region@80000000` | `0x80000000+0x600000` | `no-map` |
| `xbl_aop_region@80700000` | `0x80600000+0x260000` | `no-map` |
| `reserved-memory@80860000` | `0x80860000+0x20000` | `no-map` |
| `res_xbl_uefi_log_region@80880000` | `0x80880000+0x14000` | `no-map` |
| `smem_region@80900000` | `0x80900000+0x200000` | `no-map` |
| `removed_region@80b00000` | `0x80b00000+0x5300000` | `no-map` |
| `pil_camera_region@86200000` | `0x86200000+0x500000` | `no-map` |
| `pil_wlan_fw_region@86700000` | `0x86700000+0x100000` | `no-map` |
| `pil_ipa_fw_region@86800000` | `0x86800000+0x10000` | `no-map` |
| `pil_ipa_gsi_region@86810000` | `0x86810000+0xa000` | `no-map` |
| `pil_gpu_region@8681a000` | `0x8681a000+0x2000` | `no-map` |
| `pil_npu_region@86900000` | `0x86900000+0x500000` | `no-map` |
| `pil_video_region@86e00000` | `0x86e00000+0x500000` | `no-map` |
| `pil_cvp_region@87300000` | `0x87300000+0x500000` | `no-map` |
| `pil_cdsp_region@87800000` | `0x87800000+0x1400000` | `no-map` |
| `pil_slpi_region@88c00000` | `0x88c00000+0x2f00000` | `no-map` |
| `pil_adsp_region@8bb00000` | `0x8bb00000+0x2500000` | `no-map` |
| `pil_spss_region@8e000000` | `0x8e000000+0x100000` | `no-map` |
| `cdsp_secure_heap@8e100000` | `0x8e100000+0x4600000` | `no-map` |
| `adsp_region` | dynamic size `0xc00000`, alignment `0x400000` | `reusable` |
| `sdsp_region` | dynamic size `0x800000`, alignment `0x400000` | `reusable` |
| `cdsp_region` | dynamic size `0x400000`, alignment `0x400000` | `reusable` |
| `cont_splash_region@9c000000` | `0x9c000000+0x2300000` | mapped |
| `disp_rdump_region@9c000000` | `0xb0400000+0x1000000` | `no-map` |
| `dfps_data_region@9e300000` | `0x9e300000+0x100000` | mapped |
| `mem_dump_region` | dynamic size `0x2800000` | `reusable` |
| `sp_region` | dynamic size `0x1000000`, alignment `0x400000` | `reusable` |
| `user_contig_region` | dynamic size `0x1000000`, alignment `0x400000` | `reusable` |
| `qseecom_region` | dynamic size `0x1400000`, alignment `0x400000` | `reusable` |
| `qseecom_ta_region` | dynamic size `0x1000000`, alignment `0x400000` | `reusable` |
| `secure_display_region` | dynamic size `0xa400000`, alignment `0x400000` | `reusable` |
| `cnss_wlan_region` | dynamic size `0x1400000`, alignment `0x400000` | `reusable` |
| `linux,cma` | dynamic size `0x2000000`, alignment `0x400000` | `reusable` |
| `mailbox_region` | dynamic size `0x20000`, alignment `0x400000` | `no-map` |

`/proc/iomem` shows Linux `System RAM` split around reserved and firmware-owned regions. It is a post-boot Linux resource view, not the UEFI `GetMemoryMap` at `ExitBootServices`. The UEFI FD, stack and XNU-usable allocation are not identified by this capture.

A follow-up root ADB check returned UID 0. The running Android system has no
`/sys/firmware/memmap` or `/dev/mem`, and `/sys/kernel/debug` is absent. The
`/sys/fs/pstore` directory is empty. A `dmesg` search for RAM Partition table,
SMEM memory-partition, and reserved-memory messages returned no lines. These
interfaces therefore did not provide the firmware RAM Partition table or an
UEFI memory map.

The live `/soc/qcom,smem` node is bound to the `qcom-smem` platform driver. Its
`memory-region` phandle `0x85000000` resolves to the no-map
`/reserved-memory/smem_region@80900000` range at `0x80900000+0x200000`. The
driver does not expose an SMEM nvmem device or `/dev` node to userspace. This
confirms the shared-memory region but does not export its RAM Partition table.

Read commands used:

```sh
adb shell 'id; ls -ld /sys/firmware/memmap /sys/kernel/debug /dev/mem; ls -la /sys/fs/pstore'
adb shell 'cat /sys/devices/platform/soc/soc:qcom,smem/uevent; od -An -tx4 -v /sys/firmware/devicetree/base/soc/qcom,smem/memory-region; od -An -tx4 -v /sys/firmware/devicetree/base/reserved-memory/smem_region@80900000/phandle; od -An -tx4 -v /sys/firmware/devicetree/base/reserved-memory/smem_region@80900000/reg'
adb shell 'ls -la /sys/bus/nvmem/devices; ls -la /dev/*smem*'
adb shell 'dmesg 2>/dev/null | grep -iE "ram partition|smem.*memory|memory.*partition|memblock|reserved memory|reserved-memory"'
```

## Display and input

Live DRM reports `card0-DSI-1` connected with modes `1600x2560` at 120, 60 and 30 Hz. The boot arguments and DRM connector identify `xiaomi 42 02 0a video mode dual dsi cphy panel`, selecting the elish K81 `42_02_0a` panel variant. The panel backlight is a raw device with a maximum value of 2047.

The live touch properties are `panel_vendor=S` (ASCII 83), `panel_display=B` (ASCII 66) and `touch_vendor=4`. The touchscreen input exposes slots 0–9 and coordinates X 0–1599, Y 0–2559. The pen input exposes X 0–12799, Y 0–20479, pressure 0–4095 and tilt X/Y from -60 to 60.

The active FDT node is `/soc/spi@990000/novatek@0`, compatible with `novatek,NVT-ts-spi`, enabled with chip select 0, 19.2 MHz SPI, and TLMM IRQ GPIO 39. Android kernel logs identify the controller as NVT and report a successful update from `novatek_nt36523_k81a_fw01.bin`, firmware version `0x1a`, and controller PID `5961`. These are Android runtime results and do not verify the UniXNU input path.

Read commands used:

```sh
adb shell 'cat /proc/bus/input/devices; getevent -lp'
adb shell 'dmesg | grep -Ei "novatek|nvt|panel.*vendor|panel.*display|mdss.*panel"'
adb shell 'for p in /sys/class/touch/touch_dev/panel_vendor /sys/class/touch/touch_dev/panel_display /sys/class/touch/touch_dev/touch_vendor; do printf "%s=" "$p"; cat "$p"; done'
adb shell 'cat /sys/class/drm/card0-DSI-1/status; cat /sys/class/drm/card0-DSI-1/modes; cat /sys/class/backlight/panel0-backlight/max_brightness'
```

## UFS and userdata GPT

`/dev/block/sda` reports 253,461,790,720 bytes, 4,096-byte logical blocks and 61,880,320 LBAs. GPT partition 34 is named `userdata` and spans inclusive LBAs 6,619,136 through 61,870,314. Its size is 226,308,829,184 bytes.

The primary GPT header is at LBA 1 and its 128-entry array begins at LBA 2. The backup header is at LBA 61,880,319 and its array begins at LBA 61,880,315. Both headers and both arrays pass their CRC checks. The primary and backup arrays are byte-identical. The 20,480-byte primary export SHA-256 is `b8b416c188f9ee5cca9146ad19068b975c0eba949dfde5e950166577189ff08a`; the backup export SHA-256 is `f72f278000f453775369dcad13cdfe458e5e08ee445907b07af1e149b99c2845`.

Read commands used:

```sh
adb shell 'id; getprop ro.product.device; getprop ro.product.model'
adb shell 'sha256sum /sys/firmware/fdt; cat /proc/iomem; blockdev --getsize64 /dev/block/sda; blockdev --getss /dev/block/sda'
adb exec-out 'cat /sys/firmware/fdt' > /tmp/elish-live-fdt-20261123.dtb
adb exec-out 'dd if=/dev/block/sda bs=4096 skip=1 count=5 2>/dev/null' > /tmp/elish-gpt-primary-20261123.bin
adb exec-out 'dd if=/dev/block/sda bs=4096 skip=61880315 count=5 2>/dev/null' > /tmp/elish-gpt-backup-20261123.bin
```

The host parser checked GPT header and array CRCs, array equality, partition index/name/bounds and size. `board.toml` records the measured userdata LBAs and block size. This verifies partition geometry only. It does not verify UFS driver ownership, DMA/SMMU state, root-volume matching or Data persistence.

## Root ADB follow-up, 2026-11-24

Root ADB still identifies the connected target as `M2105K81AC`. The current Android kernel's `/proc/filesystems` does not list `debugfs`; `/sys/kernel/debug` and `/dev/mem` are absent. A read-only debugfs mount attempt could not proceed because the kernel does not expose debugfs. No SMEM RAM Partition table or UEFI `GetMemoryMap` is available through this Android session. A fresh read-only check also found `/sys/firmware/efi`, `/sys/firmware/efi/memmap` and `/sys/firmware/memmap` absent.

The live FDT was re-exported to `/tmp/elish-uefi-probe.dtb` for the host boot-image round-trip check. It remains 845,892 bytes and SHA-256 `90cf1d5333e758f001a8e21ed4e78fc000ebf3b5502338f010435a2da292a663`.

The active Android kernel exposes `/sys/bus/platform/devices/1d84000.ufshc` with compatible `qcom,ufshc`, bound to `ufshcd-qcom`, and `/sys/bus/platform/devices/1d87000.ufsphy_mem` with compatible `qcom,ufs-phy-qmp-v4`, bound to `ufs_qcom_phy_qmp_v4`. The live FDT UFS host node lists controller MMIO `0x1d84000+0x3000`, ICE MMIO `0x1d90000+0x8000`, interrupt specifier `<0 0x109 4>`, a two-lane PHY at `0x1d87000`, and clock names `core_clk`, `bus_aggr_clk`, `iface_clk`, `core_clk_unipro`, `core_clk_ice`, `ref_clk`, `tx_lane0_sync_clk`, `rx_lane0_sync_clk` and `rx_lane1_sync_clk`. The host node has no `iommus` property and the platform device has no exported `iommu_groups` link. These are live-FDT and stock Android facts only. UEFI/DarwinBoot ownership, SMMU stream IDs and DMA setup remain unverified.
