# elish bring-up progress, 2026-11-25 16:05 CST

Device: Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC` / `elish`), target iPadOS 17.7.11
(`21H461`). The latest documented device state after the diagnostic runs is
root LineageOS ADB, restored by the hash-verified reader. No device partition
was left modified.

## Official sequence

The local `/root/repo/UniXNU/docs/zh/porting/steps.md` is the eight-step guide
linked from the Chinese porting page. Steps 1-5 (device inputs, shared support,
device configuration, resolved build inputs, and firmware/payload integration)
are substantially in place, with runtime memory ownership and UFS DMA inputs
still open. Step 6 is active at its first dependencies: platform/memory and
storage. Display, input, GPU, network and later package services are not yet
being brought up. In the seven-stage bring-up guide, UEFI, UniXNU, payload and
DarwinBoot/EBS have been reached; XNU entry/MMU is under early-kernel
verification; storage/root and userspace have not been reached. Repository
completion and publication are later work, after device results are complete.

## Confirmed stages

| Stage | Evidence | Result |
| --- | --- | --- |
| FAT payload, DarwinBoot, 256 MiB XNU window, `ExitBootServices()` | run06/run11/run18/run19 ramoops logs | Directly confirmed. The window is `0xc007c000+0x10000000`; run18/run19 report EBS key `0x3ee` with the accepted snapshot. |
| XNU entry and pre-MMU checkpoint | run09 screen photo `/root/repo/A3AABAD37D0E0E787C85F39100B9ACCB.png`, SHA-256 `b03cc52878f046d7574ab1a776a3f83662f10251875438dcfbd2181ac99ee7b8` | Green EBS, blue XNU-entry and magenta pre-MMU bands are visible. This directly confirms execution through the pre-MMU checkpoint. |
| MMU enable | run09 six-second stage-3 reset and host USB timeline | USB disconnected at 15:47:32 and returned as fastboot at 15:47:50; the close photo was taken at 15:47:51. UniXNU's green-bar procedure defines stage 3 as a reset after MMU enable and treats return after that delay as confirmation. Record MMU enabled, with timing-probe evidence rather than a kernel log marker. |
| `_arm_vm_init()` return | run13 image SHA-256 `05b8620d4ccc74e95bccb3fef0edc71946d205036e0fd71da005361a2ff459b9`; run18 readback | run13's accepted 15-second return probe produced timing consistent with its reset. The later run18 pmsg readback reported `magic=0x00004000`, `complement=0`, `valid=0`; the return marker was not recovered. Treat run13 as indirect evidence only; the function return is not directly confirmed. |
| `PE_init_printf()` and later XNU startup | run17 image SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e` | After one rejected transfer attempt, fastboot accepted the retry. USB returned near the configured 15-second reset interval. This is timing evidence, without a close screen marker or pstore record; it does not confirm later platform initialization. |
| UFS SMMU ownership | run19 SMMU capture plus Android live FDT | run19 found no active ACPI IORT and read the only valid apps-SMMU match as SID `0x820`, mask `0x402`, context bank 4. Live FDT assigns the same tuple to `qcom,smmu_sde_unsec_cb` under MDSS, so the observed match belongs to display SDE, not UFS. UFS has no `iommus` property or Linux `iommu_group` link. UFS SID and DMA mode remain unknown; UFS DMA stays disabled. |

## Remaining work

1. Resolve why the run18 marker was not retained. Add a visible checkpoint immediately after `_arm_vm_init()` returns, then use a RAM-only boot and close screen photo to distinguish function return from the existing timed-reset evidence.
2. Follow the UFS hardware checklist: establish controller/PHY, clocks, reset/power, DMA addressability, stream/context ownership, and firmware handoff. The active run19 ACPI configuration has no IORT, and the Linux SDE mapping is not a UFS mapping. Do not enable DMA until its owner and state are evidenced.
3. Adapt and build the UFS/root path, then verify controller state, DMA and command completion, capacity and known block reads. Only after those pass, read the device GPT and APFS SystemOS/root windows, mount the configured root volume, and test `launchd` and userspace. Verify Data persistence after root startup.
4. Qualify runtime memory ownership and the production XNU candidate; its prelink metadata still has `loadable=false`. Continue the remaining display, input, GPU, network and package services in the official dependency order after the root path is stable.

The userdata geometry is already verified as 4096-byte logical blocks,
partition 34 LBA `6619136-61870314`. Host APFS image validation is not a
device-side mount. UFS access, APFS root mounting, launchd and iPadOS userspace
remain unverified; no ETA is supportable until the UFS path is qualified.
