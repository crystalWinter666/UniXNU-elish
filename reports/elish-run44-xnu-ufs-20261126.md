# elish run44 XNU SMMU control check

`ElishUfsProbe` stopped at the first SMMU global-register check after passing the GCC reset, power and clock checks. The physical register was `0x15000000` (`sCR0`, a 32-bit read). The screen printed `got=ffffffff00230406 want=0000000000270406`. The low 32 bits differ at `0x00040000` only. The high word of the rendered `got` is inconsistent with the source's zero extension of the 32-bit read. The next image prints the high and low words individually.

Run35 DXE and run36 SEC had already reported `0x00230406` on elish. Run38 and run40 observed `0x00270406`. Arm IHI 0062D.b §9.6.3 defines the differing bit as part of `MemAttr[19:16]`. `MTCFG=0` in both values, so the field does not override memory attributes. `CLIENTPD` remains clear and `USFCFG` and `SMCFCFG` remain set. The run44 screen precedes the route scan, DMA allocation and UFS queue operations. It does not identify the stage that set `sCR0` this boot. [Arm IHI 0062D.b](https://kib.kiev.ua/x86docs/ARM/SMMU/IHI0062D_b_system_mmu_architecture_specification.pdf)

The earlier `fastboot reboot recovery` to retrieve run43 ramoops returned `OKAY`. The host's `adb wait-for-recovery` timed out after 75 seconds without recovering the log. The tablet was back in bootloader fastboot before run44. Host USB identified `18d1:d00d` and verified serial `4bed07e4`, `product=elish` and `is-userspace=no`. The RAM-only image took 5.521 seconds to transfer and boot. Firmware and XNU issued zero UFS commands, and no partition writes were requested.

## Inputs and verification

Device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, and UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, each with the local changes documented in run43. XNU source `d8b80295118ef25ac3a784134bcf95cd8e88109f`. The run43 build commands apply with the new output directory and the register-level probe source. The XNU archive, prelink, RamBlock vtable check, FAT check and firmware build passed. [Build commands](elish-run43-xnu-ufs-20261126.md#build-and-device-commands)

Output directory `/tmp/elish-run44-xnu-ufs-registers-20261126`.

| Artifact | SHA-256 |
| --- | --- |
| `boot/boot.img` (235,528,192 bytes) | `03e2f2be29451f4996c0b80a09def9f6670dffcc0920fa693672a61b252cbd30` |
| `boot/SILICIUM_UEFI.fd` | `38041e4e1b432e5897fd5bf9a0acd63cf4ee762aa3b91a9306e37b1fae6843a2` |
| `payload.img` | `3964a716a59c28790f7de222eec9a710058c60d03a66d925f4a6963730f99c25` |
| `kernel.macho` with panic screen | `bdf31b2d27f117f39e102dc025e2c90d68c9c44fe0b771526c1abd0217f863c2` |
| Run44 `ElishUfs.cpp` | `571c60e533ab1d6c48dde328fd4b1d6b0b4b424a1c3095f03d384a04a445d201` |
| `screen-25s.jpg` | `dd0d45944d4b83f1998eecb2618947d043476b16023e7ab06004b7ee3406aa20` |
| User photo `/root/repo/28537DA3BD5B22413E25A41C480D0B93.png` | `e6566ab671be3f11028f34008a02e47221dd6646b16d5d1543b9cea98b9047c7` |

```sh
FB=/tmp/elish-platform-tools/platform-tools/fastboot
lsusb -d 18d1:d00d
"$FB" devices -l
"$FB" -s 4bed07e4 getvar product
"$FB" -s 4bed07e4 getvar is-userspace
"$FB" -s 4bed07e4 boot /tmp/elish-run44-xnu-ufs-registers-20261126/boot/boot.img
```
