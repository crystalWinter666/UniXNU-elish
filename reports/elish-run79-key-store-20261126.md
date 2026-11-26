# elish run79 AppleKeyStore on RAM Data

Run79 displayed the original iPadOS 17.7.11 (`21H461`) Setup Assistant Hello screen on the Xiaomi Pad 5 Pro Wi-Fi (`elish`, `M2105K81AC`, SM8250-AC) internal panel. The camera captured `hello`, `Swipe up to open`, the home indicator and a later greeting in another language. [Hello photo](elish-run79-hello-20261126.jpg)

Run78 trapped in the original 21H461 `liveactivitiesd` after `MKBEventsRegister` returned null. Run79 links the shared `AppleKeyStoreState` at `8c70daf814aafdf4a3d35e40dde0322655fd0a83`. `elish-aks=1` enables service publication after the writable Data mount, gated on the ephemeral-storage property and both platform identity properties. The board supplies the software wrapping identity and the no-passcode state for the RAM volume. The diagnostic adds publication status and open, call and refusal counts alongside the process and GOP counters.

## Build

Device baseline `16cedc7b8a9a417e3994e37f420dc6ec485a7da1`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`, firmware `226fbcc677568edf9a04ce7cd95459c664821a03`, DarwinBoot `5243c68ec332a227fe3680cd5da6d5a0e127035c`, GopFramebuffer `b0d812d578efa2acb005b66f73168891719af09a`, SM8250 feed `166235ec2480ff0992627b7aafa15c65ce6b4efa`. `source.sha256` records the device worktree inputs and the local SoC DTS changes. The build uses the run56 System and SystemOS handoff and Apple trust cache, and the run76 1 GiB packed RAM Data seed with APFS UUID `7D443C28-95AC-49F8-AF25-4FC12FBC93E5`.

```sh
BASE=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125
OUT=$BASE/run79-key-store
bash "$OUT/build.sh"
bash "$OUT/capture-boot.sh" "$OUT" 470
```

Run the build script from the device repository. Kernel build, prelink, the two 190-slot storage ABI checks, FAT readback, `scripts/check-elish.py --static-only` and the 14-area porting record check passed. The snapshot argument is 360 seconds and the independent RAM-only deadline is 420 seconds. A snapshot terminates the boot after recording the diagnostic.

| Artifact under `OUT` | SHA-256 |
|---|---|
| `boot.img` | `15d21e8dc7c1e902e2aba21792c1bd938cab457db640c34fcc26e0e09fa23b7d` |
| `payload.img` | `7ab2ec80779c7b515c6fe25ae74ee4d8747223de2a84882bb64361316cb21bb9` |
| `kernel.macho` | `bdeebf88a95b69ea94d8b3990a42ec9618bc6b0fc6cbd16587a9d46c284c4ddf` |
| `device-attempt.AUJSW1/transfer-timeout.jpg` | `4b7e59c2a7efd5623a33a53ec719c61d86c36f7ce00e58468c50c040c4d5bba4` |
| `device-attempt.89vwTy/camera-034.mkv` | `3b0327f18c95740218b8b493e2fa9ccc22fdf2c50cc1d5c8fc820f9c9c045699` |
| `device-attempt.89vwTy/camera-035.mkv` | `16d00fd73402b6662744754f02ca3f2ec9ff3cfa0e2b80852aa3035871240c45` |
| `device-attempt.89vwTy/camera-036.mkv` | `77eb33fb9d590baf0bbcbfe2da9e70c03c86d9f0da38c0266572a600bdde4c8e` |
| [Hello photo in the device repository](elish-run79-hello-20261126.jpg) | `5b2c314423067d70f710c57cb0424047d36f9cc09ebc2135b07d2f80b224ece8` |

## Hardware

Both attempts verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` before `fastboot -s 4bed07e4 boot`. `device-attempt.AUJSW1` stalled in `Sending 'boot.img'` and timed out at 28 seconds. The panel showed FASTBOOT. The bootloader remained enumerated after the timeout, and a five-second `getvar product` request timed out. After USB re-enumeration, `device-attempt.89vwTy` passed all four identity checks on device 049 and received `Booting OKAY` at 18:07:06 CST. The verified bootloader returned on device 050 at 18:13:28 CST, 382.55 seconds after the boot command completed. Both attempts used RAM-only boot and issued no persistent partition writes.

The home indicator is visible 4 seconds into `camera-034.mkv`. The English Hello photo is from 4 seconds into `camera-035.mkv`, about 350 seconds after `Booting OKAY`. `camera-036.mkv` records a later greeting. The owner also confirmed Hello on the device. The camera angle and glare prevent reading the terminal snapshot counters. Touch, language selection, Setup completion, the SpringBoard home screen and Data persistence remain unverified.
