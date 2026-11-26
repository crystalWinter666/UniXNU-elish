# UniXNU for Xiaomi Pad 5 Pro Wi-Fi

An experimental iPadOS 17.7.11 (`21H461`) port for `elish` (`M2105K81AC`, Snapdragon 870). This repository provides the board configuration, device tree, XNU patches and image packaging. [UEFI firmware source](https://github.com/crystalWinter666/uefi-elish)

Run79 reached the original iPadOS 17.7.11 Setup Assistant Hello screen on the elish internal panel, using the read-only UFS System and SystemOS volumes, writable RAM Data and the GOP CPU-copy presenter. Touch, language selection, Setup completion, the home screen and persistent Data remain unverified. [Hello photo](reports/elish-run79-hello-20261126.jpg) · [Run79 build and device evidence](reports/elish-run79-key-store-20261126.md)

## Build

Prepare the UniXNU toolchain and your local build inputs, including the target IPSW and platform seed. Clone the three repositories into sibling directories. [UniXNU build guide](https://unixnu.com/building)

```sh
git clone https://github.com/crystalWinter666/UniXNU-elish.git
git clone --recurse-submodules https://github.com/crystalWinter666/uefi-elish.git
git clone https://github.com/UniXNU/UniXNU.git
git -C UniXNU checkout e7bddb9a7eecdcf81f36da9fa38bd9d4d3f155c2
git -C UniXNU apply ../UniXNU-elish/patches/unixnu-build-support.patch
cd UniXNU-elish
./scripts/package.sh --check --os ipados-17.7.11-21H461
./scripts/package.sh --os ipados-17.7.11-21H461 --uefi ../uefi-elish
```

The build support patch adds Android v3 kernel packaging without an appended DTB, describes the measured disjoint DRAM ranges in the ADT, and exposes `--root-policies` for the verified-root payload. The [porting record](porting.toml) and [handoff](HANDOFF.md) document device evidence and outstanding work.

New elish images use the `oc-like` boot menu theme. Pass `--boot-menu-theme default` to `scripts/package.sh` to select the classic UniXNU menu for a build.

For System-root diagnostics, first build the host image with `../UniXNU/unixnu build userdata -D . --os ipados-17.7.11-21H461`. Refresh the active configuration with `../UniXNU/unixnu defconfig elish_defconfig -D . --os ipados-17.7.11-21H461`, then set `UNIXNU_ELISH_ROOT_IMAGE` to the resulting `userdata-root.img` when building the kernel. The archive recipe verifies both complete IPSW windows before generating `tree/build/elish-ufs/elish-root-digests.bin`. Pass that file to `unixnu build payload` with `--root-policies` and select `--kernel-arg elish-ufs-root=1`. XNU authenticates the handoff against the linked SHA-256 before publishing the UFS windows.
