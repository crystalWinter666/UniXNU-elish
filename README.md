# UniXNU for Xiaomi Pad 5 Pro Wi-Fi

An experimental iPadOS 17.7.11 (`21H461`) port for `elish` (`M2105K81AC`, Snapdragon 870). This repository provides the board configuration, device tree, XNU patches and image packaging. [UEFI firmware source](https://github.com/crystalWinter666/uefi-elish)

RAM-only `fastboot boot` reached the UniXNU menu. The firmware probe captured the Qualcomm RAM Partition table and 47 pre-BDS UEFI DRAM descriptors. DarwinBoot's handoff to XNU, the UFS/APFS root path and iPadOS startup remain unverified. [Hardware report](reports/elish-uefi-pre-bds-memory-map-20261124.md)

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
