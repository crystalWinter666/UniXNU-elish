#!/usr/bin/env python3
"""Build the CPU-copy legacy display archive for 21H461.

Binding pins follow the dizi display build at 87bcefbff7bc922b74ececd4c04e5331dc353317.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
if os.environ.get('XNU_NATIVE_LEGACY_DISPLAY') != '1':
    raise SystemExit(0)

OUT = ROOT / 'build/native-legacy-display'
OUT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT / 'kits/XnuPrelink'))
from native_prelink import backed_offset, exports, macho
from kernel_target import TRIPLE, check_object


def require(ok, message):
    if not ok:
        raise SystemExit(message)


def run(command, **kwargs):
    return subprocess.check_output([str(item) for item in command], timeout=300, **kwargs)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def synced(path, text):
    path.write_text(text)
    with path.open('rb') as stream:
        os.fsync(stream.fileno())


PROBE = ROOT / 'kits/elish/LegacyDisplayProbe.cpp'
BACKEND = ROOT / 'drivers/GopFramebuffer/NativeLegacyDisplay.cpp'
require(PROBE.is_file(), f'Missing Elish display probe: {PROBE}')
require(BACKEND.is_file(), f'Missing current legacy backend: {BACKEND}')
candidate = ROOT.parent / 'kernel' / 'kernel.macho'
metadata_path = candidate.with_name('kernel.json')
require(candidate.is_file() and metadata_path.is_file(), 'Missing source kernel and kernel.json')
metadata = json.loads(metadata_path.read_text())
raw = candidate.read_bytes()
candidate_sha = hashlib.sha256(raw).hexdigest()
require(candidate_sha == metadata.get('candidate_sha256'), 'Kernel and metadata differ')
image = macho(raw)
names = {name: address for address, group in exports(raw, image).items() for name in group}


def read(address, length):
    at = backed_offset(image['segments'], address, length, len(raw))
    return raw[at:at + length]


family = next(e for e in metadata['extents'] if
              e['module'] == 'com.apple.iokit.IOMobileGraphicsFamily' and e['segment'] == '__TEXT_EXEC')
require(hashlib.sha256(read(family['va'], family['size'])).hexdigest() ==
        '2e044ee68c1650397e1435234ca6459206262e4c7ed709cb7131c6f664f9bd9b',
        'Expected original 21H461 IOMobileGraphicsFamily instructions')


def original_vtable(pc):
    page, offset, header = struct.unpack('<3I', read(pc, 12))
    require(page & 0x9f00001f == 0x90000008 and offset & 0xffc003ff == 0x91000108 and
            header == 0x91004108, 'Original display constructor vtable reference changed')
    delta = ((page >> 5) & 0x7ffff) * 4 + ((page >> 29) & 3)
    if delta & (1 << 20):
        delta -= 1 << 21
    return (pc & ~4095) + (delta << 12) + ((offset >> 10) & 4095)


# These are the audited 21H461 IOSurface accessors. Stride is deliberately
# exposed under the raw name: the Elish provider wraps the pixel stride.
getter_pins = {
    'descriptor': (0xfffffff004b9ead0, 48, True),
    'width': (0xfffffff004b9ebbc, 88, True),
    'height': (0xfffffff004b9ebc4, 96, True),
    'stride_raw': (0xfffffff004b9ebd4, 104, True),
    'allocation': (0xfffffff004b9ebfc, 144, True),
    'format': (0xfffffff004b9ebac, 128, False),
    'planes': (0xfffffff004b9ebf4, 152, False),
}
bindings = {
    '__ZN25IOMobileFramebufferLegacyC2EPK11OSMetaClass': 0xfffffff0057865cc,
    '__ZN25IOMobileFramebufferLegacyD2Ev': 0xfffffff005786618,
    '__ZN25IOMobileFramebufferLegacy10gMetaClassE': 0xfffffff006b35940,
}
legacy_vtable = original_vtable(0xfffffff0057865d8)
bindings['__ZTV25IOMobileFramebufferLegacy'] = legacy_vtable
for symbol, pinned in tuple(bindings.items()):
    if symbol in names:
        require(names[symbol] == pinned, 'Original framebuffer symbol moved: ' + symbol)
for name, (address, field, wide) in getter_pins.items():
    instruction = (0xf9400000 if wide else 0xb9400000) | ((field // (8 if wide else 4)) << 10)
    require(read(address, 8) == struct.pack('<2I', instruction, 0xd65f03c0),
            'Original surface getter mismatch: ' + name)
    binding_name = 'native_legacy_surface_stride_raw' if name == 'stride_raw' else 'native_legacy_surface_' + name
    bindings['_' + binding_name] = address
require(read(0xfffffff005796720, 16) == bytes.fromhex('ff0302d1fd7b07a9fdc30191a0831ff8') and
        read(0xfffffff005796830, 4) == bytes.fromhex('08ed4af9') and
        read(0xfffffff00579685c, 16) == bytes.fromhex('e2230091c3008052240080521684ff97') and
        read(0xfffffff005795fbc, 4) == bytes.fromhex('0ab182d2'),
        'Original iPadOS vsync sender or client-list ABI mismatch')
bindings['_native_legacy_vsync_notify'] = 0xfffffff005796720
require(read(0xfffffff00579ebb8, 16) == bytes.fromhex('ff0302d1fd7b07a9fdc30191a0031ff8') and
        read(0xfffffff00579ed54, 16) == bytes.fromhex('086540f90173ff90218c0c9100013fd6'),
        'Original iPadOS refresh-property publisher mismatch')
bindings['_native_legacy_set_refresh_properties'] = 0xfffffff00579ebb8

# Ensure all non-owned inherited virtual calls bind to symbols exported by this
# exact kernel. Mismatches fail closed; no family pointer is guessed/rebased.
record_path = ROOT / 'build/xnu/ios-obj/DEVELOPMENT_ARM64_BCM2837/iokit/DEVELOPMENT/IOPlatformExpert.cpo.json'
record = json.loads(record_path.read_text().strip().rstrip(','))
header = '''// Generated by scripts/build-elish-legacy-display.py from the audited 21H461 ABI.
#pragma once
#include <IOKit/IOService.h>
#include <libkern/libkern.h>
struct OriginalLegacySwapRequest; struct OriginalLegacyStateRequest;
class IOMobileFramebufferLegacy : public IOService {
 OSDeclareAbstractStructors(IOMobileFramebufferLegacy)
 unsigned char originalPrivate[0x18a0-sizeof(IOService)];
public:
 void installBootDimensions(uint32_t w,uint32_t h) { *reinterpret_cast<uint32_t *>(reinterpret_cast<unsigned char *>(this)+5808)=w; *reinterpret_cast<uint32_t *>(reinterpret_cast<unsigned char *>(this)+5812)=h; }
 const OriginalLegacySwapRequest *pendingSwapRequest() const { const OriginalLegacySwapRequest *r=nullptr; memcpy(&r,reinterpret_cast<const unsigned char *>(this)+5480,sizeof(r)); return r; }
 bool activateOnlyPendingSwap(const OriginalLegacySwapRequest *r) { auto s=reinterpret_cast<unsigned char *>(this); auto b=reinterpret_cast<const unsigned char *>(r); const void *c=nullptr,*h=nullptr,*t=nullptr,*n=nullptr,*p=nullptr; memcpy(&c,s+5472,8); memcpy(&h,s+5480,8); memcpy(&t,s+5488,8); if(!r||c||h!=r)return false; memcpy(&n,b+2960,8); memcpy(&p,b+2968,8); if(p!=s+5480)return false; if(n){const void *link=s+5480; memcpy(const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(n))+2968,&link,8);} else if(t!=b+2960)return false; memcpy(s+5472,&r,8); if(n)memcpy(s+5480,&n,8); else {const void *z=nullptr,*tail=s+5480;memcpy(s+5480,&z,8);memcpy(s+5488,&tail,8);} return true; }
 void free() override; bool init(OSDictionary * =nullptr) override; IOReturn setProperties(OSObject *) override; void systemWillShutdown(IOOptionBits) override; IOService *probe(IOService *,SInt32 *) override; bool start(IOService *) override; void stop(IOService *) override; IOWorkLoop *getWorkLoop() const override; IOReturn newUserClient(task_t,void *,UInt32,IOUserClient **) override; IOReturn setPowerState(unsigned long,IOService *) override;
'''
family_pointers = struct.unpack('<358Q', read(legacy_vtable + 16, 358 * 8))
pure_symbol = names.get('___cxa_pure_virtual')
require(pure_symbol is not None, 'Kernel does not export the pure virtual handler')
pure = {slot for slot, pointer in enumerate(family_pointers) if pointer == pure_symbol}
require(pure == {211, 213, 245, 246, 306, 307, 337, 338, 345} and all(family_pointers) and
        read(legacy_vtable + 16 + 358 * 8, 24) == bytes(24), 'Original display vtable extent or abstract slots changed')
named = {184:' virtual bool isMainDisplay();\n',188:' virtual IOReturn getDotPitch(uint32_t *);\n',213:' virtual IOReturn supportedFrameInfo(uint32_t,uint32_t *,char *,uint32_t *)=0;\n',249:' virtual void getDisplayDimensions(uint32_t *,uint32_t *);\n',290:' virtual void kickSwapQueue();\n',296:' virtual bool validateSwap(const OriginalLegacySwapRequest *);\n',295:' virtual void completeCurrentSwap();\n',337:' virtual IOReturn collectCompletionReports(uint32_t,void *,uint32_t *,uint32_t)=0;\n',288:' virtual bool startHardware();\n',301:' virtual bool getRefreshAttributes(uint64_t *,bool);\n',354:' virtual int applyStateRequest(OriginalLegacyStateRequest *,bool);\n'}
for slot in range(168,358):
    header += named.get(slot) or (f' virtual __attribute__((noreturn)) void opaque{slot}()=0;\n' if slot in pure else f' virtual __attribute__((deprecated("Unclassified ABI"))) void opaque{slot}();\n')
header += '};\nstatic_assert(sizeof(IOMobileFramebufferLegacy)==0x18a0,"Original instance extent");\nenum : unsigned {kRequestSurfaces=2624,kRequestMasks=792,kRequestRect0=17,kRequestRect1=81,kRequestOptions=804,kRequestDims=816,kLegacyStateRequested=5836,kLegacyStatePowered=5840,kLegacyVsyncClients=5592,kRequestBytes=kRequestSurfaces+4*sizeof(void *)};\nstruct OriginalLegacyStateRequest {uint32_t skipRequested,requested,skipPowered,powered;uint8_t changed,pad[11];};\nstruct OriginalLegacySwapRequest {unsigned char bytes[kRequestBytes];};\n#define NATIVE_LEGACY_LEAVES(LEAF) ' + ' '.join(f'LEAF({slot})' for slot in sorted(pure - {213,337})) + '\n'
(OUT / 'OriginalLegacyFramebufferABI.h').write_text(header)
staging = OUT / 'staged'
staging.mkdir(parents=True, exist_ok=True)
staged_backend = staging / BACKEND.name
shutil.copyfile(BACKEND, staged_backend)
command = []
arguments = record['arguments']
index = 0
while index < len(arguments):
    value = arguments[index]
    if value == record['file']:
        command.append(str(staged_backend))
    elif value == '-o':
        command += ['-o', str(OUT / 'NativeLegacyDisplay.o')]
        index += 1
    elif value == '-Xclang' and index + 1 < len(arguments) and arguments[index + 1].startswith('-W'):
        index += 1
    elif not value.startswith(('-W', '-DXNU_BOOT_PROGRESS=')):
        command.append(value)
    index += 1
command += ['-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
            '-Wno-unnecessary-virtual-specifier', '-Wno-c99-designator',
            '-Wno-reorder-init-list', '-Wno-deprecated-enum-enum-conversion',
            '-Wno-missing-designated-field-initializers',
            '-I', str(record['directory']), '-I', str(ROOT / '.cache/xnu-work/osfmk'),
            '-I', str(OUT), '-I', str(BACKEND.parent)]
result = subprocess.run(command, cwd=record['directory'], stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, text=True)
(OUT / 'compile.log').write_text(result.stdout)
if result.returncode:
    raise SystemExit(result.stdout[-3500:])
obj = OUT / 'NativeLegacyDisplay.o'
check_object(obj.read_bytes(), 'NativeLegacyDisplay')

# Audit compiled virtual slot references and the inherited base symbols before
# emitting absolute bindings. The original family ABI contains 358 slots.
obj_raw = obj.read_bytes()
obj_image = macho(obj_raw)
symtab = next(p for c, p, _ in obj_image['commands'] if c == 2)
symoff, nsyms, stroff, _ = struct.unpack_from('<4I', obj_raw, symtab + 8)
symbols = []
for i in range(nsyms):
    string, kind, section, _, address = struct.unpack_from('<IBBHQ', obj_raw, symoff + i * 16)
    end = obj_raw.index(b'\0', stroff + string)
    symbols.append((obj_raw[stroff + string:end].decode(), address, kind))
vt = next((address for name, address, _ in symbols if name == '__ZTV19NativeLegacyDisplay'), None)
require(vt is not None, 'Missing owned legacy display vtable')
relocs = {}
for segment in obj_image['segments']:
    for section in segment['sections']:
        if section['va'] <= vt < section['va'] + section['size']:
            off, count = struct.unpack_from('<2I', obj_raw, section['command_offset'] + 56)
            for i in range(count):
                place, bits = struct.unpack_from('<2I', obj_raw, off + i * 8)
                if bits >> 27 & 1 and bits >> 28 == 0 and bits >> 25 & 3 == 3:
                    relocs[section['va'] + place] = symbols[bits & 0xffffff][0]
compiled = [relocs.get(vt + 16 + slot * 8) for slot in range(358)]
require(all(compiled) and vt + 16 + 358 * 8 not in relocs, 'Unexpected legacy virtual table layout')
owned, inherited = [], []
for slot, symbol in enumerate(compiled):
    if symbol.startswith(('__ZN19NativeLegacyDisplay', '__ZNK19NativeLegacyDisplay')):
        owned.append(slot)
    elif '25IOMobileFramebufferLegacy' in symbol:
        require(slot not in pure, f'Unimplemented abstract display slot {slot}')
        bindings[symbol] = family_pointers[slot]
    else:
        require(slot < 168 and names.get(symbol) == family_pointers[slot],
                f'Inherited kernel symbol mismatch at slot {slot}: {symbol}')
        inherited.append(slot)
require(set(owned) == pure | {0, 1, 7, 14, 84, 86, 87, 141, 155, 184, 188, 288, 290, 296, 301},
        f'Unexpected owned display slots: {owned}')
undefined = {name for name, _, kind in symbols if kind == 1 and 'IOMobileFramebufferLegacy' in name}
direct_base_slots = {'4freeE': 14, '5startE': 86, '4stopE': 87, '13newUserClientE': 141,
                     '13setPowerStateE': 155, '20getRefreshAttributesE': 301}
for name in sorted(undefined - bindings.keys()):
    prefix = '__ZN25IOMobileFramebufferLegacy'
    method = name.removeprefix(prefix)
    slot = next((index for key, index in direct_base_slots.items() if method.startswith(key)), None)
    require(slot is not None, 'Unbound original framebuffer family import: ' + name)
    bindings[name] = family_pointers[slot]


def direct_branch(pc):
    word, = struct.unpack('<I', read(pc, 4))
    require(word & 0xfc000000 == 0x94000000, 'Expected original protected base call')
    immediate = word & 0x03ffffff
    if immediate & 0x02000000:
        immediate -= 0x04000000
    return pc + 4 * immediate


require(direct_branch(0xfffffff0057865fc) == 0xfffffff00577b510 and
        direct_branch(0xfffffff005786630) == 0xfffffff00577b55c,
        'Original protected Legacy constructor/destructor base calls mismatch')

bindings_asm = OUT / 'original-bindings.S'
synced(bindings_asm, ''.join(f'.globl {name}\n.set {name}, {address:#x}\n'
                             for name, address in sorted(bindings.items())))
def tool(name, xcrun_name=None):
    found = shutil.which(name)
    if found:
        return found
    xcrun = shutil.which('xcrun')
    if xcrun and xcrun_name:
        try:
            return subprocess.check_output([xcrun, '--find', xcrun_name], text=True).strip()
        except subprocess.CalledProcessError:
            pass
    return None


clang = tool('clang', 'clang')
objcopy = tool('llvm-objcopy', 'llvm-objcopy')
archiver = tool('llvm-ar', 'ar') or tool('ar', 'ar')
require(clang and objcopy and archiver, 'clang, llvm-objcopy and an ar implementation are required')
run([clang, '-target', TRIPLE, '-c', bindings_asm, '-o', OUT / 'original-bindings.o'])
late = OUT / 'NativeLegacyDisplay.late.o'
run([objcopy, '--remove-section=__DATA,__mod_init_func',
     '--globalize-symbol=__GLOBAL__sub_I_NativeLegacyDisplay.cpp', obj, late])
probe_obj = OUT / 'LegacyDisplayProbe.o'
probe_cmd = list(command)
for i, value in enumerate(probe_cmd):
    if value == str(staged_backend):
        probe_cmd[i] = str(PROBE)
        break
for i, value in enumerate(probe_cmd):
    if value == str(OUT / 'NativeLegacyDisplay.o'):
        probe_cmd[i] = str(probe_obj)
        break
probe_cmd += ['-I', str(PROBE.parent)]
run(probe_cmd, cwd=record['directory'])
check_object(probe_obj.read_bytes(), 'LegacyDisplayProbe')
archive = OUT / 'libNativeLegacyDisplay.late.a'
archive.unlink(missing_ok=True)
run([archiver, 'rcs', archive, late, OUT / 'original-bindings.o', probe_obj])

sources = [Path(__file__), PROBE, OUT / 'OriginalLegacyFramebufferABI.h',
           *sorted(BACKEND.parent.glob('*.h')), BACKEND]
(OUT / 'build-report.json').write_text(json.dumps({
    'source_sha256': {str(path.relative_to(ROOT)): digest(path) for path in sources},
    'kernel_candidate_sha256': candidate_sha,
    'kernel_metadata_sha256': digest(metadata_path),
    'compiled_object_sha256': digest(obj), 'late_object_sha256': digest(late),
    'probe_object_sha256': digest(probe_obj), 'archive_sha256': digest(archive),
    'bindings': {name: hex(value) for name, value in sorted(bindings.items())},
    'virtual_entries': len(compiled), 'owned_entries': owned,
    'inherited_kernel_entries': inherited, 'original_instructions_modified': False,
    'cpu_copy_backend': True, 'gpu_maps_included': False,
}, indent=2) + '\n')
print(f'PASS: {archive.relative_to(ROOT)} ({digest(archive)})')
