#!/usr/bin/env python3
"""Add a persistent-pmsg marker to a copy of the elish XNU entry probe."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile


MARKER = b"ELISH_XNU_ENTRY_PMSG\n"
PMSG_BASE = 0xB0200000
PMSG_SIZE = 0x200000
PERSISTENT_RAM_SIG = 0x43474244


def load_probe(path):
    sys.path.insert(0, str(path.parent))
    spec = importlib.util.spec_from_file_location("entry_probe", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decode_branch(instruction, address):
    if instruction >> 26 != 0b000101:
        raise SystemExit(f"expected unconditional branch at {address:#x}")
    delta = instruction & 0x03FFFFFF
    if delta & (1 << 25):
        delta -= 1 << 26
    return address + delta * 4


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kernel", type=Path, help="copy of the visual entry-probe kernel")
    parser.add_argument("output", type=Path, help="separate output kernel path")
    parser.add_argument("--unixnu", type=Path, default=Path(__file__).resolve().parents[2] / "UniXNU")
    args = parser.parse_args()

    helper = args.unixnu.resolve() / "scripts/patch-xnu-entry-probe.py"
    probe = load_probe(helper)
    data = bytearray(args.kernel.read_bytes())
    before = hashlib.sha256(data).hexdigest()
    probe.load_segments(data)

    entry = probe.unixthread_pc(data)
    delta = entry - probe.ENTRY_VA
    stage1 = probe.CAVES[1] + delta
    stage4 = probe.CAVES[4] + delta
    code_va = stage4 + 0x100
    if len(MARKER) > PMSG_SIZE - 12 or 12 + len(MARKER) > 64:
        raise SystemExit("pmsg marker must fit the region and one cache line")
    entry_offset = probe.file_offset(entry)
    if decode_branch(struct.unpack_from("<I", data, entry_offset)[0], entry) != stage1:
        raise SystemExit("kernel entry is not patched to the expected blue-bar stage")

    target = decode_branch(struct.unpack_from("<I", data, probe.file_offset(stage1 + 0x3C))[0], stage1 + 0x3C)
    probe.file_offset(target)
    with tempfile.TemporaryDirectory(prefix="elish-pmsg-stage1-") as temporary:
        base_stage1 = probe.assemble(
            probe.SOURCES[1], stage1, {"target": target}, Path(temporary)
        )
    if len(base_stage1) != 0x40 or data[probe.file_offset(stage1):probe.file_offset(stage1) + 0x3C] != base_stage1[:0x3C]:
        raise SystemExit("existing stage-1 blue-bar sequence differs from the expected probe")

    source = f'''.text
.global _start
_start:
  movz x3, #0xb020, lsl #16
  adr x5, message
  add x6, x3, #12
  mov w7, #{len(MARKER)}
1:
  ldrb w8, [x5], #1
  strb w8, [x6], #1
  subs w7, w7, #1
  b.ne 1b
  add x9, x3, #12
  dc cvac, x9
  dsb sy
  movz w4, #0x4244
  movk w4, #0x4347, lsl #16
  str w4, [x3]
  str wzr, [x3, #4]
  mov w4, #{len(MARKER)}
  str w4, [x3, #8]
  dc cvac, x3
  dsb sy
{probe.PAINT.format(args='x0', band=1, lo='0x00FF', hi='0xFF00')}  b target
.balign 4
message:
  .ascii {json.dumps(MARKER.decode())}
.ltorg
'''

    cave_start = probe.file_offset(code_va)
    cave_limit = 0x300
    if any(data[cave_start:cave_start + cave_limit]):
        raise SystemExit(f"pmsg checkpoint cave at {code_va:#x} is not empty")
    with tempfile.TemporaryDirectory(prefix="elish-xnu-pmsg-") as temporary:
        code = probe.assemble(source, code_va, {"target": target}, Path(temporary))
    if len(code) > cave_limit:
        raise SystemExit(f"pmsg checkpoint is too large for cave: {len(code):#x}")

    data[cave_start:cave_start + len(code)] = code
    struct.pack_into("<I", data, entry_offset, probe.branch(entry, code_va))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    manifest = {
        "input": str(args.kernel.resolve()),
        "output": str(args.output.resolve()),
        "before_sha256": before,
        "after_sha256": hashlib.sha256(data).hexdigest(),
        "entry": hex(entry),
        "code_va": hex(code_va),
        "code_bytes": len(code),
        "branch_target": hex(target),
        "marker": MARKER.decode().rstrip("\n"),
        "pmsg_region": f"{PMSG_BASE:#x}+{PMSG_SIZE:#x}",
        "pmsg_header": {"signature": hex(PERSISTENT_RAM_SIG), "start": 0, "size": len(MARKER)},
        "terminal_probe": False,
        "hardware_tested": False,
    }
    args.output.with_suffix(args.output.suffix + ".pmsg.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
