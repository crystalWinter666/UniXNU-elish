# elish pstore and UFS read-only follow-up, 2026-11-25

This follow-up records the current root ADB snapshot and corrects the causal
interpretation of the empty pstore directory after run08. No UniXNU image was
booted in this round, and no device partition was written.

## Current device state

At 2026-11-25 03:26 CST, `adb devices -l` showed serial `4bed07e4` as
`lineage_elish` / `M2105K81AC` / `elish`; `adb root` reported that adbd was
already running as root. `fastboot devices -l` returned no device. The active
kernel reports:

```text
Linux version 4.19.325-cip136-st20-perf-g6b34d2476532
```

The corresponding LineageOS kernel revision is
`6b34d24765327d746074c8e53b5ad0d24a51acc7` on `lineage-24.0`.

## pstore finding

`/sys/fs/pstore` currently contains no files. The live ramoops parameters are:

```text
mem_size     4194304
mem_address  2952790016 (0xb0000000)
record_size  0
console_size 2097152
pmsg_size    2097152
ftrace_size  0
```

The warning `Invalid compression size for deflate: 0` is emitted by
`fs/pstore/platform.c` while allocating the compression buffer from
`psinfo->bufsize`. In this kernel, ramoops has `record_size=0`, so there are no
panic-dump record zones and the resulting deflate buffer size is zero. The
console ring is configured separately with a nonzero 2 MiB `console_size` and
is enabled as a pstore frontend. Therefore this warning explains the missing
panic-dump compression buffer, not the empty console pstore directory. The
console ring readout and run08 marker remain unresolved; an empty directory
does not prove that XNU did not execute.

There is a separate possible marker-loss path: the debug UEFI SEC entry calls
`RamLogReset()`, which resets the ring header's `start` and `size` before
logging the new firmware entry. If a second Silicium UEFI run occurred between
run08 and the Linux reader, it could replace the checkpoint evidence with a
new firmware log. The recorded host sequence does not establish whether that
UEFI re-entry happened. Since `RamLogReset()` also appends a new boot message,
this possibility alone does not explain why the subsequent pstore directory
was entirely empty.

Source checked: `LineageOS/android_kernel_xiaomi_sm8250`, revision
`6b34d24765327d746074c8e53b5ad0d24a51acc7`, `fs/pstore/platform.c` and
`fs/pstore/ram.c`; UEFI reset path at
`uefi-elish/Silicon/Silicium/SiliciumPkg/Sec/Sec.c` and
`Platforms/Xiaomi/elishPkg/Library/RamoopsLogLib/RamoopsLogLib.c`.

## UFS and SMMU observation

Read-only sysfs and dmesg inspection confirms Android's UFS path is active:

- Controller `1d84000.ufshc` is bound to `ufshcd-qcom`; PHY
  `1d87000.ufsphy_mem` is bound to `ufs_qcom_phy_qmp_v4`.
- SCSI vendor/product/revision are `SAMSUNG`, `KLUEG8UHDC-B0E1`, `0200`.
- Link state is Gear 4, two lanes, FAST MODE, Rate-A. Current counters are
  `pa_err_cnt_total=0`, `dl_err_cnt_total=0`, and `dme_err_cnt=0`.
- The apps-SMMU reports 87 stream-match register groups. Linux also logs
  `register_client_adhoc:Client handle 36 ufshc_mem`; this is an interconnect
  client handle, not an SMMU stream ID.
- The live UFS DT node has no `iommus` property, and no UFS `iommu_groups` link
  was exported. These observations do not identify elish's UFS stream ID or
  prove that XNU can take over DMA safely.
- `board.toml` still selects no UFS component. The host-only NativeUfs tests
  recorded in `reports/elish-ufs-host-audit-20261125.md` do not qualify this
  device's controller, clocks, PHY or SMMU configuration.

Read-only commands included:

```sh
adb -s 4bed07e4 root
adb -s 4bed07e4 shell 'cat /proc/version'
adb -s 4bed07e4 shell 'cat /sys/module/ramoops/parameters/mem_size; cat /sys/module/ramoops/parameters/mem_address; cat /sys/module/ramoops/parameters/record_size; cat /sys/module/ramoops/parameters/console_size; cat /sys/module/ramoops/parameters/pmsg_size; cat /sys/module/ramoops/parameters/ftrace_size'
adb -s 4bed07e4 shell 'cat /sys/devices/platform/soc/1d84000.ufshc/qcom/err_count; cat /sys/devices/platform/soc/1d84000.ufshc/qcom/err_state; cat /sys/devices/platform/soc/1d84000.ufshc/qcom/power_mode'
adb -s 4bed07e4 shell 'dmesg | grep -iE "stream-match|stream match|SMMU|ufshc_mem"'
adb -s 4bed07e4 shell 'ls -la /sys/fs/pstore'
```

## Boot distance

At this report's 03:26 CST snapshot, the latest tested UniXNU image was run08:

- Image: `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run08-xnu-entry-checkpoint-corrected/boot.img`
- SHA-256: `9cd0577439a5f143ff11d81cdaa6aece79da01a25cfd5cfdf67ddea0a7a5075a`
- Last confirmed stage: the user-provided green DarwinBoot handoff screen after
  `ExitBootServices()`. The direct run04 log records the accepted 256 MiB
  window `0xc007c000+0x10000000`, key `0x3ee`, and 73 memory descriptors.
- Evidence: `reports/elish-xnu-entry-checkpoint-run08-20261125.md` and
  `reports/elish-darwinboot-ebs-20261124.txt`.

At that snapshot, XNU entry and later stages were unconfirmed. The APFS
System-role UUID and root image had only been verified on the host. A later
run09 test and close photo supersede the entry-stage conclusion: the image
shows the blue XNU-entry and magenta pre-MMU bands. The MMU transition, runtime
memory ownership, UFS/SMMU handoff, APFS mount on the device, and iPadOS
userspace remain unconfirmed. Full test, pstore result, image hashes and
partition restoration are recorded in
`reports/elish-xnu-entry-visual-retest-20261125.md`.
