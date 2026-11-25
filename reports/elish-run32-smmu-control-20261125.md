# elish run32 SMMU control read-only probe, 2026-11-25

## Result

Run32 booted RAM-only on the identified `elish` device and warm-restarted to
fastboot before DarwinBoot. The recovered `console-ramoops-0` confirms the
loader probe ran. The probe performed no UFS command, DMA, SMMU write, or
persistent partition write.

The active ACPI configuration still has no IORT. At the SMMU MMIO region
`0x15000000+0xD0000`, the new read-only `sCR0` sample is `0x00270406`:
`CLIENTPD=0`, `USFCFG=1`. The Qualcomm Linux driver's `disable_bypass` policy
sets `USFCFG` when unmatched streams should fault rather than pass through;
this describes the SMMU's unmatched-stream policy, not the route used by UFS.
The scan again found only group 4 valid: SID `0x820`, mask `0x402`, S2CR type
0/context bank 4. The live FDT attributes that tuple to MDSS SDE, not UFS.
UFS could use another route or SMMU instance; its stream/context ownership and
DMA addressability remain unknown. Do not enable DMA based on this snapshot.

The power-gated UFS status reads matched run31: GCC core/interface and related
clock branches were enabled, UFS PHY GDSC was powered, and HCI CAP/VER/IS/HCS/HCE
were readable. The PHY and ICE register blocks were mapped but not read. Core
BCR remained a raw read-only value; this probe does not infer reset polarity.

The last confirmed XNU/userspace milestone remains run28: Apple `launchd` reached
`main()` as PID 1 from the Restore APFS root through RamBlock. The complete
System root, Setup, and persistent Data remain unverified.

## Build inputs and artifacts

| Artifact | Identity |
| --- | --- |
| UniXNU build checkout | `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, existing local build changes retained |
| UEFI checkout | `d489a050` base plus the local read-only `sCR0` probe change |
| Probe payload | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img`, SHA-256 `f06570608c20cca5a47c826b1f813ced5efe87d816b9b0429af68b22c2dc523f` |
| DTB | `/tmp/elish-uefi-probe.dtb`, SHA-256 `0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48` |
| Firmware FD | `/root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd`, 3,145,728 bytes, SHA-256 `0e13e5611a4fcf768067bf2233e5fefb798435239f07a56e520dfc1d03dea159` |
| RAM-only boot image | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run32-smmu-control-readonly/boot.img`, 235,528,192 bytes, SHA-256 `23f6da78160c187b9813dd6e00a9b5d9ef4698dd3c07805f443aa76108203a31` |
| Recovered log | `reports/elish-run32-smmu-control-console-ramoops-20261125.txt`, 45,589 bytes, SHA-256 `3fd9d70a9f1ba1656f002fe4551762fc09fa9e87810630757d34bccf7f9e5b4c` |
| Ramoops reader | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img`, SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4` |

Firmware compilation and board validation passed during `unixnu build firmware`.
The first boot-image packaging attempt supplied a nonexistent FD path in the
run32 output directory and stopped before packaging; the firmware build had
placed the FD under the UEFI checkout's `Build/.../FV/` directory. Packaging
then succeeded using that actual FD path.

```sh
/root/elish-build-20261123/UniXNU/unixnu build firmware \
  -D /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run32-smmu-control-readonly/device \
  --os ipados-17.7.11-21H461 \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run32-smmu-control-readonly/build \
  --uefi /root/repo/uefi-elish --edk2 /root/repo/uefi-elish/Mu_Basecore \
  --clang-bin /usr/lib/llvm-21/bin -j 8 --target DEBUG

/root/elish-build-20261123/UniXNU/unixnu bootimg \
  -D /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run32-smmu-control-readonly/device \
  --os ipados-17.7.11-21H461 \
  --out /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run32-smmu-control-readonly/build \
  --uefi /root/repo/uefi-elish --edk2 /root/repo/uefi-elish/Mu_Basecore \
  --fd /root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd \
  --dtb /tmp/elish-uefi-probe.dtb \
  --ramdisk /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/payload.img \
  --output /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run32-smmu-control-readonly/boot.img

adb -s 4bed07e4 reboot bootloader
fastboot -s 4bed07e4 getvar product
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run32-smmu-control-readonly/boot.img
fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img
adb -s 4bed07e4 exec-out cat /sys/fs/pstore/console-ramoops-0
```

The live preflight identified serial `4bed07e4`, model `M2105K81AC`, device and
fastboot product `elish`. Run32 upload/boot returned `OKAY`; the probe warm
restart returned to fastboot, and the verified read-only reader recovered the
log above.

## Next step and limits

The run32 result does not establish which SMMU route the UFS stream uses. Obtain
the actual elish XBL/HALIOMMU `UFS0` ARID-to-IORT/SID resolution or other direct
board evidence, then compare it with the active SMMU state and prove the full
DMA address range. Do not substitute the shared Kona template or alioth stream
IDs for device evidence. Keep UFS commands and DMA disabled until that mapping
and buffer reachability are established. Then continue with bounded NOP,
capacity, known-block, GPT and OS/SystemOS-window checks, followed by the System
volume UUID, APFS mount and Apple PID 1 checks. Persistent writes remain
unauthorized without a target, backup and restoration plan.
