#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-2-Clause-Patent
# Send only a verified RAM boot and capture it in one continuous local file.
set -euo pipefail
out=$(realpath "${1:?pass a touch candidate directory}")
serial=4bed07e4
capture=$(mktemp -d "$out/device-attempt.XXXXXX")
printf 'Capture directory: %s\n' "$capture"
exec > >(tee "$capture/device.log") 2>&1

identity() {
    local usb nodes reply
    usb=$(lsusb -d 18d1:d00d) && [[ "$usb" != *$'\n'* ]] || return 1
    printf '%s\n' "$usb"
    nodes=$(fastboot devices -l) || return 1
    grep -Eq "^${serial}[[:space:]]+fastboot([[:space:]]|$)" <<< "$nodes" || return 1
    reply=$(timeout 5 fastboot -s "$serial" getvar product 2>&1) || { printf '%s\n' "$reply"; return 1; }
    printf '%s\n' "$reply"
    grep -Fxq 'product: elish' <<< "$reply" || return 1
    reply=$(timeout 5 fastboot -s "$serial" getvar is-userspace 2>&1) || { printf '%s\n' "$reply"; return 1; }
    printf '%s\n' "$reply"
    grep -Fxq 'is-userspace: no' <<< "$reply" || return 1
    [[ "$usb" == "$(lsusb -d 18d1:d00d)" ]]
}

finish() {
    kill -INT "$camera" 2>/dev/null || true
    wait "$camera" || true
    if [[ -s "$capture/camera.mkv" ]]; then
        ffprobe -v error -show_entries format=duration,size "$capture/camera.mkv" > "$capture/camera-info.txt" || true
        sha256sum "$capture/camera.mkv" > "$capture/camera.sha256"
    fi
}

sha256sum -c "$out/artifacts.sha256"
deadline=$(mtype -i "$out/payload.img" ::/EFI/UniXNU/unixnu.toml | python3 -I -c '
import sys, tomllib
args = tomllib.loads(sys.stdin.read())["entry"][0]["args"].split()
values = [arg.partition("=")[2] for arg in args if arg.startswith("elish-ram-only=")]
if len(values) != 1 or not values[0].isascii() or not values[0].isdecimal() or not 1 <= int(values[0]) <= 600:
    sys.exit("Missing or invalid elish-ram-only deadline in verified FAT")
print(int(values[0]))
')
identity
usb_before=$(lsusb -d 18d1:d00d)
ffmpeg -nostdin -hide_banner -loglevel warning -f v4l2 -input_format mjpeg \
    -video_size 2592x1944 -framerate 30 -i "${ELISH_CAMERA:-/dev/video0}" -an -c:v copy \
    "$capture/camera.mkv" > "$capture/camera.log" 2>&1 &
camera=$!
trap finish EXIT
sleep 2
kill -0 "$camera"
[[ "$usb_before" == "$(lsusb -d 18d1:d00d)" ]]
date -Ins
printf 'RAM-only boot: fastboot -s %s boot %s\n' "$serial" "$out/boot.img"
timeout 28 fastboot -s "$serial" boot "$out/boot.img"
date -Ins
# The kernel deadline starts after loader and early XNU startup, not USB boot.
end=$((SECONDS + deadline + 40))
while (( SECONDS < end )); do
    sleep 2
    if usb_now=$(lsusb -d 18d1:d00d) && [[ "$usb_now" != "$usb_before" ]] && identity; then
        date -Ins
        printf 'RETURNED_TO_BOOTLOADER_FASTBOOT\n'
        exit 0
    fi
done
date -Ins
printf 'BOOTLOADER_FASTBOOT_DEADLINE_EXPIRED\n'
exit 1
