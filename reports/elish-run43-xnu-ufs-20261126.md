# elish run43 XNU UFS handoff

The RAM-only image reached `ElishUfsProbe::start` in the 21H461 XNU build. The kernel screen showed `elish UFS SMMU routes or initial faults` and `DMA=00000000 GPT=00000000 userdata=00000000 stopped=1`. This checkpoint follows the GCC power, clock and reset checks. It precedes DMA allocation, transfer-list writes and command submission. `stopped=1` here means the probe never started its own queue. The screen cannot distinguish a mapping failure, an unexpected route or a fault-status mismatch.

The host camera capture `/tmp/elish-run43-xnu-ufs-20261126/screen-24s.jpg` has SHA-256 `f32eebd2314d26b4da743b15852fc39b04b30a9148ccb750fe0210f11e730aab`. The user supplied a close-up `E6A0EE7938ECC4BC512AE8EB9531F204.png`, SHA-256 `bfee966b80b106691358284eb1bc518ceb5c764c88c0855fdbb6c83e4f4206bb`, with the same result. XNU parked on the terminal screen. USB exposed neither ADB nor fastboot after the capture.

## Build and device commands

Device revision `2561162a380f3570f5e9cb8360a74603e23bf691` plus local probe integration. Firmware revision `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30` plus the run42 diagnostic changes. UniXNU revision `71c5c6569bae6369c0e0f634df8b9bd5afc19139` plus local build support. XNU source revision `d8b80295118ef25ac3a784134bcf95cd8e88109f`.

| Artifact | SHA-256 |
| --- | --- |
| `boot/boot.img` (235,528,192 bytes) | `718bb8e187d3c3fd85823996165ce6dcd04c6b48af6a798756e90e29f9bff65a` |
| `boot/SILICIUM_UEFI.fd` | `9217e1e47fc2419e4973463660f5eb364aafb8ca9e58c09d60c75a27b5922d21` |
| `payload.img` | `ce7faefa9c872ac613f1f0ab3cabea032be91853a66696686bd398965569149b` |
| Prelinked `kernel.macho` | `b19fa091328c8afac2b0e50b26bb32c6c3fae560f41f6a5e371f012dab6f78a5` |
| Kernel with panic screen | `8c467b32ad28fabc5d15bf7b34a810a142be56a49c6ee9296689fd85a3c49334` |
| Run43 `ElishUfs.cpp` | `ce19a10c4278fbb487ef9199b2d3cdf776095cf9000c926df6d85662f5a58e91` |
| `build-elish-ufs.py` | `1366425042bb87260b303a9d72823f05f58e0d70ab37d956ed3390e30063e469` |
| `elish-ufs-probe.patch` | `774e15865f3b8ea38ecdb7fe39fffcd8eeaf41d2fcffc3c432a58a41e3ee60cb` |

The final Mach-O contains `ElishUfsProbe::start`. The RamBlock ABI verifier passed for 190 vtable slots and 24 native overrides. `fsck.fat -vn` passed, and the kernel and boot config read back from FAT matched the inputs. The payload kept the run28 Restore sidecars and added `elish-ufs-probe=1` to the entry arguments. The firmware did not submit a UFS command.

```sh
CORE=/root/elish-build-20261123/UniXNU
DEVICE=/root/repo/UniXNU-elish
OUT=/tmp/elish-run43-xnu-ufs-20261126
export UNIXNU_UEFI=/root/repo/uefi-elish
python3 "$CORE/unixnu" defconfig -D "$DEVICE" --os ipados-17.7.11-21H461 elish_defconfig
python3 "$CORE/unixnu" build kernel -D "$DEVICE" --os ipados-17.7.11-21H461 -j 12
# Copy the prelinked kernel, apply the panic screen and replace the FAT kernel/config.
PATH=/usr/lib/llvm-21/bin:$PATH python3 "$CORE/scripts/patch-xnu-entry-probe.py" "$OUT/kernel.macho" --no-reset --panic-screen
python3 scripts/build-loader-probe.py --unixnu "$CORE" --uefi "$UNIXNU_UEFI" \
  --dtb /tmp/elish-uefi-probe.dtb --out "$OUT/boot" --feeds "$CORE/out/feeds" \
  --clang-bin /usr/lib/llvm-21/bin --jobs 8 --ufs-run-kernel --payload "$OUT/payload.img"
FB=/tmp/elish-platform-tools/platform-tools/fastboot
lsusb -d 18d1:d00d
"$FB" devices -l
"$FB" -s 4bed07e4 getvar product
"$FB" -s 4bed07e4 getvar is-userspace
"$FB" -s 4bed07e4 boot "$OUT/boot/boot.img"
```

USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` were verified immediately before boot. Transfer and boot both returned `OKAY` in 5.519 seconds. No partition writes were issued. The offline Android userdata prefix backup was verified before this boot. [Backup evidence](elish-run42-ufs-root-windows-20261126.md#offline-prefix-backup)

Run44 adds the first mismatching MMIO address and observed/expected register values. It reads the context TTBR0 at the same 64-bit width used by the UEFI reference. XNU DMA, block-device publication, full System-root boot, Setup and Data persistence remain unverified.
