#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Stage the exact factory NT36523 fw01 from the user's local stock boot image."""
import argparse
import hashlib
from pathlib import Path
import struct

SHA256 = '33d2e0a9217df7e898a96728631f3b6decd85ddb7d4112b38b9d7d6d162d6938'


def stage(stock_boot, out):
    with stock_boot.open('rb') as stream:
        stream.seek(0x22c2650)
        blob = stream.read(0x30000)
    if hashlib.sha256(blob).hexdigest() != SHA256:
        raise ValueError('factory boot does not contain the pinned elish K81 fw01 at offset 0x22c2650')

    # This SHA-256 identifies the cascade boot image. Read only its bank
    # records and the two nonempty cascade header records (see MiCode
    # nt36xxx_fw_update.c). No proprietary bytes are stored in this repo.
    if blob[0x20] != 3 or blob[0x28] != 0 or blob[0x2f000:0x2f002] != b'\x1a\xe5' or blob[-3:] != b'NVT':
        raise ValueError('unexpected factory fw01 layout')
    parts = []
    for i in range(2):
        offset, address, size = struct.unpack_from('<III', blob, i * 12)
        crc, = struct.unpack_from('<I', blob, 0x18 + i * 4)
        parts.append((address, offset, size, crc))
    for at in (0xf0, 0x1f0):
        address, size, offset, crc = struct.unpack_from('<IIII', blob, at)
        parts.append((address, offset, size, crc))
    if any(not size or address + size >= 0x30000 or offset + size >= len(blob) for address, offset, size, _ in parts):
        raise ValueError('invalid SRAM partition')
    out.mkdir(parents=True, exist_ok=True)
    (out / 'fw01.bin').write_bytes(blob)
    (out / 'ElishTouchFirmware.inc').write_text(
        'extern "C" const uint8_t elish_touch_fw01[];\n'
        'struct TouchPartition { unsigned sram, offset, last, crc; };\n'
        'constexpr TouchPartition kTouchPartitions[] = {\n'
        + ''.join('    {' + ', '.join(hex(value) for value in part) + '},\n' for part in parts) + '};\n')
    print(f'NT36523 fw01: {len(blob)} bytes, SHA-256 {SHA256}')
    for address, offset, size, crc in parts:
        print(f'SRAM {address:#x} + {size + 1:#x}, image {offset:#x}, CRC {crc:#010x}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stock_boot', type=Path)
    parser.add_argument('output_directory', type=Path)
    args = parser.parse_args()
    try:
        stage(args.stock_boot, args.output_directory)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
