#!/bin/bash
# Write an iOS root image onto userdata in verified 1 GiB parts: gzip -1, adb push (checksummed), gunzip|dd on the
# phone (TWRP), sha256 of each part read back. Usage: tools/b0s-write-root.sh IMG WORKDIR 0 1 2 ... (part numbers).
# A plain "cat IMG | adb exec-in dd" lost bytes on a slow port (188 of 256 chunks bad, 2026-11-06).
set -u
IMG=$1; D=$2; shift 2
for i in "$@"; do
	f=$D/p$i.gz
	dd if=$IMG bs=64M skip=$((i*16)) count=16 2>/dev/null | gzip -1 > $f
	want=$(dd if=$IMG bs=64M skip=$((i*16)) count=16 2>/dev/null | sha256sum | cut -c1-16)
	ok=0
	for try in 1 2 3; do
		adb push $f /tmp/p.gz >/dev/null 2>&1 || continue
		adb shell "gunzip -c /tmp/p.gz | dd of=/dev/block/by-name/userdata bs=1M seek=$((i*1024)) 2>/dev/null; sync"
		got=$(adb shell "dd if=/dev/block/by-name/userdata bs=64M skip=$((i*16)) count=16 2>/dev/null | sha256sum" | cut -c1-16)
		[ "$got" = "$want" ] && { echo "part $i ok"; ok=1; break; }
		echo "part $i mismatch try $try"
	done
	[ $ok = 1 ] || echo "part $i FAILED"
	rm -f $f; adb shell rm -f /tmp/p.gz
done
