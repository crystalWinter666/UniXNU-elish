# elish handoff screen and XNU entry retest, 2026-11-25

Device: Xiaomi Pad 5 Pro Wi-Fi, `M2105K81AC` / `elish`, iPadOS 17.7.11
(`21H461`). All diagnostic images used RAM-only `fastboot boot`; no device
partition was written.

## User-provided screen

Photo: `/root/repo/FCAB2A776FDB44AE9E8A5920E572CC64.png`, SHA-256
`c1eb7499267cf31e0d69ce29396d2d5dac20759be94b6f0ad0118f1a35a82fc9`.
The image shows DarwinBoot's `memory usable 0x10000000`, 1600x2560 video
geometry, entry words `140006F5 D2B60003 B9400064 52884885`, `CurrentEL 1`,
`SCTLR_EL1 0xC01800`, `TCR_EL1 0x280803518`, and the green handoff band. It
does not show a blue or magenta entry band. The image's boot-image identity and
hash are unknown, so it is not attributed to run05.

DarwinBoot source at the pinned component revision draws the green band in
`DarwinEnter` after successful `ExitBootServices()` and before cache/MMU
handoff work and the kernel branch. The photo therefore confirms the EBS
handoff point, but not that XNU's first instruction executed. No `Darwin Kernel
Version` or later XNU checkpoint is visible.

## Pstore recovery and entry probe

The host observed fastboot USB `18d1:d00d`, serial `4bed07e4`; `fastboot getvar
product` returned `elish`. Reader image
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`
has SHA-256
`57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`. Its
RAM-only boot returned `OKAY`, ADB identified `lineage_elish` /
`M2105K81AC` / `elish`, and `adb root` succeeded. `/sys/fs/pstore` was empty
(`total 0`); run05's `before-mmu` marker remains unknown. A cleared pstore is not
evidence that the marker did not execute.

The existing six-second entry-reset image
`/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-entry-reset.img`
has SHA-256
`441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121`.
`fastboot getvar product` again returned `elish`; `fastboot boot` reported
`Sending` and `Booting` `OKAY` (1.897 s). Android ADB then returned as
`lineage_elish`; after `adb root`, `/sys/fs/pstore` was again empty. The
expected blue/magenta bars were not captured by the host, and the user's
visual observation of this run is pending. The test does not yet confirm XNU
entry or MMU enable.

Commands used:

```sh
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-entry-reset.img
adb -s 4bed07e4 root
adb -s 4bed07e4 shell ls -l /sys/fs/pstore
```

Last confirmed stage for the photo is successful EBS and DarwinBoot's
pre-branch handoff. Last host-observed state after the entry-probe run is root
ADB in `lineage_elish`. No `fastboot continue` or partition-write command was
run.

## Next action

Record whether the entry-probe run visibly painted blue at XNU entry and
magenta before MMU enable. If neither was observed, use a checkpoint with a
direct ramoops marker and collect it before another firmware boot. Keep the
existing `loadable=false` qualification and do not treat early entry as proof
of APFS root or userspace.

## Host check after the user's fastboot report

The user reported that the device had entered fastboot. At the next host check,
`fastboot devices -l` returned no devices, while `adb devices -l` showed serial
`4bed07e4` as `lineage_elish` / `M2105K81AC` / `elish`. `lsusb -d 18d1:4e11`
showed the ADB USB interface. Root ADB reported `sys.boot.reason=bootloader`,
and `/sys/fs/pstore` contained no files. This confirms an Android boot via the
bootloader, but does not confirm that USB was in fastboot mode at the time of
the host check. No reboot or fastboot command was sent during this check.

The user-provided photo still confirms DarwinBoot's successful
`ExitBootServices()` handoff only. It shows neither the blue XNU-entry bar nor
the magenta pre-MMU bar. Do not start another firmware image until the user's
observation of the six-second entry-probe run is recorded; when testing resumes,
require live fastboot enumeration and `product=elish` first.

## Latest screen and reader attempt, 2026-11-25

The user supplied `/root/repo/D5E326E8269942AF9C00495CFD941302.png`, SHA-256
`2d94beff27bbe43fd9f7d5832303debd0bb3b9946c2935d8e791921a6dde4a2c`. The
screen explicitly says, “Exiting boot services. A green bar across the top
marks the loader handoff before the kernel branch.” This confirms EBS and the
pre-branch handoff, not XNU instruction execution.

After the user reported fastboot, the host enumerated serial `4bed07e4` as
`18d1:d00d`; `fastboot getvar product` returned `elish`. The hash-verified
reader at
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`
(`57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`) was
started with RAM-only `fastboot boot`. The command stopped at
`Sending 'boot.img' (53792 KB)` and blocked in `usbfs_start_wait_urb`; after
about 90 seconds the host process was interrupted. There was no `Booting
OKAY`, no ADB recovery and no pstore read. The last host snapshot still showed
fastboot. No device partition was written.
