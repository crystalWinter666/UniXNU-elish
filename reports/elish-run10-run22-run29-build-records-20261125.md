# Run10, Run22 and Run29 host build records

These elish builds were packaged on 2026-11-25 for iPadOS 17.7.11 (21H461).
The archives contain the build metadata and logs. The firmware probes also
include the board and lock snapshots used by the build. This is host evidence.

| Run | Recorded probe | Source records |
| --- | --- | --- |
| Run10 at 04:56 UTC+8 | Firmware probe configured to stop after ExitBootServices | [Inputs](elish-run10-smmu-maskfix/inputs.json), [outputs](elish-run10-smmu-maskfix/probe.json), [firmware build](elish-run10-smmu-maskfix/firmware.txt) |
| Run22 at 19:06 UTC+8 | XNU terminal checkpoint after MMU enable, with hardware_tested=false recorded | [Checkpoint metadata](elish-run22-4g-postmmu/kernel.checkpoint.json), [build and packaging log](elish-run22-4g-postmmu/build.txt) |
| Run29 at 20:35 UTC+8 | Read-only UFS/GCC clock probe configured to warm-restart before DarwinBoot | [Inputs](elish-run29-ufs-clocks-readonly/inputs.json), [outputs](elish-run29-ufs-clocks-readonly/probe.json), [firmware build](elish-run29-ufs-clocks-readonly/firmware.txt) |

| Local build artifact | SHA-256 |
| --- | --- |
| Run10 boot.img | b805ed28f1849231d56c2692a4a9f848e2ae3545a4601aebd221f23e6f6cec33 |
| Run10 SILICIUM_UEFI.fd | 4f8043b1d51068d203a723bdcf9d1f3ac8f4904a7c171f8c4a00f647f83c345d |
| Run22 boot.img | d5a619151692f4cb4f8c08e6bfb57b7c757d7eb3a62c4c88fea07f784612ea9a |
| Run22 payload.img | a3906eed306a8bbd42612a6ee759d9e8cd6b43c1ccb2e3f8ec4b1ba3a7f40007 |
| Run22 kernel.macho | c517634022a99e8dd9b9fe39bc1b21bdb31a73192dbbda2cc674b038a301d554 |
| Run29 boot.img | c745b50052edd4438f43754af8a3e2f21bbe4d49cca2bb80cfc0006d0104db78 |
| Run29 SILICIUM_UEFI.fd | 7570c99f02291dd152b49eeb06b35ddcc281e9a35a70db5ddf4c52487e072baa |
