# elish XNU entry ramoops checkpoint, 2026-11-25

Device: Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC` / `elish`), target iPadOS 17.7.11
(`21H461`). This is a terminal RAM-only diagnostic. No persistent partition was
written and `fastboot continue` was not used.

## Checkpoint image

The run07 image uses the FAT payload path and an EBS-capable loader, then
continues into a patched XNU copy. At the kernel entry site, the patch checks
that `SCTLR_EL1.M` is clear and that the ramoops ring header and offsets are
valid. Only then does it append
`ELISH_XNU_CHECKPOINT: entry; SCTLR.M=0; terminal probe reset` at
`0xb0000000+0x200000`, clean the cache lines, and request the configured warm
reset. If any check fails, it still requests reset without writing the marker.
Therefore a reset or USB return alone cannot establish that XNU entry ran.

The patch manifest records entry site `0xfffffff00741a488`, code cave
`0xfffffff00741a48c`, and 260 bytes of checkpoint code. The patched kernel
SHA-256 is
`ff3eceba83f86ec5b9b21830371bdacd644bd48c868f965743885771f22292b0`, matching
the kernel read back from the generated FAT payload.

## Ramoops format audit and tool correction

The elish `RamoopsLogLib` uses a 12-byte header at `0xb0000000`, signature
`0x43474244`, then `start` and `size`, followed by `0x1ffff4` bytes of console
data. Its writer stores data before metadata, advances `start`, wraps it to zero
at the capacity boundary, and increments `size` only until the buffer is full.
The Linux v4.19 `persistent_ram_buffer` has the same `sig/start/size/data`
layout and wrap/saturate behavior. With this layout, `start == size` means the
log has not wrapped yet. The checkpoint uses that condition to append only
when it can preserve the existing ring contents in one contiguous write; a
wrapped/full console intentionally resets without adding the marker.

The offline review found one exact-capacity edge in the run07 writer: when the
marker ended exactly at the last console byte, it stored `start == capacity`
instead of wrapping `start` to zero as Linux does. The run07 image above is
unchanged and still contains the earlier 260-byte routine. The patch script
now emits the wrap operation. Assembly was checked on a temporary copy of the
unpatched base kernel (`ea44140c9e6a619fd14a3fd0607587c108791375eab86c0439155eee73b99961`):

```sh
python3 -m py_compile scripts/patch-xnu-ramoops-checkpoint.py
python3 scripts/patch-xnu-ramoops-checkpoint.py \
  /tmp/elish-run07-xnu-entry/wrap-boundary-test.macho --stage entry
```

The temporary patched kernel is 276 bytes of checkpoint code with SHA-256
`0ad2ca17724f66adf25afd473008e9c47842c2b00d55d8b1ec79ba6c40b9086f`. This
verifies patch generation and assembly only; no replacement boot image was
built and the changed code has not run on the device.

## Build and host verification

Source baselines were device repository
`4b3da68ea0aca815a80e8d1bad802dfec8d5c905`, UniXNU
`71c5c6569bae6369c0e0f634df8b9bd5afc19139`, and firmware
`f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`. Existing working-tree changes
were retained.

```sh
python3 scripts/build-loader-probe.py \
  --payload /tmp/elish-run07-xnu-entry/payload.img \
  --dtb /tmp/elish-uefi-probe.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run07-xnu-entry-checkpoint \
  --clang-bin /usr/lib/llvm-21/bin \
  --feeds /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/run04-fat-child/build/feeds \
  --jobs 8 --run-kernel
python3 scripts/check-elish.py --static-only
```

The resulting image is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run07-xnu-entry-checkpoint/boot.img`,
67,751,936 bytes, SHA-256
`38b548e03d2a0156f443e010cfdecca2becd7bc745f911a56dbb99e1b99c1e37`. Its
3,145,728-byte `SILICIUM_UEFI.fd` has SHA-256
`057bb569ef3773764cfe6c9a025d1fc335a2321066a1537d34d6f87bd5558f20`. The FAT
payload SHA-256 is
`7e90eeaff5d8fb04ef5750f8139a6365d9a0624f2ddb568a331f81d3f54e7416` and the
embedded DTB SHA-256 is
`0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48`. Static
preflight passed; its output explicitly leaves runtime handoff unverified.

## Device result

The bootloader accepted the RAM-only image: fastboot reported `Sending OKAY`
and `Booting OKAY`. The host USB event sequence recorded after launch was:

- 01:10:14 CST: fastboot USB disconnected.
- 01:10:15 CST: Xiaomi USB `18d1:4e11` appeared.
- 01:12:55 CST: USB disconnected.
- 01:13:03 CST: fastboot USB `18d1:d00d` appeared.
- 01:13:56 CST: USB disconnected.

This sequence is not tied closely enough to a captured screen or pstore to
identify the checkpoint's execution. The raw kernel USB event transcript was
not archived. The last confirmed stage for run07 is fastboot acceptance; this
run does not directly confirm FAT discovery, DarwinBoot, EBS, XNU entry, or
MMU enable. No pstore was recovered after this run.

At the latest host snapshots, the user reported that the tablet was in
fastboot, but `fastboot devices -l`, `adb devices -l`, and `lsusb` showed no
tablet interface. A fresh host check during the offline format audit still
found no tablet. Device probing has stopped at that point. The user report is
recorded, while host-side fastboot enumeration and `product=elish` remain
unconfirmed for the current state.

## Next measurement

When the device is again visible to the host, first confirm live fastboot
enumeration and `product=elish`. Boot the verified reader image
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`
(SHA-256
`57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`), run
`adb root`, and export `/sys/fs/pstore` before starting another firmware
image. A matching checkpoint marker would confirm execution through the first
XNU entry instructions with MMU off; it would not establish MMU enable, UFS,
APFS, or userspace. If the marker is absent, the reset path alone does not
locate the stop.
