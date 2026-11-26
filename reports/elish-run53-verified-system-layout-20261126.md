# elish run53 verified UFS root layout

Run53 moves the two verified-root digest tables to `__LASTDATA_CONST`. The linked `__TEXT_EXEC` address and size match the run51 kernel at `0xfffffff007240000` and `0x7bc000`. The digest tables start at `0xfffffff007a00020` and `0xfffffff007fa9060`. The 80-second camera frame shows the green, blue and magenta bands. The last confirmed checkpoint is pre-MMU. See [run52](elish-run52-verified-system-20261126.md) for the original executable-segment movement.

`OUT` is `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run53-verified-system-layout`. Its `source.sha256` pins local inputs over device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, and XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`. `root-digests.log`, `storage-abi.log`, `layout.log`, `payload-fsck.log`, and `boot-build.log` record checked inputs and outputs.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `2771fd3ab994caa782a7e60f4fadd8502a13e3fde177181b83c3a515a4993cf5` |
| `SILICIUM_UEFI.fd` | `fda4d664b23a58f28da7f567ff3b74fe6a54619c6649457fc29e47f4d867f55d` |
| `kernel.macho` from FAT | `3bb415058452c7f859e436410d608ed639fabe2b253162ec5b8d05c6bb0f25ea` |
| `kernel.macho.adt` | `4b2aa1e3d3def264e33c3f08aaf85762c28be99331c078bd4ba00a89107bde33` |
| `payload.img` | `77eb40e53e11abadf4c7903a16f9d2a4c5a632207a77b93a22b4a979d697cb88` |

The active `elish_defconfig` enables the XNU owner-root audit and SystemOS mount. Build with `UNIXNU_ELISH_ROOT_IMAGE` set to the host `userdata-root.img` and `unixnu build kernel -D . --os ipados-17.7.11-21H461 -j 12`. Both 190-slot storage ABI checks passed. Build the payload with the Apple static and System trust caches and `elish-ufs-root=1 elish-ram-only=75 elish-warm-reset=1 elish-launchd-checkpoint=1`. FAT trust-cache readback and `fsck.fat -vn` passed. The Android header v3 image reuses the run51 DEBUG firmware and the captured elish FDT build input.

After run52 the tablet entered the RAM-only Lineage reader. The host sees serial `4bed07e4` in ADB as `unauthorized`, with no bootloader fastboot transport. `adb -s 4bed07e4 reboot bootloader` returned `device unauthorized`. The 180-second USB preflight in `device.log` ended in the same ADB state before reaching `fastboot boot`. Resume with bootloader fastboot and recheck USB mode, serial and product before RAM-booting run53.

The follow-up USB check identified `18d1:d001` with a single `ff/42/01` ADB interface and USB product string `M2105K81AC`. `fastboot devices -l` returned no device. The USB interface confirms the current mode despite the fastboot label in `lsusb`'s ID database.

On the later attempt at 05:49 CST, USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` passed preflight. `fastboot -s 4bed07e4 boot "$OUT/boot.img"` reported `Sending OKAY` and `Booting OKAY` in 1.804 s. The video at `device-attempt.rWB3cS/camera.mkv` has SHA-256 `d361818fc0f7023415be2400f06d7c1bdeee3b20f8eb743c53805f8a460d49af`. Its `screen-80s.jpg` has SHA-256 `8116ba368c7b28f0c6f10e42b426c1feda5b3abff7d2669c041ca6499691b22e`. The tablet subsequently enumerated in bootloader fastboot. The camera capture contains no target root or PID 1 proof.

## Hardware acceptance

| Item | Evidence required | Status |
| --- | --- | --- |
| Full System root | The checkpoint reports `elish System root confirmed`, APFS UUID `04769D26-2CA0-4825-B443-E2E5F88300F2`, and the mounted BSD device. | Pending |
| Apple `launchd` PID 1 | The same checkpoint reports `PID 1 main: launchd` after the successful `kern.initproc_spawned` write. | Pending |
| Setup | Boot with an unconfigured writable Data seed and allow launchd to continue. Record Setup Assistant appearing and reaching completion on elish. | Untested |
| Data persistence | Write and sync a test marker on the Data mount, then verify its contents and the Data volume identity after a restart and recovery cycle. | Untested |

Run53 deliberately calls `panic()` at the PID 1 checkpoint to display the proof on the framebuffer. It has no Data seed, and its elish UFS service exposes read-only OS and SystemOS windows. Setup needs a separate boot without the terminal PID 1 checkpoint. Persistent Data also requires an elish-backed writable Data window with verified geometry and an approved backup and restore plan. The earlier prefix install authorization covered LUN 0 LBA 6,619,136–8,909,823 only.

For a RAM-only Setup test, enable `CONFIG_XNU_OWNER_DATA_MOUNT` and set `CONFIG_XNU_OWNER_DATA_VOLUME_UUID` to the Data seed's observed APFS volume UUID. XNU checks that RamBlock's selected device is the writable APFS mount at `/private/var`. RamBlock requires a persistent store to write to packed disks larger than 2 GiB. The standard `make-data-seed.py` path requires macOS `hdiutil` and grows the seed to at least 3 GiB, so the volatile test needs a smaller unconfigured seed. The elish build outputs contain the Restore seed from run23. A target Data seed is still needed.

For persistent Data, RamBlock needs the `native_ufs_store_*` transport callbacks. Elish currently supplies the verified read-only root windows. Qualifying a Data window starts with read-only bounds and GPT checks before requesting approval for the exact write range.
