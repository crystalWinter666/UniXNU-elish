# elish run18/run19 device checks, 2026-11-25

Device: Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC`, `elish`), target iPadOS 17.7.11
(`21H461`). The tablet was in LineageOS before testing. Root ADB reported
active slot `_b`; `/sys/fs/pstore` was empty. All diagnostic boots below used
`fastboot boot`; no persistent partition was written and no poweroff command
or UniXNU return-to-system menu action was used.

## run18 post-VM marker and readback

Fastboot enumerated as serial `4bed07e4`; `fastboot getvar product` returned
`elish`. The run18 post-VM image was RAM-booted:

| Artifact | Path | Size | SHA-256 |
| --- | --- | ---: | --- |
| Post-VM marker image | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image/boot.img` | 67,751,936 | `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30` |
| Pmsg readback image | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image/boot.img` | 67,751,936 | `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939` |
| Lineage reader | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261124/backups/lineage-reader-boot.img` | 55,083,008 | `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4` |

The post-VM image was accepted (`Sending` and `Booting` both `OKAY`). The
top-mounted camera saw green, blue and magenta bands, but the frame was too
far away to read and no close user photo was captured. The device returned to
fastboot at about the probe's configured 15-second reset interval. This timing
does not prove that `_arm_vm_init` returned.

The pmsg readback image was then accepted and returned to fastboot. Its
console ramoops reports:

```text
ELISH_PMSG_READBACK: region=0xB0200000+0x200000 signature=0x00000040 start=0x00080080 size=0x00041200
ELISH_PMSG_READBACK: return-marker address=0xB03FF000 magic=0x00004000 complement=0x00000000 valid=0
```

Thus the expected magic/complement pair (`0x4d564c45` / `0xb2a9b3ba`) was not
present. `_arm_vm_init` return remains unconfirmed. The readback run itself
captured the loader path again: FAT child offset `0x11f0abb` and size `0x4000000`,
UniXNU default `xnu` selection, a 256 MiB window at `0xc007c000`, and
`ExitBootServices()` success with key `0x3ee` and matching snapshot. That image
intentionally stopped before entering XNU.

The root-ADB reader recovered a 46,607-byte console log, SHA-256
`a2d7e0c084aa13572dc9dc9ff51faf1593f829f1ddab8716426154137b3c3627`, archived
at `reports/elish-run18-pmsg-readback-console-ramoops-20261125.txt`.

## run19 IORT/SMMUv2 read-only capture

After a fresh fastboot enumeration and `product=elish` check, the enhanced
run19 image was RAM-booted:

| Artifact | Path | Size | SHA-256 |
| --- | --- | ---: | --- |
| IORT node-ID/SMMU probe | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-nodeids/boot.img` | 67,751,936 | `c8d724467afb0822ad6caba9074a2b40a5dc8075a4947589ee947cfcef6dd371` |
| Probe firmware FD | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-nodeids/SILICIUM_UEFI.fd` | 3,145,728 | `c9b1be78eb7866155798a9a082d76b0403ed2d5331a94e7a9427c3cb67de34dc` |

The diagnostic enumerated 87 apps-SMMU groups without writing registers. It
reported `IORT table not present in ACPI configuration`. Group 4 remained the
only valid stream match: `SMR=0x84020820`, SID `0x820`, mask `0x402`,
`S2CR=0x4`, context bank 4. Context state was `CBAR=0x1f000`, `SCTLR=0xe0`,
`TTBR0=0`, `TCR=0`, `FSR=0x400`; S1 translation was not enabled. No IORT node
could be used to attribute this stream to `UFS0`, so UFS DMA remains disabled.

The probe also re-confirmed the FAT payload, 256 MiB window at `0xc007c000`,
and successful `ExitBootServices()` (key `0x3ee`, snapshot success). It
deliberately stopped before XNU. The reader recovered a 49,786-byte console
log, SHA-256
`322c28323084e3239e35300f1be43a77b062b379c08af99fbcf2842f575140f0`, archived
at `reports/elish-run19-iort-smmu-console-ramoops-20261125.txt`.

The verified reader returned the tablet to root `lineage_elish` ADB after
each diagnostic. No device partition was modified. The latest log-confirmed
stage is still DarwinBoot through successful EBS; the latest direct XNU
evidence remains the earlier run09 user photo of the pre-MMU entry bands.
Neither `_arm_vm_init` completion, UFS access, APFS root mounting nor iPadOS
userspace is confirmed.

## Follow-up correlation

The later run09 close photo and USB timeline are recorded in
`reports/elish-xnu-entry-visual-retest-20261125.md`. Its six-second stage-3
reset returned to fastboot at the expected interval; by UniXNU's diagnosis
procedure this confirms MMU enable, though it is timing evidence rather than a
kernel text marker. This does not turn run18's invalid pmsg return marker into
confirmation that `_arm_vm_init()` returned.

The live Android FDT supplies an owner for the run19 SMMU tuple: MDSS
`qcom,smmu_sde_unsec_cb` has `iommus = <0x46 0x820 0x402>`, and phandle `0x46`
is the apps-SMMU with two IOMMU cells. Therefore SID `0x820`/mask `0x402` is
the SDE display stream. UFS still has no FDT `iommus` property or Linux
`iommu_group`; its stream and DMA mode remain unknown.
