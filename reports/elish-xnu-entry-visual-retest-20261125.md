# elish visual XNU entry retest, 2026-11-25

This report records two RAM-only boots of the existing six-second XNU entry
diagnostic and a low-resolution overhead-camera recording. The later run09
section records one backed-up `boot_b` flash and restoration. `fastboot
continue` was not used.

## Image and expected markers

- Image: `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-entry-reset.img`
- SHA-256: `441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121`
- Android v3 image size: 67,727,360 bytes.
- The temporary kernel contains the blue XNU-entry and magenta pre-MMU visual
  markers, followed by a terminal reset six seconds after MMU enable. The
  build and probe layout are documented in
  `reports/elish-xnu-entry-probe-20261124.md`.

## Device runs

For both runs, root ADB rebooted the tablet to bootloader, fastboot enumerated
serial `4bed07e4`, and `fastboot getvar product` returned `elish`. Fastboot
37.0.3 accepted the image with `Sending OKAY` and `Booting OKAY`:

- First run: `fastboot boot` completed in 1.835 seconds. About 12 seconds
  later, the host again saw ADB as `lineage_elish`. After `adb root`,
  `/sys/fs/pstore` was empty. The kernel log registered ramoops at
  `0xb0000000+0x400000` and repeated `Invalid compression size for deflate: 0`.
- Second run: `fastboot boot` completed in 1.765 seconds while V4L2 recorded
  16.005 seconds at 640x480, 5 fps. The clip is
  `/tmp/elish-xnu-entry-visual-20261125.mkv`, SHA-256
  `0566bbe3a2c5e62c859050504984358946bb2c75b41924e9fc618a8a7a1cfd2b`.
  The overhead view shows a dark screen with `FASTBOOT` text; it does not
  resolve the expected blue or magenta markers. ADB returned as
  `lineage_elish`; root pstore was again empty with the same compression
  warning.

For these first two visual retests, fastboot acceptance, USB return timing, and
the overhead-camera view did not confirm the kernel-entry or MMU marker. The
run09 close photo and its more limited stage conclusions are recorded below.

## Commands and next measurement

The image was booted with:

```sh
adb -s 4bed07e4 reboot bootloader
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 getvar product
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-entry-reset.img
adb -s 4bed07e4 root
adb -s 4bed07e4 shell 'ls -la /sys/fs/pstore; dmesg | grep -iE "pstore|ramoops|compression size"'
```

The camera capture used:

```sh
ffmpeg -f v4l2 -framerate 5 -input_format mjpeg -video_size 640x480 \
  -i /dev/video0 -t 16 -c:v copy /tmp/elish-xnu-entry-visual-20261125.mkv
```

The stage-1 blue and stage-2 magenta bars are now directly visible. The stage-3
post-MMU reset remains unconfirmed by a persistent checkpoint. Investigate why
the pmsg checkpoint is absent before another XNU run. UFS/SMMU, APFS mounting,
and userspace remain unverified.

## Run09 persistent boot test

The user requested one more test after observing all three bars in person. The
device was in fastboot, serial `4bed07e4`, product `elish`, active slot `b`.
Before writing, the source image and full `boot_b` backup hashes were checked:

- run09 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run09-xnu-pmsg-entry/boot.img`, 67,751,936 bytes, SHA-256 `87ad74c775aa67d968827ce17bf56659289ebc8e2519f2e9ea2e541c314a704c`.
- Backup `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/boot_b.img`, 201,326,592 bytes, SHA-256 `fce50614e0f90a4fd42b1eef26b913712883e4133723b802b4ac65a8dd1dd890`.

Fastboot 37.0.3 wrote run09 to `boot_b` successfully (`Sending` and `Writing
OKAY`), then `fastboot reboot` started the test. The user photo
`reports/elish-run09-screen-20261125.jpg` (SHA-256
`ebec55fc542f9cfbc754ae8c36895c69a45e453679206949dec9695d0464f1bd`) shows
the green DarwinBoot handoff band, the blue XNU-entry band and the magenta
pre-MMU band together. These are the instrumented bands defined by
`patch-xnu-entry-probe.py`; the photo directly confirms XNU execution through
the pre-MMU checkpoint. The lower screen still contains loader text, including
`Capsule Interface Locked!!` and `App Log Flush : 0 ms`. Fastboot re-enumerated
after the reboot, but the screen has no post-MMU marker, so this alone does not
prove stage 3.

The run09 image was instrumented to write `ELISH_XNU_ENTRY_PMSG` at the
configured pmsg region `0xb0200000+0x200000`, but the marker was not recovered;
the write itself is therefore unconfirmed. It is an entry-stage marker, not a
post-MMU checkpoint. After the test, the original `boot_b` backup was written
back successfully. The hash-verified LineageOS reader
(`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`,
55,083,008 bytes, SHA-256
`57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`) was
then booted RAM-only. Root ADB returned as `lineage_elish` on
`M2105K81AC`/`elish`; `/sys/fs/pstore` was empty. The live ramoops parameters
were address `0xb0000000`, size 4 MiB, console 2 MiB, pmsg 2 MiB and record
size 0. The marker was not recovered, so it provides no independent
confirmation. The device is currently in the reader's root ADB environment
with the original `boot_b` restored. No `fastboot continue` was run.

Commands:

```sh
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 flash boot_b /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run09-xnu-pmsg-entry/boot.img
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 reboot
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 flash boot_b /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/boot_b.img
/tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img
adb -s 4bed07e4 root
adb -s 4bed07e4 shell ls -la /sys/fs/pstore
adb -s 4bed07e4 shell 'cat /sys/module/ramoops/parameters/mem_address; cat /sys/module/ramoops/parameters/mem_size; cat /sys/module/ramoops/parameters/console_size; cat /sys/module/ramoops/parameters/record_size; cat /sys/module/ramoops/parameters/pmsg_size'
```

## Stage-3 timing follow-up

The user-provided close photo at
`/root/repo/A3AABAD37D0E0E787C85F39100B9ACCB.png` has SHA-256
`b03cc52878f046d7574ab1a776a3f83662f10251875438dcfbd2181ac99ee7b8` and
shows the same green, blue and magenta bands. The run09 host timeline records
USB disconnect at 15:47:32 and fastboot re-enumeration at 15:47:50; the photo
was taken at 15:47:51.

UniXNU's `docs/guide/debugging.md` defines stage 3 as a timer that resets six
seconds after MMU enable and says return after that delay confirms MMU enable.
Under that prescribed diagnostic rule, record MMU as enabled from the run09
timing. The photo itself directly confirms only the pre-MMU visual checkpoint;
the MMU result is from the stage-3 timed-reset evidence, not a pstore or serial
marker. `_arm_vm_init()` return remains a separate question: run18's pmsg
readback reported `magic=0x00004000`, `complement=0`, `valid=0`.
