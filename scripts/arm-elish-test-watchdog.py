#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Arm elish's APSS watchdog before XNU enables the MMU, on a RAM-only kernel copy."""
import argparse
import importlib.util
from pathlib import Path
import struct
import sys
import tempfile

# Live elish FDT and LineageOS watchdog_v2.c at 6b34d2476532.
WDT_BASE = 0x17C10000
WDT_HZ = 32765
WDT_COUNT_MAX = 0xFFFFF
# RamoopsLogLib prepares PON SOFT_RB_SPARE in the DEBUG firmware. Also set the
# ABL restart reason immediately before arming the watchdog.
RESTART_REASON = 0x146BF65C
RESTART_BOOTLOADER = 0x77665500


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kernel', type=Path, help='copy already patched with the UniXNU entry bars')
    parser.add_argument('--unixnu', type=Path, default=Path(__file__).resolve().parents[2] / 'UniXNU')
    parser.add_argument('--seconds', type=int, default=30, choices=range(1, 32), metavar='1..31')
    args = parser.parse_args()
    root = args.unixnu.resolve()
    sys.path.insert(0, str(root / 'scripts'))
    spec = importlib.util.spec_from_file_location('entry_probe', root / 'scripts/patch-xnu-entry-probe.py')
    probe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(probe)
    data = bytearray(args.kernel.read_bytes())
    probe.load_segments(data)
    entry = probe.unixthread_pc(data)
    band = entry + (probe.CAVES[1] - probe.ENTRY_VA)
    target = next((address for address, name in probe.kernel_symbols(args.kernel).items()
                   if name == '_start_first_cpu'), None)
    if target is None:
        raise SystemExit('Kernel has no _start_first_cpu symbol')
    cave = entry + 4
    entry_offset = probe.file_offset(entry)
    if struct.unpack_from('<I', data, entry_offset)[0] != probe.branch(entry, band):
        raise SystemExit('Expected the unmodified stage-1 entry branch')
    start = probe.file_offset(cave)
    end = probe.file_offset(band)
    if any(data[start:end]):
        raise SystemExit('Entry padding is occupied')
    assembly = f'''.text
.global _start
_start:
  ldr w1, ={RESTART_REASON:#x}
  ldr w2, ={RESTART_BOOTLOADER:#x}
  str w2, [x1]
  ldr w1, ={WDT_BASE:#x}
  str wzr, [x1, #8]
  dsb sy
  ldr w2, ={WDT_COUNT_MAX:#x}
  str w2, [x1, #0x10]
  ldr w2, ={WDT_HZ * args.seconds}
  str w2, [x1, #0x14]
  dsb sy
  mov w2, #1
  str w2, [x1, #8]
  str w2, [x1, #4]
  dsb sy
  b stage1
.ltorg
'''
    with tempfile.TemporaryDirectory() as directory:
        build = Path(directory)
        original = probe.assemble(probe.SOURCES[1], band, {'target': target}, build)
        band_offset = probe.file_offset(band)
        if data[band_offset:band_offset + len(original)] != original:
            raise SystemExit('Unexpected stage-1 trampoline')
        code = probe.assemble(assembly, cave, {'stage1': band}, build)
    if len(code) > end - start:
        raise SystemExit('The watchdog code does not fit ahead of stage 1')
    data[start:start + len(code)] = code
    struct.pack_into('<I', data, entry_offset, probe.branch(entry, cave))
    args.kernel.write_bytes(data)
    print(f'Armed the elish watchdog for {args.seconds} s at XNU entry, before the stage-1 bar')


if __name__ == '__main__':
    main()
