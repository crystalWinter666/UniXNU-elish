#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Build the elish digitizer bridge against the original 21H461 IOHIDDevice."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
RECORD = Path('build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/iokit/DEVELOPMENT/IOPlatformExpert.cpo.json')
HID = 'com.apple.iokit.IOHIDFamily'
PERSONALITY = 'com.apple.iokit.IOHIDEventDriver'
HID_TEXT = '7e4c5104814f2e773d5be13d092dec53ee44ba5a61a81264b229780406e86b72'
ORIGINAL_KERNEL = 'f18b4df865405f2a287b84977d7aca745e056d656d9fc9df1662970ce2969de7'
BASE = '__ZN11IOHIDDevice'
VTABLE = 0xfffffff005d045c0  # address point, not the Itanium table header
CTOR = 0xfffffff004ad94f8
DTOR = 0xfffffff004ad9518
META = 0xfffffff006ae6fa0
SERVICE_SLOTS, DEVICE_SLOTS = 168, 207
BASE_OVERRIDES = [0, 1, 7, 12, 14, 21, 23, 76, 81, 82, 86, 87, 91, 92, 93, 101, 132, 140]
OWNED_SLOTS = [0, 1, 7, 14, 86, 87, 111, 171, 173, 180]
INITIALIZER = '__GLOBAL__sub_I_ElishTouchHid.cpp'
EVENT_DRIVER_VTABLE, EVENT_DRIVER_SLOTS = 0xfffffff005d06a68, 220
EVENT_VTABLE, EVENT_SLOTS = 0xfffffff005d07318, 37
DISPATCH_SLOT, DISPATCH = 198, 0xfffffff004aface8
EVENT_METHODS = {18: 0xfffffff004afcf90, 23: 0xfffffff004afcfb8,
                 24: 0xfffffff004afefdc, 25: 0xfffffff004afe4c4}
EVENT_CHILDREN_OFFSET = 24


def require(ok, message):
    if not ok:
        raise SystemExit(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def tool(name):
    found = shutil.which(name)
    if not found and shutil.which('xcrun'):
        result = subprocess.run(['xcrun', '--find', name], capture_output=True, text=True)
        if not result.returncode:
            found = result.stdout.strip()
    require(found, f'Missing tool: {name}')
    return found


def abi_header():
    text = '''// Generated from the qualified 21H461 IOHIDDevice. No private field access.
#pragma once
#include <IOKit/IOService.h>
#include <IOKit/IOMemoryDescriptor.h>
class IOHIDDevice : public IOService {
 OSDeclareAbstractStructors(IOHIDDevice)
 unsigned char originalPrivate[0xd0-sizeof(IOService)];
public:
 using IOService::setProperty;
 IOReturn Dispatch(IORPC) override;
 void free() override;
 bool init(OSDictionary *properties=nullptr) override;
 bool setProperty(const OSSymbol *, OSObject *) override;
 bool willTerminate(IOService *, IOOptionBits) override;
 IOReturn configureReport(IOReportChannelList *, IOReportConfigureAction, void *, void *) override;
 IOReturn updateReport(IOReportChannelList *, IOReportUpdateAction, void *, void *) override;
 bool start(IOService *) override;
 void stop(IOService *) override;
 bool handleOpen(IOService *, IOOptionBits, void *) override;
 void handleClose(IOService *, IOOptionBits) override;
 bool handleIsOpen(const IOService *) const override;
 bool matchPropertyTable(OSDictionary *, SInt32 *) override;
 IOReturn message(UInt32, IOService *, void *) override;
 IOReturn newUserClient(task_t, void *, UInt32, OSDictionary *, IOUserClient **) override;
'''
    named = {171: 'OSString *newTransportString() const', 173: 'OSString *newProductString() const',
             180: 'IOReturn newReportDescriptor(IOMemoryDescriptor **) const = 0',
             181: 'IOReturn handleReport(IOMemoryDescriptor *, unsigned, IOOptionBits)'}
    for slot in range(SERVICE_SLOTS, DEVICE_SLOTS):
        declaration = named.get(slot) or f'__attribute__((deprecated("Unqualified ABI"))) void opaque{slot}()'
        text += f' virtual {declaration};\n'
    text += '};\nstatic_assert(sizeof(IOHIDDevice) == 0xd0, "Original instance size");\n'
    pins = {'kDriverVtable': EVENT_DRIVER_VTABLE, 'kDriverSlots': EVENT_DRIVER_SLOTS,
            'kEventVtable': EVENT_VTABLE, 'kDispatchSlot': DISPATCH_SLOT, 'kDispatch': DISPATCH,
            'kEventChildren': EVENT_CHILDREN_OFFSET,
            **{f'kEventMethod{i}': addr for i, addr in EVENT_METHODS.items()}}
    text += 'namespace ElishHidABI {\n' + ''.join(f'inline constexpr uintptr_t {key} = {pin:#x}ULL;\n'
                                                for key, pin in pins.items()) + '}\n'
    return text


def qualify_reference(prelink, path):
    raw = path.read_bytes()
    plan = json.loads(path.with_name('kernel.json').read_text())
    digest = sha(raw)
    require(digest == plan['candidate_sha256'], 'Expected the matching prelinked candidate and kernel.json, not the FAT-patched kernel')
    require(plan['original_sha256'] == ORIGINAL_KERNEL and HID in plan['components'] and
            PERSONALITY in plan['codeless_components'], 'Expected the original 21H461 HID code and codeless personality')
    image = prelink.macho(raw)
    names = {n: a for a, group in prelink.exports(raw, image).items() for n in group}

    def read(address, size):
        offset = prelink.backed_offset(image['segments'], address, size, len(raw))
        return raw[offset:offset + size]

    extent = next(e for e in plan['extents'] if e['module'] == HID and e['segment'] == '__TEXT_EXEC')
    require(sha(read(extent['va'], extent['size'])) == plan['original_executable_sha256'][HID] == HID_TEXT,
            'Original IOHIDFamily instructions must match the pinned 21H461 code')
    methods = struct.unpack(f'<{DEVICE_SLOTS}Q', read(VTABLE, DEVICE_SLOTS * 8))
    base = struct.unpack(f'<{SERVICE_SLOTS}Q', read(names['__ZTV9IOService'] + 16, SERVICE_SLOTS * 8))
    require([i for i in range(SERVICE_SLOTS) if methods[i] != base[i]] == BASE_OVERRIDES,
            'Original IOHIDDevice native overrides moved')
    require([i for i, p in enumerate(methods) if p == names['___cxa_pure_virtual']] == [180] and all(methods) and
            read(VTABLE + DEVICE_SLOTS * 8, 24) == bytes(24), 'Unexpected IOHIDDevice vtable size or pure slot')
    require(read(CTOR, 32).hex() == 'fd7bbfa9fd0300912bd90094489100f008011791080000f9fd7bc1a8c0035fd6' and
            struct.unpack('<2Q', read(0xfffffff005d00f10, 16)) ==
            (names['__ZN9IOServiceC2EPK11OSMetaClass'], names['__ZN9IOServiceD2Ev']),
            'Original protected constructor, vptr store, or native constructor/destructor binding changed')
    require(read(0xfffffff004ad9aec, 0x34).hex() ==
            '680240f908a142f9e00313aae10314aa00013fd6680240f9603d003608d142f9e1630091e00313aa00013fd6a02d0035e00f40f9',
            'Original start/descriptor call boundary changed')
    require(methods[181] == 0xfffffff004add0c4 and read(0xfffffff004add110, 28).hex() ==
            '680240f9081543f9e00313aae20316aae30315aae40314aa00013fd6',
            'Original handleReport call boundary changed')
    driver = struct.unpack(f'<{EVENT_DRIVER_SLOTS}Q', read(EVENT_DRIVER_VTABLE, EVENT_DRIVER_SLOTS * 8))
    event = struct.unpack(f'<{EVENT_SLOTS}Q', read(EVENT_VTABLE, EVENT_SLOTS * 8))
    require(all(driver) and driver[DISPATCH_SLOT] == DISPATCH and all(event) and
            all(event[i] == addr for i, addr in EVENT_METHODS.items()) and
            read(EVENT_DRIVER_VTABLE + EVENT_DRIVER_SLOTS * 8, 24) == bytes(24) and
            read(EVENT_VTABLE + EVENT_SLOTS * 8, 24) == bytes(24), 'Original event ABI slots changed')
    require(read(0xfffffff004af09bc, 12).hex() == 'a89000d008a12991080000f9' and
            read(0xfffffff004afb130, 12).hex() == '6890009008610c91080000f9' and
            read(0xfffffff004afcf04, 4).hex() == '000c40f9' and read(0xfffffff004afcf50, 4).hex() == '600e00f9' and
            read(0xfffffff004afb460, 4).hex() == '600e40f9' and read(0xfffffff004afb474, 4).hex() == '7f0e00f9' and
            read(DISPATCH, 24).hex() == 'f85fbca9f65701a9f44f02a9fd7b03a9fdc30091f30302aa',
            'Original event vptr stores, event child field or dispatch routine changed')
    return digest, methods, names


def compile_bridge(tree, source, out):
    record = json.loads((tree / RECORD).read_text().strip().rstrip(','))
    obj = out / 'ElishTouchHid.o'
    command = []
    arguments = iter(record['arguments'])
    for arg in arguments:
        if arg == record['file']:
            command.append(str(source))
        elif arg == '-o':
            next(arguments)
            command += ['-o', str(obj)]
        elif not arg.startswith(('-W', '-DXNU_BOOT_PROGRESS=')):
            command.append(arg)
    command += ['-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-Wno-unnecessary-virtual-specifier',
                '-Wno-c99-designator', '-Wno-reorder-init-list', '-Wno-deprecated-enum-enum-conversion',
                '-Wno-missing-designated-field-initializers', '-I', str(out), '-I', str(tree / '.cache/xnu-work/osfmk')]
    result = subprocess.run(command, cwd=record['directory'], capture_output=True, timeout=300)
    log = result.stdout + result.stderr
    (out / 'compile.log').write_bytes(log)
    require(result.returncode == 0, log.decode(errors='replace')[-6000:])
    return obj


def object_symbols(prelink, raw):
    image = prelink.macho(raw)
    st = next(p for c, p, _ in image['commands'] if c == 2)
    symoff, count, stroff, _ = struct.unpack_from('<4I', raw, st + 8)
    symbols = []
    for i in range(count):
        offset, kind, _, _, address = struct.unpack_from('<IBBHQ', raw, symoff + i * 16)
        start = stroff + offset
        symbols.append((raw[start:raw.index(b'\0', start)].decode(), kind, address))
    return image, symbols


def qualify_object(prelink, raw, originals, names):
    image, symbols = object_symbols(prelink, raw)
    vt = next(address for name, _, address in symbols if name == '__ZTV13ElishTouchHid')
    section = next(s for g in image['segments'] for s in g['sections'] if s['va'] <= vt < s['va'] + s['size'])
    off, count = struct.unpack_from('<2I', raw, section['command_offset'] + 56)
    pointers = {}
    for i in range(count):
        place, bits = struct.unpack_from('<2I', raw, off + i * 8)
        if bits >> 27 & 1 and bits >> 28 == 0 and bits >> 25 & 3 == 3:
            pointers[section['va'] + place] = symbols[bits & 0xffffff][0]
    compiled = [pointers.get(vt + 16 + i * 8) for i in range(DEVICE_SLOTS)]
    require(all(compiled) and vt + 16 + DEVICE_SLOTS * 8 not in pointers, 'Compiled exact 207-slot vtable required')
    bindings = {BASE + 'C2EPK11OSMetaClass': CTOR, BASE + 'D2Ev': DTOR,
                BASE + '10gMetaClassE': META, '__ZTV11IOHIDDevice': VTABLE - 16}
    owned = []
    for i, name in enumerate(compiled):
        if name.startswith(('__ZN13ElishTouchHid', '__ZNK13ElishTouchHid')):
            owned.append(i)
        elif '11IOHIDDevice' in name:
            bindings[name] = originals[i]
        else:
            require(i < SERVICE_SLOTS and names.get(name) == originals[i],
                    f'Inherited native symbol mismatch at slot {i}: {name}')
    require(owned == OWNED_SLOTS, f'Unexpected owned HID slots: {owned}')
    # Compilers may emit either a qualified direct base call or an original
    # vtable lookup for these three overrides. No other private imports allowed.
    for name, slot in {BASE + '4freeEv': 14, BASE + '5startEP9IOService': 86,
                       BASE + '4stopEP9IOService': 87}.items():
        bindings[name] = originals[slot]
    undefined = {name for name, kind, _ in symbols if kind == 1}
    private = {name for name in undefined if '11IOHIDDevice' in name}
    require(private <= bindings.keys(), 'Unbound IOHIDDevice imports: ' + ', '.join(sorted(private - bindings.keys())))
    require(undefined - private <= set(names) | {INITIALIZER},
            'Unresolved kernel imports: ' + ', '.join(sorted(undefined - private - set(names) - {INITIALIZER})))
    return {name: bindings[name] for name in sorted(private)}, compiled


def text(prelink, raw):
    s = next(s for g in prelink.macho(raw)['segments'] for s in g['sections'] if s['name'] == '__text')
    return raw[s['fileoff']:s['fileoff'] + s['size']]


def verify_linked(prelink, reference, linked, audit_path):
    source = json.loads(audit_path.read_text())
    family_images = []
    for path in (reference, linked):
        digest, _, _ = qualify_reference(prelink, path)
        raw = path.read_bytes()
        image, symbols = object_symbols(prelink, raw)
        plan = json.loads(path.with_name('kernel.json').read_text())
        extents = {e['segment']: (e['va'], e['size']) for e in plan['extents'] if e['module'] == HID}
        targets = {(e['new_site'], e['kind']): e['new_target'] for e in plan['bindings'] if e['module'] == HID}
        normalized = {}
        for name, (address, size) in extents.items():
            at = prelink.backed_offset(image['segments'], address, size, len(raw))
            data = bytearray(raw[at:at + size])
            for (site, kind), target in targets.items():
                if address <= site <= address + size - 8:
                    require(struct.unpack_from('<Q', data, site - address)[0] == target,
                            f'{path}: original family binding mismatch at {site:#x}')
                    if kind == 'kernel':
                        # These words legitimately change when native symbols move.
                        data[site - address:site - address + 8] = bytes(8)
            normalized[name] = sha(data)
        family_images.append((digest, extents, normalized, targets.keys()))
    require(source['reference_candidate_sha256'] == family_images[0][0], 'Audit was built with a different reference')
    require(family_images[0][1:] == family_images[1][1:], 'Linked IOHIDFamily extent or normalized contents changed')

    # ld64 localizes some owned virtual methods, so also read local N_SECT
    # symbols. The absolute aliases must be kept and assigned the audited VAs.
    names = {n: a for n, kind, a in symbols if kind & 0xee in (2, 14)}
    for name, address in source['bindings'].items():
        require(names.get(name) == int(address, 16), f'Linked original HID binding mismatch: {name}')
    entrypoints = {'__Z19elish_touch_hid_logv', '__Z21elish_touch_hid_startv',
                   '__Z22elish_touch_hid_submitRK15ElishTouchFrame', INITIALIZER}
    require(entrypoints <= names.keys(), 'Linked image does not contain all bridge entry points')
    vt = names.get('__ZTV13ElishTouchHid')
    require(vt is not None, 'Linked HID vtable is absent')
    at = prelink.backed_offset(image['segments'], vt + 16, DEVICE_SLOTS * 8, len(raw))
    actual = struct.unpack_from(f'<{DEVICE_SLOTS}Q', raw, at)
    for slot, (symbol, pointer) in enumerate(zip(source['vtable_symbols'], actual)):
        require(names.get(symbol) == pointer, f'Linked HID vtable mismatch at {slot}: {symbol}')
    initializer = struct.pack('<Q', names[INITIALIZER])
    for segment in image['segments']:
        for section in segment['sections']:
            if section['name'] == '__mod_init_func':
                start, size = section['fileoff'], section['size']
                require(initializer not in raw[start:start + size], 'Bridge initializer incorrectly linked for early load')
    print(f'PASS: {linked} ({family_images[1][0]}); original family, HID aliases and 207 linked slots verified')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tree', type=Path, default=ROOT, help='assembled XNU build tree with compile records')
    parser.add_argument('--reference', type=Path, default=os.environ.get('UNIXNU_ELISH_TOUCH_HID_REFERENCE'),
                        help='unmodified prelinked 21H461 kernel.macho with sibling kernel.json')
    parser.add_argument('--out', type=Path, help='output/audit directory (default: TREE/build/elish-touch-hid)')
    parser.add_argument('--verify-linked', type=Path, metavar='KERNEL', help='audit a final prelinked kernel.macho and its kernel.json, without building')
    args = parser.parse_args()
    if not args.verify_linked and os.environ.get('UNIXNU_ELISH_TOUCH_HID') != '1':
        return
    require(args.reference, 'Set --reference or UNIXNU_ELISH_TOUCH_HID_REFERENCE to the prelinked 21H461 kernel.macho')
    tree, reference = args.tree.resolve(), args.reference.resolve()
    out = (args.out or tree / 'build/elish-touch-hid').resolve()
    sys.path[:0] = [str(tree / 'kits/XnuPrelink'), str(tree / 'scripts')]
    import native_prelink as prelink
    if args.verify_linked:
        verify_linked(prelink, reference, args.verify_linked.resolve(), out / 'abi-audit.json')
        return
    source = next((p for p in (ROOT / 'package/ElishTouchHid.cpp', tree / 'kits/elish/ElishTouchHid.cpp')
                   if p.is_file()), None)
    require(source is not None, 'Missing ElishTouchHid.cpp')
    from kernel_target import ARCH, IOS_VERSION, TRIPLE, check_object
    require(ARCH == 'arm64' and IOS_VERSION.split('.')[0] == '17', 'Expected plain arm64 iOS 17')

    candidate, methods, names = qualify_reference(prelink, reference)
    out.mkdir(parents=True, exist_ok=True)
    header = out / 'OriginalHidDeviceABI.h'
    header.write_text(abi_header())
    obj = compile_bridge(tree, source, out)
    raw = obj.read_bytes()
    check_object(raw, obj.name)
    bindings, vtable = qualify_object(prelink, raw, methods, names)
    assembly = out / 'original-bindings.S'
    assembly.write_text(''.join(f'.globl {name}\n.set {name}, {address:#x}\n' for name, address in bindings.items()))
    bound = out / 'original-bindings.o'
    subprocess.run([tool('clang'), '-target', TRIPLE, '-c', assembly, '-o', bound], check=True)
    check_object(bound.read_bytes(), bound.name)
    _, bound_symbols = object_symbols(prelink, bound.read_bytes())
    require({name: address for name, kind, address in bound_symbols if kind == 3} == bindings,
            'Absolute binding object does not match the audited addresses')

    late = out / 'ElishTouchHid.late.o'
    subprocess.run([tool('llvm-objcopy'), '--remove-section=__DATA,__mod_init_func',
                    f'--globalize-symbol={INITIALIZER}', obj, late], check=True)
    late_raw = late.read_bytes()
    check_object(late_raw, late.name)
    late_image, late_symbols = object_symbols(prelink, late_raw)
    require(all(s['name'] != '__mod_init_func' for g in late_image['segments'] for s in g['sections']) and
            any(n == INITIALIZER and k == 0xf for n, k, _ in late_symbols) and
            text(prelink, raw) == text(prelink, late_raw), 'Failed to defer owned metaclass initialization')
    archive = out / 'libElishTouchHid.a'
    pending_archive = out / 'libElishTouchHid.tmp.a'
    pending_archive.unlink(missing_ok=True)
    subprocess.run([tool('llvm-ar'), 'rcsD', pending_archive, late, bound], check=True)
    pending_archive.replace(archive)
    (out / 'abi-audit.json').write_text(json.dumps({
        'reference_candidate_sha256': candidate, 'original_text_sha256': HID_TEXT,
        'original_instance_size': 0xd0, 'original_slots': DEVICE_SLOTS, 'owned_slots': OWNED_SLOTS,
        'vtable_symbols': vtable, 'bindings': {name: hex(address) for name, address in bindings.items()},
        'archive_members_in_order': [late.name, bound.name], 'force_load': str(archive),
        'source_sha256': {p.name: sha(p.read_bytes()) for p in
                          (Path(__file__), source, source.with_name('ElishTouchFrame.h'), header)},
        'archive_sha256': sha(archive.read_bytes()), 'runtime_qualified': False,
    }, indent=2) + '\n')
    print(f'PASS: {archive} ({sha(archive.read_bytes())}); original ABI and deferred initializer verified')


if __name__ == '__main__':
    main()
