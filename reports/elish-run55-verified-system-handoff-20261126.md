# elish run55 verified System handoff

The kernel archive links a 32-byte SHA-256 anchor for the OS and SystemOS digest tables. DarwinBoot passes the tables in the ADT `RootPolicies` extent, outside the kernel's Mach-O segments. The XNU UFS service checks the extent and authenticates the whole bundle before publishing either read-only window. It verifies each 32 KiB chunk before delivering bytes to APFS.

`OUT` is `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run55-verified-system-handoff`. Sources use the run53 baselines with local hashes in `source.sha256`. The active configuration builds the owner root audit and mounts SystemOS at `/private/preboot` for the PID 1 startup path.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `40a5b16c6b318abcaf2784aa8ba5f2d94e3fba26ed7872ac7d64fe4c60c0a755` |
| `payload.img` | `5c7a0757e290764358df3bf8f0d46f4ceffd65927be1069cd1eb171ade227361` |
| `kernel.macho` from FAT | `6d1f373d673b58fc6f372f468e7dd981776a6ed6094684ee2b6b8feb1fea72e2` |
| `kernel.macho.adt` | `9026cc2ad31145971001eed8c5076fb5351c2901b6dcef48fe5de3ac4d907f66` |
| `root-digests.bin` | `d33ea5be6ec15c3acb1c88940687f8d21859b1903f2d9705e956be164b460066` |
| `apple.trustcache` | `7416ca4f8377146626bb284e2133c76bee6cb4e3ee0af5d648b186235a7323a7` |
| `SILICIUM_UEFI.fd` | `fda4d664b23a58f28da7f567ff3b74fe6a54619c6649457fc29e47f4d867f55d` |

The host built and prelinked XNU with `UNIXNU_ELISH_ROOT_IMAGE` set to the run51 `userdata-root.img`. Both 190-slot storage ABI checks passed. The payload used `--root-policies "$OUT/root-digests.bin"`, the original Apple trust-cache bundle from run53, and kernel arguments `elish-ufs-root=1 elish-ram-only=75 elish-warm-reset=1 elish-launchd-checkpoint=1`. The remainder of the payload and `bootimg` commands follow [run54](elish-run54-ufs-data-readonly-20261126.md#build-and-device-status). `fsck.fat -vn` passed and the FAT digest bundle and trust cache matched their inputs byte for byte.

At 06:00 CST USB `18d1:d00d`, serial `4bed07e4`, product `elish` and `is-userspace=no` passed preflight. `fastboot -s 4bed07e4 boot "$OUT/boot.img"` returned both `OKAY` results in 1.810 s. The 78-second frame still shows the early bands. The 84-second frame shows `initproc failed to start` with a dyld library load error whose visible path starts `/usr/lib/libSystem`. PID 1 reached the dynamic loader. The camera did not capture a System UUID, mount-device checkpoint, or `launchd` `main()` confirmation. The device returned to bootloader fastboot.

Evidence in `OUT/device-attempt.HCny71/` includes `device.log`, `camera.mkv` (SHA-256 `e26613b5e5c56ed6b4292a0ae4d71134c8710d2a95dd86a02e08eafaebc6d420`) and `screen-84s.jpg` (SHA-256 `8680632b9aa884eab6c101a209a6fcf3728ac0f7af35d49bcb77369c3c744024`). The boot was RAM-only and both data paths were read-only. Setup and Data persistence are untested.
