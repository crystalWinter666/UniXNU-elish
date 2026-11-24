# elish UEFI memory-map probe, 2026-11-24

## Diagnostic firmware

The optional `ElishMemoryMapProbeDxe` reads the Qualcomm RAM Partition
protocol and the UEFI memory map over the two DRAM banks measured from the
live FDT. Earlier builds registered a `ReadyToBoot` callback, used
`DEBUG_ERROR`, and held for 120 seconds. Fastboot accepted those RAM-only
images, but the device returned to LineageOS without a captured memory page.

The follow-up diagnostic build changed the probe to run synchronously from
its DXE entry point. It clears the UEFI text console, disables the UEFI
watchdog, prints the results through `UefiLib` `Print()` and `DEBUG_ERROR`,
and requests a 600-second stall. The FDF places the probe in the DXE APRIORI
list after the console, framebuffer, and RAM manager drivers. The generated
`FVMAIN.Apri` contains the probe FFS GUID `3BAF47D0-1540-4B8F-A497-02B3C580AE67`.

The diagnostic FD is 3,145,728 bytes with SHA-256
`68ecc6c8806383a095b3a6cdd44d8d7ef350a8a298963ee6f753d4016d55b26f`. The
Android v3 image is 622,592 bytes with SHA-256
`adacbcf850e5d49f6d2fb0671215a83e64f21e497f5cd70b0b7cc42bb7df6b89`. The
boot-image builder unpacked the image and verified the kernel and empty
ramdisk against the inputs. This is a firmware-only diagnostic image with no
iPadOS payload.

## Device result

`fastboot boot` accepted the image. Follow-up USB enumeration found the
installed `lineage_elish` Android system running. The user reported that the
diagnostic display appeared only briefly. Later supplied screenshots show the
standard DXE Core HOB allocation dump and are audited in
`reports/elish-live-uefi-hob-observation-20261124.md`. They do not show the
custom probe marker or its RAM Partition and `GetMemoryMap()` output. Probe
entry-point execution and the 600-second stall remain unverified. No
partition-write command was run.

The preceding 600-second synchronous-entry build, before APRIORI ordering and
direct `ConOut` output were added, was also accepted by fastboot and returned
to Android. Its image SHA-256 is
`ccb4ecbeb1e702bfb062efb5d5d4d5e767e1db1e8f8d71b00cd94906d3ca0ee3`.

## Latest fastboot retest

The FDT-export image
`diagnostics/elish-memory-map-fdt-probe.img` was booted again after ADB
authorization became active. `fastboot getvar product` returned `elish`, and
`fastboot boot` accepted the 622,592-byte image with SHA-256
`3ccdc1738073add586a8eb87c7c6ad1deff3120fff899aa6d525f87335379a93`.
Android ADB returned as `lineage_elish`. The shell was UID 2000, and
`/proc/device-tree/chosen/unixnu,uefi-memory-map` did not exist. This fallback
Android tree therefore did not expose the UEFI configuration-table copy.

The probe source was changed to call `gBS->Stall(120000000)` after the display
routine returns, covering its early-return paths. The firmware build compiled
`ElishMemoryMapProbeDxe` and retained its FFS GUID in `FVMAIN`. The firmware FD
SHA-256 is `98dc9b6ab41af1f80db8159c087d8aead4b3a315807f4b01f4bca6d714128699`.
The verified 634,880-byte firmware-only Android v3 image is
`/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/memory-map-entry-hold-120s/boot.img`,
SHA-256
`03b08a9f39c09038d2c4f57c5b0df0fbb754f6e23964d7a9c40507e2b5027cd0`.
Its ramdisk is empty and `hardware_tested` is false.

Two new firmware-only images were RAM-booted. The first, with a hold on the
successful output path, has SHA-256
`4ae7830b862bc98f5902bb3ff01672c9cdcaec2bbed5afc866b2755005345591`. The
second moves the hold after the display routine returns. Fastboot accepted
both, and Android ADB re-enumerated within 10 seconds of the second boot.
Neither run captured the probe marker or descriptors. The result does not
distinguish a driver dispatch failure from a non-advancing platform Stall.
No partition-write command was run.

A third image adds a visible entry marker and a 15-second stall before the
memory-map read, retaining the 120-second post-return stall. Its path is
`/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/memory-map-entry-window-15s-120s/boot.img`,
size 634,880 bytes, and SHA-256
`70af40d78b3e46e1b3faf0fe3d8079aef642e22d41022c4582114325c1b2b7c5`. The FD
SHA-256 is
`38a564abce3a5b93f1aceb722b9c03b3dc13c20beab0ca33d24862146e5aa113`.
Fastboot accepted it, but Android ADB had re-enumerated after 10 seconds.
The host therefore did not observe either the entry delay or the map hold.
Whether the driver was dispatched or the platform timer advanced remains
unresolved. ADB again identified `elish` with shell UID 2000, and the fallback
Android `/chosen` property was absent.

The supplied HOB photographs show the DXE Core allocation-HOB walk, not the
full `GetMemoryMap()` descriptor array. That DXE probe output remains
unverified. DarwinBoot still needs `GetMemoryMap()` at runtime, while exporting
the complete map before a host build remains unnecessary.

## Remaining checks

The routine board setting remains `ELISH_MEMORY_MAP_PROBE = "FALSE"`. Rebuild
the routine firmware after the diagnostic build before using its default output
path. The RAM Partition entries, the UEFI `GetMemoryMap()` descriptor array,
and the final `ExitBootServices` map remain unobserved. The XNU candidate is
still marked `loadable=false`. This diagnostic image has no iPadOS payload.
The separate host-built payload and Android v3 image are recorded in
`reports/elish-ipados-host-image-20261124.md`.
