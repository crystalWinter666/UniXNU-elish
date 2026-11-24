# elish EnvDxe and HII initialization

## EnvDxe repair

The previous ramoops trace ended before the return from `CoreStartImage` for `EnvDxe`. The catalog PE32 matches the stock elish XBL version. Its `llvm-objdump -d` output shows the main routine at RVA `0x231c` calling a helper at `0x420c`. The helper looks up GUID `90a49afd-422f-08ae-9611-e788d3804845` and loads the pointer at HOB offset `0x18`. The argument GUID is stored at PE file offset `0xd0a8`. A null pointer leads to the `InfoBlkPtr != ((void *) 0)` assertion and the self-branch at `0x23a8`.

The elish DSC disabled `PcdBuildXblHobs`, which guards the shared `BuildXblHobs()` path in `Silicon/Qualcomm/QcomPkg/Library/PlatformPeiLib/PlatformPei.c`. The board now enables it. `BuildXblHobs()` derives the `Info Blk` HOB from the elish XBL memory map at `0x9ffff000`. No `FD Reserved` entry is enabled for this board, so the shared scheduler/DTB-extension scan returns without using a vendor scheduler pointer.

The corrected RAM boot reached `EnvDxe` and installed Qualcomm RAM Partition protocol `5172ffb5-4253-7d51-c641-a701f973103c`. It then dispatched the status router, Runtime, CPU, GIC, timers and `SmemDxe`. After a warm reset and `fastboot continue`, 14,317 bytes of raw pstore were captured with SHA-256 `a9fccb95e1989e0969840cb9bab22871d0309a5b59df36a4c6e8d9f9db8dc531`. [Firmware log](elish-uefi-info-hob-20261124.txt)

## Console dispatch

The same capture ends with `ASSERT [ConPlatformDxe] UefiHiiServicesLib.c(82)`. The failing call is `LocateProtocol` for the HII String protocol. The elish APRIORI list forced `ConPlatformDxe` to run before `HiiDatabaseDxe`, which publishes HII String, Database and Config Routing. The latter is now listed first, matching the order used by pipa and sheng.

Fastboot accepted the second image at 21:53:32 CST. USB disappeared at 21:53:36 and reappeared as bootloader fastboot at 21:54:11. After `fastboot continue`, Linux enumerated at 21:54:22 as `05c6:f000` with a Xiaomi identity and `File-Stor Gadget` mass storage, reporting no medium. The supplied ROM's `/etc/init/hw/init.qcom.usb.rc` in `vendor.img` assigns this ID to `sys.usb.config=mass_storage`. ADB did not become available for pstore extraction. The last UEFI stage of this second image remains unverified. Both runs used `fastboot boot`, with no partition writes.

## Build and artifacts

The core base is `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, the board base is `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`, the UEFI base is `66421cb7bb5694843aec8de1ca3d1de4c20cca38`, and Mu_Basecore is `cfb93724a06ff86ff5ba971413368a7809ee42a5`. Both images include local port changes. The output directories under the core's `out/elish/ipados-17.7.11-21H461/elish/boot/` retain the FD, image, manifests and command logs.

```sh
./unixnu build firmware -D /root/repo/UniXNU-elish \
  --uefi /root/repo/uefi-elish --edk2 /root/repo/uefi-elish/Mu_Basecore \
  --clang-bin /usr/lib/llvm-21/bin -j 8
./unixnu bootimg -D /root/repo/UniXNU-elish --uefi /root/repo/uefi-elish \
  --fd /path/to/SILICIUM_UEFI.fd --clang-bin /usr/lib/llvm-21/bin -o /path/to/boot.img
fastboot boot /path/to/boot.img
```

| Output directory | Boot image SHA-256 | Firmware SHA-256 |
| --- | --- | --- |
| `envdxe-info-hob` | `c95ecfd1fa9164b37be15857b71b6294a8e6b9ea7e2438f58646e5d4f5ac2b6c` | `8837905f1890c5e3a7b5ba9324980c0144964f647095b5dd67f91239e98e05ac` |
| `hii-before-console` | `ee86b326a8cc48ba11a3124a6342caa432c3cea124404a544a99525fbec51e0b` | `242ac664945bf2e54c7856f213f4d119fd3d5d5ecb02e5020b0e764ccac16ae0` |

## Memory capture before console startup

The DEBUG diagnostic build enables both `ELISH_MEMORY_MAP_PROBE` and `ELISH_RAMOOPS_LOG`. It runs `RamManagerDxe` and the existing map probe before the HII and console drivers, writes the Qualcomm RAM Partition entries and the UEFI DRAM descriptors to ramoops, then requests a warm restart into fastboot. This path uses no screen delay or input. Its map snapshot is taken before BDS. The production board configuration keeps `ELISH_MEMORY_MAP_PROBE=FALSE`.

The Android v3 boot image is in `memory-map-ramoops/boot.img`, 67,747,840 bytes, SHA-256 `82386ddda8c5d8923a3b71638273bb94faf9af1df870cd0e5a17ac36efc0f274`. The corresponding FD SHA-256 is `3c57bcedce797b515fd0b4cb4cb5ec8b18e67f255a463d49d0d8907f16676db8`. The built probe contains the pstore capture and restart markers. The device has yet to run this image. The package preflight for iPadOS 17.7.11 passed and resolved 63 patches.
