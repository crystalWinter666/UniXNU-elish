# elish iPadOS host image build, 2026-11-24

## Build

The host packaging command completed for the pinned iPadOS 17.7.11 profile.

```sh
./scripts/package.sh --os ipados-17.7.11-21H461 --uefi /root/repo/uefi-elish
```

The package manifest records the device, profile, output hashes, and component revisions. The boot-image builder's Android v3 image was unpacked and compared with its kernel, ramdisk, firmware, and device-tree inputs. A second round-trip image at `/tmp/elish-ipados-boot-roundtrip.img` is byte-identical to the published build output.

## Artifacts

| Artifact | Size | SHA-256 | Result |
| --- | ---: | --- | --- |
| XNU `kernel.macho` | 51,122,176 bytes | `7ab7c4c90647e17ae1a301117fc3e067616431a17b164a5ef01b0867603605b2` | Host-built experimental candidate |
| iPadOS payload | 67,108,864 bytes | `eb3f2dd1ab8b662ead42c5c05b86d530162f35804f6ba4593a57f18ffc9357b9` | Built and package-verified |
| Android v3 `boot.img` | 67,727,360 bytes | `0f6fdd711ff2a22284e32f969716041d5e79ed7c46503fc4335666d26eec3aef` | Built and byte-for-byte round-trip verified |
| Mu-Silicium `SILICIUM_UEFI.fd` | 3,145,728 bytes | `16bf48826a8c99ecaacd53288e50f834078b64cdc8d753a6c50394da7f68179e` | Embedded in the host-built image |

The boot manifest reports header version 3, a 64 MiB uncompressed ramdisk, and `hardware_tested=false`. `payload.json` also records `hardware_tested=false`. The image has not been started on the target.

The kernel qualification report still sets instruction relocation, register-dataflow, ABI, runtime, and `loadable` fields to false. A valid host package and image round trip do not establish that this experimental kernel can execute on elish.

The 8.74 GiB userdata-root container is separately recorded in `reports/elish-userdata-build-20261124.md`. It has not been flashed or read by an elish UFS driver.

## Current worktree rebuild

The device-repository entry point completed a fresh package build using the current worktree:

```sh
./scripts/package.sh --no-deps --os ipados-17.7.11-21H461 --uefi /root/repo/uefi-elish
```

The core revision is `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, the device repository HEAD is `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`, and Mu-Silicium HEAD is `66421cb7bb5694843aec8de1ca3d1de4c20cca38`. The package resolved 63 patches and rebuilt all four artifacts.

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| XNU `kernel.macho` | 51,122,176 bytes | `7ab7c4c90647e17ae1a301117fc3e067616431a17b164a5ef01b0867603605b2` |
| iPadOS payload | 67,108,864 bytes | `73a1bfa3f410a68246456bc4f85042b4753b714e0a7718294c39f9e98a66aec6` |
| Android v3 `boot.img` | 67,727,360 bytes | `2920a4deb130a789f2e2992e5fc6eb944e77a58e7464a8446215d7ef1b5678a9` |
| Mu-Silicium `SILICIUM_UEFI.fd` | 3,145,728 bytes | `33c28a3dea6fc5e54f967cb0fc765318bec1f566ca429359d272b9ce5e0c6bed` |

The package verifier completed successfully. `scripts/check-elish.py` passed with the board's runtime-managed memory configuration, while noting that device handoff remains unverified. Boot-image and template tests, all 65 XnuPrelink tests, and the 14-area porting-record check passed.

The generated `kernel.json` still sets instruction relocation, register-dataflow, ABI, runtime, and `loadable` qualification to false. Both payload and boot manifests set `hardware_tested=false`. The host currently enumerates ADB serial `ELISH_SERIAL` as `unauthorized`, with no fastboot device, so this image has not been started on elish and no partition has been written.

## Device handoff retry

The requested RAM-only test command was attempted against the reported serial:

```sh
timeout 20s fastboot -s ELISH_SERIAL boot /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img
```

Fastboot printed `waiting for ELISH_SERIAL` and timed out before sending the image. A subsequent `adb -s ELISH_SERIAL reboot bootloader` returned `error: device unauthorized`. The host's default ADB key exists, but the target has not authorized it. A host run of DarwinBoot's production Mach-O parser passed for the packaged kernel. There is still no device boot observation, and no partition was written.

## Authorized RAM-only device tests

ADB authorization later became active. The host identified `lineage_elish` / `M2105K81AC`, and `adb -s ELISH_SERIAL reboot bootloader` succeeded. The bootloader appeared as USB `18d1:d00d`.

The current image was started with:

```sh
fastboot -s ELISH_SERIAL boot /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img
```

Fastboot sent the 66,140 KiB image and returned `OKAY` for both send and boot, with total time 1.889 seconds. About five seconds later, the device enumerated as Android ADB. No UEFI, DarwinBoot or XNU log was captured, so this does not verify an iPadOS boot.

The host then RAM-booted the diagnostic image `diagnostics/elish-memory-map-fdt-probe.img`. Fastboot accepted it in 0.257 seconds. It is 622,592 bytes, has no ramdisk, and SHA-256 `3ccdc1738073add586a8eb87c7c6ad1deff3120fff899aa6d525f87335379a93`. Android USB returned again, with ADB `unauthorized`. The diagnostic property `/chosen/unixnu,uefi-memory-map` has not been read, so its reported RAM Partition entries and UEFI descriptors remain unknown.

The host build is complete. Current blockers to a verified iPadOS boot are the kernel report's `loadable=false` and false relocation, register-dataflow, ABI and runtime qualifications, unverified runtime UEFI/XNU memory ownership, and the absent elish XNU UFS/APFS root path. These make the remaining runtime/debug time unpredictable. No partition was written.

## Latest RAM-only boot re-test

On 2026-11-24, `fastboot -s ELISH_SERIAL getvar product` returned `elish`. The
current 67,727,360-byte `boot.img` has SHA-256
`2920a4deb130a789f2e2992e5fc6eb944e77a58e7464a8446215d7ef1b5678a9`.
`fastboot -s ELISH_SERIAL boot .../boot.img` returned `OKAY` for transfer and
boot. USB then enumerated Android with fingerprint
`Xiaomi/elish/elish:13/RKQ1.211001.001/V816.0.2.0.TKYCNXM:user/release-keys`.
This run did not capture UEFI, DarwinBoot or XNU output and does not verify an
iPadOS boot.

The Android shell ran as uid 2000. It could not read `/sys/fs/pstore`, and
`su` was unavailable, so this boot did not expose persistent firmware logs.
The current artifact remains a host-built candidate with
`hardware_tested=false` and XNU `loadable=false`.

## `_arm_vm_init` return probe

The existing 67,727,360-byte diagnostic image
`diagnostics/elish-xnu-arm-vm-return.img` has SHA-256
`d206d18d7a8bd4f8cdadb46a0e42dd3bf578baa2e6961384417fb0e42ad57e3f`.
After `adb -s ELISH_SERIAL reboot bootloader`, the host ran:

```sh
fastboot -s ELISH_SERIAL boot /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-arm-vm-return.img
```

Fastboot returned `OKAY` for transfer and boot in 1.888 seconds. Android ADB
reappeared during the following 10-second poll and identified stock Android 13.
Host USB logs show a disconnect at 20:09:13 and Android enumeration at
20:09:26. This timing is indirect evidence only. No framebuffer or serial
output confirms that `_arm_vm_init` returned and triggered the probe reset.
