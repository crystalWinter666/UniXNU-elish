# elish run61 RAM Data snapshot and run62 control

Run61 carried the checked 21H461 RAM Data seed, with a 135-second userspace snapshot and a separate 240-second fastboot deadline. The tablet returned to bootloader fastboot 157.65 seconds after `Booting OKAY`. The camera file ends at packet timestamp 124.58 s. The [120-second frame](elish-run61-pre-mmu-20261126.jpg) shows the last directly observed checkpoint, the magenta pre-MMU band. The return time alone cannot identify the reset path. The live `/private/var` mount, sustained PID 1, backboardd, Setup Assistant, SpringBoard, touch and Data persistence remain unverified.

The unseeded run60 control showed `elish Data mount: errno=2` and returned to verified bootloader fastboot about 21 seconds after launch. This confirms execution reached Data media lookup when the payload has no seed. [Run60 screen](elish-run60-data-enoent-20261126.jpg)

## Build and readback

Device `16cedc7b8a9a417e3994e37f420dc6ec485a7da1`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`, firmware checkout `226fbcc677568edf9a04ce7cd95459c664821a03`. The firmware bytes match run56. Run61 `source.sha256` records the local kernel inputs. `kernel.config` selects the 1 GiB RAM Data volume `7D443C28-95AC-49F8-AF25-4FC12FBC93E5`. The existing verified System and SystemOS UFS paths, RootPolicies and Apple trust cache were retained. The terminal PID 1 checkpoint was disabled. XNU compilation, prelink, both 190-slot storage ABI checks, FAT validation and payload readback passed.

Use the [run58 build procedure](elish-run58-writable-ram-data-20261126.md#kernel-and-payload) with the run61 `kernel.config` and `source.sha256`, and add `--kernel-arg elish-userspace-snapshot=135` to the payload command. The remaining diagnostic overrides were `elish-ufs-root=1 elish-ram-only=240 elish-warm-reset=1 elish-launchd-checkpoint=0`.

Run62 uses the run61 FAT payload and replaces `EFI/UniXNU/unixnu.toml` with the output of an otherwise identical payload command using `elish-userspace-snapshot=95`. The regenerated ADT differed from run61, so the run61 ADT was retained in the control. Readback confirmed the config differs only in the snapshot value and the kernel, ADT, RootPolicies, trust cache and seed all match run61 byte for byte. `fsck.fat -vn` passed. Android v3 packaging used the same FD and FDT as run61. See the [run62 device evidence](elish-run62-data-springboard-20261126.md).

`BASE=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125`. File paths below are relative to `BASE` except the report photo.

| Artifact | SHA-256 |
| --- | --- |
| `run61-userspace-snapshot/boot.img` | `7fbdb2cb35ea96c69456ad5734efa13d64053165b529ab074afa41ae1c7d62ef` |
| `run61-userspace-snapshot/payload.img` | `05162b3d092fa9e78bf15450c83dcd87689ecce6dd1bd9eeb1e2f7656a4644d9` |
| `run61-userspace-snapshot/device-attempt.ydC0Ib/camera.mkv` | `b67dd17bf4fb91e9bda018d0c450fbb7bd8ab16c02b40610c15a7eed339c0f35` |
| `elish-run61-pre-mmu-20261126.jpg` | `1216029a8fc0ce6515b789e32ef492f0c1e54d7fcfeba51f6c018bde48dbf973` |
| `run62-earlier-snapshot/boot.img` | `80e7067f4069e92f103cb678cf333bd496e404292aa335e73bb7af78454686a7` |
| `run62-earlier-snapshot/payload.img` | `70bc5e155f3a5831e3cae2a03c9a7003c85f15590759547960bd674935f3a38a` |

## Device and log retrieval

`run61-userspace-snapshot/device-attempt.ydC0Ib/device.log` records USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` both before boot and after fresh USB enumeration. The boot command was `fastboot -s 4bed07e4 boot "$BASE/run61-userspace-snapshot/boot.img"` through `run58-writable-ram-data/capture-boot.sh`. The camera log records invalid MJPEG input. The resulting Matroska file is unfinalized and lacks the last part of the run.

The verified Lineage reader at `run52-verified-system/boot-b-reader-candidate.img` matched SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`. The first attempted upload after successful identity checks stalled at `Sending`. Reauthorizing the host USB port `1-3` restored transfer. Official fastboot 37.0.3 passed all four identity checks and completed `fastboot -s 4bed07e4 boot "$BASE/run52-verified-system/boot-b-reader-candidate.img"` in 1.554 seconds. `reader-usb-reauthorized.log` records the successful upload and boot. USB `18d1:d001` subsequently exposed ADB interface `ff/42/01`, serial `4bed07e4`, product string `M2105K81AC`. ADB with the libusb backend reported `unauthorized`, leaving ramoops unavailable and the device outside bootloader fastboot. The diagnostic commands issued no persistent partition writes.

