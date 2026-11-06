#!/bin/bash
# Build the Galaxy S22 Ultra shim into a v4 boot image (stock header: os 14.0.0, 2024-08,
# raw arm64 Image, no cmdline; the stock vendor_boot stays). AVB footer to the 64 MiB
# boot partition. Usage: build-b0s.sh [RAMDISK] -> out/${OUT:-b0s-boot.img}
set -euo pipefail
ramdisk=$(realpath "${1:-/dev/null}")
cd "$(dirname "$0")"
X=aarch64-linux-gnu-
CFLAGS="-O2 -ffreestanding -nostdlib -fno-pic -mgeneral-regs-only -mstrict-align -fno-stack-protector -Wall -Wextra -DB0S ${EXTRA_CFLAGS:-}"
o=out/b0s; mkdir -p $o
for f in start.S b0s.c console.c fdt.c loader.c; do
	${X}gcc $CFLAGS -c $f -o $o/${f%.*}.o
done
abs=$(${X}readelf -r $o/b0s.o $o/console.o $o/fdt.o $o/loader.o | grep -c ABS64 || true)
[ "$abs" = 0 ] || { echo "error: $abs absolute relocations in C objects" >&2; exit 1; }
${X}gcc $CFLAGS -static -T shim.ld -Wl,--build-id=none -Wl,--no-warn-rwx-segments -Wl,-N \
	$o/start.o $o/b0s.o $o/console.o $o/fdt.o $o/loader.o -lgcc -o $o/shim.elf
${X}objcopy -O binary $o/shim.elf $o/Image
size=$(${X}nm $o/shim.elf | awk '$3=="_end"{print $1}')
truncate -s $((16#$size)) $o/Image
[ -s "$ramdisk" ] || { printf 'b0s-shim\n' > $o/empty-rd; ramdisk=$PWD/$o/empty-rd; }
mkbootimg --header_version 4 --os_version 14.0.0 --os_patch_level 2024-08 \
	--kernel $o/Image --ramdisk "$ramdisk" --cmdline '' --out out/${OUT:-b0s-boot.img}
avbtool add_hash_footer --image out/${OUT:-b0s-boot.img} --partition_name boot --partition_size 67108864
ls -la out/${OUT:-b0s-boot.img}
