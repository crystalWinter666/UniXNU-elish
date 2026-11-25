# elish UFS host-only audit, 2026-11-25

This report records offline NativeUfs checks and the device recovery attempt on
2026-11-25. No persistent device partition was written.

## Device and image record

- Fastboot USB enumerated as `18d1:d00d`, serial `4bed07e4`; `fastboot getvar
  product` returned `elish`.
- Reader image: `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`, 55,083,008 bytes, SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`.
- RAM-only `fastboot boot` started but remained at `Sending 'boot.img' (53792 KB)` without `Booting OKAY`. The host process blocked in `usbfs_start_wait_urb`; it was interrupted after about 90 seconds. Fastboot remained enumerated, ADB did not return, and pstore was not read. This is not a successful reader boot.
- A later read-only descriptor check showed fastboot EP `0x01` OUT / `0x81` IN, 512-byte bulk packets, at 480 Mbit/s. `getvar max-download-size` returned no value; a subsequent `getvar product` returned `GetVar Variable not found`. The kernel log showed no new disconnect or xHCI error for this device. The initial `product=elish` result predates the stalled transfer; current bootloader identity is not reconfirmed.
- No iPadOS diagnostic image was launched this round. The corrected run08 candidate remains `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run08-xnu-entry-checkpoint-corrected/boot.img`, SHA-256 `9cd0577439a5f143ff11d81cdaa6aece79da01a25cfd5cfdf67ddea0a7a5075a`.
- The user's latest screen photo, `/root/repo/D5E326E8269942AF9C00495CFD941302.png`, SHA-256 `2d94beff27bbe43fd9f7d5832303debd0bb3b9946c2935d8e791921a6dde4a2c`, says: “Exiting boot services. A green bar across the top marks the loader handoff before the kernel branch.” It confirms the EBS handoff, not XNU entry.
- Last directly confirmed boot-chain stage remains run04: successful `ExitBootServices()` with the 256 MiB window at `0xc007c000+0x10000000`, map key `0x3ee`, and 73 descriptors. Its image is `run04-fat-child/boot.img`, SHA-256 `8ef2e0a78c37171b74d50601703aa94b2f8ec7ed7a1aed00fbb0aebfb2bb33e7`; the full log is `reports/elish-darwinboot-ebs-20261124.txt`.

## Offline tests

The host test copy is based on NativeUfs-alioth revision
`336680c353e6c3f25d6b29284532c4fd16104f68`. Its uncommitted
`NATIVE_UFS_PROBE_ONLY` define and test-shim adjustments are confined to
`/tmp/elish-nativeufs-probe-only`; no driver source in the device repository or
core checkout was changed.

- `RootLayout` passed GPT CRC/geometry and partition-fault checks, manifest validation, and 2,048 bounded mutations. It parsed the 4 KiB manifest from `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/userdata/userdata-root.img` (SHA-256 `fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093`); it did not rehash the full image.
- `RootWindowRead` passed 512-byte, 4 KiB, and 128 KiB boundaries, tail handling, partial progress, and concurrency-lock checks.
- `RootPublication` passed preparation/rollback and media-start failure cases, ordering, and ancestry bounds.
- `RootDiagnostics` emitted its expected synthetic `transport timeout fixture` record; this is a fault-injection case, not a device failure.
- `SmmuTakeover` passed synthetic geometry, conflict, stream-ID, address-width, reuse, repair, and blocked-write cases.
- `check_arm64_root_commands.py` passed five QEMU TCG EL1 cases after adapting only the temporary test shim to the current `ufsThreadId` and `IOLockTryLock` interfaces. Low and high DMA, inner-shareable mapping, truncated high DMA, and virtual-as-physical refusal all passed. The run exercised NOP, READ CAPACITY, 4 KiB/128 KiB reads, and bad-LBA no-copy behavior.
- `check_legacy_descriptors.py` passed all 14 comparisons after the same temporary shim adjustment.

The test binaries and logs are under
`/tmp/elish-nativeufs-test-20261125/`. They validate host logic and synthetic
DMA/MMIO behavior only. They do not prove elish UFS clocks, PHY state, stream
ownership, SMMU context configuration, physical reads, APFS mounting, or Data
persistence. `board.toml` still does not select a UFS component.

## Next step

Restore a responsive fastboot USB bulk transfer, then RAM-boot the hash-verified
reader and export pstore before starting another UniXNU image. Resolve run07's
entry marker first; use run08 only if that capture is absent or inconclusive.
After XNU entry is established, run the existing read-only SMMU probe and
recover its log before changing UFS/DMA initialization. Keep alioth stream IDs
and SM8550 clock constants out of the elish implementation until board-specific
evidence supports them.
