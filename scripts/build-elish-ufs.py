#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Build the elish UFS service with the selected XNU compile record."""
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'kits/XnuPrelink'))
from kernel_target import check_object


CHUNK_BYTES = 32 * 1024
MANIFEST_SHA256 = 'bc908a6cf66491055077798550660f40128bfe6dce2fdc0db56e6af96511208d'


def build_root_digests(image, out):
    """Anchor read-time digests to the exact raw windows of the pinned IPSW."""
    tables = []
    with image.open('rb') as stream:
        header = stream.read(4096)
        if hashlib.sha256(header).hexdigest() != MANIFEST_SHA256:
            raise SystemExit(f'{image}: SHNGROOT manifest does not match the 21H461 layout')
        for index, window in enumerate(('OS', 'SystemOS')):
            name, offset, size, block, flags, digest = struct.unpack_from('<32sQQII32s', header, 24 + 128 * index)
            if name.rstrip(b'\0').decode() != window or block != 512 or flags != 1:
                raise SystemExit(f'{image}: unexpected {window} manifest entry')
            stream.seek(offset)
            image_digest = hashlib.sha256()
            chunks = bytearray()
            for cursor in range(0, size, CHUNK_BYTES):
                want = min(CHUNK_BYTES, size - cursor)
                data = stream.read(want)
                if len(data) != want:
                    raise SystemExit(f'{image}: truncated {window} at offset {cursor:#x}')
                image_digest.update(data)
                chunks.extend(hashlib.sha256(data).digest())
            if image_digest.digest() != digest:
                raise SystemExit(f'{image}: {window} contents do not match the pinned SHA-256')
            tables.append(chunks)
            print(f'{window}: {len(chunks) // 32} chunks at {offset:#x}, {size} bytes, '
                  f'SHA-256 {digest.hex()}, table SHA-256 {hashlib.sha256(chunks).hexdigest()}', flush=True)
    # VROOTB01 transport envelope. The linked SHA-256 authenticates the complete
    # bundle, including its table offsets and order, before either table is used.
    first = 32 + 16 * len(tables)
    second = first + len(tables[0])
    bundle = struct.pack('<8s7Q', b'VROOTB01', 1, len(tables), second + len(tables[1]),
                         first, len(tables[0]), second, len(tables[1])) + b''.join(tables)
    (out / 'elish-root-digests.bin').write_bytes(bundle)
    digest = hashlib.sha256(bundle).digest()
    (out / 'elish-root-digests.h').write_text(
        f'constexpr size_t kVerifiedChunk = {CHUNK_BYTES};\n'
        'constexpr uint8_t kRootDigestsSha256[] = {' + ', '.join(f'{byte:#04x}' for byte in digest) + '};\n')
    print(f'RootPolicies: {len(bundle)} bytes, SHA-256 {digest.hex()}', flush=True)


def main():
    source = Path(os.environ['UNIXNU_DEVICE']) / 'package/ElishUfs.cpp'
    records = ROOT / 'build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/iokit/DEVELOPMENT'
    record = json.loads((records / 'IOPlatformExpert.cpo.json').read_text().strip().rstrip(','))
    out = ROOT / 'build/elish-ufs'
    out.mkdir(parents=True, exist_ok=True)
    obj = out / 'ElishUfs.o'
    root = os.environ.get('UNIXNU_ELISH_ROOT_IMAGE')
    if root:
        build_root_digests(Path(root), out)
    else:
        (out / 'elish-root-digests.bin').unlink(missing_ok=True)
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
    command.extend(['-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                    '-Wno-c99-designator', '-Wno-reorder-init-list', '-Wno-deprecated-enum-enum-conversion',
                    '-Wno-unnecessary-virtual-specifier',
                    '-I', str(ROOT / '.cache/xnu-work/osfmk'), '-I', str(ROOT / 'build/storage-abi/include')])
    if root:
        command.extend(['-DELISH_VERIFIED_ROOT=1', '-iquote', str(out)])
    subprocess.run(command, cwd=record['directory'], check=True)
    check_object(obj.read_bytes(), 'ElishUfs')
    archive = out / 'libElishUfs.a'
    archive.unlink(missing_ok=True)
    subprocess.run(['xcrun', 'ar', 'rcs', archive, obj], check=True)


if __name__ == '__main__':
    main()
