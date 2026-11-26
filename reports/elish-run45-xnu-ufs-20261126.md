# elish run45 XNU UFS command probe

XNU passed the SMMU register, exact stream route, CB2/CB3 and UFSHCI guards. It allocated the contiguous arena at `0xD0131000`, replaced the idle transfer list, and advanced past the NOP check. The terminal screen showed `READ CAPACITY(10)`, `reads=0 stopped=1 faults-same=1`. The failure arose within command submission or completion validation. Capacity decoding and the seven known-block reads were not reached.

Comparison against the run40 firmware path and the UPIU layouts in `MdePkg/Include/IndustryStandard/Ufs.h` identified a header error. Command UPIU byte 2 is LUN and byte 3 is Task Tag. Run45 placed `0x38` in byte 2, making the encoded LUN `0x38` and leaving the Tag zero. NOP byte 2 is reserved and byte 3 is Task Tag. The run45 NOP check compared the reserved response byte. Its advance confirms controller/descriptor activity, with a protocol-valid NOP still to be checked. Run46 corrects the wire offsets and prints completion registers, OCS, and the response header.

Before boot, USB `18d1:d00d` reported serial `4bed07e4`, product `elish`, and `is-userspace=no`. `fastboot boot` transferred and started the image in 5.544 seconds. The probe uses NOP and SCSI read opcodes. It issued no partition writes. The device parked on the terminal screen and USB exposed neither fastboot nor ADB at the post-run check.

## Build and artifacts

Device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, and UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139` with local probe and build changes. XNU source `d8b80295118ef25ac3a784134bcf95cd8e88109f`. The target kernel built and prelinked, FAT Mach-O readback and `fsck.fat -vn` passed, and the `--ufs-run-kernel` firmware build and boot image assembly passed.

Output directory `/tmp/elish-run45-xnu-ufs-known-reads-20261126`.

| Artifact | SHA-256 |
| --- | --- |
| `boot/boot.img` (235,528,192 bytes) | `78ecece0001a9b15e3819b73fb8b83877b5b4bcba15497de96453c8092492dd9` |
| `boot/SILICIUM_UEFI.fd` | `38d02efe1be8e8d0c9f46f27322fc59bc453e60abf893e9f171416cf2ea0714d` |
| `payload.img` | `c1cb5f900077316ca51eac28c4370e40426996b31024bd37c2ca5a90e6b95552` |
| `kernel.macho` with panic screen | `1fdac09532b509d843d6cd6312ff33e8aeb55d27bd8a54d934206f87ec2449d9` |
| Run45 `ElishUfs.cpp` | `d38581c7b3539bcd450fd59795df8434c9ccc5e490263dfbf60355cec7bc00e4` |
| `screen-25s.jpg` | `cacfb5c53f7dff5e62edaabc120a5c85962a3fb9f9414cedf3fd3834e792d842` |

The run43 build commands apply with the new output directory. For this run, the prior diagnostic FAT image supplied the entry arguments, ADT, trust cache and Restore ramdisk. Only the prelinked kernel changed. Capture paths and commands are in the run directory. [Build commands](elish-run43-xnu-ufs-20261126.md#build-and-device-commands)

```sh
FB=/tmp/elish-platform-tools/platform-tools/fastboot
lsusb -d 18d1:d00d
"$FB" devices -l
"$FB" -s 4bed07e4 getvar product
"$FB" -s 4bed07e4 getvar is-userspace
"$FB" -s 4bed07e4 boot /tmp/elish-run45-xnu-ufs-known-reads-20261126/boot/boot.img
```
