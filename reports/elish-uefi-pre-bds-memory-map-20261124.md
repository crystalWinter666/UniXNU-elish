# elish pre-BDS memory map and firmware menu

The device was a Xiaomi Pad 5 Pro Wi-Fi (`elish`, `M2105K81AC`). UniXNU was based on `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, the device checkout on `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`, Mu-Silicium on `66421cb7bb5694843aec8de1ca3d1de4c20cca38` and Mu_Basecore on `cfb93724a06ff86ff5ba971413368a7809ee42a5`. The images include local board and firmware changes. The build commands and image hashes are in [the HII and memory-probe report](elish-uefi-info-hob-20261124.md).

## RAM Partition and UEFI descriptors

At 22:06 CST, LineageOS accepted `adb root` and the target returned `product: elish` after rebooting to fastboot. Fastboot transferred `memory-map-ramoops/boot.img` and accepted its boot command. The probe restarted the tablet into fastboot after capture. `fastboot continue` returned to LineageOS. Root ADB exported the 21,189-byte pstore console with SHA-256 `d2d33403040d1439524f105824eb3e9908fb3fde99c8fb0f6c7a84ef68600b53`. The repository copy removes carriage returns and has SHA-256 `b5691a6e398e69ade93459b6a017c8cb18252a0407d4893d1d41d6e98f105814`. [Console capture](elish-uefi-pre-bds-memory-map-20261124.txt#L258)

```sh
adb reboot bootloader
fastboot getvar product
fastboot boot boot/memory-map-ramoops/boot.img
# After the probe returns to fastboot:
fastboot continue
adb wait-for-device
adb root
adb wait-for-device
adb exec-out cat /sys/fs/pstore/console-ramoops-0 > console-ramoops.raw
```

| Qualcomm RAM Partition base | Length | End, exclusive |
| --- | --- | --- |
| `0x80000000` | `0x3bb00000` | `0xbbb00000` |
| `0xc0000000` | `0x1c0000000` | `0x280000000` |

Both banks exactly match the live Linux FDT. `GetMemoryMap()` returned success with descriptor size `0x30` and format version 1. The probe recorded 47 non-overlapping DRAM descriptors from that call. The second bank appeared as one conventional-memory descriptor. The `0xa0000000–0xafffffff` firmware reservation and `0xb0000000–0xb03fffff` ramoops region both appeared as reserved memory. [Linux measurements](elish-live-hardware-20261123.md#live-fdt-and-memory)

The conventional second bank can accommodate a 256 MiB window at this stage. The actual window, initrd placement and final map key still need to be captured during DarwinBoot. In the first bank, the capture classifies `0xb0400000–0xb13fffff` as conventional even though Linux marks it `disp_rdump_region` with `no-map`. Most of Linux's `hyp_region` is also conventional in the snapshot. These discrepancies require ownership evidence before treating those first-bank pages as usable in XNU.

## HII and UniXNU menu

At 22:07 CST, fastboot accepted `hii-before-console/boot.img`, SHA-256 `ee86b326a8cc48ba11a3124a6342caa432c3cea124404a544a99525fbec51e0b`. The user saw the UniXNU boot menu on the tablet. It re-entered fastboot 37 seconds after the boot command completed. Following `fastboot continue`, USB enumerated as `05c6:f000` mass storage and ADB did not enumerate. No pstore export was available for that run. The menu confirms BDS reached the UniXNU application. The selected entry, FAT payload discovery, DarwinBoot handoff and XNU execution are still unverified. Both tests used `fastboot boot` with zero partition writes.
