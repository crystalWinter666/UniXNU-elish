# Elish Apple Restore ramp-up

## Run23

The target is the Apple Restore userspace from iPadOS 17.7.11 (`21H461`). The RAM-only test pairs the `9071148B-B025-402D-AD7A-48464D8B5FFF` APFS restore volume with its 267-entry Apple trust cache. The selected kernel argument is `rd=md0-ramblock`. The FAT payload is 224 MiB and DarwinBoot requests a 4 GiB XNU memory window.

| Input or output | SHA-256 |
| --- | --- |
| `boot.img` | `65ab87b23c9ff946d86e8ccf4b85747ba60ea623ff22d52602f1256b9b525fcc` |
| Patched payload `kernel.macho` | `6a818a7bf73982f18513f1a754c0f9d8239385ff287c97fa3cfe51c090405e98` |
| Run21 firmware reused for run23 | `ef645c8c18e0befd00f975ef31d168da20c2720ee6eea598d82e3478aad6ebdb` |
| Packed APFS restore image | `fab7639b00c9683403c0e4a0a3641b04d04d4fcffadb38319d3e237c7d96c841` |
| Converted restore trust cache | `750b5d8ac5f5b52b8fd02d261caf96dd318db0904b3651ca05d9ae4939de488a` |
| User photo `F02A820DAA486596141364E09E62F5E4.png` | `fa6739ab2658b6907b0946c801ac3b2da3bc7120a6408b59f2138c3b29efecaf` |

`fastboot -s 4bed07e4 boot run23-restore-root/boot.img` returned `Sending OKAY` and `Booting OKAY` in 5.532 seconds. The photo and recording show green for ExitBootServices, blue for XNU entry and magenta for the instruction immediately before MMU enable. At 43 seconds the camera still showed those bars and the last UEFI lines. There is no device evidence of the APFS mount or `launchd` for this run. The full capture and manifests are in `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run23-restore-root/`.

## Runs 24–27

These RAM-only tests use the same restore APFS image, trust cache, FAT layout and firmware as run23. The kernel comes from the device-owned build below. The host read the patched kernel back from the FAT image and compared it byte for byte before each boot.

| Run | Diagnostic | Boot image SHA-256 | Device observation |
| --- | --- | --- | --- |
| 24 | APSS watchdog set for 30 s at XNU entry | `bb61d1847e54fc5b6d0fca75d4322b0007ebb6790cd1726b3c2c85a6244856ad` | User saw green, blue and magenta bars, then fastboot. The camera still showed bars at the end of its 58 s capture, and USB enumerated in fastboot after the monitor ended. The reset cause is undetermined. |
| 25 | PSCI reset 7 s after the MMU-enable sequence | `550676b33b437acb622b9338e7a890f5af11200d9f160c105e81da20a78830af` | USB resumed after 20.19 s, with `getvar product` returning `elish`. |
| 26 | PSCI reset 11 s after `_arm_vm_init()` returns | `7211173cf40d29295d60a9465cc8a9b639b30b7810452a70f9bedc5e18facaaf` | USB resumed after 24.19 s, with `getvar product` returning `elish`. |
| 27 | PSCI reset 9 s after `IOSecureBSDRoot()` returns | `3b618be819f2bac26098bc1200d81b498012c8423d6ccca2dc1652fb9d7da5f1` | USB resumed after 23.23 s, with `getvar product` returning `elish`. |

Run24's entry patch checks the Mach-O `LC_UNIXTHREAD` branch and the installed stage-1 trampoline. The final kernel passed the RamBlock verifier with 190 linked vtable slots and 24 native overrides. Runs 25 and 26 use `patch-xnu-entry-probe.py --reset-delay 7` and `--reset-after _arm_vm_init --reset-delay 11`, respectively. Their device-side timing follows the 4 s change in configured delay and supports VM initialization completing with the restore payload. The latest direct screen evidence for BSD root lookup comes from run21. Device logs and hashes are under `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/`.

Run27 uses `patch-xnu-entry-probe.py --in _bsd_init --reset-after _IOSecureBSDRoot --reset-delay 9`. The patched call at `0xfffffff0076f2fa8` occurs after the successful `vfs_mountroot()` branch exits the retry loop. Its 23.23 s USB interval is consistent with the terminal reset executing, which supports a successful restore APFS root mount. This test has no on-screen mount or PID 1 marker.

## Run28

The prelinked kernel's `_sysctl_initproc_spawned` handler rejects a write from any process other than PID 1 and rejects writes after the one-time flag is set. The probe at `0xfffffff007724ed4` preserves that check and calls the original `sysctl_handle_int`. It draws a cyan band only after a write succeeds and the stored flag equals `1`, then returns to the caller. XNU documents the flag as indicating that `launchd` reached `main()`.

`fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/boot.img` completed both transfer and boot in 5.523 s. The user-supplied close-up shows green, blue, magenta and cyan bands, matching the host camera recording at 24 s. The cyan marker confirms that Apple PID 1 executed from the 21H461 restore APFS root. The test used the restore volume and trust cache from run23. Full System-root boot, Setup, UFS and persistent Data remain unverified. [Device photo](elish-run28-launchd-20261125.jpg)

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `7e4b05e374c381aefaef41eaa355490d74b3980159adda8b7f01b52a75ab1c13` |
| Patched payload `kernel.macho` | `76028c66926a64e1a60c8eaac2e52176e0acc7c77f2f0d87f08894bcb9145b55` |
| User-supplied device photo | `e7dab3066c55a4f1aec0c7d04e90fb41f473b8cf5ee1a0ef168e8a27e15a6243` |

The host applied `scripts/patch-xnu-launchd-checkpoint.py`, then added the entry bars with `patch-xnu-entry-probe.py --no-reset`. The RamBlock verifier checked all 190 linked vtable slots and 24 native overrides on the patched kernel. The kernel read back from the FAT payload matched that file byte for byte. The Android v3 boot image and camera recording are in the run28 directory.

The run28 image uses local changes atop device baseline `24de4f60e84762378ce9b1e906b86827b1b2121c` and UniXNU baseline `71c5c6569bae6369c0e0f634df8b9bd5afc19139`.

## Build ownership

The initial run23 test used temporary edits in the UniXNU build checkout. Those edits were reverted after moving the adaptation into `patches/ram-block-volatile.patch` and `scripts/bind-elish-ram-block.py`. The binder stages the modified C++ in its output directory, compiles against the active XNU compile record and checked storage headers, then emits an archive without UFS persistence or loader page release. `unixnu build kernel` completed with the device patch and the final-kernel verifier checked all 190 storage vtable slots and 24 native overrides. `python3 scripts/check-elish.py --static-only` passed. The rebuilt kernel SHA-256 is `56dd7497513f0b7f5e35b163d0419da2458f2cc96423fc0ab39a9aad4d1bd1d3`.
