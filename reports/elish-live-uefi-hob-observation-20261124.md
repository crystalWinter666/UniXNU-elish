# elish live UEFI HOB screen audit, 2026-11-24

## Screen evidence

The supplied frames identify `Project Silicium for Xiaomi Xiaomi Pad 5 Pro Wi-Fi` and show firmware version 4.0. The SEC output reports `MMU: 45 regions`, `MMU: map valid`, successful MMU enablement, successful platform HOB construction, and HOB list counts of 94 and 98 entries. It then reports successful FV decompression, DXE Core loading at `0x9b4c7000` with entry point `0x9b4c823c`, and a DXE HOB-list address of `0x9b2fe018`.

The visible `Memory Allocation` rows are printed by the pinned Mu-Silicium Basecore DXE Core while it walks `EFI_HOB_TYPE_MEMORY_ALLOCATION` HOBs. The numeric field is the UEFI memory type, followed by the inclusive physical base and end addresses. These rows are runtime allocation-HOB evidence from this firmware boot.

| Memory type | Base | Inclusive end | Cross-check |
| --- | --- | --- | --- |
| `0x4` `EfiBootServicesData` | `0x805d0000` | `0x805effff` | IPC SHM |
| `0x0` `EfiReservedMemoryType` | `0x80600000` | `0x806fffff` | MPSS EFS |
| `0x4` `EfiBootServicesData` | `0x80700000` | `0x807fffff` | Boot info |
| `0x0` `EfiReservedMemoryType` | `0x80884000` | `0x80893fff` | XBL log buffer |
| `0x7` `EfiConventionalMemory` | `0x98900000` | `0x9bbfffff` | DXE heap |
| `0x4` `EfiBootServicesData` | `0x9bc00000` | `0x9bffffff` | Scheduler heap |
| `0x4` `EfiBootServicesData` | `0x9f800000` | `0x9f9fffff` | FV region |
| `0x0` `EfiReservedMemoryType` | `0x9fa00000` | `0x9fbfffff` | ABOOT FV |
| `0x4` `EfiBootServicesData` | `0x9fc00000` | `0x9fefffff` | UEFI FD |
| `0x4` `EfiBootServicesData` | `0x9ff00000` | `0x9ff8bfff` | SEC heap |
| `0x4` `EfiBootServicesData` | `0x9ff8c000` | `0x9ff8cfff` | CPU vectors |
| `0x4` `EfiBootServicesData` | `0x9ff8d000` | `0x9ff8ffff` | MMU page tables |
| `0x4` `EfiBootServicesData` | `0x9ff90000` | `0x9ffcffff` | UEFI stack |
| `0x6` `EfiRuntimeServicesData` | `0x9fff7000` | `0x9fffefff` | Log buffer |
| `0x6` `EfiRuntimeServicesData` | `0x9ffff000` | `0x9fffffff` | Info block |
| `0x0` `EfiReservedMemoryType` | `0xa0000000` | `0xafffffff` | XBL kernel reservation |

The first-bank addresses and memory types agree with the elish XBL-derived map and the public Xiaomi pipa Kona configuration. The XBL-derived elish map separately marks `0x9e400000+0x00f00000` as `NoHob/MMAP_IO`, so its absence from allocation HOB output is consistent with that configuration.

The frames also show FV HOBs for `0x9fc00000–0x9fefffff` and the decompressed firmware volume at `0x9b4f5000–0x9b862fff`. The memory-allocation rows include further DXE pool allocations, but this report records only the stable, legible ranges that match named platform regions.

## Scope

The 45-region count comes from the static `MemoryMapLib` table validated during PEI. The displayed memory-allocation HOBs describe memory used or allocated during PEI and passed to DXE. Neither observation is the complete `GetMemoryMap()` descriptor array. The frames do not show Qualcomm RAM Partition entries, the second DRAM bank's usable-range table, the map immediately before `ExitBootServices`, or DarwinBoot's selected 256 MiB physical window.

The custom `ElishMemoryMapProbeDxe` marker and its `RAM partitions` / `UEFI memory map count` lines are not visible in these frames. Its callback output remains unobserved. Keep the live firmware-allocation evidence separate from the unresolved final UEFI-to-XNU ownership.

## Image evidence

| Frame | SHA-256 |
| --- | --- |
| `/root/repo/IMG_3176(20261124-144243).PNG` | `7820563e3bed848eee286ff977613a4378300f3ded6cff4f65f2ba7e4fc49075` |
| `/root/repo/IMG_3177(20261124-144250).PNG` | `c12609861e874e5d1caad6af4dba15f292613e7dcce90ad54abd3683b7205f71` |
| `/root/repo/IMG_3182(20261124-145145).PNG` | `7273afdb51072ab843f6fdb0fb084734b1eaedbf8667d9a47aa81a25439ebd4f` |
| `/root/repo/IMG_3183(20261124-145203).PNG` | `9ce9fbc6c287d11e9670b77a6efa5f6cf97376c4bb90a05b75915522485d4bc6` |

The HOB row interpretation was checked against the Mu-Silicium DXE Core HOB walk and the elish PEI memory-map implementation at revision `66421cb7bb5694843aec8de1ca3d1de4c20cca38`. The pipa configuration is a public SM8250 Kona reference whose listed elish DDR rows match the extracted elish XBL configuration. Its values are cross-checks, while the screen supplies the live allocation observations.

- [Mu-Silicium DXE Core HOB walk](https://github.com/microsoft/mu_basecore/blob/cfb93724a06ff86ff5ba971413368a7809ee42a5/MdeModulePkg/Core/Dxe/DxeMain/DxeMain.c#L401-L435)
- [PI HOB definitions and memory-allocation semantics](https://github.com/microsoft/mu_basecore/blob/cfb93724a06ff86ff5ba971413368a7809ee42a5/MdePkg/Include/Pi/PiHob.h#L100-L158)
- [UEFI Boot Services `GetMemoryMap()`](https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-getmemorymap)
- [Project-Aloha pipa Kona memory map](https://github.com/Project-Aloha/mu_aloha_platforms/blob/a6f19eab272e9df368acbd0c246e90d9bab66505/Platforms/KonaPkg/Device/xiaomi-pipa/Binaries/RawFiles/uefiplat.cfg#L8-L33)
- [Qualcomm RAM Partition protocol](https://github.com/Project-Silicium/Mu-Silicium/blob/66421cb7bb5694843aec8de1ca3d1de4c20cca38/Silicon/Qualcomm/QcomPkg/Include/Protocol/EFIRamPartition.h#L62-L65)
