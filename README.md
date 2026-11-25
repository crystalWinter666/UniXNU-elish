# UniXNU for Xiaomi Pad 5 Pro Wi-Fi

An experimental iPadOS 17.7.11 (`21H461`) port for `elish` (`M2105K81AC`, Snapdragon 870). This repository provides the board configuration, device tree, XNU patches and image packaging. [UEFI firmware source](https://github.com/crystalWinter666/uefi-elish)

RAM-only tests confirm FAT payload discovery, DarwinBoot, the 256 MiB window and successful `ExitBootServices()`. A run09 close photo confirms XNU entry through the pre-MMU checkpoint; its six-second stage-3 reset and USB timeline meet UniXNU's diagnostic criterion for MMU enable, while `_arm_vm_init()` return remains unconfirmed. Run19's only valid apps-SMMU mapping belongs to MDSS SDE, not UFS. The kernel remains `loadable=false`; UFS access, device-side APFS mount and iPadOS userspace have not been verified. A same-SoC alioth port reaches UFS root and launchd, but its board streams and clocks do not establish elish hardware support. [XNU entry evidence](reports/elish-xnu-entry-visual-retest-20261125.md) · [SMMU probe](reports/elish-smmu-readonly-probe-20261125.md) · [UFS evidence and reference audit](reports/elish-ufs-reference-audit-20261125.md)

## Build

Prepare the UniXNU toolchain and your local build inputs, including the target IPSW and platform seed. Clone the three repositories into sibling directories. [UniXNU build guide](https://unixnu.com/building)

```sh
git clone https://github.com/crystalWinter666/UniXNU-elish.git
git clone --recurse-submodules https://github.com/crystalWinter666/uefi-elish.git
git clone https://github.com/UniXNU/UniXNU.git
git -C UniXNU checkout 8542980d02b4d2cc5c32ee3542de1760b2562aac
git -C UniXNU apply ../UniXNU-elish/patches/unixnu-build-support.patch
cd UniXNU-elish
./scripts/package.sh --check --os ipados-17.7.11-21H461
./scripts/package.sh --os ipados-17.7.11-21H461 --uefi ../uefi-elish
```

The build support patch adds Android v3 kernel packaging without an appended DTB and describes the measured disjoint DRAM ranges in the ADT. The [porting record](porting.toml) and [handoff](HANDOFF.md) document device evidence and outstanding work.
