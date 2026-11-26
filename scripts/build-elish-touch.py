#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Build the board's QUP PIO and NT36523 touch path using the XNU compile record."""
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
DEVICE = Path(os.environ['UNIXNU_DEVICE'])
OUT = ROOT / 'build/elish-touch'


def run(args, **options):
    subprocess.run(args, check=True, **options)


def main():
    sys.path.insert(0, str(ROOT / 'kits/XnuPrelink'))
    from kernel_target import TRIPLE, check_object

    OUT.mkdir(parents=True, exist_ok=True)
    probes = ROOT / 'build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/iokit/DEVELOPMENT'
    record = json.loads((probes / 'IOPlatformExpert.cpo.json').read_text().strip().rstrip(','))
    source = DEVICE / 'package/ElishTouch.cpp'
    transport = ROOT / 'drivers/QcomGeniSpi'
    obj = OUT / 'ElishTouch.o'
    command = []
    arguments = iter(record['arguments'])
    for arg in arguments:
        if arg == record['file']:
            command.append(str(source))
        elif arg == '-o':
            next(arguments)
            command.extend(['-o', str(obj)])
        elif arg == '-Xclang':
            flag = next(arguments)
            if not flag.startswith('-W'):
                command.extend([arg, flag])
        elif not arg.startswith('-W'):
            command.append(arg)
    command += ['-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-Wno-c99-designator',
                '-Wno-reorder-init-list', '-Wno-deprecated-enum-enum-conversion',
                '-Wno-unnecessary-virtual-specifier', '-I', str(ROOT / '.cache/xnu-work/osfmk'),
                '-I', str(transport), '-I', str(OUT)]
    if os.environ.get('UNIXNU_ELISH_TOUCH_HID') == '1':
        command.append('-DELISH_TOUCH_HID=1')
    compiler = record['arguments'][0]
    geni_obj = OUT / 'GeniSpi.o'
    run([compiler, '-target', TRIPLE, '-ffreestanding', '-fno-stack-protector', '-Os', '-Wall', '-Wextra', '-Werror',
         '-c', transport / 'GeniSpi.c', '-o', geni_obj])
    objects = [obj, geni_obj]
    if stock := os.environ.get('UNIXNU_ELISH_TOUCH_STOCK_BOOT'):
        run([sys.executable, '-I', DEVICE / 'scripts/gen-elish-touch-firmware.py', Path(stock).resolve(), OUT])
        assembly = OUT / 'Firmware.S'
        # Keep the blob after __TEXT_EXEC so it does not move the early entry mapping.
        assembly.write_text('.section __LASTDATA_CONST,__touch_fw\n.p2align 4\n.globl _elish_touch_fw01\n'
                            '_elish_touch_fw01:\n.incbin "fw01.bin"\n')
        fw_obj = OUT / 'Firmware.o'
        run([compiler, '-target', TRIPLE, '-c', assembly, '-o', fw_obj], cwd=OUT)
        objects.append(fw_obj)
        command.append('-DELISH_TOUCH_FW=1')
    run(command, cwd=record['directory'])
    for path in objects:
        check_object(path.read_bytes(), path.name)
    archive = OUT / 'libElishTouch.a'
    archive.unlink(missing_ok=True)
    run(['xcrun', 'ar', 'rcs', archive, *objects])


if __name__ == '__main__':
    main()
