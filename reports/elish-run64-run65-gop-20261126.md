# elish run64 GOP startup and run65 frame counters

Run64 sampled the iPadOS 17.7.11 (`21H461`) userspace at 185 seconds. It showed `Data APFS rw=1`, `launchd=1 main=1`, `backboardd=57`, `SpringBoard=27`, `Setup=0`, and `GOP stage=10 opens=5`. Stage 10 is assigned after the GOP-backed service starts. An open counts only when the display user client was returned successfully. Camera coverage cuts off the completed-frame count. Sampled frames after the loader markers were dark up to the diagnostic snapshot. No Setup Assistant page, SpringBoard home screen, or touch response was captured. Setup completion and Data persistence remain untested. [Run64 snapshot](elish-run64-gop-snapshot-20261126.jpg)

Run65 moves the GOP counters to the first short rows and reports successful transfers, retirements, accepted swaps, rejected swaps, and the last validation rejection reason. A successful transfer includes a black fill. It samples at 230 seconds and sets the independent RAM-only reset deadline to 275 seconds.

Read-only APFS lookups on the 21H461 System window confirmed `/Applications/Setup.app/Setup`, `/System/Library/CoreServices/SpringBoard.app/SpringBoard`, and `/usr/libexec/backboardd`. The snapshot matches those executable names in the live process list. `Setup=0` means the snapshot found no matching process at that instant. Use the screen recording to verify whether Setup actually presents a page.

## Build and readback

`BASE=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125`. Each run directory contains `build.sh`, `build.log`, `kernel.config`, `source.sha256`, `display-build-report.json`, and the effective `unixnu.toml`. Source baselines are device `16cedc7b8a9a417e3994e37f420dc6ec485a7da1` with local changes, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, GopFramebuffer `b0d812d578efa2acb005b66f73168891719af09a`, XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`, and firmware checkout `226fbcc677568edf9a04ce7cd95459c664821a03`. The FD SHA-256 is `fda4d664b23a58f28da7f567ff3b74fe6a54619c6649457fc29e47f4d867f55d`.

The recipe builds the kernel for `ipados-17.7.11-21H461` with `CONFIG_XNU_NATIVE_LEGACY_DISPLAY=y`, checks the linked storage ABI, builds the FAT payload with run56 RootPolicies and Apple trust cache and the 1 GiB 21H461 RAM Data seed, removes `-v` from the generated menu entry, then packs the Android boot image with the same FD and recovery FDT. The display archive qualification covers 358 virtual slots with 24 native overrides. Run65 passed XNU build and prelink, both 190-slot storage vtable checks, FAT validation, payload readback, `scripts/check-elish.py --static-only`, and the 14-area porting record check. A FAT-to-FAT comparison confirmed byte-identical RootPolicies, trust cache, and packed Data seed between runs 64 and 65. The decoded ADTs differ only in the 256-byte `/chosen/random-seed` property.

```sh
BASE=/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125
OUT=$BASE/run65-gop-counters
bash "$OUT/build.sh"            # from the device checkout, with the recorded sources
bash "$OUT/capture-boot.sh" "$OUT" 307
```

| Artifact | SHA-256 |
| --- | --- |
| `run64-gop-cpu/boot.img` | `b3956b0d323285d846230fc710545b905a691fc5f6df249c97dcb7ecd576956c` |
| `run64-gop-cpu/payload.img` | `03dfb1698ea6c80dd4dd138530b9d6fe0275dc131fef277adca5806644856c48` |
| `run65-gop-counters/boot.img` | `5f78b342a914a5193741dd189069926b1fec36dfd70bad3a495dee888d37709e` |
| `run65-gop-counters/payload.img` | `fe244cfce1cb25d45fe1d47d70599f3021c44be4a6ffdf8100576871aeb2d8f8` |
| Run64 photo | `b3b4a3029c55aa664d6e7eafcfce5d0b29a0006ff8aa21e61ec237b2c159e56c` |
| Run64 `camera-020.mkv` | `a04e5125d53a6b8c39f88f8fc7cf98858df3ea4b995d40a87516054996172f48` |

## Tablet and camera

The host verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no` before `fastboot -s 4bed07e4 boot "$BASE/run64-gop-cpu/boot.img"`. The device returned to verified bootloader fastboot 207.85 seconds after boot. The capture directory is `run64-gop-cpu/device-attempt.4v33k5/` under `BASE`. It holds `device.log` and 22 H.264 segments with `camera.sha256`. The snapshot is visible about four seconds into `camera-020.mkv`.

A brightness scan covered 4,693 decoded frames from segments 003–021. The visible panel region lost the loader bars at about 7.8 s into `camera-013.mkv` and stayed dark until the snapshot appeared at about 1.5 s into `camera-020.mkv`. The source clips, `screen-transitions.jpg`, and `screen-roi-brightness.csv` are in the run64 capture directory. The camera sees only part of the panel. This review leaves completed GOP copies unconfirmed.

The first run65 attempt passed all four identity checks. Download stalled at `Sending 'boot.img'` and timed out after 28 seconds. Subsequent attempts stopped before download when `getvar product` timed out. USB reauthorization, a targeted USB device reset, and disabling and re-enabling host port `1-3` failed to recover command responses. After the last step the device enumerated again as `18d1:d00d`, serial `4bed07e4`, and official fastboot 37.0.3 still could not complete `getvar product`. See `run65-gop-counters/device-attempt.{YEKqYu,fENv3p,tl9gcl}/device.log` under `BASE`. No persistent partition writes were issued.

A separate libusb probe verified the USB ID and serial, claimed interface 0, and sent the read-only `getvar:product` command. Bulk OUT endpoint `0x01` transferred all 14 bytes. Bulk IN endpoint `0x81` returned `LIBUSB_ERROR_TIMEOUT` with zero bytes after four seconds. The transcript is `run65-gop-counters/fastboot-bulk-diagnostic.log` under `BASE`.

After a physical cable reconnect, the kernel recorded device 016 disconnecting and device 017 enumerating with the same USB ID and serial. Official fastboot 37.0.3 timed out again on `getvar product`. The capture script stopped at identity verification, before recording or sending the image. The retry logs are `run65-gop-counters/device-attempt.mX4tv7/{device,product-query}.log` under `BASE`.

After manual re-entry into bootloader fastboot, the device at USB address 032 passed all four identity checks. The capture command above returned `Booting OKAY` for run65 at 15:25:02 CST. Verified bootloader fastboot returned at 15:29:15 CST, about 253 seconds later. The capture under `run65-gop-counters/device-attempt.wJk4m6/` contains the command transcript, 26 video segments and their hashes. The 230-second diagnostic is visible in `camera-024.mkv` (SHA-256 `817c4f4035bb421c11c7bc568ee6a042b7464e8f5d262db0491e63fc64eadf4d`). `snapshot.jpg` (SHA-256 `3847da4c105c41891cde578ff824c08fb89240ba93159b252689dfb56e413079`) shows part of the diagnostic. Its right side is outside the frame and the text is out of focus. No numeric result or native UI can be verified from this capture.

## Run66

`$BASE/run66-gop-copies/build.sh` uses the same config, boot arguments, RootPolicies, Apple trust cache, and packed RAM Data seed as run65. The display snapshot uses the backend's post-copy timing hook for `COPY` and its explicit black-fill hook for `BLANK`. It also reports retirements (`END`) and validation counters. XNU build and prelink, storage ABI checks, payload readback, `scripts/check-elish.py --static-only`, and the 14-area record check passed. `source.sha256` records the source inputs. The boot image SHA-256 is `052c02930c6fb782d1471b1b9ef879bf7aea5a5fe1a1e674fbe6f8e87b484215`; payload SHA-256 is `29480d11ab77d23e8e0fb6d03cba003ab3dac87f9b396fc2186612570b492ebc`.

`bash "$BASE/run66-gop-copies/capture-boot.sh" "$BASE/run66-gop-copies" 307` verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no`, then used `fastboot -s 4bed07e4 boot` on the candidate. The transcript in `device-attempt.WHd0TT/device.log` records `Booting OKAY` at 15:36:52 CST and verified bootloader fastboot at 15:41:05 CST. Recording resumed after the tablet was repositioned. At about 191 seconds, the internal panel shows a white Apple logo in `camera-resumed.jNhgsD/resumed-screen.jpg` (SHA-256 `60a7293705f0c71743c8e74af6d5f0cdb1dfa3f4d1417e0ad9cb59fc1b44ea62`). The source is `camera-000.mkv` in the same directory.

The resumed capture still showed the Apple logo at about 243 seconds. `camera-011.mkv` (SHA-256 `3f135e12088aabd25aec201bdf5cad800f7f39b995af6885b8d631a76a287cb9`) shows the transition to the diagnostic. `snapshot.jpg` shows `launchd=1 main=1`, `backboardd=57`, `SpringBoard=26`, and `Setup=0`. The GOP counters are above the camera's top edge. No Hello page, language picker, SpringBoard home screen or touch response was captured. Setup completion and Data persistence remain untested. The boot required no persistent partition writes.

## Run67

`$BASE/run67-longer-ui-window/build.sh` moved the diagnostic eight text rows lower and scheduled it at 450 seconds, with an independent RAM-only reset deadline of 500 seconds. The boot image SHA-256 is `962ac89be5f5d53822515f756e924399a990d9d11278dbbdb03510ce527fe944`. The payload SHA-256 is `b23926e0a396f5c78f6794e37969209a93b5c4caea9af151d03fb532c04145d6`. `source.sha256` and `display-build-report.json` pin the local build inputs. Board static and 14-area porting record checks passed.

The command `bash "$BASE/run67-longer-ui-window/capture-boot.sh" "$BASE/run67-longer-ui-window" 560` verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no` before `fastboot boot`. `device-attempt.iydvRb/device.log` records `Booting OKAY` and the verified bootloader returning about 473 seconds later. The camera still showed the Apple logo near 460 seconds. The terminal snapshot recorded `GOP 10 OPEN 5`, `COPY 4 BLANK 0`, `END 4`, `ACCEPT 4 REJECT 0`, `REASON 0`, writable APFS Data, `launchd=1 main=1`, `backboardd=57`, `SpringBoard=27`, and `Setup=0`. Evidence in the attempt directory is `camera-046.mkv` (SHA-256 `51e08fba0980e170848f07255fcfffc8016a7ed83e6c1f314f711409e54a4b0b`) and `snapshot.jpg` (SHA-256 `ac72c42a361d3900b3dbe20ed0e8b0112ca7c9a9912e0cc7c3fe0ae8c70099f3`). Setup, the SpringBoard home screen, touch, and Data persistence remain unverified. The boot issued no persistent partition writes.

## Run68

`bash "$BASE/run68-userspace-stacks/build.sh"` built and prelinked XNU, checked the storage ABI, verified payload contents and passed `scripts/check-elish.py --static-only` and the 14-area porting record check. A failed compilation left `task.o` absent, so the recorded `task.o.json` compiler arguments were replayed before the successful build. `source.sha256` pins the local inputs. The base revisions are device `16cedc7b8a9a417e3994e37f420dc6ec485a7da1`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f`, GopFramebuffer `b0d812d578efa2acb005b66f73168891719af09a`, DarwinBoot `5243c68ec332a227fe3680cd5da6d5a0e127035c`, and firmware checkout `226fbcc677568edf9a04ce7cd95459c664821a03`. The boot image SHA-256 is `e3f3fc67400e9933b26a8675b23d9c4e9945447d0186bff5ae7e716f8d4048c2`. The payload SHA-256 is `68043114b5ac76af5d2cb633c092ccf38be1ae3b43a666ee15e750e420773691`.

`bash "$BASE/run68-userspace-stacks/capture-boot.sh" "$BASE/run68-userspace-stacks" 365` checked USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no` before RAM-only `fastboot boot`. `device-attempt.NmGgLr/device.log` records `Booting OKAY` and the verified bootloader returning 293 seconds after the boot command completed. The camera shows the Apple logo until the terminal snapshot. The snapshot records writable Data, `launchd=1 main=1`, `Setup=0`, four copies, four completed swaps and zero rejects. `camera-028.mkv` captures the transition to the snapshot (SHA-256 `ae2db8b7f5074304074389f185afac0ea17bc9910ccb4bfaecda9731268d4449`). `snapshot.jpg` shows both user stacks (SHA-256 `f4ca6cdf851ac90ff415dfb08304929769b3f2d71ffec0ac62b18c216ef2c67b`).

Both stack captures report slide `0xa724000` and suspend and unwind status zero. `symbolicated-stacks.log` uses `ipsw dyld a2s --slide 0xa724000 --image` against the 21H461 SystemOS shared cache read through a read-only QEMU guest. The guest reported compressed block mapping errors in the link-edit and local-symbol files. `ipsw` indexed the public and private symbols and resolved the known dyld entry, but its Objective-C table scan returned EOF. The independently resolved Mach, Foundation and CoreServices frames place SpringBoard PID 27 in a synchronous XPC wait from `_LSCopyServerStore` and `_LSContextInitCommon`. Backboardd PID 57 was in `__CFRunLoopServiceMachPort` under `__CFRunLoopRun`. A single sample does not establish a deadlock or whether LaunchServices is making progress. The next diagnostic samples the LaunchServices daemon. Setup, the SpringBoard home screen, touch, and Data persistence remain unverified. The boot issued no persistent partition writes.

## Run69

`bash "$BASE/run69-launchservices/build.sh"` kept the run68 boot arguments and RAM Data seed. XNU build and prelink, payload readback, the board static check, and the 14-area record check passed. `source.sha256` pins the local sources against the run68 base revisions. The boot image SHA-256 is `43f174a6cfd93d18e2ee94b1bc852e69d9542a7f49ffc517b08f3129774168ba`. The payload SHA-256 is `39b77583e5273ca0b7c2adab8dd2437a14a09b4af3e937b7a03f680946ee8437`.

`bash "$BASE/run69-launchservices/capture-boot.sh" "$BASE/run69-launchservices" 365` checked USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no`, then used RAM-only `fastboot boot`. The bootloader returned after 293 seconds. The Apple logo remained until the snapshot. `device-attempt.UKLYBG/camera-028.mkv` records the transition (SHA-256 `1841e577f52acd150c9e03e416d1cd3e52a0eb1155f70bd83b604d83b443e3ca`). `snapshot.jpg` shows `Setup=0`, 4 copies, 4 completed swaps, and 0 rejects (SHA-256 `acd034a2f9eada32dafacd122fad845adf65fdb2c2d1a13bde95f31b7db2446c`).

`symbolicated-stacks.log` resolves `lsd` PID 118's oldest thread through `LSDatabaseBuilder::createAndSeedLocalDatabase`, `_LSDatabaseClean`, `LSHandlerPref::CopyHandlers`, `_LSPreferences::with`, and `__DISPATCH_WAIT_FOR_QUEUE__` into `__ulock_wait`. The other sampled thread was in `__workq_kernreturn`. A read-only guest inspection of the source root found separate mobile and system `lsd` jobs using the same executable and confirmed that Apple Setup is `com.apple.purplebuddy`. The snapshot does not identify the job instance or the queue holder. Setup, touch, and Data persistence remain unverified. The device boot issued no persistent partition writes.

## Run70

`bash "$BASE/run70-preferences-wait/build.sh"` used the same source baselines and run69 boot arguments. The local source hashes are in `run70-preferences-wait/source.sha256`. XNU build and prelink, payload readback, the board static check and the 14-area porting record check passed. The boot image SHA-256 is `1931f57170b688a8f82f085480278a60d4c29040291885183e1ad6e08a7d6135`. The payload SHA-256 is `494b1b3a80f73057d79a35b1eb977d718bdea2930458bd1185d01c23d4911898`.

`bash "$BASE/run70-preferences-wait/capture-boot.sh" "$BASE/run70-preferences-wait" 365` verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no` before RAM-only `fastboot boot`. The verified bootloader reappeared 292 seconds after the boot command completed. In `device-attempt.3OcuBt`, `camera-028.mkv` records the Apple logo before the snapshot (SHA-256 `2c2306b2c11b7734c42c183d0074e6be69d168dd18791364d7c22503f9a64cc6`). `snapshot.jpg` records the diagnostic (SHA-256 `d4796e0146c6ae41d842af6d82ef5797cea1d04f876921b6e775758f08d78f7a`).

At the 270-second sample, mobile `lsd` PID 123 had one thread outside the parked workqueue wait. With slide `0x1fc2c000`, `symbolicated-stacks.log` resolves that thread through `_LSDatabaseClean`, `LSHandlerPref::CopyHandlers`, `_LSPreferences::with`, and `__DISPATCH_WAIT_FOR_QUEUE__` into `__ulock_wait`. The sample showed system `cfprefsd` PID 90 and no matching system `lsd` or mobile `cfprefsd` PID. SpringBoard PID 27, backboardd PID 57, writable RAM Data, and four completed display swaps were present. `Setup=0`, and Setup UI, touch, and Data persistence remain unverified. The device boot issued no persistent partition writes.

## Run71

`bash "$BASE/run71-lsd-workqueue/build.sh"` kept the run70 boot arguments and inputs, adding a try-locked snapshot of XNU workqueue counters. XNU build and prelink, payload readback, the board static check and the 14-area record check passed. The boot image SHA-256 is `37892cb8971345c4618f2bdaac5da3c386fabc4f8677dad24a01dd2bbe34bc18`. The payload SHA-256 is `7041b765b5c72f2ecd720a7888642c4def17e7259558b4c8387cc096f594fbbc`.

`bash "$BASE/run71-lsd-workqueue/capture-boot.sh" "$BASE/run71-lsd-workqueue" 365` verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no` before RAM-only `fastboot boot`. Bootloader fastboot returned after 292 seconds. `device-attempt.r7naAj/snapshot.jpg` (SHA-256 `ca44a1d2deb11db2f4cda1e9e0f9691e5e162b2837bec0615e45b772ccc6ed85`) shows mobile `lsd` PID 118 with one idle worker, five pending workqueue requests, 15 fulfilled requests, `WQ_DELAYED_CALL_SCHEDULED` and no creator. It also shows writable RAM Data, four completed swaps, and `Setup=0`. Setup UI, touch and persistence remain unverified. The device boot issued no persistent partition writes.

Disassembly confirmed that `_LSPreferences::load(true)` performs `dispatch_barrier_async` on the queue before `_LSPreferences::with` calls `dispatch_sync` on it. The linked kernel dispatches `qos_max_parallelism` through `sched_edge_dispatch`, which sizes CPU-based admission using E/P topology counts. Run71 FAT readback showed all eight CPU nodes lacked `cluster-type`, so XNU classified them as SMP. Run72 uses `--local-module soc:sm8250=.../UniXNU-sm8250-common` to validate the shared E/P mapping on hardware.

## Run72

`bash "$BASE/run72-workqueue-admission/build.sh"` used the run71 device, core, XNU and component baselines with SM8250 `166235ec2480ff0992627b7aafa15c65ce6b4efa` plus the local `dts/sm8250.dtsi` changes pinned by `source.sha256`. FAT readback verified `cluster-type` E on CPU affinities `0x000`–`0x300` and P on `0x400`–`0x700`. The 360-second snapshot and independent 420-second return deadline retained the writable 1 GiB RAM Data seed. Kernel compilation, prelink, storage ABI checks, FAT readback, `scripts/check-elish.py --static-only` and the 14-area record check passed.

| Artifact in `run72-workqueue-admission/` | SHA-256 |
|---|---|
| `boot.img` | `21f4e946a6b6b3d41afe589b2313f20a533154a0853a48392287312ff8f8ffee` |
| `payload.img` | `0c26804113f495954bb19fa38a55d18fa38b0fa65a227b725fc279e2455890f8` |
| `device-attempt.HmVl0j/camera-037.mkv` | `57bfb2c9ff8b19afbbc0bae1ebaac0e5cec9719caaddf9ca06a20df0964cb576` |
| `device-attempt.HmVl0j/camera-038.mkv` | `f2ad27e6bbd363ecfc93f7c857d8965c83cdf2b090b75cd57919c75284e3213b` |
| `device-attempt.HmVl0j/final-038.jpg` | `55519a6fb22b32653d5278b495aa03301d55a5f99894d033329aabb70fb47b1c` |

`bash "$BASE/run72-workqueue-admission/capture-boot.sh" "$BASE/run72-workqueue-admission" 470` verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no`. The successful attempt followed a host USB reset of the same serial. `device-attempt.HmVl0j/device.log` records RAM-only `fastboot boot` and the verified bootloader returning after about 383 seconds. The internal panel showed the Apple logo until the diagnostic.

The sample shows `Data 1 Launchd 1 Main 1 Setup 0`, `Copies 6 Ends 6 Rejects 0`, SpringBoard PID 26, backboardd PID 57 and system `cfprefsd` PID 90. Mobile `lsd` PID 99 shows `WQ total 3 idle 3 req 0 done 4349 F4 C0`. Hello, language selection, the home screen, touch, Setup completion and Data persistence remain unverified. This boot issued no persistent partition writes.

`symbolicated-stacks.log` uses the photographed slide `0x1e58c000`. The only non-parked mobile `lsd` thread was receiving Mach messages in `runServerMainRunLoop` from `_LSServerMain`.

## Run73

`bash "$BASE/run73-springboard-stack/build.sh"` retained the run72 source baselines, local SoC DTSI, boot arguments and seed. The source hashes identify the added first-thread SpringBoard stack capture. Kernel compilation, prelink, both storage ABI checks, FAT readback, board static checks and the 14-area porting record check passed.

| Artifact in `run73-springboard-stack/` | SHA-256 |
|---|---|
| `boot.img` | `b7ef0044def312d25656d9b638cf2829364d45e3fab51ff68ca70b86c93c4b63` |
| `payload.img` | `908e51108d16858a14eaf651e0ac3b0cae150600e2ecd8beec8b664983ca9d52` |
| `device-attempt.QQHkld/camera-038.mkv` | `9a631d54b20d4acc43c1629bc2add9550ac3923a6c99697e5fa9ceffd4d226d3` |
| `device-attempt.QQHkld/final-038.jpg` | `1a784c9b295de8bb9a03c6d2f13487652ad8ccd6585283b2548826046404ac2a` |

`bash "$BASE/run73-springboard-stack/capture-boot.sh" "$BASE/run73-springboard-stack" 470` checked USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no`, then issued RAM-only `fastboot boot`. The device transcript records the return to the verified bootloader 382.55 seconds later. The internal panel showed the Apple logo in `device-attempt.QQHkld/panel-018.jpg`.

The 360-second snapshot showed writable Data, `launchd=1 main=1`, `backboardd=57`, `SpringBoard=27`, `Setup=0`, and six GOP copies and completed swaps with zero rejects. Mobile `lsd` PID 98 had four idle workers, 4,371 fulfilled requests and zero pending requests. With slide `0x19c3c000`, `springboard-stack.log` places thread 258 in a synchronous XPC wait through `-[BSXPCServiceConnectionMessage _sendSynchronously:]` and `-[BSXPCServiceConnectionMessage sendSynchronouslyWithError:]`. The enlarged `frame-7-detail.jpg` confirms the seventh runtime address `0x1b90b8498`. The capture ends at the generated BoardServices proxy frame. Hello, the home screen, touch, Setup completion and Data persistence remain unverified. The device boot issued no persistent partition writes.

## Run74

`bash "$BASE/run74-springboard-deep-stack/build.sh"` reused the run73 boot arguments, RAM Data and source baselines. `source.sha256` pins the XNU patch that expands the SpringBoard capture from 9 to 24 frames. Kernel compilation, prelink, storage ABI checks, FAT readback, board static checks and the 14-area porting record check passed.

| Artifact in `run74-springboard-deep-stack/` | SHA-256 |
|---|---|
| `boot.img` | `99f00245102b7b84c9455913cbd20b58a7a93efcc0fe7efc871aff63241056b2` |
| `payload.img` | `a5fe1974959e07185b49a2046332e00d515a60777c5a21f49c4c85faa6d713a8` |
| `device-attempt.ISVBvO/camera-037.mkv` | `818267d0c7955e4aa1273130481124e88b65c859843b3cccac907d0406002e5f` |
| `device-attempt.ISVBvO/camera-038.mkv` | `ab327a62875a6504181588ae70c1c30abf63a34314d31e5b584ba3129b058379` |
| `device-attempt.ISVBvO/final-038.jpg` | `d66e1a608b02f6c98cfd89da251a69700be65665c1eeb67006e964ffec32a741` |

`bash "$BASE/run74-springboard-deep-stack/capture-boot.sh" "$BASE/run74-springboard-deep-stack" 470` checked USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` before RAM-only `fastboot boot`. The verified bootloader returned 382.55 seconds after boot completed. `device-attempt.ISVBvO/final-037.jpg` shows the Apple logo before the diagnostic. At 360 seconds the snapshot records writable RAM Data, `launchd=1 main=1`, `backboardd=57`, `SpringBoard=27`, `Setup=0`, six copies, six completed swaps and zero rejects. No persistent partition writes were issued. Hello, the home screen, touch, Setup completion and Data persistence remain unverified.

With slide `0x49f8000`, `springboard-stack.log` resolves the synchronous wait from `-[SBHIconManager init]` through `-[CHSWidgetExtensionProvider _lock_loadContentInitiallySynchronouslyIfNecessary]` and `-[CHSChronoServicesConnection _subscribeToExtensions]`. The call immediately preceding frame `0x194799f18` targets `subscribeToExtensionsIncludingIntents:completion:`. Read-only QEMU inspection of the 21H461 System image exported `com.apple.chronod.plist`. It registers `com.apple.chronoservices` and launches `/System/Library/PrivateFrameworks/ChronoCore.framework/Support/chronod` as `mobile` on demand. The run74 snapshot does not establish the daemon's runtime state.

## Run75

`bash "$BASE/run75-chronod/build.sh"` used the run74 baselines, boot arguments and RAM seed. `source.sha256` pins the process sampling changes. The kernel, prelink, storage ABI, FAT and board checks passed. `bash "$BASE/run75-chronod/capture-boot.sh" "$BASE/run75-chronod" 470` verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` before RAM-only `fastboot boot`. The device returned to verified bootloader fastboot in 382.56 seconds.

| Artifact in `run75-chronod/` | SHA-256 |
|---|---|
| `boot.img` | `d4c25473abdbeedd06aa090971fb888d0e4acb8dc0ff0d3e996a36a1548e87b9` |
| `payload.img` | `61172c979717a9973dd10a5479713fdac56faec7dfa020ea921c71e7fc24cbf9` |
| `device-attempt.Y5rFos/camera-038.mkv` | `4dbda7bf5c2121c0c05d9f5a1a1baa3e265992cab0e3287ab31845fa7a72c4bd` |
| `device-attempt.Y5rFos/final-038.jpg` | `f2bbba44dca8632f20b2cfde77d7e7f5cee8197e0d549523c04574a27c5b2943` |

The 360-second snapshot recorded `chronod` PID 98 with five workqueue threads, two idle, and zero pending requests. `pkd` was PID 384, mobile `lsd` PID 99, SpringBoard PID 26 and `backboardd` PID 57. The first sampled `chronod` thread resolves with slide `0x1a6d4000` to the synchronous BoardServices proxy and `mach_msg2_trap`. Its caller lies beyond the nine captured frames. The sample recorded writable RAM Data, six copies, six completed swaps and `Setup=0`. No Hello or home screen was captured. The device issued no persistent partition writes.

## Run76

Read-only guest inspection in `run74-springboard-deep-stack/inspect-chronod.log` found `/private/var/mobile/tmp` owned by `0:0` with mode `0755`. `data-seed-tools/fix-mobile-tmp-guest` changed only that directory to `501:501` on a copy of the 1 GiB RAM seed. A read-only remount verified the owner, the global `tmp` mode `01777`, and absence of both `com.apple.purplebuddy.plist` files and `db/.AppleSetupDone`. Host `apfsck` passed. The Data volume UUID remained `7D443C28-95AC-49F8-AF25-4FC12FBC93E5`. Packed reconstruction verified the complete raw image digest.

`bash "$BASE/run76-mobile-tmp/repack.sh"` verified all seed hashes, replaced `scratch-data.img` in a copy of the run75 payload, checked the FAT and read back the seed, config, kernel, ADT, trust cache and RootPolicies.

| Artifact in `run76-mobile-tmp/` | SHA-256 |
|---|---|
| `data-mobile-tmp.img` | `c24a7ec85f93c19cd7954473a80b375494fd9cd4d160b45e6ee6cced7821d1d5` |
| `data-21H461.dbr` | `3a8939c99e7fd7732b0615a23f13faaeb3279facfa4ebe05377422b0647b4640` |
| `boot.img` | `caa454b9869b6f04fd450d466b68fc6f61b207fb244dfb221af02119cc5f4c48` |
| `payload.img` | `9c686e85457dcd9e730b4ad9d6ce1c81f6cda404eb71ba059921759acc7e7996` |
| `device-attempt.snaEYG/camera-037.mkv` | `4bde3e7249c6f33decfaa945de02d85ae5f34a541c211bef1283d6da4f1fb849` |
| `device-attempt.snaEYG/final-038.jpg` | `256135bd4c090d7e1cb936aecc854541d4440a4e5fecb171e758b51392877310` |

`bash "$BASE/run75-chronod/capture-boot.sh" "$BASE/run76-mobile-tmp" 470` checked USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` before the RAM-only boot. The verified bootloader returned 382.52 seconds after the boot command completed. The panel showed the Apple logo before the 360-second snapshot. The diagnostic recorded six copies, six completed swaps, zero rejects, writable RAM Data, `launchd=1 main=1`, `backboardd=57`, SpringBoard PID 26, `chronod` PID 98 and `Setup=0`. The SpringBoard and `chronod` stacks still show synchronous XPC waits. Hello, the home screen, touch, Setup completion and Data persistence remain unverified. No persistent partition writes were issued.

## Run77

`bash "$BASE/run77-session-daemon/build.sh"` retained the run76 RAM Data seed, run56 trust inputs and run75 CPU topology. `source.sha256` identifies the XNU patch and board archive. Kernel build, prelink, storage ABI, FAT readback, board static checks and the 14-area record check passed.

| Artifact in `run77-session-daemon/` | SHA-256 |
|---|---|
| `boot.img` | `960a69a946b9a2566ecb2c3f2ea2f50daa8127de11e003e873d7a8288e3d995e` |
| `payload.img` | `b8dad888848d225ffd9fb5f2a523529f244bffb05d4cff4d06287028c784d8b2` |
| `device-attempt.36vFKX/camera-037.mkv` | `217e17e9db5e8ef6306fbf2743a4079c328e9ff07e75102bb31903dd8492a965` |
| `device-attempt.36vFKX/final-038.jpg` | `83aa1f68da7a0411860100103bcb765bb1e80b191f86dbc06cae31f202ee9081` |

`bash "$BASE/run77-session-daemon/capture-boot.sh" "$BASE/run77-session-daemon" 470` checked USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no` before RAM-only boot. The verified bootloader returned in 382.54 seconds. The panel showed the Apple logo until the 360-second diagnostic, which recorded writable RAM Data, launchd PID 1, backboardd PID 57, SpringBoard PID 27 and `Setup=0`. Six GOP swaps completed with zero rejects. `liveactivitiesd` was absent with 15 exits and latest wait status `0x5` (`SIGTRAP`). No persistent partition writes were issued.

`chronod-stack.log` maps the sampled synchronous BoardServices call's return address to `0x21ca26a1c` in ActivityKit, using slide `0x1c3c4000`. Read-only guest inspection of the 21H461 System found `chronod` registered as `com.apple.chronoservices` and `liveactivitiesd` registered as `com.apple.sessionservices`, both on demand. The relationship between the sampled wait and the daemon's repeated exit remains to be tested.

## Run78

`bash "$BASE/run78-session-breakpoint/build.sh"` passed kernel build, prelink, storage ABI verification, FAT readback, board static check and the 14-area porting record check. The source baselines and RAM seed match run77. `source.sha256` identifies the breakpoint capture patch and `unixnu.toml` sets the snapshot at 160 seconds with a separate 220-second reset deadline.

| Artifact in `run78-session-breakpoint/` | SHA-256 |
|---|---|
| `boot.img` | `6d5edad9d82b89c633b46cd47d28b8a78149e7dfd1552ac602dbb8b3b4b19ccf` |
| `payload.img` | `0e1192d990dbd18584be2bc8a5732f202cc4f6e53dd2bd4f31c6fa14fef47ccc` |
| `device-attempt.4mb2ey/camera-018.mkv` | `371883c9a7eb2024c4808b3499a3f65171f31f42c692cfd6b8633dcca3dbece4` |
| `device-attempt.4mb2ey/final-018.jpg` | `d9424d7a2060dbe309243e524ab104156f983585dcef4624a4310c8115c9490a` |

`bash "$BASE/run78-session-breakpoint/capture-boot.sh" "$BASE/run78-session-breakpoint" 255` verified USB `18d1:d00d`, serial `4bed07e4`, `product=elish` and `is-userspace=no`. The bootloader returned 182.75 seconds after accepting RAM-only boot. The panel diagnostic recorded writable Data, six completed swaps, `Setup=0` and eight `liveactivitiesd` exits, latest wait status `0x5`. No persistent partition writes were issued.

The first ARM64 breakpoint was in PID 100 at PC `0x1c69424f8`, LR `0x1c69424cc`, slide `0x504c000`. The 21H461 cache maps both addresses to SessionSyncEngine. `breakpoint-symbols.log` and `keybag-imports.log` show `MKBEventsRegister` returning null, followed by `CBZ` to `BRK #1` at unslid `0x1c18f64f8`. MobileKeyBag matches the `AppleKeyStore` IOKit class. The [DBY-W09 21H461 port](https://github.com/WQSQW/UniXNU-dby_w09/tree/715814d07840c817317892d5a585f4a3e769e343) uses the shared `AppleKeyStoreState` component. The [umi 21H461 port](https://github.com/reAAAq/UniXNU-umi/tree/3eb72578597d2ca16471b172d45eddc096a34a04) reports Hello on an SM8250 CPU-copy presenter. Run79 integrates the pinned shared key store for volatile Data.
