#!/usr/bin/env bash
# One kernel boot image for the Galaxy Tab S4: kernel Mach-O -> ADT (mkadt --gts4l) -> SWEETXNU
# payload -> STAGE2 shim boot image with the payload as ramdisk.
# debug=0xc: a panic waits instead of rebooting, so the text stays on screen for a photo.
# taimenwdt=0: S-Boot leaves the APSS watchdog off; keep it off.
# ROOTFIX=FILE: files the kernel writes into the iOS root (tools/taimen/mkrootfix.py).
# TAIMEN_MEM=BYTES: XNU's RAM in the upper bank (mkadt.py; default 512 MiB).
# GPUFW=DIR: a530_pm4.fw, a530_pfp.fw (vendor partition), a540_zap.mdt, a540_zap.b02 (apnhlos) of this tablet.
# Usage: tools/gts4l-boot-xnu.sh KERNEL DTB_TAIL [extra boot-args]   -> shim/out/gts4l-xnu.img
set -euo pipefail
P=$(cd "$(dirname "$0")/.." && pwd)
kernel=$(realpath "$1"); tail=$(realpath "$2"); shift 2
out=$P/shim/out/gts4l-payload; mkdir -p "$out"
python3 -I "$P/tools/mkadt.py" "$out/gts4l.adt" --gts4l
# Pixel 2 XL drivers that drive board-specific GPIOs, buses and PHYs: off until each has its Tab S4 values.
off="taimentouch=0 sweettouch=0 taimenhaptic=0 taimensensors=0 sweetusb=0 taimenaudio=0 taimenmic=0 taimenwifi=0"
cmdline="debug=0xc serial=3 cpus=1 keepsyms=1 -noprogress taimenwdt=0 sweetkeys=0 sweetperf=0 sweetbattery=0 $off $*"
# XNU takes the last occurrence: drop the earlier ones (cpus=1, sweetkeys=0, ...), as boot-xnu.sh does
cmdline=$(printf '%s\n' "$cmdline" | awk '{ for (i = NF; i >= 1; i--) { k = $i; sub(/=.*/, "", k);
	if (!(k in seen)) { seen[k] = 1; keep[++n] = $i } }
	for (i = n; i >= 1; i--) printf "%s%s", keep[i], (i > 1 ? " " : "\n") }')
[ ${#cmdline} -le 607 ] || { echo "boot-args are ${#cmdline} bytes, over the 607 mkpayload keeps" >&2; exit 1; }
blobs=()
[ -n "${ROOTFIX:-}" ] && blobs+=(--blob "ROOTFIX=$(realpath "$ROOTFIX")")
if [ -n "${GPUFW:-}" ]; then
	for f in pm4 pfp; do tail -c +5 "$GPUFW/a530_$f.fw" > "$out/gpu-$f.bin"; done
	blobs+=(--blob "GPU-PM4=$out/gpu-pm4.bin" --blob "GPU-PFP=$out/gpu-pfp.bin" \
		--blob "GPU-ZAP-MDT=$GPUFW/a540_zap.mdt" --blob "GPU-ZAP-SEG=$GPUFW/a540_zap.b02")
fi
python3 -I "$P/tools/mkpayload.py" "$kernel" "$out/gts4l.adt" "$out/payload" --l2block 0x200000 --cmdline "$cmdline" "${blobs[@]}"
EXTRA_CFLAGS="-DSTAGE2 -DNOKICK" OUT=gts4l-xnu.img "$P/shim/build-gts4l.sh" "$tail" "$out/payload"
