# elish stock UFS/SMMU firmware audit, 2026-11-25

## Scope

This host-only audit rechecked the stock elish XBL `UFSDxe`, `SdccDxe` and
`HALIOMMUDxe` PE32 sections against the Mu-Silicium catalog and traced their
protocol calls with Ghidra 12.2. No device command, boot, power transition or
partition write was run.

| Input | Identity |
| --- | --- |
| Stock `xbl.img` | SHA-256 `9a3a3bb92e6cf478b227f70dd1bd140fde32982994a6c34a07bd314ea0acb72f` |
| Extracted inner FV | `/tmp/elish-xbl-inner-fv-20261125.fv`, SHA-256 `1f7dc78a5204f1662e6628bf19f16ca874c3d398fd21b0f35f8f71016a57caa1` |
| Stock elish `UFSDxe` PE32 | SHA-256 `ffeaed467f8251b7e30a45075da84a1c563e2bac7dc4be0f79f0ed49a88b8dc9`; byte-identical to alioth/pipa catalog copies |
| Stock elish `SdccDxe` PE32 | SHA-256 `b62eca095e3a5776735d8ceb8dafbf3d35a5f9d786b0d4d2f98813ac01f7b344`; byte-identical to alioth/pipa catalog copies |
| Stock elish `HALIOMMUDxe` PE32 | 45,056 bytes, SHA-256 `36a5adbe91a796940f4c09eb53ae4ffb1632ef20d8c3085f7cceb2a706c1e306`; byte-identical to alioth/pipa catalog copies |
| Ghidra | 12.2_PUBLIC_20261016 |
| Device repository revision | `4b3da68ea0aca815a80e8d1bad802dfec8d5c905` |
| UEFI repository revision | `f03839d31a3f07e9e4b032fcf850ac0dfb9961a0` |

## Findings

- **The earlier GUID identification was wrong.** At UFS RVA `0x14124`,
  `UFSDxe` passes `0x19248` to `LocateProtocol`; at SDCC RVA `0x40a8`,
  `SdccDxe` passes `0x17268`. Both addresses contain the same 16-byte EFI GUID:
  `54b6d3b4-5d33-4f91-8600-6c41d5deb19a`. The prior value
  `416c0086-ded5-9ab1-b673-0e22db6b1344` starts eight bytes into that GUID and
  continues into adjacent data. It is not the GUID passed by either call site.
- **The stock provider is present.** In `HALIOMMUDxe`, the interface initializer
  at RVA `0x1504` installs the GUID at `0x9048` with the interface at `0x9098`
  through `InstallMultipleProtocolInterfaces`. The stock HALIOMMU PE32 exactly
  matches both catalog copies, so the provider is confirmed in the stock elish
  XBL and the earlier “provider not found” result is withdrawn.
- **The API resolves through ACPI IORT.** The HALIOMMU implementation reads the
  ACPI `IORT` table, looks up the supplied device path and ID mapping, allocates
  a context bank, then programs SMMU stream-match and context mappings. UFS
  calls attach for `\\_SB_.UFS0` with ARIDs `0x0` and `0x10`. SDCC calls attach
  with `\\_SB_.SDC1` and ARIDs `0x0` and `0x1`; its diagnostic labels say SDC2
  and SDC4, an unresolved naming inconsistency.
- The HAL implementation means an ARID is an input to the IORT mapping lookup;
  it is not itself evidence for the resulting apps-SMMU SID. The exact elish
  runtime IORT mapping was not extracted. Therefore this audit still cannot
  attribute run11's observed SID `0x0820`, mask `0x0402`, context bank 4 to UFS
  or any other device.
- The stock Android UFS DT lacks an `iommus` property, while the firmware
  provider consumes ACPI IORT. These are separate data sources; the missing DT
  property does not mean the firmware mapping is absent. XBL settings remain
  `EnableUfsIOC = 0` and `UfsSmmuConfigForOtherBootDev = 1`. None of this
  establishes XNU's post-EBS controller ownership, active clocks, PHY state,
  DMA buffer reachability or a safe SMMU takeover. UFS DMA remains disabled.

## Reproduction

The stock SDCC PE32 begins at FV-relative offset `0x30d6fc` and is `0x1b000`
bytes. The stock HALIOMMU PE32 begins at FV-relative offset `0x2c5410` and is
`0xb000` bytes. Both were extracted from the FMMT-validated FV and compared
with both catalog files:

```sh
dd if=/tmp/elish-xbl-inner-fv-20261125.fv \
  of=/tmp/elish-stock-HALIOMMUDxe.efi bs=1 skip=$((0x2c5410)) \
  count=$((0xb000)) status=none
dd if=/tmp/elish-xbl-inner-fv-20261125.fv \
  of=/tmp/elish-stock-SdccDxe.efi bs=1 skip=$((0x30d6fc)) \
  count=$((0x1b000)) status=none
sha256sum /tmp/elish-stock-HALIOMMUDxe.efi \
  /root/repo/uefi-elish/Binaries/alioth/QcomPkg/Drivers/HALIOMMUDxe/HALIOMMUDxe.efi \
  /root/repo/uefi-elish/Binaries/pipa/QcomPkg/Drivers/HALIOMMUDxe/HALIOMMUDxe.efi
sha256sum /tmp/elish-stock-SdccDxe.efi \
  /root/repo/uefi-elish/Binaries/alioth/QcomPkg/Drivers/SdccDxe/SdccDxe.efi \
  /root/repo/uefi-elish/Binaries/pipa/QcomPkg/Drivers/SdccDxe/SdccDxe.efi
```

`AuditGuidXrefs.java` is in `scripts/audit-ufs-dxe-ghidra/`. It prints bytes,
cross-references and decompilation for each requested RVA. The consumer checks
used these commands:

```sh
JAVA_HOME=/opt/jdk-25 PATH=/opt/jdk-25/bin:$PATH \
  /opt/ghidra-12.2/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  /tmp/elish-ghidra-projects ElishUfsGuidAudit \
  -import /root/repo/uefi-elish/Binaries/alioth/QcomPkg/Drivers/UFSDxe/UFSDxe.efi \
  -overwrite -analysisTimeoutPerFile 300 -max-cpu 8 \
  -scriptPath /root/repo/UniXNU-elish/scripts/audit-ufs-dxe-ghidra \
  -postScript AuditGuidXrefs.java 0x19248

JAVA_HOME=/opt/jdk-25 PATH=/opt/jdk-25/bin:$PATH \
  /opt/ghidra-12.2/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  /tmp/elish-ghidra-projects ElishSdccGuidAudit \
  -import /root/repo/uefi-elish/Binaries/alioth/QcomPkg/Drivers/SdccDxe/SdccDxe.efi \
  -overwrite -analysisTimeoutPerFile 300 -max-cpu 8 \
  -scriptPath /root/repo/UniXNU-elish/scripts/audit-ufs-dxe-ghidra \
  -postScript AuditGuidXrefs.java 0x17268

JAVA_HOME=/opt/jdk-25 PATH=/opt/jdk-25/bin:$PATH \
  /opt/ghidra-12.2/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  /tmp/elish-ghidra-projects ElishHalIommuGuidAudit \
  -import /root/repo/uefi-elish/Binaries/alioth/QcomPkg/Drivers/HALIOMMUDxe/HALIOMMUDxe.efi \
  -overwrite -analysisTimeoutPerFile 300 -max-cpu 8 \
  -scriptPath /root/repo/UniXNU-elish/scripts/audit-ufs-dxe-ghidra \
  -postScript AuditGuidXrefs.java 0x9048
```

The stock HAL module hash match confirms the catalog disassembly applies to
elish. It does not supply the live ACPI IORT contents or prove a UFS DMA
configuration. The next firmware step is to capture/parse the elish IORT
mapping for `\\_SB_.UFS0` ARIDs `0x0` and `0x10`, then compare the resolved
stream IDs and context ownership with the run11 SMMU snapshot before enabling
any UFS DMA path.

## Kona IORT template cross-check

This is host-only source evidence, not a capture from the elish runtime:

- The shared UEFI source table `Silicium-ACPI/Silicon/Qualcomm/Kona/IORT.aml`
  has SHA-256
  `da13b7ef280d4c81a56baa50bfc5c82b6e0bcd03da5b8246a7a3cdcbe6671b95` at
  UEFI revision `f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`.
- Its decompiled `\\_SB.UFS0` Named Component has two mappings: input bases
  `0x81030000` and `0x81030001` map to output bases `0x4e0` and `0xe0`,
  respectively, through the SMMU node at table offset `0x30`. These encoded
  IORT input IDs are not the raw `UFSDxe` ARID arguments `0x0` and `0x10`; the
  table alone does not establish how the HAL resolves those arguments.
- The same-SoC alioth firmware explicitly includes `Kona/IORT.aml` in
  `AcpiTables.inf` and packages it with `AcpiTableDxe`. Elish includes
  `KonaPkg.dsc.inc`, but its `elish.dsc`/`elish.fdf` do not include an
  `AcpiTables.inf` or `AcpiTableDxe`. Kona's `UpdateAcpiTables()` only updates
  names in an already-installed DSDT; it does not install the IORT table.
  Therefore this source table is a useful comparison, not evidence that the
  active elish UEFI system table contains it.
- The template's output IDs `0x4e0`/`0xe0` do not match run11's only observed
  valid apps-SMMU match (`SID=0x820`, mask `0x402`). The run11 SID remains
  unassigned, and UFS DMA remains disabled.

When device access returns, add a read-only check of the live UEFI ACPI
configuration tables and dump the IORT header/nodes for `\\_SB_.UFS0`; only
then compare the HAL-resolved stream IDs with a fresh SMMU snapshot.
