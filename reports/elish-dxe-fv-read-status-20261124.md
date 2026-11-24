# elish DXE firmware-volume read-status diagnosis

## Runtime failure

The screen capture `/root/repo/946BF84D23D5AC424DD564A47EFE82EA.png` shows
two firmware-volume HOBs. FVB protocol installation returned `Success` for
both volumes. The `FwVol` notification then processed both handles, and
`GetFwVolHeader` returned `EFI_ACCESS_DENIED` for each. No FV2 protocol was
installed, so the DXE dispatcher returned without dispatching drivers. The
architecture-service check then failed with `EFI_NOT_FOUND` and asserted in
`DxeMain.c`.

In the pinned Mu_Basecore implementation,
`FwVolBlockReadBlock()` returns `EFI_ACCESS_DENIED` when the FV header's
`EFI_FVB2_READ_STATUS` bit is clear. The generated outer and inner FV headers
had attributes `0x000C0C00` and `0x00030C00`, respectively, and neither had
that bit set.

## Firmware correction

The elish FDF now sets `READ_ENABLED_CAP = TRUE` and `READ_STATUS = TRUE` in
both `[FV.FvMain]` and `[FV.FVMAIN_COMPACT]`. The rebuilt headers have
attributes `0x000C0C06` and `0x00030C06`. A routine 3 MiB FD built with
`ELISH_MEMORY_MAP_PROBE=FALSE` has SHA-256
`809131c7ffc4273e8e5e25deaa423499172113ba135366c06c144c50c7b9247f`.
The corrected FV attributes are also present in the separately built
diagnostic FD.

Temporary `ELISH TRACE` instrumentation in Mu_Basecore was removed after the
screen capture identified the failure. The permanent FDF attributes remain.

## Device observations

The corrected firmware-only image
`/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/elish-dxe-fv-readable.img`
is 622,592 bytes with SHA-256
`3b1044c711dc5c6b0ee2ddf6222faf45dc2a00fa124407e9c4a136abcbf52ab0`. It has
an empty ramdisk. `fastboot -s ELISH_SERIAL boot` returned `Sending` and `Booting`
`OKAY`. The user reported that the device entered its system. A subsequent
ADB enumeration identified `product:lineage_elish`, model `M2105K81AC`, and
device `elish`. No partition-write command was run. This image contains no
iPadOS payload.

## Memory-map probe visibility

The first corrected-FV memory-map probe used `DEBUG_INFO`. The elish DEBUG
firmware sets `PcdDebugPrintErrorLevel` to `0x8007EE0F`, which does not include
the `DEBUG_INFO` bit (`0x40`). That build therefore did not make the probe
text visible through the framebuffer debug logger.

The probe now emits its startup marker, ReadyToBoot registration status, RAM
Partition table, and UEFI descriptors at `DEBUG_ERROR`, which appeared on the
earlier DXE screen. A new 3 MiB diagnostic FD has SHA-256
`a1ace46239854d9af0e1fce69efd1ca6e7d5dd3f9c174e72c2da6090beb2c394`. Its
Android v3 image is 622,592 bytes with SHA-256
`75bbfd228f57b22fdc1bd03349be01720ab9fac3b31cd392900ae874638ddbae`. The
probe holds the output for 120 seconds. The PE32 output contains the expected
probe and `DEBUG_ERROR` strings. The image has not been booted since this
visibility correction.

The user asked to continue offline work while away and will report when
available. Resume device testing with the visible probe image, then record the
RAM Partition entries and pre-application UEFI descriptors. The final
`ExitBootServices` map remains unobserved.

## Build records and remaining gates

The routine firmware build command was:

```sh
./unixnu build firmware -D /root/repo/UniXNU-elish \
  --uefi /root/repo/uefi-elish \
  --edk2 /root/repo/uefi-elish/Mu_Basecore -j 8
```

The firmware-only diagnostic image was packed with:

```sh
./unixnu bootimg -D /root/repo/UniXNU-elish \
  --uefi /root/repo/uefi-elish \
  --fd /root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
  --dtb /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/device-tree/hardware.dtb \
  --ramdisk none \
  -o /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-dxe-memory-map-probe-visible.img
```

The UEFI sources are based on Mu-Silicium revision
`66421cb7bb5694843aec8de1ca3d1de4c20cca38` and Mu_Basecore revision
`cfb93724a06ff86ff5ba971413368a7809ee42a5`. The routine board setting remains
`ELISH_MEMORY_MAP_PROBE = "FALSE"`.

This DXE correction does not qualify the experimental XNU kernel. Its
`kernel.json` still reports `loadable=false`. No iPadOS payload or iPadOS boot
image has been produced. UEFI/XNU memory ownership and the elish UFS/APFS root
path remain unresolved.
