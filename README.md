# UniXNU for Xiaomi Pad 5 Pro Wi-Fi

An experimental iPadOS 17.7.11 (`21H461`) port for `elish` (`M2105K81AC`, Snapdragon 870). This repository provides the board configuration, device tree, XNU patches and image packaging. [UEFI firmware source](https://github.com/crystalWinter666/uefi-elish)

On elish, RAM-only run56 mounted the full 21H461 System APFS root and SystemOS from UFS and confirmed Apple `launchd` reaching `main()` as PID 1. Setup and persistent Data remain untested. [Run56 device evidence](reports/elish-run56-verified-systemos-union-20261126.md)

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

For System-root diagnostics, first build the host image with `../UniXNU/unixnu build userdata -D . --os ipados-17.7.11-21H461`. Refresh the active configuration with `../UniXNU/unixnu defconfig elish_defconfig -D . --os ipados-17.7.11-21H461`, then set `UNIXNU_ELISH_ROOT_IMAGE` to the resulting `userdata-root.img` when building the kernel. The archive recipe verifies both complete IPSW windows before generating `tree/build/elish-ufs/elish-root-digests.bin`. Pass that file to `unixnu build payload` with `--root-policies` and select `--kernel-arg elish-ufs-root=1`. XNU authenticates the handoff against the linked SHA-256 before publishing the UFS windows.
