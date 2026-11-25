# elish run34 Android-kernel diagnostic attempt, 2026-11-26

## Result

The RAM-only fastboot request for the instrumented Android kernel was accepted,
but the tablet did not re-enumerate as fastboot or ADB afterward. A subsequent
RAM-only boot of the verified reader waited for serial `4bed07e4` and timed out;
no ramoops or kernel log was recovered. At the last host check, `fastboot
devices`, `adb devices`, and `lsusb` showed no tablet. No SMMU or UFS diagnostic
values were obtained. The last confirmed XNU checkpoint remains run28:
Apple `launchd` reached `main()` as PID 1 from the Restore APFS root via
RamBlock. The full System root, Setup, and persistent Data remain unverified.

## Safety and evidence limits

This test used `fastboot boot`, not `flash`, and issued no explicit host-side
UFS command, manually programmed DMA, SMMU register write, or persistent
partition write. However, a post-test configuration review found that the
Android kernel configuration has `CONFIG_ARM_SMMU=y`, `CONFIG_SCSI_UFSHCD=y`,
`CONFIG_SCSI_UFS_QCOM=y`, and `CONFIG_BLK_DEV_SD=y`. If the bootloader supplied
the elish device tree and those platform devices probed, built-in initialization
could have reset the apps SMMU and the UFS host driver could have issued link,
query, and DMA operations before the diagnostic PID 1. Whether either driver
probed is unknown because no kernel log was recovered. Treat any transient
hardware/SMMU state change as possible. This run provides neither a read-only
SMMU snapshot nor proof that no UFS command or DMA occurred.

The diagnostic SMMU hook was placed in the Android driver's context-handoff
path. Source inspection confirms that this path later calls
`arm_smmu_device_reset()`. Thus even a recovered snapshot would describe state
before that driver's reset step, not a non-invasive observation of state
preserved for UniXNU. The UFS hook logged DMA masks and descriptor addresses
after the controller's DMA mask setup; it did not gate UFS probing or commands.
The diagnostic design is not suitable for another device run.

The host kernel image contains the experimental instrumentation, but there is
no runtime proof that the initramfs ran. Preserve the run28 boot image as the
known-good regression artifact. Before any further hardware run, the diagnostic
must be redesigned so it cannot bind/reset the SMMU or UFS host and cannot issue
UFS commands or DMA; verify the relevant platform device tree and boot path
before booting it.

## Inputs and artifacts

| Artifact | Identity |
| --- | --- |
| Device | Serial `4bed07e4`, model `M2105K81AC`, product/device `elish` immediately before the test |
| Android kernel source | `6b34d24765327d746074c8e53b5ad0d24a51acc7` |
| Diagnostic boot image | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run34-android-smmu-diagnostic/boot.img`; 53,567,488 bytes; SHA-256 `e6624ee4d4e47f9c6d6c29ad563ae4da1ce207a4790509c6be188a12c8e8488d` |
| Kernel Image | Same directory, 53,555,224 bytes; SHA-256 `4e430e4b80ffdf2be53cf70f0383eb1deaf0304dc87ce3f8a1a036779e10a2a8` |
| Initramfs | Same directory, SHA-256 `b61da5f923d0a0f6f426b2956bddabfc0ddd63dfac3d1d70e745e2c5e9f7c5df` |
| Instrumentation patch | Same directory, SHA-256 `41bac30b58a67a8778c0cd1ff42448259e4e45815bbc253a0565d7a7f78f677c` |
| Build config | Same directory, SHA-256 `ba6924984313207a4a7c59644bbbafc066af50c34bbe37637acf19b127359d74` |
| Verified reader image | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`, SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4` |
| Host transfer logs | `/tmp/elish-run34-fastboot-preflight-20261126.log`, `/tmp/elish-run34-fastboot-boot-20261126.log`, `/tmp/elish-run34-reader-preflight-20261126.log` |

The kernel build command was:

```sh
PATH=/tmp/usr/bin:$PATH make -C /tmp/elish-kernel-diag-20261126 \
  O=/tmp/elish-kernel-diag-out-20261126 ARCH=arm64 LLVM=1 -j16 Image
```

`unpack_bootimg.py` successfully extracted the packaged kernel and ramdisk and
both compared byte-for-byte with the inputs. The boot command was:

```sh
fastboot -s 4bed07e4 boot \
  /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run34-android-smmu-diagnostic/boot.img
```

Fastboot reported `Sending ... OKAY` and `Booting OKAY` (1.539 seconds total).
The reader preflight then remained at `< waiting for 4bed07e4>` until its
240-second timeout. No device image, kernel log, pstore entry, or UFS/SMMU value
was collected.
The host kernel journal recorded the fastboot interface `18d1:d00d` appearing
at 01:24:31 and disconnecting at 01:24:35 local time on USB port `1-3`.
The fastboot device list immediately after `Booting OKAY` still showed that
interface before the disconnect. It was not evidence of a kernel-triggered reset.

## Reader attempt after manual fastboot entry

The user brought the tablet back to fastboot. USB enumerated `18d1:d00d`
on port `1-3` with serial `4bed07e4`. `fastboot getvar product` returned
`elish` and `getvar is-userspace` returned `no`. The reader SHA-256 matched
the verified value above. The bounded RAM upload below stalled at `Sending`
and timed out after 48 seconds. The last observed checkpoint was the upload.

```sh
timeout 48s fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img
```

The same device remained enumerated. Host USB reauthorization and
`USBDEVFS_RESET` completed, but `getvar product` timed out with both fastboot
37.0.3 and Debian 34.0.5. A direct fastboot `getvar:product` took all 14 bytes
on endpoint `0x01` (bulk OUT). Endpoint `0x81` (bulk IN) returned `ETIMEDOUT`.
`fastboot reboot bootloader` also timed out. The upload log is preserved as
`reader-after-manual-fastboot.log` beside the run34 image.

## Recovery after fresh fastboot entry

After the user re-entered fastboot, `getvar product` on serial `4bed07e4`
returned `elish`. `fastboot -s 4bed07e4 reboot recovery` returned `OKAY`.
`adb devices -l` then listed the tablet in `recovery` as `M2105K81AC`,
`elish`. Root ADB returned `total 0` from `ls -l /sys/fs/pstore`. There was
no run34 record to export on that boot. The next device run used the passive
[run35 UEFI probe](elish-run35-ufs-queues-20261126.md).
