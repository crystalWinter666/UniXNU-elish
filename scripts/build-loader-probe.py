#!/usr/bin/env python3
"""Build an elish loader diagnostic while reusing an existing FAT payload."""

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
    parser.add_argument("--payload", type=Path, required=True)
    parser.add_argument("--dtb", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--feeds", type=Path, help="Reuse an existing output root's feeds cache")
    parser.add_argument("--clang-bin", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=8)
    probe_mode = parser.add_mutually_exclusive_group()
    probe_mode.add_argument("--run-kernel", action="store_true", help="Continue after EBS into a terminal XNU checkpoint payload")
    probe_mode.add_argument("--smmu-readonly", action="store_true", help="Capture read-only SMMUv2 stream mappings before DarwinBoot")
    probe_mode.add_argument("--pmsg-readback", action="store_true", help="Log pmsg memory and the post-MMU return marker before DarwinBoot")
    args = parser.parse_args()
    core, uefi, payload, dtb, out = [
        p.resolve() for p in (args.unixnu, args.uefi, args.payload, args.dtb, args.out)
    ]
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
    replacement = 'ELISH_LOADER_PROBE = "TRUE"'
    if args.run_kernel:
        replacement += '\nELISH_LOADER_PROBE_RUN_KERNEL = "TRUE"'
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
         "--dtb", str(dtb), "--ramdisk", str(payload), "--output", str(out / "boot.img")],
    ]
    probe_description = (
        "Read pmsg memory and return marker, then stop after successful ExitBootServices"
        if args.pmsg_readback else
        "Capture read-only SMMUv2 state before DarwinBoot"
        if args.smmu_readonly else
        "Continue to terminal XNU checkpoint"
        if args.run_kernel else
        "Stop after successful ExitBootServices; XNU is not entered"
    )
    manifest = {"commands": commands, "payload": {"path": str(payload), "sha256": digest(payload)},
                "dtb": {"path": str(dtb), "sha256": digest(dtb)}, "probe": probe_description}
    (out / "inputs.json").write_text(json.dumps(manifest, indent=2) + "\n")
    subprocess.run(commands[0], cwd=core, check=True)
    shutil.copyfile(uefi / "Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd", out / "SILICIUM_UEFI.fd")
    subprocess.run(commands[1], cwd=core, check=True)
    if digest(payload) != manifest["payload"]["sha256"] or digest(dtb) != manifest["dtb"]["sha256"]:
        raise SystemExit("An input changed during the build")
    manifest["outputs"] = {name: {"bytes": (out / name).stat().st_size, "sha256": digest(out / name)}
                           for name in ("SILICIUM_UEFI.fd", "boot.img")}
    (out / "probe.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest["outputs"], indent=2))


if __name__ == "__main__":
    main()
