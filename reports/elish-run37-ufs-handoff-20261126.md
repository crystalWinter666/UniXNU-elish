# elish run37 UFS handoff and DMA arena

Run37 verified `0x4E0 → CB2` and `0xE0 → CB3` before SEC and reached the DXE
probe. DXE found only the MDSS match. Both UFS context banks had changed to
`CBAR=0x000100FF`. The SEC module's generated build flags included
`ELISH_UFS_HANDOFF_PROBE`. The Kona `PlatformSecLib` generated flags omitted
it because the DSC module-type filter did not apply to that library. This
build therefore still used the display-only detach list.

The gated UFS host was enabled and ready (`HCE=1`, `HCS=0x10f`). The transfer
list was running with a zero doorbell. The task list was stopped with a zero
doorbell. `AllocatePages(AllocateMaxAddress, EfiBootServicesData, 3, ...)`
reserved `[0x9A8B8000,0x9A8BB000)`, entirely below 4 GiB and within the 36-bit
architectural limit. DXE then freed the unused reservation. The test issued
zero UFS commands and used zero DMA buffers in a transfer.

The run followed [run36](elish-run36-sec-smmu-20261126.md) with the same
source bases and DTB. It used `--ufs-handoff-readonly` with the unchanged
run28 FAT payload, SHA-256 `f06570608c20cca5a47c826b1f813ced5efe87d816b9b0429af68b22c2dc523f`.
The artifacts are under
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run37-ufs-handoff-readonly/`.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` (235,528,192 bytes) | `6c66ecbb189c7c6096683e11f69aa94db04e54f63a66b9a828742b2c1cfb60a6` |
| `SILICIUM_UEFI.fd` (3,145,728 bytes) | `6b11a1669d820b8bdda6423ede18ebed1534f8e1efa25eca519b8420946c40ed` |
| `Sec.c` | `8a6deb52b171327e617ea2c2dd6bc836515776de69e937b4289c1cf054b8bf4b` |
| `PlatformSec.c` | `c344375ef426fcce54fb7ac936afce4c4a24e83cc0450a4db03ec020889d442b` |
| `ElishLoaderProbeDxe.c` | `a743f9f94ffbc789f9d27324f1b698a827f06f3294bd54a97d3a52b60825b77d` |
| `build-loader-probe.py` | `3ec25dd56aec88d91715c629c08f0bb8ef724d2439041c2e01d0c5948f34d6bd` |
| Console ramoops (56,130 bytes) | `4571de5cb5b8daff7f9a2a9353fd4439d641283567dcdc4c7fa2e8b977372da7` |

Serial `4bed07e4`, USB `18d1:d00d`, and `product=elish` passed preflight.
`fastboot -s 4bed07e4 boot "$OUT/boot.img"` completed in 5.898 seconds. The
warm reset changed USB device number 35 to 36. The verified RAM-only reader
booted in 1.573 seconds. Root ADB exported `console-ramoops-0`. No device
partition write command ran. [Raw log](elish-run37-ufs-handoff-console-ramoops-20261126.txt)
