# run14 fastboot transfer retry

Device: Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC` / `elish`), target iPadOS 17.7.11 (21H461).

## Image

- RAM-only diagnostic image: `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run14-vm-return-30-image/boot.img`
- Size: 67,751,936 bytes
- SHA-256: `499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f`
- The kernel wrapper would call `_arm_vm_init`, wait 30 seconds after it returned, then request PSCI `SYSTEM_RESET`. The image was not fully transferred, so the wrapper and reset path did not run.

## Host and device result

- At 06:42 CST, USB enumerated as fastboot `18d1:d00d`; host serial was `4bed07e4`. `/usr/bin/fastboot` 34.0.5 returned `product: elish`. `version`, `version-bootloader`, `is-userspace`, `max-download-size`, `secure` and `unlocked` were not implemented by this bootloader (`GetVar Variable Not found`).
- `fastboot -s 4bed07e4 boot .../run14-vm-return-30-image/boot.img` started at 06:43:25.700 CST and printed `Sending 'boot.img' (66164 KB)`. At 06:43:59 the host process was blocked in `usbfs_start_wait_urb`; it had not printed `Sending OKAY` or `Booting OKAY`. Ctrl-C ended the host command with status 130.
- At 06:44:51 CST, `lsusb` still showed `18d1:d00d`; no fastboot process or ADB device was present. No further fastboot protocol request was issued after the transfer failure.
- A 48-second, 640x480 overhead-camera recording is `/tmp/elish-run14-overhead.mkv`, SHA-256 `e7c59fbda0919ed82d642b62725d8f6097d12e19db5ebd526918beb45e4ff6d5`. The last frame is too dark to identify the display bars or read text, so it provides no boot-stage evidence.

## Evidence boundary and next step

The failed transfer did not run this diagnostic image and adds no XNU checkpoint. The latest direct XNU evidence remains the run09 screen photo showing execution through the pre-MMU marker. `_arm_vm_init` completion, UFS/SMMU handoff, APFS root mounting and iPadOS userspace remain unconfirmed.

This retry used Debian `/usr/bin/fastboot` 34.0.5. The fastboot 37.0.3 binary used by prior successful RAM-only runs is available at `/tmp/elish-platform-tools/platform-tools/fastboot`. Resume device tests only after confirming reliable transport with that binary; do not repeat large transfers while it stalls. No partition was written, `fastboot continue` was not used, and no poweroff request was sent.

## Offline XNU and port reference check

The pinned build tree's `osfmk/arm/arm_init.c` calls `arm_vm_init()` at line 535, then runs low-global patching, debug inheritance, `serial_init()` and kprintf initialization before the later `_PE_init_printf` site used by run12's visible marker. An absent run12 marker therefore leaves both the body of `arm_vm_init()` and the intervening post-return setup unresolved.

In `osfmk/arm64/arm_vm_init.c`, the function reads the window fields from `boot_args`, builds the bootstrap maps, switches TTBR and flushes the TLB, then continues through AuxKC setup, `pmap_bootstrap()` and L3 mappings before return. A return from this function is a meaningful checkpoint for the 256 MiB window's XNU VM setup; it is not evidence that UFS, APFS or userspace work.

The UniXNU UKE device record at core revision `5ed46420025be331b51d98207ea6aa679faac9c7` documents the same after-`_arm_vm_init` reset candidate as host-built but not device-tested. Its later framebuffer text-ring attempt produced no new stage marker; the record identifies the difference between the physical `PE_state.video` pointer and the mapped `video_console` framebuffer. This supports retaining the direct timer/PSCI return probe instead of relying on that unverified text-ring technique.

## Subsequent fastboot control check

At 06:54 CST, `/tmp/elish-platform-tools/platform-tools/fastboot` 37.0.3 ran `fastboot -s 4bed07e4 getvar product` with an 8-second timeout. It produced no output and exited through `timeout` with status 124. At 06:55:27 CST, USB still enumerated as `18d1:d00d`, but no fastboot process or ADB device was present. No image transfer, USB reset, or device reboot was attempted in this check. The control protocol is not currently responsive even though USB enumeration remains; the cause is not confirmed.
