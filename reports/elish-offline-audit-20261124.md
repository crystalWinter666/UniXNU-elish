# elish offline memory, kernel and UFS audit

## Scope and pinned inputs

This audit used the existing device records, the stock ROM extracts, and the
host build trees. No ADB, fastboot, USB-enumeration, image-write, or device
boot command was run.

The board configuration now sets `ELISH_MEMORY_MAP_PROBE=FALSE`, so routine
firmware builds omit the ReadyToBoot diagnostic DXE.

| Input | Revision or identity |
| --- | --- |
| Device repository | `208b47d20e87d3876fd862bb1f0ee0c17467b0c2` |
| UniXNU core and DarwinBoot | `71c5c6569bae6369c0e0f634df8b9bd5afc19139` |
| elish Mu-Silicium UEFI source | `66421cb7bb5694843aec8de1ca3d1de4c20cca38` |
| NativeUfs-alioth reference | `336680c353e6c3f25d6b29284532c4fd16104f68` |
| Target XNU | `10063.141.1`, revision `d8b80295118ef25ac3a784134bcf95cd8e88109f` |

## Memory handoff findings

The existing live-FDT capture reports DRAM at
`0x80000000+0x3bb00000` and `0xc0000000+0x1c0000000`, with a gap between the
banks. The selected iPadOS profile does not provide the excluded-range XNU
support used by newer profiles. A source search in the pinned XNU worktree
found no parser for `darwinboot-memory-excluded`.

The current board request is a 256 MiB DarwinBoot window and does not request
an excluded-range table. In that path, `DarwinBoot.c` calls `AllocateWindow`,
which finds one `EfiConventionalMemory` run large enough for the entire window
and allocates all of it as `EfiLoaderCode`. The loader then reports that full
window in `boot_args.memSize` and `memSizeActual`. It does not combine free
pieces across reservations or the bank gap. The allocation decision uses the
live UEFI map at runtime, so the host does not yet know the selected physical
base.

The target XNU's `arm_vm_init` takes `physBase` and `memSize` as one physical
span. `memSizeActual` supplies the physical-memory accounting value. This
supports the existing single-window configuration and rules out assuming the
newer excluded-range table is consumed by this OS profile. It does not
establish which bank the current UEFI build will allocate from. The strict
memory-map state therefore remains `unverified`.

## UFS source review

The stock elish DT identifies the generic `qcom,ufshc` host at
`0x1d84000+0x3000`, ICE at `0x1d90000+0x8000`, and QMP v4 PHY at
`0x1d87000+0xe00`. It provides no UFS `iommus` assignment.

The pinned SM8250 common SoC package uses Linux v6.6 DTSI revision
`5b0764a53235d3c77e1ed674e836e1b3fe1a4b1f` as its register source. The matching
`gcc-sm8250.c` identifies UFS PHY branch registers at `0x77018` (AHB),
`0x77010` (AXI), `0x77064` (ICE), `0x7709c` (PHY auxiliary), `0x7701c`
(TX lane 0), `0x77020` and `0x770b8` (RX lanes 0 and 1), `0x7705c` (UniPro),
and `0x770cc` (aggregate UFS PHY AXI). Its reset table gives UFS card and PHY
block resets at `0x75000` and `0x77000`. The source file SHA-256 is
`b3b3865de3a297f906d5562a720d8d304538cef3c8d3703dee026cfd6dba06c8`; the
matching `qcom,gcc-sm8250.h` binding SHA-256 is
`34682c01e5b21bb13db125afa3504c73606c294c23cfb3a96f6ea52cdffd2ff7`.

The stock elish DT declares nine named clocks and one `core_reset`, but its GCC
clock specifiers do not match several IDs in the pinned upstream SM8250
binding. For example, the stock overlay uses GCC clock IDs `0xa7`, `0x06`,
`0xa6`, `0xb3`, `0xaa`, `0xb2`, `0xb0` and `0xb1` for the controller clocks.
Those values must be resolved against the exact Xiaomi GCC provider before
assigning rates or writing registers. The stock reset specifier is `<0x16
0x21>`, which is reset ID 33. Keep the stock provider wiring and the upstream
Linux SoC register baseline as separate evidence until their binding relation
is established.

NativeUfs-alioth is an SM8250 adaptation reference. Its UFS personality accepts
`qcom,ufshc`, while its stream IDs `0xe0` and `0x4e0` are selected through boot
arguments and its defaults still select stream `0x60`. The README attributes
the two SM8250 stream IDs to alioth's existing SMMU mappings. The elish stock
DT supplies no evidence for those mappings. Its `GccUfs.h` labels the clock
register definitions as SM8550-derived, and several offsets differ from the
pinned SM8250 Linux GCC source. Reset, RPMh, TCSR and QMP-PHY programming also
remain undefined. The reference is useful for transport and command-path
review, but it is not ready to select for elish or enable in the board
configuration.

## Kernel qualification findings

The host candidate is 51,122,176 bytes with SHA-256
`759574c4d0a6c8cbd72bf8a8e1026c59390712d1a819a826379a0512d21ed8ec`. Its
report inventories 260,779 preserved PC-relative encodings, 69,294 pointer
bindings and 119 component extents. It records
`instruction_relocation_complete=false`,
`register_dataflow_qualified=false`, `abi_qualified=false`,
`runtime_qualified=false` and `loadable=false`.

At the pinned core revision, the prelink emitters preserve executable bytes
and explicitly leave the qualification fields false. These fields represent
missing relocation, register-dataflow, ABI and runtime proof. Editing the JSON
report cannot provide those proofs. The existing 65 XnuPrelink host tests pass,
but they do not qualify this candidate for loading.

## Host verification

The board now sets `ELISH_MEMORY_MAP_PROBE=FALSE` for routine firmware builds.
The standard elish FD rebuilt at 3,145,728 bytes with SHA-256
`5c1cc81f58df5e255ea2e87e29169ae17e3f8aecbce464feb9ff16df13ba0b3e`. The
generated `FVMAIN.Fv.txt`, `FVMAIN.Fv.map` and `FVMAIN.inf` contain no
`ElishMemoryMapProbeDxe` entry. The build used Mu-Silicium revision
`66421cb7bb5694843aec8de1ca3d1de4c20cca38` and Mu_Basecore revision
`cfb93724a06ff86ff5ba971413368a7809ee42a5`.

Command:

```sh
./unixnu build firmware -D /root/repo/UniXNU-elish --uefi /root/repo/uefi-elish --edk2 /root/repo/uefi-elish/Mu_Basecore -j 8
```

The DarwinBoot tests passed for memory-window selection, ADT parsing, and
loader-input range handling. The host uses Debian Clang 21. The test shims use
unaligned buffer writes that trigger Clang's alignment sanitizer, so the test
commands used a temporary Clang wrapper to disable only alignment checks for
the shim and preloaded the matching UBSan runtime. The memory-window test also
compared 400 generated maps against its reference model. The loader-input test
compared 1,200 generated maps against its model.

Commands run:

```sh
PATH=/tmp/elish-clang-noalign:$PATH LD_PRELOAD=/usr/lib/llvm-21/lib/clang/21/lib/linux/libclang_rt.ubsan_standalone-x86_64.so python3 Test/test-memory-window.py
PATH=/tmp/elish-clang-noalign:$PATH LD_PRELOAD=/usr/lib/llvm-21/lib/clang/21/lib/linux/libclang_rt.ubsan_standalone-x86_64.so python3 Test/test-adt-window.py
PATH=/tmp/elish-clang-noalign:$PATH LD_PRELOAD=/usr/lib/llvm-21/lib/clang/21/lib/linux/libclang_rt.ubsan_standalone-x86_64.so python3 Test/test-loader-inputs.py
python3 -m unittest discover -s kits/XnuPrelink -p 'test_*.py'
python3 scripts/check-elish.py --static-only
```

The first three tests reported `PASS`. XnuPrelink ran 65 tests with `OK`.
Static elish preflight passed. Strict preflight remains blocked by the
unverified runtime memory handoff.

## Remaining offline work

- Keep the elish physical-bank inputs tied to the captured live FDT and the
  DarwinBoot full-window allocation contract. Runtime UEFI map ownership is
  still unverified.
- Continue source-level XnuPrelink qualification work. The current candidate
  remains experimental and `loadable=false`.
- Separate SM8250 UFS transport logic from alioth-specific streams and
  SM8550-derived clock inputs. Keep UFS disabled until elish ownership and
  initialization inputs are established.
- Re-run strict preflight, then build the iPadOS payload and boot image after
  the memory and kernel gates are satisfied.

## Stock UFS DXE dependency audit

The PE32 section extracted from the elish stock XBL has SHA-256
`ffeaed467f8251b7e30a45075da84a1c563e2bac7dc4be0f79f0ed49a88b8dc9` and
size 118,784 bytes. It is byte-identical to
`Binaries/alioth/QcomPkg/Drivers/UFSDxe/UFSDxe.efi` and
`Binaries/pipa/QcomPkg/Drivers/UFSDxe/UFSDxe.efi` in the checked-out
Mu-Silicium repository. Both catalog copies also have an identical 36-byte
`UFSDxe.depex`, SHA-256
`d24ba46b9b195a4affe0c6d783f99361fb08fa85894d0e9ec3a070a995ef5d44`.
The DEPEX expression pushes protocol GUIDs
`f4e5c7d0-d239-47cb-aacd-7f66ef763238` and
`fa5f306b-f47d-4ac4-a47d-882f8204ec30`, then ANDs them before dispatch.

Ghidra 12.2 headless analysis of the alioth catalog modules resolved the
providers. `SmemDxe.efi` references the first GUID in a function that calls
`EFI_BOOT_SERVICES.InstallMultipleProtocolInterfaces`. The elish stock
`SmemDxe` PE32 was independently recorded with the same SHA-256 as the alioth
and pipa copies, `4ad21b61ab64d1d91403403c8ce23b60648a2fb29c781fe4a7cb5a3996d727aa`.
`SPMIDxe.efi` references and installs the second GUID in the same Boot
Services call slot. The alioth and pipa `SPMIDxe.efi` hashes both equal
`b062b30ffe231da3333c1a4a7ba57896f3e59e0d6d5478e8e89c259b8f63766f`.
`DALSYSDxe.depex` also waits for the first GUID. The exact SPMI module in the
elish stock FV has not yet been hash-compared with the catalog copies.

The UFS PE32 strings also identify dynamic lookup of `NpaProtocol`, UFS IOMMU
domain setup and attachment for ARIDs `0x0` and `0x10`, the option names
`UfsSmmuConfigForOtherBootDev` and `EnableUfsIOC`, and nine GCC UFS/PHY clock
names. The embedded XBL `uefiplat.cfg` has a `## UFS ##` section at file offset
`0xca06d` with `EnableUfsIOC = 0` and
`UfsSmmuConfigForOtherBootDev = 1`. The elish
`ConfigurationMapLib.c` contains the same values. These settings do not
establish the active stream IDs, context ownership, clock rates, reset state,
RPMh requests, TCSR programming or QMP PHY state.

The Ghidra project is `/tmp/elish-ghidra-projects/ElishUfs`; the script is
`/tmp/elish-ghidra-scripts/AuditUfsProtocols.java`. The analysis command was:

```sh
JAVA_HOME=/opt/jdk-25 PATH=/opt/jdk-25/bin:$PATH \
  /opt/ghidra-12.2/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  /tmp/elish-ghidra-projects ElishUfs \
  -import /root/repo/uefi-elish/Binaries/alioth/QcomPkg/Drivers/UFSDxe/UFSDxe.efi \
  -import /root/repo/uefi-elish/Binaries/alioth/QcomPkg/Drivers/SmemDxe/SmemDxe.efi \
  -import /root/repo/uefi-elish/Binaries/alioth/QcomPkg/Drivers/SPMIDxe/SPMIDxe.efi \
  -overwrite -analysisTimeoutPerFile 180 -max-cpu 8 \
  -scriptPath /tmp/elish-ghidra-scripts -postScript AuditUfsProtocols.java
```

The stock UFS module and its protocol providers are useful firmware integration
inputs. Firmware integration still needs the elish runtime resources and the
remaining platform initialization inputs audited before the UFS path is ready.

The provider call slots were checked against the pinned Mu_Basecore
`MdePkg/Include/Uefi/UefiSpec.h`. Both modules initialize `gBS` from the
`EFI_SYSTEM_TABLE.BootServices` field at `gST + 0x60`. The call at
`gBS + 0x148` is `InstallMultipleProtocolInterfaces`; `gBS + 0x140` is
`LocateProtocol`. This verifies the provider routines' protocol-install and
dynamic-lookup roles in the catalog binaries.

## Stock XBL UFS protocol-chain resolution

The gzip stream in `xbl.img` begins at file offset `0xca830`. RFC 1952
inflation produces 6,656,008 bytes containing an FV at offset `0x8`, with FV
length `0x659000` and header length `0x48`. Parsing the FV yields 147 FFS
files. The target DXE PE32 sections are:

| XBL UI name | PE32 bytes | PE32 SHA-256 | Catalog comparison |
| --- | ---: | --- | --- |
| `SmemDxe` | 40,960 | `4ad21b61ab64d1d91403403c8ce23b60648a2fb29c781fe4a7cb5a3996d727aa` | Matches alioth and pipa `SmemDxe` |
| `SPMI` | 45,056 | `b062b30ffe231da3333c1a4a7ba57896f3e59e0d6d5478e8e89c259b8f63766f` | Matches alioth and pipa `SPMIDxe` |
| `NpaDxe` | 81,920 | `8c31a7f77bb0c6f0944ef3f55590df27d7a03d4a55863883536c7514f4bd7f32` | Matches alioth and pipa `NpaDxe` |
| `UFSDxe` | 118,784 | `ffeaed467f8251b7e30a45075da84a1c563e2bac7dc4be0f79f0ed49a88b8dc9` | Matches alioth and pipa `UFSDxe` |
| `DALSys` | 344,064 | `e6b151c5506f52196e6ea687d15ffa074b7e3d518e8c52621ef62413000a8204` | Stock elish variant; alioth/pipa hashes differ |

The elish `UFSDxe` FFS DEPEX is 36 bytes and its SHA-256 matches the alioth
and pipa catalog DEPEX at
`d24ba46b9b195a4affe0c6d783f99361fb08fa85894d0e9ec3a070a995ef5d44`. It
requires protocol GUIDs `f4e5c7d0-d239-47cb-aacd-7f66ef763238` and
`fa5f306b-f47d-4ac4-a47d-882f8204ec30`. The stock `SmemDxe` and `SPMI`
modules install these interfaces through `InstallMultipleProtocolInterfaces`.
The stock `DALSys` FFS DEPEX waits on the first GUID. `SmemDxe`, `SPMI` and
`NpaDxe` each have a `TRUE` DEPEX.

Ghidra analysis of the extracted elish `UFSDxe` shows that
`FUN_00014318` calls `gBS + 0x140` (`LocateProtocol`) with a GUID at
`0x192c8`. That GUID is `79d6c870-725e-489e-a0a1-27e7a5d0cb35`. In elish
`NpaDxe`, `FUN_000013dc` calls `gBS + 0x148`
(`InstallMultipleProtocolInterfaces`) with the same GUID at `0x11048` and an
interface at `0x110a8`. The `NpaDxe` global `gBS` comes from `gST + 0x60`.
This statically closes the stock UFS driver's NPA protocol lookup path.

The stock module extraction command is `python3 /tmp/elish-audit-xbl-dxe.py`.
It writes the extracted PE32 sections under `/tmp/elish-stock-*.efi`. Ghidra
12.2 imported the elish `UFSDxe` and `NpaDxe` modules and ran
`FindNpaProtocol.java`, `FindNpaInstall.java`, `ReadNpaGuid.java`,
`AuditNpaProvider.java` and `InspectNpaBootServices.java` from
`/tmp/elish-ghidra-scripts`. The project is
`/tmp/elish-ghidra-projects/ElishStockUfs`.

The stock DXE dispatch and NPA protocol dependencies are now identified. A
read-only SHA-256 comparison also found exact pipa and alioth catalog matches
for the elish XBL PE32 sections of `ClockDxe`, `CmdDbDxe`, `PwrUtilsDxe`,
`RpmhDxe`, `HWIODxeDriver` (catalog module `HWIODxe`), and `VcsDxe`, in
addition to `SmemDxe`, `SPMI`, `NpaDxe` and `UFSDxe`. The elish `DALSys` PE32
is a board-specific variant and does not match either catalog copy. These
hash matches identify usable firmware-source candidates. Runtime UFS
initialization, controller/PHY clocks, SMMU ownership and the native XNU
UFS/APFS root path remain unverified or unimplemented.

## Stock UFS configuration and module comparison

The `## UFS ##` block in the supplied XBL's embedded `uefiplat.cfg` records
`EnableUfsIOC = 0` and `UfsSmmuConfigForOtherBootDev = 1`. The local elish UEFI
configuration map already carries both values. The corresponding entries in
the pipa and alioth maps use `1` for both options, so those board maps cannot
be substituted for elish.

The XBL FV PE32 sections for the listed provider modules were hashed directly
from the read-only extraction and compared with the catalog binaries. Exact
matches were found for `SmemDxe`, `SPMI`, `NpaDxe`, `UFSDxe`, `ClockDxe`,
`CmdDbDxe`, `PwrUtilsDxe`, `RpmhDxe`, `HWIODxeDriver`/`HWIODxe`, and `VcsDxe`
in both pipa and alioth. `DALSys` has SHA-256
`e6b151c5506f52196e6ea687d15ffa074b7e3d518e8c52621ef62413000a8204`, distinct
from the catalog variants. A matching module hash does not show that the
module set has been integrated into this platform or that the UFS controller
is initialized at the point UniXNU takes control.

## DarwinBoot kernel-structure check

The pinned DarwinBoot parser reads an ARM entry point from `LC_UNIXTHREAD`,
not `LC_MAIN`. Running its production parser test against the built elish
`kernel.macho` passed. The candidate has 18 `LC_SEGMENT_64` commands, a valid
thread-state entry in executable memory, aligned segment ranges and a total
virtual span within DarwinBoot's 256 MiB window limit.

Command, from the pinned UniXNU checkout:

```sh
python3 drivers/DarwinBoot/Test/test-kernel-parser.py \
  out/elish/ipados-17.7.11-21H461/elish/kernel/kernel.macho
```

The test reported `PASS: actual kernel` and passed the parser's architecture,
entry and segment-bound checks. Its synthetic controls also passed for
arm64/arm64e subtypes and malformed ranges. This establishes that the candidate
matches the loader's structural Mach-O requirements. It does not change
`instruction_relocation_complete=false`, `register_dataflow_qualified=false`,
`abi_qualified=false`, `runtime_qualified=false` or `loadable=false` in
`kernel.json`.
