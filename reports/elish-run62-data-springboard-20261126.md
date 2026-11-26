# elish run62 writable RAM Data and SpringBoard process

The 95-second XNU snapshot showed `Data APFS rw=1`, `launchd=1 main=1`, `backboardd=0` and `SpringBoard=26 Setup=0`. The Data check runs after `/private/var` resolves to a writable APFS mount with the same BSD device selected by exact IOMedia UUID `7D443C28-95AC-49F8-AF25-4FC12FBC93E5` through the RamBlock provider. The frame crops the end of the device name and UUID. Zero is the process probe's result for no matching PID at the time of sampling. No rendered SpringBoard frame, Setup page or touch interaction was captured. [Snapshot photo](elish-run62-data-springboard-20261126.jpg)

Build inputs and revisions are the run61 inputs. The only change inside the FAT is `elish-userspace-snapshot=95` in `EFI/UniXNU/unixnu.toml`. FAT structure and byte-for-byte readback of the kernel, ADT, RootPolicies, Apple trust cache and Data seed passed. [Build and hashes](elish-run61-userspace-snapshot-20261126.md#build-and-readback)

The host checked USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` before and after `fastboot -s 4bed07e4 boot "$OUT/boot.img"`. The command completed in 1.805 s. Fresh USB enumeration and identity checks completed 117.49 s after `Booting OKAY`. The RAM-only diagnostic issued no persistent partition writes. Data persistence and Setup completion remain untested.

`OUT=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run62-earlier-snapshot`. The USB transcript is `device-attempt.IGWRLq/device.log`. The H.264 capture spans `camera-000.mkv` through `camera-012.mkv` in the same directory, with hashes in `camera.sha256`. The snapshot is visible at about 4–6 s of `camera-011.mkv`.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `80e7067f4069e92f103cb678cf333bd496e404292aa335e73bb7af78454686a7` |
| `camera-011.mkv` | `34a00aa86b1626de46654da166ca3b4decd119c3c7b59806bb97bb233960c9bf` |
| `elish-run62-data-springboard-20261126.jpg` | `b1be538f2c9f3aa16512e1a681a01ea70ddc63ec7344994f994f3001761f57e4` |
