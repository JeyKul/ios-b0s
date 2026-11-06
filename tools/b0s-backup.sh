#!/bin/bash
# Back up every Galaxy S22 Ultra partition except userdata, plus the sdb-sde boot LUNs, over adb (TWRP).
# Then compare sizes, and sha256 of boot, vendor_boot and super. Usage: tools/b0s-backup.sh OUTDIR
# Use a USB 3 port: on a slow port adb exec-out/exec-in streams lost bytes (2026-11-06).
set -u
out=$1; mkdir -p "$out"; cd "$out"
adb shell 'cd /dev/block/by-name; for n in *; do case $n in sd?|userdata) ;; *) echo $n;; esac; done' | tr -d '\r' > names.txt
for n in $(cat names.txt); do adb exec-out "dd if=/dev/block/by-name/$n bs=4M 2>/dev/null" > $n.img || echo "FAIL $n"; done
for l in sdb sdc sdd sde; do adb exec-out "dd if=/dev/block/$l bs=1M 2>/dev/null" > lun_$l.img; done
for n in $(cat names.txt); do
	d=$(adb shell blockdev --getsize64 /dev/block/by-name/$n | tr -d '\r')
	[ "$d" = "$(stat -c%s $n.img)" ] || echo "SIZE MISMATCH $n"
done
for n in boot vendor_boot super; do
	[ "$(adb shell sha256sum /dev/block/by-name/$n | cut -d' ' -f1)" = "$(sha256sum $n.img | cut -d' ' -f1)" ] && echo "$n sha ok" || echo "$n SHA MISMATCH"
done
