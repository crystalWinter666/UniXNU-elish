#!/usr/bin/env python3
"""Patch a kernel COPY to record an elish MMU-off checkpoint and warm-reset.

The elish firmware must first prepare PON warm reset and the Android ramoops
console. This is a terminal diagnostic, not a kernel intended to run iPadOS.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kernel', type=Path, help='unmodified COPY of the prelinked kernel')
    parser.add_argument('--unixnu', type=Path, default=Path(__file__).resolve().parents[2] / 'UniXNU')
    parser.add_argument('--stage', choices=['entry', 'before-mmu'], default='before-mmu')
    args = parser.parse_args()
    helper = args.unixnu / 'scripts/patch-xnu-entry-probe.py'
    sys.path.insert(0, str(helper.parent))
    spec = importlib.util.spec_from_file_location('entry_probe', helper)
    probe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(probe)
    data = bytearray(args.kernel.read_bytes())
    before = hashlib.sha256(data).hexdigest()
    probe.load_segments(data)
    probe.rebase(args.kernel, data)
    site = probe.ENTRY_VA if args.stage == 'entry' else probe.MMU_ENABLE_VA
    expected = probe.branch(probe.ENTRY_VA, probe.START_FIRST_CPU) if args.stage == 'entry' else 0xD5181000
    if struct.unpack_from('<I', data, probe.file_offset(site))[0] != expected:
        raise SystemExit('Checkpoint instruction does not match the unmodified kernel')
    cave = probe.ENTRY_VA + 4
    text = f'ELISH_XNU_CHECKPOINT: {args.stage}; SCTLR.M=0; terminal probe reset\n'
    # No stack, firmware call or virtual address is used. Preserve the existing
    # 2 MiB console's header and append only when its ring geometry is valid.
    source = f'''.text
.global _start
_start:
  movz x3, #0xb000, lsl #16
  ldr w4, [x3]
  movz w5, #0x4244
  movk w5, #0x4347, lsl #16
  cmp w4, w5
  b.ne reset
  ldr w4, [x3, #4]
  ldr w5, [x3, #8]
  ldr w6, =0x1ffff4-{len(text)}
  cmp w4, w6
  b.hi reset
  cmp w5, w4
  b.ne reset
  mrs x7, sctlr_el1
  tbnz x7, #0, reset
  add x6, x3, #12
  add x6, x6, x4
  adr x7, message
  mov w8, #{len(text)}
1:
  ldrb w9, [x7], #1
  strb w9, [x6]
  dc cvac, x6
  add x6, x6, #1
  subs w8, w8, #1
  b.ne 1b
  dsb sy
  add w4, w4, #{len(text)}
  add w5, w5, #{len(text)}
  ldr w10, =0x1ffff4
  cmp w4, w10
  csel w4, wzr, w4, eq
  str w4, [x3, #4]
  str w5, [x3, #8]
  dc cvac, x3
  dsb sy
reset:
  ldr x3, =0x146bf65c
  ldr w4, =0x77665500
  str w4, [x3]
  dsb sy
  movz w0, #0x0009
  movk w0, #0x8400, lsl #16
  mov x1, xzr
  mov x2, xzr
  mov x3, xzr
  smc #0
2:
  wfe
  b 2b
.balign 4
message:
  .ascii {json.dumps(text)}
.ltorg
'''
    with tempfile.TemporaryDirectory(prefix='elish-xnu-checkpoint-') as temporary:
        code = probe.assemble(source, cave, {}, Path(temporary))
    at = probe.file_offset(cave)
    if len(code) > 0x400 or len(data[at:at + len(code)]) != len(code) or any(data[at:at + len(code)]):
        raise SystemExit('Checkpoint does not fit the verified empty entry padding')
    data[at:at + len(code)] = code
    struct.pack_into('<I', data, probe.file_offset(site), probe.branch(site, cave))
    args.kernel.write_bytes(data)
    manifest = {'kernel': str(args.kernel.resolve()), 'stage': args.stage, 'site': hex(site),
                'cave': hex(cave), 'code_bytes': len(code), 'before_sha256': before,
                'after_sha256': hashlib.sha256(data).hexdigest(), 'message': text,
                'ramoops': '0xb0000000+0x200000', 'imem_restart_reason': '0x146bf65c',
                'requires': 'elish firmware RamLogReset prepared PON warm fastboot',
                'terminal_probe': True, 'hardware_tested': False}
    args.kernel.with_suffix('.checkpoint.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
