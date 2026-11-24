# elish Recovery Linux memory map, 2026-11-24

## Capture

The device was attached over Recovery ADB as `M2105K81AC` / `elish`, serial
`ELISH_SERIAL`. The shell ran as root on Linux
`4.19.325-cip136-st20-perf-g6b34d2476532`.

Read-only commands:

```sh
adb -s ELISH_SERIAL shell 'id; uname -a; cat /proc/cmdline; cat /proc/meminfo; cat /proc/iomem'
adb -s ELISH_SERIAL pull /sys/firmware/fdt reports/elish-recovery-fdt-20261124.dtb
```

The FDT `/memory` node reports two physical DRAM banks:

| Base | Size | End, exclusive |
| --- | --- | --- |
| `0x80000000` | `0x3bb00000` | `0xbbb00000` |
| `0xc0000000` | `0x1c0000000` | `0x280000000` |

Their combined size is `0x1fbb00000` bytes, with a `0x4500000`-byte gap between
the banks. `/proc/iomem` reports Linux `System RAM` resources and nested
reservations, including kernel code and data. `/proc/meminfo` reports
`MemTotal: 7772916 kB` and `CmaTotal: 356352 kB` for this Recovery boot.

## Scope

This is a live Linux Recovery memory/resource map. The FDT establishes the two
DRAM banks, while `/proc/iomem` records ranges exposed and reserved by this
Linux kernel. These interfaces do not provide UEFI `GetMemoryMap()` descriptor
types, Qualcomm RAM Partition protocol entries, or the final map passed to
`ExitBootServices()`. Keep this evidence separate from the UEFI HOB observations
and the unresolved UEFI-to-XNU memory handoff.

## Captured files

The repo includes the text captures, with the serial and CPU ID removed from the command line. The raw FDT contains device identifiers and is kept with the local captures.

| File | Size | SHA-256 |
| --- | ---: | --- |
| `elish-recovery-fdt-20261124.dtb` | 845860 | `61ebe37bf537d8302abd5b82d22615ea0e5c92e6766eb1c75c034f8fd5d774f3` |
| `elish-recovery-proc-iomem-20261124.txt` | 3607 | `068238fd499f1bb7aec1717d577480e7477426f7fd00493132ada97d0782561f` |
| `elish-recovery-proc-meminfo-20261124.txt` | 1093 | `f486fb5806798fbf5b80cddcb5dd76293859218699c1a2c01aa1db3d0e79e163` |
| `elish-recovery-cmdline-20261124.txt` | 1189 | `417925dd52e5a2c11e5f2b0de501b9b44b21ad8c20b10f185e236cb5a58624e2` |
