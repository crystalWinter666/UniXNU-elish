#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Mark the first successful launchd initproc_spawned write on an elish test image."""
import argparse
import importlib.util
from pathlib import Path
import struct
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kernel', type=Path, help='unmodified copy of the prelinked kernel')
    parser.add_argument('--unixnu', type=Path, default=Path(__file__).resolve().parents[2] / 'UniXNU')
    args = parser.parse_args()
    helper = args.unixnu / 'scripts/patch-xnu-entry-probe.py'
    sys.path.insert(0, str(helper.parent))
    spec = importlib.util.spec_from_file_location('entry_probe', helper)
    probe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(probe)

    data = bytearray(args.kernel.read_bytes())
    probe.load_segments(data)
    names = probe.rebase(args.kernel, data)
    handler = '_sysctl_initproc_spawned'
    start, end = probe.function_range({va: name for name, va in names.items()}, handler)
    state = names['_initproc_spawned']
    callee = names['_sysctl_handle_int']
    # Match the compiler's tail dispatch and the request field used by the handler.
    if struct.unpack_from('<I', data, probe.file_offset(start))[0] != 0xf9401868:
        raise SystemExit('Unexpected initproc_spawned request layout')
    sites = []
    for site in range(start + 12, end, 4):
        page_delta = (state >> 12) - ((site - 12) >> 12)
        expected = (
            0x90000001 | ((page_delta & 3) << 29) | (((page_delta >> 2) & 0x7ffff) << 5),
            0x91000021 | ((state & 0xfff) << 10),
            0x52800002,
            probe.branch(site, callee),
        )
        if struct.unpack_from('<4I', data, probe.file_offset(site - 12)) == expected:
            sites.append(site)
    if len(sites) != 1:
        raise SystemExit('Expected one initproc_spawned tail dispatch')
    site, = sites
    source = '''.text
.global _start
_start:
  ldr x8, [x3, #0x30]
  cbnz x8, write_request
  b handle_int
write_request:
  stp x19, x30, [sp, #-16]!
  mov x19, x1
  bl handle_int
  cbnz w0, done
  ldr w8, [x19]
  cmp w8, #1
  b.ne done
  adrp x8, vinfo
  add x8, x8, :lo12:vinfo
  ldr w9, [x8, #8]
  cmp w9, #32
  b.ne done
  ldr x9, [x8, #16]
  cbz x9, done
  ldr w10, [x8, #12]
  ldr w11, [x8]
  mul x10, x10, x11
  lsr x10, x10, #4
  bic x10, x10, #7
  cbz x10, done
  add x9, x9, x10, lsl #1
  add x9, x9, x10
  mov w11, #0xffff
  movk w11, #0xff00, lsl #16
paint:
  stp w11, w11, [x9], #8
  subs x10, x10, #8
  b.hi paint
  dsb sy
done:
  ldp x19, x30, [sp], #16
  ret
'''
    # Following stages 1–4, in the padding otherwise used by the panic renderer.
    # This diagnostic can coexist with the entry bars but not the panic renderer.
    cave = probe.CAVES[4] + probe.CAVE_SIZE
    cave_offset = probe.file_offset(cave)
    with tempfile.TemporaryDirectory() as directory:
        code = probe.assemble(source, cave, {'handle_int': callee, 'vinfo': names['_vinfo']}, Path(directory))
    if len(code) > probe.PANIC_CAVE_END - cave or any(data[cave_offset:cave_offset + len(code)]):
        raise SystemExit('Launchd probe padding is occupied')
    data[cave_offset:cave_offset + len(code)] = code
    struct.pack_into('<I', data, probe.file_offset(site), probe.branch(site, cave))
    args.kernel.write_bytes(data)
    print(f'Launchd PID 1 checkpoint at {site:#x}, cyan band, nonterminal')


if __name__ == '__main__':
    main()
