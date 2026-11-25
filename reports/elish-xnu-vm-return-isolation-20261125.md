# elish XNU VM-init return isolation

Device: Xiaomi Pad 5 Pro Wi-Fi (M2105K81AC / elish), target iPadOS 17.7.11 (21H461).

## Probe

The diagnostic patch script adds an after-vm-init mode. It replaces the arm_vm_init call in arm_init with a wrapper that forwards the original arguments, calls the original function, then waits for a configured interval before requesting PSCI SYSTEM_RESET. This distinguishes a return from arm_vm_init from a later console setup failure using host USB timing.

The run12 entry-bars kernel contains only the blue XNU-entry and magenta pre-MMU probes. Its MMU-after instruction remains the original mov x0, #0x300000, and the stage-3 reset cave is empty.

The 15-second wrapper kernel SHA-256 is 03577d502adc6b6461ac0464f6a9b04d9471cc8b1f608e1c35ffa34c7d774d8b. Its FAT payload SHA-256 is c0770f7a749ced52e512f724576916b709e413a1e0ed1629cc7c7dca6fabaf37. The Android v3 image is /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run13-vm-return-image/boot.img, SHA-256 05b8620d4ccc74e95bccb3fef0edc71946d205036e0fd71da005361a2ff459b9.

The 30-second wrapper kernel SHA-256 is e4407c70fb55f359bd8a3c4c1a64e0f946f63b1836ade6a26af3ed834402f08b. Its FAT payload SHA-256 is b09d0760e3989b93c584b70caa0f5fcefac5b248ed6aa80793ca469f5289fd29. The Android v3 image is /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run14-vm-return-30-image/boot.img, SHA-256 499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f.

## Device result

Fastboot 37.0.3 confirmed product=elish before both probes. Run13 was accepted with Sending OKAY and Booting OKAY in 1.838 seconds. The device re-enumerated as fastboot about 13.9 seconds after the command returned, approximately 15.7 seconds from command start. This is close to the configured 15-second hold and remains timing evidence only; it does not conclusively prove arm_vm_init returned.

Run14's 30-second image built successfully, but its transfer stalled at Sending boot.img in usbfs_start_wait_urb. The command never reported Booting OKAY. After interrupting the host process, lsusb still showed the tablet as fastboot USB 18d1:d00d. A read-only fastboot getvar request then stalled in the same URB and was interrupted. No device partition was written, fastboot continue was not used, and no poweroff command was issued.

The device is currently powered on and enumerated as fastboot USB; the last successful product query before the stalled transfer returned elish. ADB is not enumerated. Stop further fastboot protocol requests until USB transport responds reliably. XNU execution to the pre-MMU marker is confirmed. arm_vm_init completion, UFS/SMMU, APFS root mounting, and iPadOS userspace remain unconfirmed.
