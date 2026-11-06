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

## Platform (live DT)
- Kernel runs at EL1: PSCI `method = "hvc"` (Samsung EL2 below us). Arch timer 25.6 MHz.
- GICv3: GICD 0x10200000, GICR 0x10240000 (2 MiB). CPUs MPIDR 0x000-0x700 (aff1 = core): 4x A510, 3x A710, 1x X2.

## GPU plan (Xclipse 920)
- sgpu = amdgpu fork, `CHIP_VANGOGH_LITE` (gfx10.3). Node: 0x16e00000 (1 MiB, SMN target), 0x16f00000,
  0x16dc0000, 0x16cd0000, 0x16c00000; IRQ SPI 0x1d3; power domain pd_g3dcore; CMU cal id 0xb04000d; 302-1306 MHz.
- Firmware: one signed unified blob (`unified_firmware_sign.h`, v2.15.1, 225280 B) in the kernel source, copied to
  `sgpu_rmem` 0x8ff00000 (0xe0000), RLC backdoor autoload, then `exynos_verify_subsystem_fw("G3D_TMR")` (EL2).
  No PSP. Same shape as the a540 zap shader on gts4l.
- Userland: Mesa radeonsi/RADV for gfx10.3 (ACO), amdgpu winsys over a SweetGPU-style XNU service, replacing
  freedreno in sweetgl/sweetmtl.

## Liquid Glass (iOS 26) target
- CPU has PAC (paca/pacg) and BTI: arm64e userland possible, unlike gts4l. Parts 0xd46 A510 x4, 0xd47 A710 x3, 0xd48 X2.
- Kernel source: xnu-12377.121.6 (iOS/macOS 26) is public; no iOS 27 source, so the 24A IPSW is out.
- Userland candidate: iPhone 11 Pro Max (A13, ARMv8.4, 1242x2688): oldest arm64e device on iOS 26, no SME.
- Work: port the 4K-page and platform patches from xnu-8796 to 12377; generic ARMv8.3 PAC in place of Apple's
  key registers; APFS/cryptex root for 26; Metal on Xclipse before CoreAnimation draws Liquid Glass.
