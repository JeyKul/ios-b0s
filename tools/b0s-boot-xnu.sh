#!/usr/bin/env bash
# Galaxy S22 Ultra boot image: kernel Mach-O -> ADT (mkadt --b0s) -> SWEETXNU payload -> STAGE2 shim
# boot image with the payload as its ramdisk. The TAIMEN kernel with every Qualcomm driver off.
# No rd= by default: the Qualcomm UFS driver must never run here (rd=ufsroot0 starts it).
# Usage: tools/b0s-boot-xnu.sh KERNEL [extra boot-args]   -> shim/out/b0s-xnu.img
set -euo pipefail
P=$(cd "$(dirname "$0")/.." && pwd)
kernel=$(realpath "$1"); shift
out=$P/shim/out/b0s-payload; mkdir -p "$out"
python3 -I "$P/tools/mkadt.py" "$out/b0s.adt" --b0s
off="taimentouch=0 sweettouch=0 taimenhaptic=0 taimensensors=0 sweetusb=0 taimenaudio=0 taimenmic=0 taimenwifi=0"
off="$off taimenwdt=0 sweetkeys=0 sweetperf=0 sweetbattery=0 sweetlog=0 gts4ltouch=0 gts4lkeys=0 gts4lpen=0"
cmdline="debug=0xc serial=3 cpus=1 keepsyms=1 -v $off $*"
cmdline=$(printf '%s\n' "$cmdline" | awk '{ for (i = NF; i >= 1; i--) { k = $i; sub(/=.*/, "", k);
	if (!(k in seen)) { seen[k] = 1; keep[++n] = $i } }
	for (i = n; i >= 1; i--) printf "%s%s", keep[i], (i > 1 ? " " : "\n") }')
[ ${#cmdline} -le 607 ] || { echo "boot-args are ${#cmdline} bytes, over 607" >&2; exit 1; }
echo "boot-args: $cmdline"
python3 -I "$P/tools/mkpayload.py" "$kernel" "$out/b0s.adt" "$out/payload" --l2block 0x200000 --cmdline "$cmdline"
EXTRA_CFLAGS="-DSTAGE2" OUT=b0s-xnu.img "$P/shim/build-b0s.sh" "$out/payload"
