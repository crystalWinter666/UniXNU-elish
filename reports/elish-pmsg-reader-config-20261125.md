# elish pmsg reader configuration, 2026-11-25

## Input and extraction

The reader image used after run09 is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`,
55,083,008 bytes, SHA-256
`57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`. Its
manifest says it was made from the full `boot_b.img` backup
(SHA-256 `fce50614e0f90a4fd42b1eef26b913712883e4133723b802b4ac65a8dd1dd890`)
by removing partition padding only; the boot header, kernel, and ramdisk were
unchanged. The Android v3 header gives a 53,555,224-byte kernel beginning at
offset 4096.

The kernel was extracted from that exact reader image and has SHA-256
`b030f217fe256e02038fc3e76398985b02e4377b8197909657ddb7ca8e10c04a`. Linux's
`scripts/extract-ikconfig`, taken from local kernel source revision
`52a2f180865f8368828985d6384130acd093fa93`, extracted its embedded config.
The config is 174,816 bytes, SHA-256
`7f9604f0764cc986c4912b4599a80e922b6525bdb75589c6f0c19e59355e9bc6`.

Commands:

```sh
dd if=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img of=/tmp/elish-lineage-reader-kernel-20261125.bin bs=1 skip=4096 count=53555224 status=none
git -C /tmp/elish-linux-kernel-elish-r-oss show HEAD:scripts/extract-ikconfig | sh -s /tmp/elish-lineage-reader-kernel-20261125.bin > /tmp/elish-lineage-reader-config-20261125
```

## Result and evidence limit

The extracted config identifies Linux/arm64 4.19.325 and enables
`CONFIG_PSTORE=y`, `CONFIG_PSTORE_CONSOLE=y`, `CONFIG_PSTORE_PMSG=y`,
`CONFIG_PSTORE_RAM=y`, and `CONFIG_IKCONFIG_PROC=y`. The pmsg frontend was
therefore compiled into the exact reader kernel used after run09. Its
`/sys/module/ramoops/parameters/pmsg_size` was also observed as 2 MiB, and
`/sys/fs/pstore` was empty after the run09 image.

This rules out a disabled pmsg frontend as the explanation for the empty
directory. It does not prove that XNU's write to `0xb0200000` reached or
survived in the reader's pmsg zone, nor does it explain why no pmsg record was
exposed. The run09 screen remains direct evidence that XNU reached the
pre-MMU checkpoint; no new device test was run for this audit. The latest host
USB snapshot is still 09:07 CST, when fastboot and ADB listed no tablet.

Next device session: verify fastboot reports `product=elish`, RAM-only boot
the run18 post-VM marker image, then RAM-only boot the pmsg-readback image and
the verified root-ADB reader. Inspect pmsg and console files separately. The
readback image logs the marker directly to the separate console ring, so an
empty pstore pmsg file does not erase that evidence. If no record appears,
preserve the raw `0xb0200000+0x200000` bytes before another firmware boot, if
the reader exposes a read-only path to that reserved region. Do not treat an
empty pstore directory as evidence that the XNU entry photo is false.
