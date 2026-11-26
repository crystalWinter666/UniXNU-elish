#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-2-Clause-Patent
# Rebuild the pinned run79 kernel with touch, then update only the kernel and
# boot args in the run80 FAT. Reuses the verified run80 UEFI and FDT.
set -euo pipefail
cd "$(dirname "$(realpath "$0")")/.."
: "${UNIXNU_ELISH_TOUCH_STOCK_BOOT:?set this to the local factory Android boot.img}"
base=$(realpath "${1:?pass the baseline artifact directory}")
out=$(realpath -m "${2:?pass a new candidate output directory}")
mode=${3:-spi}
[[ "$mode" == spi || "$mode" == hid ]]
deadline=${UNIXNU_ELISH_TOUCH_DEADLINE:-420}
if [[ ! "$deadline" =~ ^[1-9][0-9]{0,2}$ ]] || (( deadline > 600 )); then
    printf 'UNIXNU_ELISH_TOUCH_DEADLINE must be an integer from 1 to 600 seconds\n' >&2
    exit 2
fi
mkdir "$out"
exec > >(tee "$out/build.log") 2>&1
core=$(realpath "$base/../..")
ref=$base/run79-key-store
baseline=$base/run80-installed-hello
fw=$(realpath "${UNIXNU_UEFI:-../uefi-elish}")
fd=$out/SILICIUM_UEFI.fd
dtb=reports/elish-recovery-fdt-20261124.dtb
work=$core/out/elish/ipados-17.7.11-21H461/elish
export PATH=/usr/lib/llvm-21/bin:$PATH UNIXNU_UEFI=$fw
export UNIXNU_ELISH_ROOT_IMAGE="$work/userdata/userdata-root.img"
export UNIXNU_ELISH_TOUCH_STOCK_BOOT=$(realpath "$UNIXNU_ELISH_TOUCH_STOCK_BOOT")
export UNIXNU_ELISH_TOUCH_HID_REFERENCE="$ref/kernel.macho"
export UNIXNU_ELISH_TOUCH_HID=0
[[ "$mode" != hid ]] || UNIXNU_ELISH_TOUCH_HID=1
sha256sum -c "$baseline/artifacts.sha256"
python3 - "$baseline/boot.img" "$ref/boot.json" "$fd" <<'PY'
import gzip, hashlib, json, struct, sys
from pathlib import Path

boot, manifest, output = map(Path, sys.argv[1:])
info = json.loads(manifest.read_text())
with boot.open('rb') as stream:
    header = stream.read(4096)
    if header[:8] != b'ANDROID!' or struct.unpack_from('<I', header, 40)[0] != 3:
        sys.exit('The run80 carrier is not Android boot v3')
    carrier = stream.read(struct.unpack_from('<I', header, 8)[0])
if hashlib.sha256(carrier).hexdigest() != info['kernel_sha256']:
    sys.exit('The run80 carrier does not match run79')
fd = gzip.decompress(carrier)[info['shim_bytes']:]
if hashlib.sha256(fd).hexdigest() != info['fd_sha256']:
    sys.exit('The extracted run80 firmware does not match run79')
output.write_bytes(fd)
PY
jq -r --arg fd "$fd" --arg dtb "$dtb" '.fd_sha256 + "  " + $fd, .dtb_sha256 + "  " + $dtb' \
    "$ref/boot.json" | sha256sum -c
# Validate unchanged inputs from the original run; hash deliberate edits below.
grep -vE '  (board\.toml|package/ElishUfs\.cpp)$' "$ref/source.sha256" | sha256sum -c
[[ $(sha256sum "$ref/kernel.macho" | cut -d' ' -f1) == bdeebf88a95b69ea94d8b3990a42ec9618bc6b0fc6cbd16587a9d46c284c4ddf ]]
# The baseline legacy display ABI generator reads the last candidate. Keep
# the original build result next to its metadata for every new iteration.
cp "$ref/kernel.macho" "$ref/kernel.json" "$work/kernel/"
python3 - "$ref/kernel.config" "$out/kernel.config" "$mode" <<'PY'
from pathlib import Path
import re, sys
source, out, mode = sys.argv[1:]
config = Path(source).read_text()
archives = ' {tree}/build/elish-touch/libElishTouch.a'
if mode == 'hid':
    archives += ' {tree}/build/elish-touch-hid/libElishTouchHid.a'
config, count = re.subn(r'^(CONFIG_XNU_EXTRA_LD_ARCHIVES="[^"]+)"$', lambda m: m[1] + archives + '"', config, flags=re.M)
if count != 1:
    raise SystemExit('run79 archive configuration not found')
Path(out).write_text(config)
PY
cp "$out/kernel.config" "$work/.config"
dev=(-D "$PWD" --os ipados-17.7.11-21H461 --local-module "soc:sm8250=$ref/UniXNU-sm8250-common"
     --local-module "driver:AppleKeyStoreState=$ref/AppleKeyStoreState")
sha256sum board.toml configs/elish_defconfig package/Elish{Touch*,Ufs.cpp} scripts/{build-elish-touch*,gen-elish-touch-firmware}.py \
    scripts/build-elish-touch-candidate.sh "$out/kernel.config" "$UNIXNU_ELISH_TOUCH_STOCK_BOOT" > "$out/source.sha256"
python3 "$core/unixnu" build kernel "${dev[@]}" -j "${JOBS:-12}"
python3 "$work/tree/scripts/bind-elish-ram-block.py" --check "$work/kernel/kernel.macho"
cp "$work/kernel/kernel.macho" "$work/kernel/kernel.json" "$out/"

# Retain the payload-only panic renderer and preserve the exact CPU ADT,
# System/SystemOS policy, trust cache and initial RAM Data bytes from run80.
# Use the normal payload step to apply the kernel patches, not a raw copy of
# the prelink builder output into the FAT.
mapfile -t args < <(python3 - "$baseline/unixnu.toml" <<'PY'
import sys, tomllib
with open(sys.argv[1], 'rb') as src:
    print(*tomllib.load(src)['entry'][0]['args'].split(), sep='\n')
PY
)
kargs=()
for arg in "${args[@]}" elish-touch=1 elish-touch-snapshot=1 "elish-ram-only=$deadline"; do kargs+=("--kernel-arg=$arg"); done
python3 "$core/unixnu" build payload "${dev[@]}" --uefi "$fw" --kernel "$out/kernel.macho" \
    --trust-cache "$base/run56-verified-systemos-union/apple.trustcache" \
    --root-policies "$base/run56-verified-systemos-union/kernel.macho.vroot" \
    --data-ramdisk "$base/run76-mobile-tmp/data-21H461.dbr" "${kargs[@]}"
fat=::/EFI/UniXNU
# The normal payload step regenerates /chosen/random-seed. Discard its entire
# ADT and read back the unchanged run80 ADT from the copied FAT instead.
for asset in kernel.macho.vroot kernel.macho.trustcache scratch-data.img; do
    cmp <(mtype -i "$baseline/payload.img" "$fat/Payloads/$asset") \
        <(mtype -i "$work/payload/payload.img" "$fat/Payloads/$asset")
done
mtype -i "$work/payload/payload.img" "$fat/Payloads/kernel.macho" > "$out/packed-kernel.macho"
cp "$baseline/payload.img" "$out/payload.img"
python3 - "$baseline/unixnu.toml" "$out/unixnu.toml" "$deadline" <<'PY'
from pathlib import Path
import sys, tomllib
src, dst = map(Path, sys.argv[1:3])
deadline = int(sys.argv[3])
config = src.read_text()
original = tomllib.loads(config)
assert 'elish-userspace-snapshot' not in config and 'elish-ram-only' not in config
args = original['entry'][0]['args']
updated = args + f' elish-touch=1 elish-touch-snapshot=1 elish-ram-only={deadline}'
assert config.count(args) == 1
text = config.replace(args, updated)
original['entry'][0]['args'] = updated
assert tomllib.loads(text) == original
dst.write_text(text)
PY
mcopy -o -i "$out/payload.img" "$out/packed-kernel.macho" "$fat/Payloads/kernel.macho"
mcopy -o -i "$out/payload.img" "$out/unixnu.toml" "$fat/unixnu.toml"
cmp "$out/packed-kernel.macho" <(mtype -i "$out/payload.img" "$fat/Payloads/kernel.macho")
cmp "$out/unixnu.toml" <(mtype -i "$out/payload.img" "$fat/unixnu.toml")
for asset in kernel.macho.adt kernel.macho.vroot kernel.macho.trustcache scratch-data.img; do
    cmp <(mtype -i "$baseline/payload.img" "$fat/Payloads/$asset") \
        <(mtype -i "$out/payload.img" "$fat/Payloads/$asset")
done
fsck.fat -vn "$out/payload.img" > "$out/fat-check.log"
python3 "$core/unixnu" bootimg "${dev[@]}" --uefi "$fw" --fd "$fd" --dtb "$dtb" \
    --ramdisk "$out/payload.img" --output "$out/boot.img"
sha256sum "$out/boot.img" "$out/payload.img" > "$out/artifacts.sha256"
sha256sum -c "$out/artifacts.sha256"
