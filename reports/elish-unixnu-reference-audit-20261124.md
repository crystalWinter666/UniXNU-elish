# UniXNU port reference audit

Read-only comparison performed on 2026-11-24 while elish was unavailable over USB. No external source was copied into the elish tree and no device image was started.

## XNU checkpoints on enuma

The `wh1stl3/UniXNU-enuma` record at device-repository revision `74dc21dd5c323a763a225f98a6eefc3815272046` documents stage-5 probes on enuma, the SM8250-AC 5G sibling. Its entry probe uses visible entry/MMU markers and paired reset-before/reset-after branch checkpoints. The filmed device observations passed `_IOFindBSDRoot` and did not pass the `_vfs_mountroot` call-site checkpoint; the record infers a wait inside root-device discovery, as expected without storage. This is useful checkpoint placement guidance, not proof for elish.

That record also reports that an automatic PSCI reset did not leave `console-ramoops-0`, while a manual long-press returned a firmware-only log. Elish uses a DRAM ramoops console at `0xb0000000` with a 2 MiB console zone. A complete power-off can therefore lose run05's log; the result remains unknown until root ADB checks pstore. The enuma `fastboot continue` recovery recipe is not used on elish because elish previously entered USB mass-storage mode or shut down after that command.

## UFS references

`FFT-infra/NativeUfs-alioth` revision `336680c353e6c3f25d6b29284532c4fd16104f68` records SM8250 stream values `0xe0` and `0x4e0`; its code and README tie them to alioth's SMMU setup. Elish's stock UFS node has no `iommus` property, so these values do not establish elish ownership and must not be enabled by analogy.

The `UniXNU-marble` device record at revision `78f4e7bf00e88129f8b6ef5680d81770d386832d` demonstrates a useful evidence sequence on a different Qualcomm SoC: inspect the live SMMU mapping, identify its context-bank owner, then perform read-only GPT discovery and known-block checks. It records SID `0xe0` because that board's vendor tree and live registers support it. The sequence is reusable; its stream, clocks, register values and takeover policy are not elish inputs.

The official UFS guide keeps register/link inspection, firmware-state ownership, DMA/command completion, known-block reads, APFS root mount and persistence as separate milestones. Elish remains before the first device-side UFS milestone. Its verified userdata geometry and host-built APFS UUID do not demonstrate UFS DMA or an XNU root mount.

## Sources

- UniXNU core docs at `71c5c6569bae6369c0e0f634df8b9bd5afc19139`: `docs/devices/enuma.md`, `docs/devices/alioth.md`, `docs/devices/marble-milacy.md`, `docs/devices/kebab-yecw.md`, and `docs/porting/ufs.md`.
- `wh1stl3/UniXNU-enuma`, device record commit `74dc21dd5c323a763a225f98a6eefc3815272046`, `docs/entry-probe-bisect.md` and `docs/firmware-log-capture.md`.
- `alghiffaryfa19/UniXNU-enuma-5g`, main commit `994c348b6f2c903f8e14820305c4343f5c6db900`, README and `docs/operations/2026-10-23-ufs-entry-boundary.md`.
- `UniXNU-marble`, evidence links pinned to `78f4e7bf00e88129f8b6ef5680d81770d386832d` in the core device record.

## Current elish observation

After the user reported powering the tablet back on, the host polled USB, ADB and fastboot for 90 seconds and then checked them again. No elish interface appeared. No pstore read or device boot command has run in this session. Resume by checking ADB/pstore first when the tablet is connected; treat a missing or cleared log as inconclusive.
