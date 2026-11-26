# elish run49 XNU reset to fastboot

Run49 used the same read-only root probe as run47 and corrected the inline assembly to emit every PSCI and timer instruction. Disassembly of the Mach-O read back from FAT shows `CNTFRQ_EL0`, both `CNTPCT_EL0` reads, the full `0x84000009` function ID and `smc #0` at `0xfffffff0079e1ca8`. Kernel prelink, both 190-slot storage ABI checks, and `fsck.fat -vn` passed.

The screen at 15 seconds shows the missing `SHNGROOT` checkpoint with `stopped=1 faults-same=1`. The fastboot endpoint was verified as USB `18d1:d00d`, serial `4bed07e4`, product `elish` and `is-userspace=no` before both RAM boots. `fastboot -s 4bed07e4 boot "$OUT/boot/boot.img"` returned both `OKAY` results in 1.775 seconds. USB re-enumerated from device 076 to 077 and responded to both `getvar` calls approximately 21 seconds after the boot command completed. The RAM-only Lineage reader then started in 1.473 seconds. Root ADB listed an empty `/sys/fs/pstore`.

The [run47 build and identity commands](elish-run47-xnu-ufs-root-20261126.md#build-and-device-evidence) apply with `OUT=/tmp/elish-run49-xnu-psci-asm-20261126`. Source baselines match run47. The run directory contains `source.sha256`, `reset-asm.txt`, and `device.log`.

| Artifact | SHA-256 |
| --- | --- |
| `boot/boot.img` | `cf7ee508b3233833abc481cadf71edddfc5612604631e4e85bacb1710dabab54` |
| `boot/SILICIUM_UEFI.fd` | `c20dc05c2cb0b3e13ab3d99c0b6fcc4ce354a972afd635551525e7f4001717ee` |
| `payload.img` | `7bd1e552d44dd31fa297f730933b9e7ae23236c754352336faff9b1d68c79658` |
| `kernel.macho` | `b4e57d24fdbd09fdd8669a4b158a762d33731da0688ef38cdd7957617853cc12` |
| `screen-15s.jpg` | `2776663795f1617db9a41de13d3d773a6cbd7d06ce9289b97b8766bc873b67a1` |
