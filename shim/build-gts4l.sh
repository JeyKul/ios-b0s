#!/bin/bash
# Build the Tab S4 shim into a Samsung boot image, with the mkbootimg arguments of
# the working uniLoader build. Usage: build-gts4l.sh DTB_TAIL RAMDISK -> out/${OUT:-gts4l-boot.img}
set -euo pipefail
tail=$(realpath "$1"); ramdisk=$(realpath "$2")
cd "$(dirname "$0")"
X=aarch64-linux-gnu-
CFLAGS="-O2 -ffreestanding -nostdlib -fno-pic -mgeneral-regs-only -mstrict-align -fno-stack-protector -Wall -Wextra -DGTS4L ${EXTRA_CFLAGS:-}"
mkdir -p out/gts4l
for f in start.S gts4l.c console.c fdt.c loader.c; do
	${X}gcc $CFLAGS -c $f -o out/gts4l/${f%.*}.o
done
abs=$(${X}readelf -r out/gts4l/gts4l.o out/gts4l/console.o out/gts4l/fdt.o out/gts4l/loader.o | grep -c ABS64 || true)
[ "$abs" = 0 ] || { echo "error: $abs absolute relocations in C objects" >&2; exit 1; }
${X}gcc $CFLAGS -static -T shim.ld -Wl,--build-id=none -Wl,--no-warn-rwx-segments -Wl,-N \
	out/gts4l/start.o out/gts4l/gts4l.o out/gts4l/console.o out/gts4l/fdt.o out/gts4l/loader.o -lgcc -o out/gts4l/shim.elf
${X}objcopy -O binary out/gts4l/shim.elf out/gts4l/Image
size=$(${X}nm out/gts4l/shim.elf | awk '$3=="_end"{print $1}')
truncate -s $((16#$size)) out/gts4l/Image
gzip -9nc out/gts4l/Image > out/gts4l/Image.gz
cat out/gts4l/Image.gz "$tail" > out/gts4l/kernel
mkbootimg --header_version 0 --os_version 10.0.0 --os_patch_level 2022-06 \
	--kernel out/gts4l/kernel --ramdisk "$ramdisk" --pagesize 0x1000 --base 0 \
	--kernel_offset 0x8000 --ramdisk_offset 0x2000000 --second_offset 0 --tags_offset 0x1e00000 \
	--board SRPRA17A005KU --cmdline 'console=0' --out out/${OUT:-gts4l-boot.img}
