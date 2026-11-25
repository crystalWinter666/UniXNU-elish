# elish host package and device availability, 2026-11-25

## Host build

The iPadOS 17.7.11 (`21H461`) package completed on the host with:

```sh
./scripts/package.sh --no-deps --os ipados-17.7.11-21H461 --uefi /root/repo/uefi-elish --out /root/elish-build-20261125/UniXNU/out -j 8
```

Source revisions recorded by the package are device repository
`4b3da68ea0aca815a80e8d1bad802dfec8d5c905`, UniXNU
`71c5c6569bae6369c0e0f634df8b9bd5afc19139`, Mu-Silicium
`f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`, and SM8250 SoC package
`166235ec2480ff0992627b7aafa15c65ce6b4efa`.

Outputs are under
`/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/`:

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `boot/boot.img` | 67,747,840 bytes | `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140` |
| `payload/payload.img` | 67,108,864 bytes | `9aaca2b53648d2dd8bb9ae7f362b8069c951e1f6760c048f8b8b18e45b5f3821` |
| `kernel/kernel.macho` | 51,073,024 bytes | `acb92e0807afc554567d99f0e0105cf8d15abb45ce108b064afdc7e64b79638b` |
| selected UEFI FD embedded in `boot.img` | 3,145,728 bytes | `286fd558b9988a7e9c6e9dbde30cf10b9a62a2ba5c0e379db1efbc102ec230` |

The package manifest and `boot/boot.json` agree on the image and selected FD
hashes. The boot image is Android header v3, carries the 64 MiB FAT payload as
its ramdisk, and has no appended DTB. `fsck.fat -vn` found seven files and no
filesystem errors. `python3 scripts/check-elish.py --static-only`, the
14-area porting-record check, and `git diff --check` passed. UniXNU's
`test_bootimg.py` (1 test) and `test_boot_template.py` (13 tests) also passed.

The generated `kernel/kernel.json` still has
`instruction_relocation_complete=false`, `register_dataflow_qualified=false`,
`abi_qualified=false`, `runtime_qualified=false`, and `loadable=false`.
`boot/boot.json` and `payload/payload.json` both say
`hardware_tested=false`. These are host package results, not a device boot.
Build logs are in the package's `logs/` directory, including `package.log`,
`firmware.log`, `kernel.log`, and `kernel-bootstrap.log`.

## Device snapshot

At 2026-11-25 09:07 CST, `lsusb -d 18d1:d00d`, fastboot
`37.0.3-16887417 devices -l`, and `adb devices -l` showed no tablet. No
fastboot command, image transfer, reboot, reset, `fastboot continue`, or
partition write was issued.

The top camera frame `/tmp/elish-live-current-20261125.mjpg` is 1920x1080,
SHA-256 `bca33dc31d89db9da124b142e441b475e567f477e00b820e3e9f0a0579fcf44d`.
It is nearly black and does not identify the screen or boot state.

No candidate was run in this round. The latest directly confirmed device stage
remains run09's close photo showing the green ExitBootServices handoff, blue
XNU entry band, and magenta pre-MMU band. MMU completion, device-side UFS/APFS
mount, and iPadOS userspace are not confirmed. The latest boot-chain logs that
directly establish FAT payload discovery, DarwinBoot, the 256 MiB window, and
successful ExitBootServices remain run06/run11; those probes stopped before
XNU. See `reports/elish-run14-run17-xnu-timing-20261125.md` and
`reports/elish-loader-fat-initrd-20261124.md`.

The regular candidate built above is not the run17 checkpoint image. Run17 is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run17-printf-init/boot.img`,
67,751,936 bytes, SHA-256
`3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`. It calls
`PE_init_printf()` then waits 15 seconds before resetting; it has not run.
When USB and fastboot control requests return, use it as a RAM-only boot, then
use the reader image for root ADB/pstore only after fastboot confirms boot
acceptance.

## UFS component fit

The pinned catalog describes `ShengUfs` as an SM8550 UFS driver with SM8550
clocks and SMMU dependencies. UniXNU's cupid/SM8450 porting record says that
driver does not match SM8450 as-is and requires SoC-specific compatible,
register, and stream handling. Its `0xe0` stream and SM8450 GCC offsets are not
elish inputs. Elish's stock Kona DT and Xiaomi Android R source instead map the
controller clocks to Kona GCC plus RPMh reference clocks, with reset ID 33 at
`0x77000`; runtime votes, PHY setup and the UFS SMMU stream remain unknown.
Keep UFS DMA disabled until a post-MMU device probe identifies the live owner
and state. Sources: `docs/porting/ufs.md`,
`docs/devices/cupid-lengcece.md`, and
`reports/elish-ufs-kona-clock-map-20261125.md` in the UniXNU checkout.
