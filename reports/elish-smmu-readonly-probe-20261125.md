# elish read-only SMMUv2 probe, 2026-11-25

This diagnostic was built and RAM-only booted on elish. It reads SMMU state
without writing UFS or SMMU registers. No device partition was written.

## Inputs and implementation

The stock elish UFS node is `0x1d84000+0x3000`; its QMP v4 PHY is
`0x1d87000+0xe00`. The node has no `iommus` property, so it does not identify
an active UFS stream. The firmware memory map describes the apps SMMU at
`0x15000000+0xd0000` as MMIO. Existing `ArmSmmuDetachLib` reads SMMUv2
identification and stream registers but its detach path writes S2CR, SMR,
context-bank and TLB state; the diagnostic deliberately does not call it.

The opt-in `ElishLoaderProbeDxe` mode locates the firmware's named `SMMU`
region, validates its MMIO type, alignment and size, then reads ID0/ID1. It
uses ID0's NUMSMRG field to bound a scan of SMR entries and reads S2CR only for
valid SMRs. Each attempted group is logged before access so a stalled read has
a last-index marker. No register writes or UFS MMIO reads are issued. The mode
runs only when the diagnostic loader receives the DarwinBoot request; the
normal board default and production firmware leave the probe disabled. A
20-second loader timer exists, though it cannot guarantee recovery from a
platform-level MMIO bus stall.

This can report active stream mappings and context-bank indices. It cannot by
itself associate one mapping with UFS; that still needs elish-specific source
or firmware evidence. Alioth's `0xe0` / `0x4e0` stream values and SM8550
ShengUfs settings are not copied.

## Build result

Sources were built over device revision
`4b3da68ea0aca815a80e8d1bad802dfec8d5c905`, core revision
`71c5c6569bae6369c0e0f634df8b9bd5afc19139`, and firmware revision
`f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`, all with the pre-existing local
working-tree changes preserved. The isolated output is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run06-smmu-readonly/`.

```sh
python3 scripts/build-loader-probe.py \
  --payload /root/elish-build-20261123/UniXNU/out/elish-resume/elish/ipados-17.7.11-21H461/elish/payload/payload.img \
  --dtb /tmp/elish-uefi-probe.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run06-smmu-readonly \
  --clang-bin /usr/lib/llvm-21/bin \
  --feeds /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/run04-fat-child/build/feeds \
  --jobs 8 --smmu-readonly
```

The 64 MiB payload input SHA-256 is
`79ab59fda90509756a4b703295a5cc19c5b6932c6e66c2d25a5f864e983023ea`; the DTB
input SHA-256 is
`0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48`. Firmware
build and Android v3 image packaging both succeeded. The image is
67,751,936 bytes, SHA-256
`dc552b5eb9a2795f1de63abd3f48ab02003add18d2bd26b153f4af1789d0e24f`. The
3,145,728-byte FD SHA-256 is
`6067d8dcc754b6a8f6fa71ec8bc5e6fc8bdc9c319b06a4addf1c02d72ab754b5`. Build
options include `ELISH_LOADER_PROBE_SMMU=TRUE`; the compiled module contains
the `ELISH_SMMU_PROBE` markers. The image was subsequently tested on hardware;
the original run06 image and raw console log are preserved.

## Device run

Fastboot 37.0.3 accepted the 67,751,936-byte image via RAM-only `boot`:

```sh
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run06-smmu-readonly/boot.img
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img
adb -s 4bed07e4 root
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0
```

The recovered 49,328-byte log is archived at
`reports/elish-run06-smmu-console-ramoops-20261125.txt`, SHA-256
`fdaa24dcf70d4dbb41d63a49b2c705b8f7c33517128135c8439d56d6a9b1d7b2`. A live
root-ADB read returned the same SHA-256. The log proves the initrd block device
and unaligned FAT child were published, `unixnu.toml`, `kernel.macho` and its
ADT were opened, and the `xnu` entry reached DarwinBoot. The loader selected
window `0xc007c000+0x10000000` (256 MiB) and `ExitBootServices()` returned
Success with `accepted-snapshot=1`, key `0x3ee`. The final map is `0xD80`
bytes at stride `0x30`, or 72 descriptors (`map 0` through `map 71`); the
last is reserved memory at `0xB0000000`, 4 MiB. This diagnostic deliberately
stopped after the map capture and explicitly logged `XNU not entered`.

The read-only apps-SMMU scan found 87 stream-match groups and exactly one
valid group: group 4, raw `SMR=0x84020820`, `S2CR=0x00000004`, translation
type 0, context bank 4. The diagnostic's original mask print included
`SMR_VALID` and showed `0x8402`; SMMUv2 defines VALID at bit 31 and MASK at
bits 30:16, so the correctly decoded mask is `0x0402`, with SID `0x0820`.
The entry matches SIDs `0x0820`, `0x0822`, `0x0c20` and `0x0c22`. This does
not identify the stream as UFS: the stock elish UFS DT has no `iommus`
property, and no elish-specific stream-to-device mapping has been found.
The probe source now masks off the VALID bit for future builds; that source
correction is not present in the tested run06 image.

At 2026-11-25 04:42 CST, the device was back in Android 13 `lineage_elish`,
root ADB, slot `_b`; fastboot was not enumerated. The live pstore file matched
the archived run06 log exactly. The run was RAM-only, did not invoke
`fastboot continue`, and did not modify `boot_b` or any other partition.

## Run11 context-bank follow-up

The mask decode was corrected in the probe source, and the read-only scan was
extended to inspect the context bank selected by a valid translation S2CR. It
reads CBAR, SCTLR, TTBR0, TCR and FSR only after checking the advertised
context-bank count and MMIO bounds. No registers are written.

The isolated Android v3 image is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run11-smmu-context-readonly/boot.img`,
67,751,936 bytes, SHA-256
`9e6dbce96f9551c56265c375a05877d3b0fcbc283b9c03c322987f8ca18f8767`. Its
3,145,728-byte FD SHA-256 is
`c4f8b465c5b55bd4e1a3578643e551fe2f4f8d3cd25318f9bb0fb89365bbc58d`. The
build used device revision `4b3da68ea0aca815a80e8d1bad802dfec8d5c905`, core
revision `71c5c6569bae6369c0e0f634df8b9bd5afc19139` and firmware revision
`f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`, with the described local probe
changes. Fastboot 37.0.3 accepted the RAM-only image (`Sending OKAY`,
`Booting OKAY`, 1.833 seconds). The probe rebooted into fastboot as designed.
The hash-verified reader was then booted RAM-only; root ADB returned and read
`console-ramoops-0`.

The 49,439-byte run11 log is
`reports/elish-run11-smmu-context-console-ramoops-20261125.txt`, SHA-256
`f16b04efae4fb5859b84b7f6351fe970de52d2b89cfaf7f3525a5239a48b8600`. A second
live root-ADB read returned the same hash. The fixed decoder reports group 4
as SID `0x0820`, mask `0x0402`, `S2CR=0x4`, type 0, context bank 4. ID1
`0x6000004a` specifies 4 KiB pages and places context bank 4 at SMMU offset
`0x84000`; its CBAR is at `0x1010`.

The context-bank registers read as `CBAR=0x0001f000`, `SCTLR=0x000000e0`,
`TTBR0=0`, `TCR=0`, `FSR=0x00000400`. CBAR type 1 means S1 translation/S2
bypass; `SCTLR.M` is bit 0 and is clear, and TTBR0/TCR do not describe an
enabled S1 page table. FSR is recorded raw and not decoded. This does not
identify the matching SID as UFS or establish UFS DMA behavior.

The same run again confirmed FAT payload access, DarwinBoot's 256 MiB window,
and successful `ExitBootServices()` with accepted key snapshot `0x3ee` and
72 descriptors (`0xD80` bytes at `0x30` stride). The probe explicitly stopped
before XNU. At 2026-11-25 05:08 CST the tablet was back in Android 13
`lineage_elish`, root ADB, slot `_b`; fastboot was not enumerated. No device
partition was written and `fastboot continue` was not used.

The earlier `reports/elish-stock-storage-audit-20261124.md` remains the
hardware-input source. The run09 close photo confirms XNU through its pre-MMU
checkpoint; MMU enable is still not independently confirmed. Do not assign the
observed SMMU mapping to UFS or enable DMA until elish-specific ownership,
clock, reset and PHY evidence is available.
