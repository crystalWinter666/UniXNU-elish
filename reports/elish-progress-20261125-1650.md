# elish bring-up progress, 2026-11-25 16:50 CST

Device: Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC` / `elish`), target iPadOS 17.7.11
(`21H461`). Current device observation is root ADB on its installed Android
system. This turn made no reboot, MMIO access or partition write.

## Current confirmed stage

- FAT payload discovery, DarwinBoot, its 256 MiB XNU window and successful
  `ExitBootServices()` are directly present in run06/run11/run18/run19 ramoops.
- Run09's close photo directly shows the green EBS, blue XNU-entry and magenta
  pre-MMU bands. Its six-second stage-3 reset and USB timeline meet UniXNU's
  green-bar procedure for MMU enabled. This is timing-based evidence; it is
  not a kernel log marker.
- `_arm_vm_init()` return remains unconfirmed: run18 pmsg readback has an
  invalid marker. The production kernel metadata still reports `loadable=false`.
- No XNU UFS command, device APFS root mount or iPadOS userspace milestone has
  been recorded.

## UFS finding and correction

Read-only root ADB confirms Android's `ufshcd-qcom` owns UFS. Its boot log says
the host is reinitialized for proper PHY settings, then reaches Gear 4 on two
lanes in FAST mode. The current UFS device has no `iommu_group`; the live SMMU
parameter is `disable_bypass=Y`. An earlier interpretation mistook
`ptrace_scope=0` for that parameter and inferred direct DMA. That inference is
withdrawn: the matching Linux SMMU driver says unmatched streams fault when
`disable_bypass` is true. The Android observations therefore do not identify
the UFS SID or prove its transaction path.

The public UniXNU alioth port (SM8250-AC) records a UFS root and launchd result.
Its source validates exact streams, exclusive contexts, bypass state and DMA
width before reuse. These are useful process and test references. Its `0xe0` /
`0x4e0` IDs and clock values are alioth inputs and do not establish elish board
values. The complete source and revision audit is in
[`elish-ufs-reference-audit-20261125.md`](elish-ufs-reference-audit-20261125.md).

## Next checks

1. Add a read-only UEFI handoff capture for elish GCC power/clock/reset and
   SMMU global control plus stream mappings.
2. Read UFS controller registers only after confirming the power domain and
   access window are live.
3. Establish UFS stream routing and DMA-buffer reachability before any command.
   Then verify NOP/read-capacity, known-block reads, GPT/root windows, APFS
   mounting and launchd in order. Keep UFS DMA disabled until routing is proven.
4. Independently resolve post-MMU/XNU memory handoff and production-kernel
   loadability before treating the storage path as the only remaining gap.

The run19 SID `0x820` / mask `0x402` belongs to MDSS SDE. UFS/APFS root and
userspace remain unverified; available references do not support a reliable ETA.
