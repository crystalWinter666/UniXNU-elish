#!/usr/bin/env python3
"""Build an elish loader or SEC diagnostic image."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    device = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unixnu", type=Path, default=device.parent / "UniXNU")
    parser.add_argument("--uefi", type=Path, default=device.parent / "uefi-elish")
    parser.add_argument("--payload", type=Path, help="Existing FAT payload for a loader probe")
    parser.add_argument("--dtb", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--feeds", type=Path, help="Reuse an existing output root's feeds cache")
    parser.add_argument("--clang-bin", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=8)
    probe_mode = parser.add_mutually_exclusive_group()
    probe_mode.add_argument("--run-kernel", action="store_true", help="Continue after EBS into a terminal XNU checkpoint payload")
    probe_mode.add_argument("--smmu-readonly", action="store_true", help="Capture read-only SMMUv2 stream mappings before DarwinBoot")
    probe_mode.add_argument("--ufs-clocks-readonly", action="store_true", help="Sample Kona GCC and gated UFSHCI status and queue registers, then stop before DarwinBoot")
    probe_mode.add_argument("--sec-smmu-readonly", action="store_true", help="Capture inherited SMMU routes before SEC detaches them, without a ramdisk")
    probe_mode.add_argument("--ufs-handoff-readonly", action="store_true", help="Preserve qualified UFS routes through SEC and sample them in DXE without issuing DMA")
    probe_mode.add_argument("--ufs-nop", action="store_true", help="Check the inherited UFS routes and submit one bounded no-data NOP before reboot")
    probe_mode.add_argument("--ufs-gpt", action="store_true", help="Run NOP, read LUN 0 capacity, and validate the known elish GPT blocks")
    probe_mode.add_argument("--ufs-root-windows", action="store_true", help="Validate both elish GPT copies, then sample the manifest and OS/SystemOS LBAs without writing")
    probe_mode.add_argument("--ufs-run-kernel", action="store_true", help="Preserve qualified UFS routes and enter XNU without touching the inherited queues")
    probe_mode.add_argument("--pmsg-readback", action="store_true", help="Log pmsg memory and the post-MMU return marker before DarwinBoot")
    probe_mode.add_argument("--ramoops-viewer", action="store_true", help="Display preserved touch diagnostics in DXE without a ramdisk")
    args = parser.parse_args()
    firmware_only = args.sec_smmu_readonly or args.ramoops_viewer
    if firmware_only and args.payload is not None:
        parser.error("firmware-only diagnostics require an empty ramdisk; omit --payload")
    if not firmware_only and args.payload is None:
        parser.error("--payload is required for loader probes")
    core, uefi, dtb, out = [p.resolve() for p in (args.unixnu, args.uefi, args.dtb, args.out)]
    payload = args.payload.resolve() if args.payload is not None else None
    # Never overwrite a previous test's artifacts or reuse its changed inputs.
    out.mkdir(parents=True, exist_ok=False)
    shadow = out / "device"
    shadow.mkdir()
    for path in device.iterdir():
        if path.name in {".git", "out", "build", "board.toml", "unixnu.lock"}:
            continue
        (shadow / path.name).symlink_to(path, target_is_directory=path.is_dir())
    original = (device / "board.toml").read_text()
    switch = 'ELISH_LOADER_PROBE = "FALSE"'
    if original.count(switch) != 1:
        raise SystemExit("Expected exactly one disabled ELISH_LOADER_PROBE in board.toml")
    if args.ramoops_viewer:
        # The normal DEBUG SEC logger clears the preserved console on entry.
        logging = 'ELISH_RAMOOPS_LOG = "TRUE"'
        if original.count(logging) != 1:
            raise SystemExit("Expected exactly one enabled ELISH_RAMOOPS_LOG in board.toml")
        original = original.replace(logging, 'ELISH_RAMOOPS_LOG = "FALSE"')
        replacement = 'ELISH_RAMOOPS_VIEWER = "TRUE"'
    elif args.sec_smmu_readonly:
        replacement = 'ELISH_SEC_SMMU_PROBE = "TRUE"'
    else:
        replacement = 'ELISH_LOADER_PROBE = "TRUE"'
    if args.run_kernel or args.ufs_run_kernel:
        replacement += '\nELISH_LOADER_PROBE_RUN_KERNEL = "TRUE"'
    if args.ufs_clocks_readonly:
        replacement += '\nELISH_LOADER_PROBE_UFS_CLOCKS = "TRUE"'
    if args.ufs_handoff_readonly or args.ufs_nop or args.ufs_gpt or args.ufs_root_windows or args.ufs_run_kernel:
        replacement += '\nELISH_UFS_HANDOFF_PROBE = "TRUE"'
    if args.ufs_nop:
        replacement += '\nELISH_UFS_NOP_PROBE = "TRUE"'
    if args.ufs_gpt or args.ufs_root_windows:
        replacement += '\nELISH_UFS_GPT_PROBE = "TRUE"'
    if args.ufs_root_windows:
        replacement += '\nELISH_UFS_ROOT_WINDOWS_PROBE = "1"'
    if args.smmu_readonly:
        replacement += '\nELISH_LOADER_PROBE_SMMU = "TRUE"'
    if args.pmsg_readback:
        replacement += '\nELISH_LOADER_PROBE_PMSG = "TRUE"'
    (shadow / "board.toml").write_text(original.replace(switch, replacement))
    shutil.copyfile(device / "unixnu.lock", shadow / "unixnu.lock")
    build_root = out / "build"
    build_root.mkdir()
    if args.feeds:
        (build_root / "feeds").symlink_to(args.feeds.resolve(), target_is_directory=True)
    common = [str(core / "unixnu"), "-D", str(shadow), "--os", "ipados-17.7.11-21H461",
              "--out", str(build_root), "--uefi", str(uefi),
              "--edk2", str(uefi / "Mu_Basecore"), "--clang-bin", str(args.clang_bin.resolve())]
    commands = [
        [common[0], "build", "firmware", *common[1:], "-j", str(args.jobs), "--target", "DEBUG"],
        [common[0], "bootimg", *common[1:], "--fd", str(out / "SILICIUM_UEFI.fd"),
         "--dtb", str(dtb), "--ramdisk", str(payload) if payload is not None else "none",
         "--output", str(out / "boot.img")],
    ]
    probe_description = (
        "Read touch lines from the preserved console ring in DXE and return to fastboot"
        if args.ramoops_viewer else
        "Capture inherited SMMUv2 stream mappings in SEC before PlatformInitialize; warm-restart without entering DXE"
        if args.sec_smmu_readonly else
        "Validate native capacity and both GPTs, then read bounded userdata root-window samples before DarwinBoot"
        if args.ufs_root_windows else
        "After bounded NOP, read LUN 0 capacity and validate the known elish primary and backup GPT"
        if args.ufs_gpt else
        "Qualify the inherited UFS routes, submit one bounded no-data NOP, and restart before DarwinBoot"
        if args.ufs_nop else
        "Preserve qualified UFS routes through SEC and enter XNU with the inherited queues"
        if args.ufs_run_kernel else
        "Preserve inherited UFS routes through SEC, read back DXE state, and test low DMA arena allocation"
        if args.ufs_handoff_readonly else
        "Read pmsg memory and return marker, then stop after successful ExitBootServices"
        if args.pmsg_readback else
        "Validate UFS/GCC windows, sample Kona clocks, reset and GDSC, read UFSHCI status and queue registers only with reset released and GDSC and core/interface clocks on, capture IORT/SMMUv2 state, and warm-restart before DarwinBoot"
        if args.ufs_clocks_readonly else
        "Capture read-only SMMUv2 state before DarwinBoot"
        if args.smmu_readonly else
        "Continue to terminal XNU checkpoint"
        if args.run_kernel else
        "Stop after successful ExitBootServices; XNU is not entered"
    )
    manifest = {"commands": commands, "payload": None if payload is None else {
                    "path": str(payload), "sha256": digest(payload)},
                "dtb": {"path": str(dtb), "sha256": digest(dtb)}, "probe": probe_description}
    (out / "inputs.json").write_text(json.dumps(manifest, indent=2) + "\n")
    subprocess.run(commands[0], cwd=core, check=True)
    shutil.copyfile(uefi / "Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd", out / "SILICIUM_UEFI.fd")
    subprocess.run(commands[1], cwd=core, check=True)
    if ((payload is not None and digest(payload) != manifest["payload"]["sha256"])
            or digest(dtb) != manifest["dtb"]["sha256"]):
        raise SystemExit("An input changed during the build")
    manifest["outputs"] = {name: {"bytes": (out / name).stat().st_size, "sha256": digest(out / name)}
                           for name in ("SILICIUM_UEFI.fd", "boot.img")}
    (out / "probe.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest["outputs"], indent=2))


if __name__ == "__main__":
    main()
