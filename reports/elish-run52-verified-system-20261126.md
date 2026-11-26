# elish run52 verified UFS root

The build verifies the SHA-256 of each complete OS and SystemOS window from the installed 21H461 image's host copy. It embeds 185,474 OS and 100,800 SystemOS chunk digests. XNU hashes CPU-owned 32 KiB chunks before returning reads to APFS. The root authorization checks the selected BSD device, mounted APFS device, read-only mount, expected volume UUID, and the elish UFS provider. The SystemOS attachment uses the existing XNU owner-cache mount path.

The host identified `18d1:d00d`, serial `4bed07e4`, `product=elish`, and `is-userspace=no`. `fastboot -s 4bed07e4 boot "$OUT/boot.img"` reported both `OKAY` results in 1.826 seconds. The camera recorded the green, blue and magenta bands. The magenta band marks the instruction before MMU enable. The device displayed the same state at 65 seconds. Bootloader fastboot was visible in a later USB snapshot. The reset cause and any later XNU stages remain unknown.

The original RAM-only Lineage reader was recovered by a read-only export of the first 55,083,008 bytes of `boot_b`. Its SHA-256 matched `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`. Fastboot accepted the reader and the USB ADB transport returned `unauthorized`. No ramoops was recovered. The diagnostics issued no partition writes. Setup and Data persistence remain untested.

The digest tables were linked into `__TEXT` and shifted `__TEXT_EXEC` from `0xfffffff007240000` to `0xfffffff007afc000`. Run53 moves the tables to `__LASTDATA_CONST` to test the effect of the early executable mapping.

`OUT` is `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run52-verified-system`. See `source.sha256`, `kernel-build.log`, `storage-abi.log`, `boot-build.log`, `device.log`, and `reader.log`. Sources are device `2561162a380f3570f5e9cb8360a74603e23bf691`, firmware `d9d45b3e8ec4b79f6299e35f5890e3b72a1bfc30`, UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, and XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f` with the recorded local changes.

| Artifact | SHA-256 |
| --- | --- |
| `boot.img` | `7a3839d0a455409c7663024ac1b8483ed08287bf8fbaca27f1b55e054c39f12f` |
| `SILICIUM_UEFI.fd` | `fda4d664b23a58f28da7f567ff3b74fe6a54619c6649457fc29e47f4d867f55d` |
| `kernel.macho` from FAT | `600b7ec5a31a17281e598caac97b40987bad605c538544ff3ee788ebabf9da60` |
| `payload.img` | `ac60fea78efff839194feb6a930ef25e90d384304f797a4dcce093184f40f12a` |
| [65-second photo](elish-run52-pre-mmu-20261126.jpg) | `cd20787659273ca5ddf64cfe71518f80064300eed58a58ab7453c7f622685077` |
