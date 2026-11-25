# elish read-only IORT/SMMUv2 probe build, 2026-11-25

## Purpose and limits

The opt-in `ELISH_LOADER_PROBE_SMMU` path now reads the active ACPI IORT table
before DarwinBoot and logs the named component containing `UFS0` plus its ID
mappings. It validates the table size/checksum, node offsets and lengths, and
mapping-array bounds. The same path performs the existing read-only SMMUv2
scan. It does not write SMMU registers, install an ACPI table, enter XNU, or
write a device partition. The normal elish configuration keeps the diagnostic
driver disabled.

This build does not establish that an IORT table is present at runtime or that
an IORT input ID is the same value as a UFSDxe ARID. Hardware execution and
the resulting UFS-to-SID attribution remain pending.

## Build inputs

| Input | Revision or identity |
| --- | --- |
| Device repository | `4b3da68ea0aca815a80e8d1bad802dfec8d5c905`; pre-existing working-tree changes were retained |
| UniXNU build checkout | `71c5c6569bae6369c0e0f634df8b9bd5afc19139`; existing local changes in the build tree were used |
| Mu-Silicium UEFI | `f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`; working-tree diagnostic changes were used |
| Mu_Basecore | `cfb93724a06ff86ff5ba971413368a7809ee42a5` |
| Silicium-ACPI | `2d465ad77c25de908f0ec7bdfcb57f865b61233c` |
| Payload | `/root/elish-build-20261123/UniXNU/out/elish-resume/elish/ipados-17.7.11-21H461/elish/payload/payload.img`, SHA-256 `79ab59fda90509756a4b703295a5cc19c5b6932c6e66c2d25a5f864e983023ea` |
| Firmware DTB | `/tmp/elish-uefi-probe.dtb`, SHA-256 `0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48` |

## Command and result

```sh
python3 /root/repo/UniXNU-elish/scripts/build-loader-probe.py \
  --smmu-readonly \
  --unixnu /root/elish-build-20261123/UniXNU \
  --uefi /root/repo/uefi-elish \
  --payload /root/elish-build-20261123/UniXNU/out/elish-resume/elish/ipados-17.7.11-21H461/elish/payload/payload.img \
  --dtb /tmp/elish-uefi-probe.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-readonly \
  --feeds /root/elish-build-20261123/UniXNU/out/feeds \
  --clang-bin /usr/lib/llvm-21/bin \
  --jobs 8
```

The DEBUG firmware build and Android v3 image packaging succeeded. The compiled
`ElishLoaderProbeDxe.efi` contains the `ELISH_IORT_PROBE` and
`ELISH_SMMU_PROBE` log strings.

| Output | Size | SHA-256 |
| --- | ---: | --- |
| `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-readonly/SILICIUM_UEFI.fd` | 3,145,728 bytes | `2f40713e964cd2ba54d66759244e4dba7bfe1340746b21676864a9f795b797dc` |
| `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-readonly/boot.img` | 67,751,936 bytes | `ab653c2ebee272a7a56fc0c60eba6490b14e77132dd3b3eccc3a91d7aaf9fe0b` |
| `/root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/AARCH64/ElishLoaderProbeDxe.efi` | build output | `cb6fe1f7d677cf7cacf215aa2df1ad0fa4b0960b3f94cae849b4e3d4bc4982e5` |

## Device status

Run19 has not been sent to the device. The most recent USB snapshot at 11:40
CST did not enumerate the tablet in USB, fastboot or ADB, and the camera frame
was too dark to identify the display. No new device protocol command, boot,
reboot, partition write or power-state request was made for this build.

The latest direct XNU stage remains the run09 close photo showing the pre-MMU
checkpoint. run06/run11 ramoops logs still confirm FAT payload discovery,
DarwinBoot, the 256 MiB window and successful `ExitBootServices()`. MMU
completion, UFS DMA ownership, device APFS root mounting and iPadOS userspace
remain unconfirmed. When the device is available, test the run18 post-VM marker
and pmsg readback first; use run19 as a separate read-only IORT/SMMU capture.
