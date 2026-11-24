# elish XBL platform-config lookup audit, 2026-11-24

## Inputs and tools

- Stock input `/root/repo/elish_os1.0.2.0/xbl.img`, SHA-256 `9a3a3bb92e6cf478b227f70dd1bd140fde32982994a6c34a07bd314ea0acb72f`.
- Ghidra `12.2_PUBLIC_20261016`, downloaded from the National Security Agency's GitHub release. The archive SHA-256 matched the release API digest `2c2cd776db3c1f6a097ba418922378ac3649cadd5e844a6f5f088ae628a7be67`.
- Eclipse Temurin JDK `25.0.5.1+2`, downloaded from Adoptium. The archive SHA-256 matched its package metadata `760f91b22c3efc9bd1b64ff950598231be82928cc807e30691a0e7a2995a62c0`.
- Ghidra project `/tmp/elish-ghidra-projects/ElishXbl`, analysis scripts under `/tmp/elish-ghidra-scripts`. No firmware binary was copied into this repository.

## Commands

```sh
JAVA_HOME=/opt/jdk-25 PATH=/opt/jdk-25/bin:$PATH \
  /opt/ghidra-12.2/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  /tmp/elish-ghidra-projects ElishXbl \
  -import /root/repo/elish_os1.0.2.0/xbl.img -overwrite \
  -analysisTimeoutPerFile 300 -max-cpu 8 \
  -scriptPath /tmp/elish-ghidra-scripts \
  -postScript FindElishCfgReferences.java

JAVA_HOME=/opt/jdk-25 PATH=/opt/jdk-25/bin:$PATH \
  /opt/ghidra-12.2/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  /tmp/elish-ghidra-projects ElishXbl -process xbl.img -noanalysis \
  -scriptPath /tmp/elish-ghidra-scripts \
  -postScript DecompileElishCfgLoader.java

JAVA_HOME=/opt/jdk-25 PATH=/opt/jdk-25/bin:$PATH \
  /opt/ghidra-12.2/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  /tmp/elish-ghidra-projects ElishXbl -process xbl.img -noanalysis \
  -scriptPath /tmp/elish-ghidra-scripts \
  -postScript DecompileElishConfigParams.java
```

Ghidra loaded the XBL as AArch64 ELF and analyzed 4,230 functions.

## Findings

- `FUN_9fc107e0` requests `uefiplat.cfg` through `FUN_9fc12890`, then parses the `Config`, `MemoryMap`, `RegisterMap`, `ConfigParameters` and `ChipIDConfig` sections.
- `FUN_9fc10e14` handles `ConfigParameters`. It reads `ConfigParameterCount`, allocates a key/value table, and copies each parameter into that table.
- The embedded `ConfigParameters` values include `PlatConfigFileName = "uefiplatLA.cfg"` and `OsTypeString = "LA"`. The `uefiplatLA.cfg` string is at XBL address `0x9fc413da`.
- The analysis did not identify the routine that consumes `PlatConfigFileName` to open the named file. The supplied stock ROM images contain the reference in XBL, while searches found no file contents under those images or the checked-out UEFI repository.
- The embedded `MemoryMap` describes no range above `0xb0000000`. This remains static configuration evidence and does not expose the live UEFI `GetMemoryMap` or DarwinBoot's runtime allocation base.

The config-name consumer, file source, selected contents and runtime memory map remain unresolved. This analysis adds no device-side observation.
