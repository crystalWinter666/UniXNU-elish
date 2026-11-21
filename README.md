# Xiaomi Pad 5 Pro Wi-Fi (`elish`)

Format 2 device repository for iPadOS 17.7.11 (`21H461`) on Xiaomi Pad 5 Pro Wi-Fi, model `M2105K81AC`, SoC `SM8250-AC` / Snapdragon 870.

## Status

This is a configuration scaffold, not a bootable port. No UEFI platform image or UniXNU image has been built or booted. The Android OS/SystemOS and Data paths are not implemented for elish. The cellular `enuma` variant is a separate target and is not supported here.

Direct observations include `product=elish`, SoC ID `356` revision `2.1`, CPUs `0-7`, Linux `MemTotal=7775836 kB`, display mode `1600x2560`, Novatek input device names, and UFS route `1d84000.ufshc`. `MemTotal` does not establish DRAM banks or reservations. The stock ROM confirms DTBO entry 12 / board ID 47 for elish, but does not provide a runtime memory map or GPT layout. The active panel variant is unknown.

The local evidence ledger and stock-image hashes are in `reports/elish-hardware-inventory.md` in the UniXNU checkout. Proprietary ROM images are not included in this repository.

## Boot and storage model

An initial `fastboot boot boot.img` test would load the boot image and its firmware/payload into RAM and would not install that image to a boot partition. It would only test the early boot chain; it would not by itself provide a complete iPadOS system.

The planned UniXNU layout keeps the IPSW `OS` and `SystemOS` disk images in windows inside a userdata image on UFS. A Data seed is carried by the boot payload; a later persistent Data store can use a separately allocated window on userdata. Neither path is implemented or validated on elish. Exact userdata GPT geometry and logical sector size remain unknown, so the checked-in zero values are deliberate sentinels and prevent userdata image sizing.

Persisting the system on UFS is technically distinct from installing it into bootloader ROM. Writing a userdata image or creating a separate partition is a destructive device operation and requires fresh explicit authorization; this repository does not perform either operation. No boot, vendor, recovery, or bootloader partition write is planned.

## Sources

- SoC feed: `FFT-infra/UniXNU-sm8250-common`, pinned in `feeds.toml`.
- OS profile: `ipados-17.7.11-21H461` (`iPad7,1` / `J120AP`).
- Source revisions: `unixnu.lock`; resolve and verify them with the supported macOS/Linux toolchain.
- The existing community elish branch and enuma port are audit references, not sources for unverified memory or partition values.

## Validation

Run static checks from the UniXNU checkout:

```sh
python3 scripts/porting-record.py check --device ../UniXNU-elish-device
python3 ../UniXNU-elish-device/scripts/check-elish.py --static-only
```

The normal board preflight intentionally fails until the runtime DRAM map, userdata GPT geometry, and elish UEFI platform are verified. Do not bypass it for an image intended to boot on hardware.

The packaging entry point, once those blockers are resolved, is:

```sh
../UniXNU-elish-device/scripts/package.sh --check --os ipados-17.7.11-21H461
```

All hardware claims and test results belong in `porting.toml`; unverified items stay planned. No build, boot, or persistent-storage result is claimed here.
