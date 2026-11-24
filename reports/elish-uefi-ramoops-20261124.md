# elish UEFI log capture through ramoops

## Reference implementations

- enuma documents the flow that also worked on elish. UEFI writes to the Linux ramoops console, a warm reset preserves RAM, and `fastboot continue` starts Android so root ADB can export `console-ramoops-0`. [enuma firmware log capture](https://github.com/wh1stl3/UniXNU-enuma/blob/74dc21dd5c323a763a225f98a6eefc3815272046/docs/firmware-log-capture.md)
- pipa uses the same channel with a dedicated Linux reader. Its DXE order starts `EnvDxe` before `SmemDxe` and runs `RamManagerDxe` before the later vendor modules to add the ranges returned by the Qualcomm RAM Partition protocol. [pipa RamoopsLogLib](https://github.com/xVanTuring/uefi-pipa/blob/1f6a1520f84c699a6f748e7b0b0ee72b79c7997b/Platforms/Xiaomi/pipaPkg/Library/RamoopsLogLib/RamoopsLogLib.c) · [pipa DXE order](https://github.com/xVanTuring/uefi-pipa/blob/1f6a1520f84c699a6f748e7b0b0ee72b79c7997b/Platforms/Xiaomi/pipaPkg/Include/APRIORI.inc)
- sheng preserves tagged captures using a timed RAM boot and its Linux reader. The ABL memory report records adopted pages and reported memory after boot. The UFS report records XNU storage windows and APFS mount results. [sheng unattended capture](https://github.com/UniXNU/UniXNU-sheng/blob/c8f92b208068eeaa4cd2b0f592671f04ee70076e/reports/macos-unattended-desktop.md) · [ABL memory](https://github.com/UniXNU/UniXNU-sheng/blob/c8f92b208068eeaa4cd2b0f592671f04ee70076e/reports/macos27-abl-memory.md) · [UFS windows](https://github.com/UniXNU/UniXNU-sheng/blob/c8f92b208068eeaa4cd2b0f592671f04ee70076e/reports/macos27-internal-storage.md)

## elish capture

Root ADB read Linux ramoops module parameters on elish. The base is `0xb0000000`, with a 4 MiB reservation, a 2 MiB console, a 2 MiB pmsg zone and ECC disabled. The local DEBUG firmware uses `ELISH_RAMOOPS_LOG=TRUE` and `ELISH_MEMORY_MAP_PROBE=FALSE`. The first export contained 5,986 bytes, including binary data. The framebuffer serial loop had advanced `Buffer` before passing it to `RamLogWrite`. Indexing it with `Buffer[i]` preserves the original pointer and produced a readable capture at the same length.

Commands run from the pinned UniXNU build checkout. Run `fastboot continue` after the diagnostic firmware returns to fastboot.

```sh
./unixnu build firmware -D /root/repo/UniXNU-elish \
  --uefi /root/repo/uefi-elish --edk2 /root/repo/uefi-elish/Mu_Basecore \
  --clang-bin /usr/lib/llvm-21/bin -j 8
./unixnu bootimg -D /root/repo/UniXNU-elish \
  --uefi /root/repo/uefi-elish --clang-bin /usr/lib/llvm-21/bin -o /path/to/boot.img
fastboot boot /path/to/boot.img
fastboot continue
adb wait-for-device
adb root
adb wait-for-device
adb exec-out cat /sys/fs/pstore/console-ramoops-0 > console-ramoops.txt
```

Core is `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, elish device base is `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`, firmware base is `66421cb7bb5694843aec8de1ca3d1de4c20cca38` and Mu_Basecore base is `cfb93724a06ff86ff5ba971413368a7809ee42a5`. The builds use local device and firmware edits. Each boot image uses Android header v3 and the existing 64 MiB payload. The two runs used `fastboot boot` and returned to LineageOS through `fastboot continue` without a flash command.

Images, manifests and original console bytes are under `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/`. The text files in this repository normalize line endings.

| Run directory | Boot image SHA-256 | Raw log bytes | Raw log SHA-256 |
| --- | --- | ---: | --- |
| `ramoops-fixed` | `47e0b07cc3bd531c7a10b30e1321dd5f92291332f9f7d0ce95ec8fe153652dc5` | 5,986 | `bc49277cce9cfc57cc7b61bf614e051c3f53b2332c5012783ef663337af5759d` |
| `ramoops-envdxe` | `74109d3b01324367eb0a03ddb518a47937068a4680b442788a49ff72ec2ce892` | 6,184 | `652d784b0c558c7befac1be80393de700d70726a0e0150252c54a005b8586b3d` |

## Confirmed stage

The fixed capture records SEC MMU setup, HOB creation, firmware-volume decompression, DXE Core entry, the HOB allocation walk, and DXE loading of `PcdDxe.efi` and `EnvDxe.efi`. A second run added temporary `DEBUG_ERROR` lines around the dispatcher's `CoreStartImage` call. `PcdDxe` returned `Success`. The last line is the begin marker for `EnvDxe`, GUID `90A49AFD-422F-08AE-9611-E788D3804845`. Its return status and the reset cause remain unobserved. The temporary dispatcher patch is retained with that run's image and was removed from the Mu_Basecore checkout. [First readable log](elish-uefi-console-ramoops-20261124.txt) · [Dispatch trace](elish-uefi-envdxe-dispatch-20261124.txt)

The enuma APRIORI audit also supplied a method to check the elish stock order. Inflate the gzip stream at `0xca830` in the supplied `xbl.img`, parse the firmware volume at decompressed offset `0x8` with `uefi_firmware` 1.16, and read the raw section of FFS file `fc510ee7-ffdc-11d4-bd41-0080c73c8881` as successive little-endian GUIDs. Resolve each GUID to its FFS UI name. The elish table has 84 entries. `DxeCore` is first, `EnvDxe` second and `SmemDxe` tenth. Stock elish, enuma and pipa all place `EnvDxe` ahead of `SmemDxe`. [enuma OEM APRIORI audit](https://github.com/wh1stl3/UniXNU-enuma/blob/74dc21dd5c323a763a225f98a6eefc3815272046/scripts/audit_oem_apriori.py)

The firmware loads the same `EnvDxe` PE32 found in the installed elish XBL. It is the candidate producer of the Qualcomm RAM Partition protocol. Its initialization must progress before the existing memory-map probe can collect the RAM Partition entries and a later UEFI map. DarwinBoot's selected window and the `ExitBootServices` map also remain pending. Root ADB already measured the Linux FDT DRAM banks and verified the `userdata` geometry from both GPT copies. [Memory and GPT measurements](elish-live-hardware-20261123.md)
