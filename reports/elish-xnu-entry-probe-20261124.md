# elish XNU entry probe, 2026-11-24

## Test setup

The host-built iPadOS 17.7.11 candidate uses core revision `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, device repository revision `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`, and Mu-Silicium revision `66421cb7bb5694843aec8de1ca3d1de4c20cca38`. The unmodified candidate kernel is 51,122,176 bytes with SHA-256 `7ab7c4c90647e17ae1a301117fc3e067616431a17b164a5ef01b0867603605b2`.

The normal payload already enabled the blue kernel-entry bar, magenta pre-MMU bar and panic screen. Combining that panic screen with a six-second reset probe exceeds the target kernel's available padding, so the diagnostic payload was made from a copy and uses the reset probe without the panic screen. No tracked or source kernel was modified.

The stage-3 probe was applied to a temporary kernel copy with:

```sh
python3 /root/UniXNU/scripts/patch-xnu-entry-probe.py \
  /tmp/elish-entry-reset.jT7heT/kernel.macho --reset-delay 6
```

It reported stage 1, stage 2 and a stage-3 reset six seconds after MMU enable. The patched kernel SHA-256 is `5567975efe546bc6027a9a939bbbd4cd2fdc5e4e62a31591979864577d2aff25`.

The copied 64 MiB payload was updated with `mcopy`, then packed as an Android v3 image using the routine 3 MiB elish FD and hardware DTB:

```sh
/root/repo/UniXNU/unixnu bootimg \
  --config /root/elish-build-20261123/UniXNU/unixnu.local.toml \
  -D /root/repo/UniXNU-elish --os ipados-17.7.11-21H461 \
  --uefi /root/repo/uefi-elish \
  --fd /root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
  --dtb /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/device-tree/hardware.dtb \
  --ramdisk /tmp/elish-xnu-entry-reset-payload.img \
  -o /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-entry-reset.img
```

The resulting image is 67,727,360 bytes, retains the 64 MiB payload and Android v3 header, and has SHA-256 `441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121`. Its boot manifest correctly says `hardware_tested=false`.

## Device observation

ADB had root access before entering the bootloader. The user had authorized RAM-only boot testing. `adb reboot bootloader` detached the Android USB interface at 18:25:25 local time; fastboot enumerated at 18:25:34. The stage-3 diagnostic image was sent with `fastboot boot`. Fastboot reported `Sending` and `Booting` as `OKAY`, with 1.900 seconds total command time. USB disconnected at 18:25:38 and Android ADB enumerated again at 18:25:51.

The roughly 17-second interval from fastboot enumeration to Android USB return is consistent with the three-second UniXNU menu timeout, the six-second post-MMU probe delay and the return to Android. This is indirect timing evidence that the XNU entry probe reached its post-MMU stage. No screen recording or serial log from this run was captured, so the stage is not conclusively observed. The ADB transport returned as `unauthorized`, preventing a shell check or boot-property read.

No partition was written. The earlier FDT probe property, Qualcomm RAM Partition entries, complete UEFI memory map, and DarwinBoot final allocation remain unavailable.

## Next probe

A second diagnostic image moves the six-second reset to the return from the first `_arm_vm_init` call in `_arm_init`:

```sh
python3 /root/UniXNU/scripts/patch-xnu-entry-probe.py \
  /tmp/elish-arm-vm-probe.4SluZJ/kernel.macho \
  --reset-delay 6 --reset-after _arm_vm_init
```

The patched kernel SHA-256 is `5c1492e25342ca83abc13e84ce00449e62f3880e3ff4200672ddbfe427a2f0f1`. Its Android v3 image is `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-arm-vm-return.img`, 67,727,360 bytes, SHA-256 `d206d18d7a8bd4f8cdadb46a0e42dd3bf578baa2e6961384417fb0e42ad57e3f`. Fastboot later accepted this image and Android USB returned. The host logs show a USB disconnect at 20:09:13 and Android enumeration at 20:09:26. No framebuffer or serial output confirms the `_arm_vm_init` return marker, so the checkpoint remains indirect evidence.

## Remaining qualification

The candidate's `kernel.json` still reports `instruction_relocation_complete=false`, `register_dataflow_qualified=false`, `abi_qualified=false`, `runtime_qualified=false` and `loadable=false`. The XnuPrelink emitter intentionally leaves these fields false, and the existing candidate cannot be promoted by editing the report. Runtime UEFI/XNU memory ownership also remains unverified. Separately, elish UFS/SMMU initialization and the XNU APFS root path are not implemented or qualified. Reaching an early XNU checkpoint would not establish a bootable iPadOS system.

The newly cloned `/root/UniXNU` pins `XnuPrelink` at `440589b0c31625f17edf363e24b3c82c51c70bb9` and `DarwinBoot` at `5243c68ec332a227fe3680cd5da6d5a0e127035c`. These are the same component commits present in the device build tree. Source inspection confirms the prelink emitters hard-code the relocation, register-dataflow, ABI, runtime and loadability qualifications as false. The repository copy therefore provides current documentation and the exact component sources used by this build, with no newer qualification implementation to substitute.
