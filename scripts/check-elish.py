#!/usr/bin/env python3
"""Validate the elish scaffold and gate builds on unverified hardware inputs."""
import argparse
import os
import sys
import tomllib
from pathlib import Path


PROFILE = "ipados-17.7.11-21H461"


def load_toml(path):
    with path.open("rb") as stream:
        return tomllib.load(stream)


def main():
    default_device = Path(os.environ.get("UNIXNU_DEVICE", Path(__file__).resolve().parents[1]))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", type=Path, default=default_device)
    parser.add_argument(
        "--static-only",
        action="store_true",
        help="validate the scaffold and report readiness blockers without failing on them",
    )
    args = parser.parse_args()
    device = args.device.resolve()
    errors = []
    blockers = []

    try:
        board = load_toml(device / "board.toml")
        feeds = load_toml(device / "feeds.toml")
        lock = load_toml(device / "unixnu.lock")
    except (OSError, tomllib.TOMLDecodeError) as exc:
        print(f"elish preflight: cannot read device configuration: {exc}", file=sys.stderr)
        return 2

    identity = board.get("board", {})
    if board.get("schema_version") != 2:
        errors.append("board.toml must use format 2")
    if identity.get("name") != "elish" or identity.get("example") is not False:
        errors.append("board identity must be a non-example elish device")
    if identity.get("soc") != "SM8250-AC":
        errors.append("board SoC must be SM8250-AC")
    if identity.get("os_profile") != PROFILE:
        errors.append(f"board OS profile must be {PROFILE}")

    kernel = board.get("kernel", {})
    defconfig = kernel.get("defconfig", "")
    for relative in (f"configs/{defconfig}" if defconfig else "", "dts/elish.dts", "scripts/package.sh"):
        if relative and not (device / relative).is_file():
            errors.append(f"required device file is missing: {relative}")

    source = board.get("device_tree", {}).get("source", "")
    if source != "device:dts/elish.dts":
        errors.append("device_tree.source must select device:dts/elish.dts")

    source_entries = feeds.get("source", [])
    soc_pins = [item.get("commit") for item in source_entries if item.get("name") == "sm8250"]
    locked_soc = lock.get("source", {}).get("sm8250", {}).get("commit")
    if len(soc_pins) != 1 or not soc_pins[0]:
        errors.append("feeds.toml must contain one pinned sm8250 source")
    elif locked_soc != soc_pins[0]:
        errors.append("unixnu.lock sm8250 commit differs from feeds.toml")

    validation = board.get("validation", {})
    memory_state = validation.get("memory_map", "unverified")
    userdata_state = validation.get("userdata_geometry", "unverified")
    uefi_state = validation.get("uefi_platform", "pending")
    if memory_state != "verified":
        blockers.append("runtime DRAM banks and reserved ranges are not verified")
    if userdata_state != "verified":
        blockers.append("userdata GPT LBAs and logical sector size are not verified")
    if uefi_state != "ready":
        blockers.append("the elish Mu-Silicium UEFI platform is not ready")

    userdata = board.get("userdata", {})
    if userdata_state != "verified":
        if any(userdata.get(key) != 0 for key in ("first_lba", "last_lba", "lba_bytes")):
            errors.append("unverified userdata geometry must remain zero-valued sentinels")
    elif not (0 < userdata.get("lba_bytes", 0) and 0 < userdata.get("first_lba", 0) <= userdata.get("last_lba", 0)):
        errors.append("verified userdata geometry must have positive, ordered bounds")

    dts_path = device / "dts/elish.dts"
    if dts_path.is_file():
        dts = dts_path.read_text(encoding="utf-8")
        if memory_state != "verified" and 'device_type = "memory"' in dts:
            errors.append("DTS must not publish an unverified DRAM range")
        if memory_state == "verified" and 'device_type = "memory"' not in dts:
            errors.append("verified DRAM state requires a memory node in the DTS")

    config_path = device / "configs" / kernel.get("defconfig", "")
    if config_path.is_file():
        config = config_path.read_text(encoding="utf-8").splitlines()
        stage_one_forbidden = {
            "CONFIG_XNU_NATIVE_UFS",
            "CONFIG_XNU_NATIVE_MOBILE_DISPLAY",
            "CONFIG_XNU_NATIVE_LEGACY_DISPLAY",
            "CONFIG_XNU_NATIVE_ADRENO",
        }
        enabled = {
            line.split("=", 1)[0]: line.split("=", 1)[1]
            for line in config
            if "=" in line and not line.lstrip().startswith("#")
        }
        for option in stage_one_forbidden:
            if enabled.get(option) in ("y", "1"):
                errors.append(f"stage-1 defconfig must not enable {option}")

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 2

    print("elish static scaffold checks passed")
    for blocker in blockers:
        print(f"BLOCKED: {blocker}")
    if args.static_only:
        return 0
    if blockers:
        print("elish preflight: refusing firmware/image build until blockers are resolved", file=sys.stderr)
        return 1

    uefi_value = os.environ.get("UNIXNU_UEFI")
    if not uefi_value:
        print("elish preflight: UNIXNU_UEFI is not set", file=sys.stderr)
        return 2
    dsc = board.get("firmware", {}).get("dsc", "")
    if not (Path(uefi_value) / dsc).is_file():
        print(f"elish preflight: UEFI platform DSC is missing: {dsc}", file=sys.stderr)
        return 2
    print("elish preflight: hardware evidence and UEFI platform are ready")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
