# elish run48 XNU PSCI diagnostic

Run48 reached the same XNU `userdata has no SHNGROOT manifest` checkpoint as run47. The recording shows `stopped=1 faults-same=1` at 55 seconds. The host waited 60 seconds for a responding bootloader and saw no automatic return. The user then entered fastboot manually.

The linked Mach-O explains the reset failure. The Darwin ARM64 assembler treated semicolons in the inline assembly as comment delimiters. The delay loop contained `isb` and a comparison, with no counter read inside the loop. The reset sequence contained `mov w0, #9` followed by `ret`, with the remaining PSCI arguments and `smc #0` absent. Run49 uses newline-delimited instructions.

Source baselines match run47. `source.sha256` in the run directory pins the working-tree inputs. The [run47 build and identity commands](elish-run47-xnu-ufs-root-20261126.md#build-and-device-evidence) apply with `OUT=/tmp/elish-run48-xnu-psci-root-20261126`. The build, both 190-slot vtable checks, and `fsck.fat -vn` passed. The host verified USB `18d1:d00d`, serial `4bed07e4`, product `elish`, and `is-userspace=no`. `fastboot -s 4bed07e4 boot "$OUT/boot/boot.img"` completed in 1.766 seconds. The diagnostic only submitted read commands to UFS.

| Artifact | SHA-256 |
| --- | --- |
| `boot/boot.img` | `37ae7e2e6f8d9db83c4f434f9fb67d1bd077c085bbe549dac79270fa57e12281` |
| `boot/SILICIUM_UEFI.fd` | `ae95996d88cce0620ecd40fb07c55a751fcb9567aec9317abd0414754b08b7b7` |
| `payload.img` | `bbc292eef8af3c6199b72e977a7d550f08ac02d9951af03bfbbc58804b6c0dc6` |
| `kernel.macho` | `392a02dd4917d60e2eca2683c167e515ed8fc2b283bee021a37eacd7c860e2d9` |
| `screen-55s.jpg` | `cb23453181580c8c1a7fbf2c2675e666343915f89d3ac853edea04f2d7e4b070` |
