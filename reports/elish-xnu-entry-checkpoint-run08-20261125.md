# elish corrected XNU entry checkpoint, run08

Run08 rebuilds the terminal entry checkpoint with corrected ramoops ring wrap handling. The image was started with RAM-only fastboot on elish. The user photo shows DarwinBoot's green ExitBootServices handoff screen, but XNU entry remains unconfirmed because no checkpoint marker was recovered from pstore.

## Inputs and output

- Base run07 FAT payload: /tmp/elish-run07-xnu-entry/payload.img, SHA-256 7e90eeaff5d8fb04ef5750f8139a6365d9a0624f2ddb568a331f81d3f54e7416.
- Existing run07 kernel extracted from FAT: SHA-256 ff3eceba83f86ec5b9b21830371bdacd644bd48c868f965743885771f22292b0.
- Corrected entry-checkpoint kernel: 276 bytes of checkpoint code; SHA-256 0ad2ca17724f66adf25afd473008e9c47842c2b00d55d8b1ec79ba6c40b9086f.
- Corrected payload: /tmp/elish-run08-xnu-entry/payload.img, 67,108,864 bytes, SHA-256 ab5b2b00fc2fa592bf71434e01c93fd793ff48d6c13aa763a45ecdaae297ac1d.
- FAT read-back of EFI/UniXNU/Payloads/kernel.macho matched the corrected kernel hash exactly.
- DTB input SHA-256 0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48.
- UEFI FD: /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run08-xnu-entry-checkpoint-corrected/SILICIUM_UEFI.fd, 3,145,728 bytes, SHA-256 2f23213f3e5f3ebc3706c58de58339558a74b61a48a84e8d7f047dddb62755d2.
- Android v3 image: /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run08-xnu-entry-checkpoint-corrected/boot.img, 67,751,936 bytes, SHA-256 9cd0577439a5f143ff11d81cdaa6aece79da01a25cfd5cfdf67ddea0a7a5075a.
- Build manifest is in run08-xnu-entry-checkpoint-corrected/probe.json. The boot-image report records header version 3, a 67,108,864-byte ramdisk, and hardware_tested=false.

## Build and host verification

The corrected kernel was written to a copy of the run07 FAT payload with mcopy, then extracted again and compared by SHA-256. The original run07 payload and image were not modified.

Build command:

    python3 scripts/build-loader-probe.py --payload /tmp/elish-run08-xnu-entry/payload.img --dtb /tmp/elish-uefi-probe.dtb --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run08-xnu-entry-checkpoint-corrected --clang-bin /usr/lib/llvm-21/bin --feeds /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run07-xnu-entry-checkpoint/build/feeds --jobs 8 --run-kernel

The command rebuilt the DEBUG firmware and Android v3 image. The packaging path verified the image by unpacking it and comparing the ramdisk with the corrected payload. The following checks passed: scripts/check-elish.py --static-only; the UniXNU boot-image unit test; py_compile for the checkpoint and build scripts; git diff --check; and the 14-area porting-record check.

Source baselines are device repository 4b3da68ea0aca815a80e8d1bad802dfec8d5c905, UniXNU 71c5c6569bae6369c0e0f634df8b9bd5afc19139, and firmware f03839d31a3f07e9e4b032fcf850ac0dfb9961a0, with pre-existing working-tree changes retained.

## Device status and next measurement

The host used fastboot 37.0.3 (`/tmp/elish-platform-tools/platform-tools/fastboot`). Serial `4bed07e4` enumerated as fastboot USB `18d1:d00d`; `getvar product` returned `elish`. RAM-only `fastboot boot` accepted run08: `Sending OKAY`, `Booting OKAY`, total 1.760 seconds. The image was 67,751,936 bytes with SHA-256 `9cd0577439a5f143ff11d81cdaa6aece79da01a25cfd5cfdf67ddea0a7a5075a`. No partition was written.

About 10 seconds after the boot command, ADB, fastboot and USB snapshots did not show the tablet. The user-provided screen photo `reports/elish-run08-screen-20261125.jpg` (SHA-256 `1fbd8b10ec5bd3c17d7ebc960d909ee9232a8929d549c740e7179de224a6f9a4`) shows the green handoff band and the on-screen text describing the ExitBootServices handoff before the kernel branch. A V4L2 overview taken about 30 seconds after boot showed the same general screen state (SHA-256 `9abe128646560d18ba29137d9bcfc434f887774c7aef47e2901b32ab6bda192a`). These observations confirm the handoff screen, not XNU's first instruction.

After the user reported fastboot again, serial `4bed07e4` re-enumerated and `getvar product` again returned `elish`. The verified LineageOS reader (55,083,008 bytes, SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`) booted RAM-only with `Sending OKAY` / `Booting OKAY` using fastboot 37.0.3. ADB returned as `lineage_elish`; `adb root` succeeded with UID 0. `/sys/fs/pstore` was mounted and `ls -la` showed only `.` and `..` (`total 0`). The kernel log contained:

```text
[    0.000000] ramoops: msm_reserve_ramoops_memory addr=b0000000,size=400000
[    0.318355] pstore: Registered ramoops as persistent store backend
[    0.318360] ramoops: attached 0x400000@0xb0000000, ecc: 0/0
[    1.990951] pstore: Invalid compression size for deflate: 0
```

No run08 marker was recovered. The warning and empty directory do not establish whether the marker was never written or could not be exposed as a pstore file, so XNU entry remains unconfirmed.

The last direct firmware stage is the green ExitBootServices handoff. XNU entry, MMU enable, UFS/SMMU, APFS mounting and userspace remain unconfirmed. Do not start the SMMU probe until the ramoops readout issue and entry checkpoint result are understood. If screen text needs interpretation, use a user-provided close photo; the overhead V4L2 camera is only suitable for general state.

Commands used:

```sh
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 getvar product
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run08-xnu-entry-checkpoint-corrected/boot.img
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img
adb -s 4bed07e4 root
adb -s 4bed07e4 shell ls -la /sys/fs/pstore
adb -s 4bed07e4 shell dmesg | rg -i 'ramoops|persistent_ram|compression size'
```

