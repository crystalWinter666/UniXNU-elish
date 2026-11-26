# elish run46 XNU-owned UFS DMA

The RAM-only image qualified the elish GCC, SMMU and UFSHCI state, ran NOP and READ CAPACITY(10) on LUN 0, and verified all seven 4 KiB read CRCs. Capacity matched 61,880,320 blocks. The terminal screen showed `elish UFS known reads complete`, `reads=7 stopped=1 faults-same=1`, and final LBA `8909823` with CRC `c71c0011`. The DMA arena started at `0xD0132000`. The SCSI command and response used LUN at UPIU byte 2 and Task Tag at byte 3. Only NOP, READ CAPACITY and READ(10) were submitted.

The checks covered LBA 1 and the six run42 userdata samples. Those samples represent the current Android media, where the target manifest and APFS signatures are absent. This run did not publish a block device or attempt to mount an APFS volume. The terminal screen parked until the device was restarted. Root ADB later found an empty `/sys/fs/pstore`. The screen and USB transcript are the device evidence for this run.

## Build and artifacts

Device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, and UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139` with local changes. XNU source `d8b80295118ef25ac3a784134bcf95cd8e88109f`. The build followed the run43 procedure with the corrected UPIU layout and the run45 configuration. The FAT retained the Restore sidecars. [Build commands](elish-run43-xnu-ufs-20261126.md#build-and-device-commands)

Output directory `/tmp/elish-run46-xnu-ufs-upiu-20261126`.

| Artifact | SHA-256 |
| --- | --- |
| `boot/boot.img` | `bd53c67b772303e619f7e3ee99beadc53fde65b30ad348dfb77271c637e6d08a` |
| `boot/SILICIUM_UEFI.fd` | `ae843a835d99ed911901f2bd860ff2039811ac39068fa880c87f2f4a8104a561` |
| `payload.img` | `7f05a3b96575465e4419eb0f0d74b7642ce4b0d75e77d987714fa01ce5134e5c` |
| `kernel.macho` | `6eec25d4e31f593411809a692cd2774f2b7585c42a91a3b6d8449c78a74946ee` |
| `package/ElishUfs.cpp` | `966d995465014bcad113f217878806d9a39f7045caccfbdfb13c463776b3a31d` |
| `screen-fresh-22s.jpg` | `a8a7f99247b2765203be04624a6b6fc51a6fa575402c87cc34c054c4e6a4f7d0` |

At 03:53:48 CST, `lsusb -d 18d1:d00d` and `fastboot devices -l` identified bootloader serial `4bed07e4`. `getvar product` returned `elish` and `getvar is-userspace` returned `no`. `fastboot -s 4bed07e4 boot /tmp/elish-run46-xnu-ufs-upiu-20261126/boot/boot.img` returned both `OKAY` results in 5.523 seconds. The read transmission did not modify a partition.
