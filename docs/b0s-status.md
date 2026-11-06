# Galaxy S22 Ultra (SM-S908B, b0s, Exynos 2200 / s5e9925) status

Base: the gts4l tree (XNU xnu-8796 4K-page port, iOS/iPadOS 16 userland). Goal: the same on b0s, GPU first
(Xclipse 920, RDNA2; Samsung `sgpu` = amdgpu fork, `drivers/gpu/drm/samsung/sgpu` in ExtremeXT/android_kernel_samsung_s5e9925).

## Device (measured, TWRP, 2026-11-06)
- Bootloader S908BXXUDFYD9, unlocked (orange), TWRP 5.10.223 kernel.
- RAM 12 GB: 0x80000000-0xffffffff (holes) + 0x880000000-0xaffffffff.
- Boot image: v4, os 14.0.0 / 2024-08, raw arm64 Image, empty cmdline, AVB footer to 64 MiB. DTB from vendor_boot.
- S-Boot framebuffer: `s3cfb.bootloaderfb=0xf6200000`; panel 1440x3088 (modes 1440x3088, 1080x2316, 720x1544).
- DECON0 0x19f00000 (HW_SW_TRIG_CONTROL 0x30 = 0x1281 lets fb writes reach the panel, uniLoader g0s).
- WDT 0x10050000; CL0 NONCPU_INT_EN 0x15861244 bit 2 = nWDTRESET_EN.
- GIC 0x10200000. UFS 0x11100000. EL2 code reserved at 0xc1400000 (Samsung hypervisor), s2mpu table 0xc1800000.
- initrd (vendor + boot ramdisks) at 0x85100000.

## Shim
`shim/build-b0s.sh` -> `shim/out/b0s-boot.img`: stage 1 diag (colour bars, EL, ID regs, WDT off, DECON regs).
