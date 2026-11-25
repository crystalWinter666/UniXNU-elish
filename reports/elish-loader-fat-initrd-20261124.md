# elish FAT discovery and accepted DarwinBoot memory map

Device: Xiaomi Pad 5 Pro Wi-Fi, M2105K81AC/elish, iPadOS 17.7.11 (21H461).
All tests used RAM-only `fastboot boot`; no device partition was written.
The last confirmed stage in run04 is successful ExitBootServices. That image
deliberately resets after capturing the accepted memory map and executes no XNU.

## Revisions and recovery

The working sources are device `4b3da68ea0aca815a80e8d1bad802dfec8d5c905`,
UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, DarwinBoot
`5243c68ec332a227fe3680cd5da6d5a0e127035c`, firmware
`f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`, and Mu_Basecore
`cfb93724a06ff86ff5ba971413368a7809ee42a5`, plus the uncommitted changes
described below. These are source baselines, not commits containing the fixes.

The output root is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124`.
`backups/boot_b.img` (192 MiB) and `backups/vendor_boot_b.img` (96 MiB) were
read through root ADB and their host/device hashes match:

| Input | SHA-256 |
| --- | --- |
| boot_b partition | `fce50614e0f90a4fd42b1eef26b913712883e4133723b802b4ac65a8dd1dd890` |
| vendor_boot_b partition | `0fe100295976bc1b2cfa08edc3b6598d0a8f5bca0b6eea59869e7424dba9b356` |
| lineage-reader-boot.img | `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4` |

The reader image removes only trailing boot-partition padding; its header,
kernel and ramdisk bytes match the installed boot backup. After each diagnostic
returns to enumerated fastboot, boot this reader image, wait for ADB, run
`adb root`, and export `/sys/fs/pstore/console-ramoops-0`. `fastboot continue`
previously led to USB mass storage or shutdown; it is not the recovery path.
Bootloader detection uses actual enumeration and `getvar product`, allowing
up to 120 seconds. A five-second delay is not evidence of reaching fastboot.

## Failure and direct layout evidence

The first loader probe image, SHA-256
`d440c851d242726bf5cbf4b030d8836941b089e806cee8f396013bf9fccac2a8`,
published an initrd block device but logged `config: no boot entry found` and
`timeout inside-loader=0`. Its 43,008-byte log has SHA-256
`d2f0b24b41c974bad1efd3882c10e9c85a276b9565910e17217827f6af2c001f`.
The same persistent log was recovered after the user powered Android back on;
it does not describe an additional UEFI run.

Parsing the active vendor_boot v3 header gives page size 4096 and compressed
vendor ramdisk size 18,811,579 (`0x11f0abb`). The gzip archive is 44,662,016
bytes uncompressed. The FDT initrd length `0x51f0abb` is exactly that compressed
size plus the 64 MiB raw FAT payload. Host `fsck.fat -vn` accepts the payload.

Run03's [complete layout log](elish-initrd-layout-20261124.txt) directly shows:

```text
ELISH_LOADER_PROBE: initrd start=0xAAE0E000 bytes=0x51F0ABB head=1F 8B 08 00 00 00 00 00 00 03 EC
ELISH_LOADER_PROBE: FAT boot sector offset=0x11F0ABB aligned512=0
UNIXNU: config: no boot entry found
ELISH_LOADER_PROBE: timeout inside-loader=0; warm restart
```

ABL prepends the vendor gzip ramdisk with no sector padding. Reading LBA 0
of the full composite initrd cannot discover the FAT boot sector. Run03 used
`run03-fat-layout-retry/boot.img`, 67,751,936 bytes, SHA-256
`8896cb744d8f66ee730fb8dbb2a56182566db1a59bf11ac64844f1d467ce6be0`;
FD SHA-256 `1f96add37b39e34ced597f7a2fefeaaacf737585cf6eb36ad8544bca3e267f98`.
The 43,174-byte log hash is
`fcb8ecba208fb1d6f05c0e5894ee1306d0ed276f4d7fc11e05199e106fbec271`.

## Implementation and run04

Shared `UniXNURamDiskDxe` now preserves its original full initrd disk and
looks for a structurally valid FAT32 suffix after a gzip prefix. It requires
consistent sector/cluster/FAT/root geometry and the complete FAT to end at
the original initrd end. The read-only child uses a media relative-offset
device path, including the unaligned byte offset. No board address or vendor
ramdisk length is hardcoded in the shared code.

DarwinBoot follows that relative-offset path to the raw parent, checks child
bounds against the original FDT size, and compares both devices' first/last
sectors to the corresponding RAM before marking the full input range verified.
An ordinary raw boot volume retains its previous validation. These changes live
in the core package and DarwinBoot component, not in board-specific hardware code.

The elish `ElishLoaderProbeDxe` is included only with DEBUG, ramoops, and
`ELISH_LOADER_PROBE=TRUE`. It intercepts the existing loader protocol and EBS,
uses a fixed memory-map buffer and direct ring writes, and resets after a
successful EBS. The board default remains `ELISH_LOADER_PROBE=FALSE`.
The probe's timeout is 20 seconds; the earlier 120-second setting belongs to
the separate memory-map probe.

Build command (same form for run03, with its separate output directory):

```sh
python3 scripts/build-loader-probe.py \
  --payload /root/elish-build-20261123/UniXNU/out/elish-resume/elish/ipados-17.7.11-21H461/elish/payload/payload.img \
  --dtb /tmp/elish-uefi-probe.dtb \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/run04-fat-child \
  --clang-bin /usr/lib/llvm-21/bin \
  --feeds /root/elish-build-20261123/UniXNU/out/elish-resume/feeds --jobs 8
```

Run04 used the unchanged payload SHA-256
`79ab59fda90509756a4b703295a5cc19c5b6932c6e66c2d25a5f864e983023ea`.
Its FD hash is `45e0211299368e1a96ab6b3b39dc2577a10158cfa1580b2a7ca25426c37b196b`.
Its 67,751,936-byte boot image hash is
`8ef2e0a78c37171b74d50601703aa94b2f8ec7ed7a1aed00fbb0aebfb2bb33e7`.
Full command timestamps are in `run04-fat-child/device-commands.json`.
Fastboot accepted it at 23:17:03 CST; fastboot was confirmed again at 23:17:20;
the reader restored ADB at 23:17:31 and root pstore export completed at 23:17:34.

The [complete run04 log](elish-darwinboot-ebs-20261124.txt), 45,984 bytes,
SHA-256 `507186f7e5b12ccb5b32c45c31b761c752fa329ff3a9812b8ef08109a9ad1f97`, shows:

```text
UNIXNURAMDISK: FAT child offset 0x11F0ABB size 0x4000000 Success
UNIXNU: config: loaded \EFI\UniXNU\unixnu.toml
UNIXNU: menu: shown 4 entries, default 0 'xnu', timeout 3000 ms, input yes, rotate 0 ms
ELISH_LOADER_PROBE: DarwinBoot request=9FFCE6F0
DARWINBOOT: ARM64 va=0xFFFFFFF00427C000 pa=0xC007C000 args=0xC40B4000 entry=0xC321A488 owned=0x10000000
DARWINBOOT: window head=0x6040000 usable=0x10000000 excluded ranges=0 of 16
ELISH_LOADER_PROBE: EBS call image=9AC0CC18 requested-key=0x3EE snapshot=Success key=0x3EE bytes=0xDB0 stride=0x30 version=1
ELISH_LOADER_PROBE: EBS returned Success accepted-snapshot=1
ELISH_LOADER_PROBE: capture complete after EBS; XNU not entered
```

The accepted map has 73 descriptors (`0xdb0 / 0x30`). The 256 MiB window is
`0xc007c000–0xd007bfff`: its `0x6040000`-byte allocated head is EfiLoaderCode,
and the rest is EfiConventionalMemory. Both are in the measured second DRAM
bank. The composite initrd is outside this window in the existing reserved
`0xa0000000–0xafffffff` range. GOP reports 1600×2560, 6400 bytes/row, 32-bit pixels,
at `0x9c000000`; XNU presentation has not been tested.

Host checks passed: `scripts/test-initrd-fat-suffix.py` (real FAT32, unaligned
prefixes, truncation and malformed BPB), DarwinBoot `Test/test-payload-disk.py`
(ASan/UBSan, parent identity/bounds/read corruption), and `Test/test-loader-inputs.py`
(1,200 random maps). The latter's memcpy-based unaligned test stubs now use
void pointers to avoid Clang 21 attaching an alignment requirement to the stub.
Linux loads its UBSan runtime with `LD_PRELOAD` when running that existing
ctypes-based test. Both diagnostic firmware/image builds passed; initial local
layout-probe build failures were missing Trace declaration and FdtLib/HobLib INF
dependencies, corrected before any device run.

Run04's ADT did not request `darwinboot-loader-inputs`, so it establishes FAT
discovery and successful EBS, not execution of the new runtime input verifier.
Run05 rebuilt the payload after `board.toml` set
`payload.adt.loader_inputs=true`; a host parser readback of the Apple ADT
confirmed `/chosen/darwinboot-loader-inputs` contains exactly 64 zero bytes.
The device-side record is still unknown without run05 pstore. Its temporary
kernel copy was patched at `0xfffffff00741c350`, before MMU enable, to append
`ELISH_XNU_CHECKPOINT: before-mmu` to the preinitialized ramoops console and
request warm fastboot reset. The patched kernel is 51,122,176 bytes, SHA-256
`91fa2b034920a7beb8b8892d3067236875140634699b3bdd00ba6184e07249ed`; readback
from the FAT image has the same hash. The 64 MiB run05 payload hash is
`2a7271e25e7a79cd231d8f934c22cb3047cc0ff8a2ed234c2c163ce161cb2076` and FD hash
is `961fc9dc41492b5519a5cbe32123f85a2860ac321f46c07bcd0eafb2ec76fe3b`.
Fastboot accepted the image at 23:25:18 CST. No fastboot device was detected
within 120 seconds; no pstore from this run has been recovered. The user was
asked to reboot directly to LineageOS or root Recovery, after which pstore must
be read before any new firmware boot. This is not evidence that the checkpoint
ran. The run05 terminal probe and firmware `--run-kernel` option are diagnostic
only and must not be used as a production image.

Kernel qualification (`loadable=false`), storage, APFS root, input, GPU and
user-space startup remain unverified.
