# elish fastboot read-only status, 2026-11-25

## Host and device snapshot

At 07:06:55 CST, `lsusb` showed Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC`,
serial `4bed07e4`) as fastboot USB `18d1:d00d`. ADB listed no devices. The
available fastboot binary was `/tmp/elish-platform-tools/platform-tools/fastboot`
version `37.0.3-16887417`.

`fastboot devices -l` listed `4bed07e4` as `fastboot usb:1-2`. The following
read-only `fastboot -s 4bed07e4 getvar product`, bounded by an 8-second timeout,
produced no response and exited with timeout status 124. The USB device remained
enumerated; this does not establish that its fastboot command endpoint is
responsive. No further device protocol command was sent.

The top-mounted camera captured `/tmp/elish-current-state.png` at 07:07:49 CST,
640x480, 56,882 bytes, SHA-256
`95bb6002b989b62437beb87bce02d492d8a6b6ea9114b850de875e7970416dba`. The image
is nearly black and cannot establish the screen state or boot stage.

## Image and boot-stage result

No image was transferred in this check. The available run14 candidate is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run14-vm-return-30-image/boot.img`,
67,751,936 bytes, SHA-256
`499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f`. It remains
untested because fastboot control requests are unresponsive.

There is no new boot-stage evidence. The latest direct evidence remains the
run09 user photo showing XNU execution through the pre-MMU marker. Completion of
`_arm_vm_init`, UFS/SMMU ownership, device-side APFS mounting, and iPadOS
userspace remain unconfirmed.

No partition was written, and this check issued no reboot, reset, poweroff, or
`fastboot continue` request. Resume device tests after fastboot control responds
again; continue offline analysis meanwhile.

## Host-side validation

The package preflight completed for `elish_defconfig` and
`ipados-17.7.11-21H461`, resolving 63 patches. The static board preflight passed
and reported runtime memory handoff as unverified. The porting-record checker
validated all 14 structured areas, and `git diff --check` passed. The run14 file
was rechecked at 67,751,936 bytes with the SHA-256 listed above. These checks do
not execute the image on the device.
