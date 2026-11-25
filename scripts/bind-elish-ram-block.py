#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Bind RamBlock to the 21H461 IOStorageFamily and check the final kernel."""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'kits/XnuPrelink'))
from kernel_target import TRIPLE, check_object
from native_prelink import backed_offset, macho

# 21H461 kernelcache with collection slide -0x1000000. The same contract
# booted a restore ramdisk on fuxi at 11db5c00e84246681b9c9c06582d4eb31fb6ae26.
TABLE = 0xfffffff005cb44c0
SLOTS = 190
OVERRIDES = {0, 1, 7, 14, 86, 87, *range(168, 185), 188}
METHODS = {
    0: 0xfffffff00496edbc, 1: 0xfffffff00496edc0, 7: 0xfffffff00496edc4,
    21: 0xfffffff00496ee30, 33: 0xfffffff00496ee88, 44: 0xfffffff00496ef38,
    171: 0xfffffff00496f074, 182: 0xfffffff00496f038, 183: 0xfffffff00496f044,
    185: 0xfffffff00496f050, 186: 0xfffffff00496f05c, 187: 0xfffffff00496f068,
    188: 0xfffffff00496f080, 189: 0xfffffff00496f08c,
}
BASE = {
    '__ZN20IOBlockStorageDeviceC2EPK11OSMetaClass': 0xfffffff00496ed98,
    '__ZN20IOBlockStorageDeviceD2Ev': 0xfffffff00496edb8,
    '__ZN20IOBlockStorageDevice10gMetaClassE': 0xfffffff006add1d0,
    '__ZTV20IOBlockStorageDevice': TABLE,
}
CLASS_TABLE = '__ZTV14NativeRamBlock'


def symbols(data, image):
    at = next(at for cmd, at, _ in image['commands'] if cmd == 2)
    start, count, strings, _ = struct.unpack_from('<4I', data, at + 8)
    for i in range(count):
        off, kind, _, _, value = struct.unpack_from('<IBBHQ', data, start + 16 * i)
        name = data[strings + off:data.index(b'\0', strings + off)].decode()
        yield name, kind, value


def compile_ram_block(out):
    source_dir = ROOT / 'drivers/RamBlock'
    patch = Path(os.environ['UNIXNU_DEVICE']) / 'patches/ram-block-volatile.patch'
    records = ROOT / 'build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/iokit/DEVELOPMENT'
    record = json.loads((records / 'IOPlatformExpert.cpo.json').read_text().strip().rstrip(','))
    out.mkdir(parents=True, exist_ok=True)
    source = out / 'NativeRamBlock.cpp'
    obj = out / 'NativeRamBlock.o'
    subprocess.run(['patch', '--batch', '--forward', '--output', source,
                    source_dir / source.name, patch], check=True)
    command = []
    arguments = iter(record['arguments'])
    for value in arguments:
        if value == record['file']:
            command.append(str(source))
        elif value == '-o':
            next(arguments)
            command.extend(['-o', str(obj)])
        elif value == '-Xclang':
            flag = next(arguments)
            if not flag.startswith('-W'):
                command.extend([value, flag])
        elif not value.startswith('-W'):
            command.append(value)
    command.extend([
        '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
        '-Wno-c99-designator', '-Wno-reorder-init-list', '-Wno-deprecated-enum-enum-conversion',
        '-Wno-unnecessary-virtual-specifier', '-DRAM_BLOCK_VOLATILE_ONLY=1',
        '-I', str(ROOT / '.cache/xnu-work/osfmk'), '-I', str(ROOT / 'build/storage-abi/include'),
        '-iquote', str(source_dir),
    ])
    subprocess.run(command, cwd=record['directory'], check=True)
    check_object(obj.read_bytes(), 'NativeRamBlock')
    return obj


def bind():
    out = ROOT / 'build/ram-block'
    obj = compile_ram_block(out)
    data = obj.read_bytes()
    image = macho(data)
    entries = list(symbols(data, image))
    table = next(value for name, _, value in entries if name == CLASS_TABLE)
    section = next(s for seg in image['segments'] for s in seg['sections']
                   if s['va'] <= table < s['va'] + s['size'])
    start, count = struct.unpack_from('<2I', data, section['command_offset'] + 56)
    relocations = {}
    for index in range(count):
        offset, word = struct.unpack_from('<2I', data, start + 8 * index)
        if word >> 24 == 0xe:
            relocations[section['va'] + offset] = entries[word & 0xffffff][0]
    imported = {name for name, kind, _ in entries if kind == 1 and '20IOBlockStorageDevice' in name}
    bindings = {name: BASE[name] for name in imported & BASE.keys()}
    for slot in range(SLOTS):
        name = relocations.get(table + 16 + 8 * slot, '')
        overridden = name.startswith(('__ZN14NativeRamBlock', '__ZNK14NativeRamBlock'))
        if overridden != (slot in OVERRIDES) or not name:
            raise SystemExit(f'Unexpected NativeRamBlock vtable slot {slot}: {name}')
        if overridden:
            continue
        if slot in METHODS:
            if '20IOBlockStorageDevice' not in name:
                raise SystemExit(f'Expected IOBlockStorageDevice vtable slot {slot}: {name}')
            bindings[name] = METHODS[slot]
        elif slot >= 168 or not any(owner in name for owner in
                                   ('15OSMetaClassBase', '8OSObject', '15IORegistryEntry', '9IOService')):
            raise SystemExit(f'Unexpected inherited vtable slot {slot}: {name}')
    if bindings.keys() != imported or table + 16 + 8 * SLOTS in relocations:
        raise SystemExit('Unqualified NativeRamBlock imports or vtable size')
    asm = out / 'original-bindings.S'
    asm.write_text(''.join(f'.globl {name}\n.set {name}, {value:#x}\n' for name, value in sorted(bindings.items())))
    adapter = out / 'original-bindings.o'
    subprocess.run(['xcrun', 'clang', '-target', TRIPLE, '-c', asm, '-o', adapter], check=True)
    archive = out / 'libRamBlock.a'
    archive.unlink(missing_ok=True)
    subprocess.run(['xcrun', 'ar', 'rcs', archive, obj, adapter], check=True)


def check(kernel):
    data = kernel.read_bytes()
    image = macho(data)
    names = {name: value for name, kind, value in symbols(data, image) if kind & 0xe == 0xe}

    def read(address, size):
        offset = backed_offset(image['segments'], address, size, len(data))
        return data[offset:offset + size]

    for va, code in ((0xfffffff00496ed98, 'fd7bbfa9fd0300919f440094289a00d008411391080000f9fd7bc1a8c0035fd6'),
                     (0xfffffff00496edb8, '9c440014200020d4200020d4')):
        expected = bytes.fromhex(code)
        if read(va, len(expected)) != expected:
            raise SystemExit(f'IOStorageFamily instructions changed at {va:#x}')
    got = struct.unpack('<2Q', read(0xfffffff005cb3430, 16))
    if got != (names['__ZN9IOServiceC2EPK11OSMetaClass'], names['__ZN9IOServiceD2Ev']):
        raise SystemExit('IOStorageFamily base calls are not bound to this IOService')
    original = struct.unpack(f'<{SLOTS}Q', read(TABLE + 16, SLOTS * 8))
    if any(original[slot] != address for slot, address in METHODS.items()):
        raise SystemExit('IOBlockStorageDevice vtable differs from the 21H461 contract')
    concrete = struct.unpack(f'<{SLOTS}Q', read(names[CLASS_TABLE] + 16, SLOTS * 8))
    native_text = next(s for s in image['segments'] if s['name'] == '__TEXT_EXEC')
    owned = {i for i, address in enumerate(concrete) if address != original[i]}
    if owned != OVERRIDES:
        raise SystemExit(f'NativeRamBlock vtable differs at unexpected slots: {sorted(owned ^ OVERRIDES)}')
    if any(not native_text['va'] <= concrete[i] < native_text['va'] + native_text['size'] for i in owned):
        raise SystemExit('NativeRamBlock overrides must target native executable text')
    print(f'RamBlock: {SLOTS} linked vtable slots checked, {len(owned)} native overrides')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', type=Path, help='Verify the final prelinked Mach-O')
    args = parser.parse_args()
    if args.check:
        check(args.check)
    else:
        bind()
