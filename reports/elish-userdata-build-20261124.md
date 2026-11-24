# elish userdata root image build, 2026-11-24

## Inputs

- Device: Xiaomi Pad 5 Pro Wi-Fi, `elish` / `M2105K81AC`.
- Device repository baseline: `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`, with local board and porting changes.
- UniXNU core: `/root/elish-build-20261123/UniXNU`, revision `71c5c6569bae6369c0e0f634df8b9bd5afc19139`.
- Target IPSW: `iPad_Pro_HFR_17.7.11_21H461_Restore.ipsw`, SHA-256 `c84ad24b6981a859eec499b1802be9ac593acfd3dc0a185a76d06e4d767a1451`.
- Measured userdata partition capacity: 226,308,829,184 bytes, with 4,096-byte logical blocks.

## Build

The dry run reported OS and SystemOS disk images of 4,918 MiB and 3,150 MiB. The build command was:

```sh
./unixnu build userdata -D /root/repo/UniXNU-elish \
  --os ipados-17.7.11-21H461 \
  --ipsw /root/elish-build-20261123/UniXNU/.cache/firmware/iPad_Pro_HFR_17.7.11_21H461_Restore.ipsw
```

The output is `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/userdata/userdata-root.img`, 9,382,658,048 bytes (8.74 GiB), SHA-256 `fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093`.

| Window | Offset | Size | SHA-256 |
| --- | ---: | ---: | --- |
| OS | `0x100000` | 6,077,580,800 | `e5634d04a8ac35f024845f583394e0a35deb26a04cb0c00286a8674e25a9a4f2` |
| SystemOS | `0x16a600000` | 3,303,014,400 | `b4908cde41d23e6bc05e6af51a2c9eeb00cf0f863a6a18b1600c96a3fd70414f` |

The SHNGROOT v1 manifest SHA-256 is `bc908a6cf66491055077798550660f40128bfe6dce2fdc0db56e6af96511208d`. An independent read-back verified the file size, manifest digest and fields, both embedded window hashes, offsets and sizes. The generated `userdata-root.json` records `hardware_tested=false`.

The APFS partition inside the OS window starts at LBA 34. Its `NXSB` and
`APSB` superblock Fletcher64 checksums both validate. The System-role APFS
volume is named `DawnUpdate21H461.J120J121J207J208OS`; its volume UUID is
`04769D26-2CA0-4825-B443-E2E5F88300F2`, matching
`board.toml [payload.adt].root_match_volume_uuid`. The field layout was checked
against `apfs-fuse` revision `66b86bd525e8cb90f9012543be89b1f092b75cf3`.
This confirms the IPSW root-volume identity, not a device-side mount.

## Limits

This is a host-built root-image container only. It has not been flashed or read by an elish XNU driver. The UFS/SMMU path, device-side root mount, Data volume, platform seed and device boot remain unverified. No device partition was written.
