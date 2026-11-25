# elish UFS handoff and reference audit, 2026-11-25

## Scope

This audit combines read-only state from the installed Android system, its
published kernel source, the current UniXNU UFS guide, and the same-SoC
`alioth` port. It did not reboot the device, access UFS/SMMU MMIO, change any
device setting, or write a partition.

## Live Android observation

The root ADB capture is in
[`elish-live-ufs-smmu-20261125.txt`](elish-live-ufs-smmu-20261125.txt),
SHA-256 `95402cb216f86c2c576ddbfed47d97def68b7b64ffd53efe90fc406362c09e56`.
It identifies `elish`, Android 13 `V816.0.2.0.TKYCNXM`, and Lineage kernel
`4.19.325-cip136-st20-perf-g6b34d2476532` (source revision
`6b34d24765327d746074c8e53b5ad0d24a51acc7`). The UFS platform device binds to
`ufshcd-qcom`; its sysfs `iommu_group` link is absent; and
`/sys/module/arm_smmu/parameters/disable_bypass` reads `Y`.

The same live `dmesg` says `ufshcd_needs_reinit` reinitializes the UFS host to
use proper PHY settings, then reports a transition from Gear 1 / one lane in
SLOWAUTO to Gear 4 / two lanes in FAST mode. This is direct evidence that the
Android driver changes UFS/PHY state during its own boot. It does not establish
the controller state before Android initializes it, after UEFI's EBS, or for
XNU.

The missing `iommu_group` and `iommus` property do **not** prove physical
bypass. In the matching Linux source, `of_iommu_configure()` obtains platform
stream specifiers from the device's `iommus` property. The SMMU driver's
`disable_bypass` defaults to true; its description says unmatched streams fault,
and it sets sCR0.USFCFG when the live parameter is true. Thus the current
Android observations do not explain which translation path the UFS transactions
use. Possibilities require hardware evidence; no UFS SID or DMA mode is inferred
here. The UEFI run19 tuple (`SID 0x820`, mask `0x402`) is identified by elish's
live FDT as MDSS SDE and remains unrelated to UFS.

## Public implementation reference

The current UniXNU source is `71c5c6569bae6369c0e0f634df8b9bd5afc19139`.
Its [UFS guide](https://github.com/UniXNU/UniXNU/blob/71c5c6569bae6369c0e0f634df8b9bd5afc19139/docs/porting/ufs.md)
sets the useful order: establish controller/PHY, clocks, reset/power and SMMU;
inspect the firmware handoff; establish buffer addressability and request-list
ownership; then test commands, capacity, known blocks, GPT and root windows.
The guide also calls out cache-line separation for 32-byte UTRDs, doorbell-based
completion when UTRD interrupt bits are absent, and validating all bypass
address-width limits. These are candidate design checks, not elish hardware
results.

The same-SoC [NativeUfs-alioth source](https://github.com/FFT-infra/NativeUfs-alioth/tree/336680c353e6c3f25d6b29284532c4fd16104f68)
is at revision `336680c353e6c3f25d6b29284532c4fd16104f68`. Its SMMU code
checks an exact stream match, exclusive context ownership, bypass state, and
the complete DMA range before reuse. That validation pattern is reusable.
Its own README and the [alioth port record](https://github.com/FFT-infra/UniXNU-alioth/blob/8540a2638aeb768567cc25f478572b7225be0a7d/porting.toml)
record a same-SoC UFS root and launchd result, which shows the broad sequence
can reach userspace on another SM8250-AC board.

The board values are not transferable: alioth uses stream IDs `0xe0` and
`0x4e0`, while elish has no measured UFS stream; the alioth clock header labels
its GCC values SM8550-derived and its reset/RPMh/TCSR/QMP-PHY setup is not an
elish implementation. The selected catalog component `ShengUfs` is still
pinned to the SM8550-scoped UniXNU source at
`2b6638be3e5630969e316dd1755065542fcee505`; neither this component nor the
alioth fork is selected in elish's board configuration.

## Current port distance

Elish has verified Android UFS capacity and GPT, a host-built and hash-checked
APFS userdata image, and the stock controller/PHY/clock/reset descriptions.
It has not verified XNU access to the controller, DMA or command completion,
device block reads, root publication, APFS mounting, or launchd. The active
XNU candidate remains `loadable=false`, and `_arm_vm_init()` return remains
unconfirmed. The public alioth result cannot fill those elish milestones.

The next measurement should establish the elish UEFI-side controller power,
clock and reset state, then capture its controller registers and SMMU global
control/stream mapping while the device is still in UEFI. Any controller MMIO
read should follow proof that its power domain and access window are live.
Only after stream routing and buffer addressability are proven should a bounded
NOP/read-capacity command be attempted; then known-block reads, root windows,
APFS mount and userspace follow in order. Keep UFS DMA disabled until this
evidence exists.

## Reproduction

Live Android capture:

```sh
adb -s 4bed07e4 shell 'printf "device="; getprop ro.product.device; printf "\\nfingerprint="; getprop ro.build.fingerprint; printf "\\nkernel="; uname -a; printf "\\nSMMU disable_bypass="; cat /sys/module/arm_smmu/parameters/disable_bypass; printf "\\nUFS iommu_group="; if test -e /sys/devices/platform/soc/1d84000.ufshc/iommu_group; then readlink /sys/devices/platform/soc/1d84000.ufshc/iommu_group; else echo absent; fi; printf "\\nUFS driver="; readlink /sys/devices/platform/soc/1d84000.ufshc/driver; printf "\\nUFS diagnostics:\\n"; dmesg | toybox grep -i -E "ufshcd-qcom 1d84000|print_pwr_info|Re-init UFS host"'
```

Kernel source checkout and revision:

```sh
git -C /tmp/android_kernel_xiaomi_sm8250-lineage-24.0 rev-parse HEAD
git -C /tmp/android_kernel_xiaomi_sm8250-lineage-24.0 show HEAD:drivers/iommu/of_iommu.c
git -C /tmp/android_kernel_xiaomi_sm8250-lineage-24.0 show HEAD:drivers/iommu/arm-smmu.c
```
