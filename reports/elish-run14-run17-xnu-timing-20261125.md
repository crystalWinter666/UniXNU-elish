# elish XNU checkpoint timing, 2026-11-25

Device: Xiaomi Pad 5 Pro Wi-Fi (`M2105K81AC` / `elish`), target iPadOS 17.7.11
(`21H461`). These were RAM-only diagnostic images. No persistent partition was
written and `fastboot continue` was not used.

## Build provenance

Workspace HEADs at review time: device repository
`4b3da68ea0aca815a80e8d1bad802dfec8d5c905`, UniXNU
`71c5c6569bae6369c0e0f634df8b9bd5afc19139`, and UEFI firmware
`f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`. The XNU feed snapshot is
`d8b80295118ef25ac3a784134bcf95cd8e88109f`.

The run15 build manifest records `unixnu build firmware` and `unixnu bootimg`
for profile `ipados-17.7.11-21H461`, using `/tmp/elish-uefi-probe.dtb` and
`/tmp/elish-run15-console-create/payload.img`; the payload SHA-256 is
`2f7c8c7418bb333f3e89f41cb6aa8b8382a93394cbde6b87abf6a4986a7fc517`. The
checkpoint patch modes were `--after-console-create`, `--before-printf`, and
`--after-printf-init`, each with a 15-second hold. Run14 used
`--after-vm-init --hold-seconds 30`. The run15 command manifest is
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run15-console-create/probe.json`.

## Images and device observations

| Run | Image SHA-256 | Intended checkpoint | Observation |
| --- | --- | --- | --- |
| run14 | `499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f` | Call `_arm_vm_init`, wait 30 seconds after return, then PSCI reset | One reported run returned to fastboot about 43 seconds after launch. A later retry stalled in USB bulk transfer before `Sending OKAY`/`Booting OKAY`; that retry did not execute the image. Treat the earlier timing as indirect and the repeated transport as unreliable. |
| run15 | `cfcdf0e2e7d0ff1f384537c9834321f7d15bd008e95a7db0fad940a9097b618b` | Call `_PE_create_console`, wait 15 seconds, then PSCI reset | Device returned to fastboot about 27 seconds after launch. Timing supports reaching and returning from the call site, but there is no captured screen or serial marker. |
| run16 | `6922981c36442748a8cdd97e657875e9e5409f5ce28f72d9b9c75fae0ecf81d0` | At the `_PE_init_printf` call site, wait 15 seconds without calling it, then PSCI reset | Device returned to fastboot about 28 seconds after launch. Timing supports reaching the call site after console creation; it does not test `PE_init_printf()` itself. |
| run17 | `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e` | Call `PE_init_printf`, wait 15 seconds, then PSCI reset | An initial attempt returned `unknown command` before image acceptance. A later retry was accepted (`Sending OKAY`, `Booting OKAY`) and the device returned to fastboot near the configured reset interval. This timing is indirect; no close screen photo or run17 pstore readback confirms the call or MMU checkpoint. A hash-verified reader subsequently restored root LineageOS ADB. |

Each image is 67,751,936 bytes. The run15-run17 manifests share kernel SHA-256
`7af91a7e047b4d448387ec018cdcb01f867fef78c927977eefb68b70bfd16335`, FD
SHA-256 `0eb05020b877787e0f283a8eb9c10a4283b903e6e8fcb741f177595bbe33fb45`,
and DTB SHA-256 `0fe90a0e1b8bd745d266c04111991c9e61ff3187fd8807b05928780626cdec48`.
Run17 is at
`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run17-printf-init/boot.img`.

After runs14-16, and again after the accepted run17 retry, a hash-verified
LineageOS reader was used with root ADB to inspect `/sys/fs/pstore`. It was
empty. The reader kernel reports ramoops at
`0xb0000000+0x400000`, `record_size=0`, and
`Invalid compression size for deflate: 0`; this is not evidence that the XNU
markers did not execute. No new XNU text or pstore marker was recovered.

## Source-path review

The built XNU source snapshot is commit
`d8b80295118ef25ac3a784134bcf95cd8e88109f` under
`/root/elish-build-20261123/UniXNU/out/feeds/xnu-b7545ced`.
`_arm_init()` calls `arm_vm_init()`, then performs low-global/debug setup,
`serial_init()`, kprintf startup, `PE_create_console()`, and
`PE_init_printf()`. The run15 and run16 hook locations therefore lie after the
`arm_vm_init()` call in source order.

`PE_init_printf(FALSE)` itself only sets `PE_putc = console_write_char` and
returns. Thus run16 stops immediately before this assignment, while run17 calls
it and resets before the first later `kprintf` at that site. A successful run17
reset would show the call returned; it would not prove that the framebuffer or
serial backend renders output, nor that later platform initialization succeeds.

Inside `arm_vm_init()`, the code builds the bootstrap mappings, sets the new
TTBR and flushes the TLB, then initializes AuxKC, calls `pmap_bootstrap()`, and
creates L3 mappings before returning. A timed return after the later call sites
is consistent with `arm_vm_init()` having returned, but USB re-enumeration
alone does not prove the timer or reset path ran at the intended instruction.
Even a confirmed return would establish only this VM bootstrap path, not UFS,
APFS, or userspace.

## Current host state and evidence boundary

At 07:53 CST, `lsusb -d 18d1:d00d -v` briefly listed serial `4bed07e4` as a
fastboot interface. The subsequent read-only command
`timeout 8s /tmp/elish-platform-tools/platform-tools/fastboot -s 4bed07e4 getvar product`
immediately returned `usb_read failed: No such device`. ADB listed no device.
No image transfer or reset was attempted after that failure.

The top-mounted camera frame
`/tmp/elish-current-state-followup.png` (640x480, SHA-256
`a782ae8c5e04db2822d03c06b2da9e934c280386d3d64952aeedc2ce07b18468`) is
underexposed and cannot identify the screen stage. No user screen photo was
available for these runs.

The latest direct XNU-stage evidence remains the run09 close photo showing the
blue XNU-entry band and magenta pre-MMU band. MMU completion remains unconfirmed;
UFS/SMMU ownership, device-side APFS root mounting, and iPadOS userspace remain
unverified. The most useful next test is run17 after fastboot control requests
work reliably, followed by a reader/pstore check only after confirmed bootloader
acceptance. If direct evidence is still unavailable, add a narrowly placed
checkpoint within `arm_vm_init()` rather than infer success from USB timing.

## 08:14 CST host recheck

`lsusb -d 18d1:d00d`, fastboot 37.0.3 `devices -l`, and `adb devices -l`
showed no target interface. No fastboot control request or image transfer was
sent. The device is not available for the run17 check at this snapshot.
