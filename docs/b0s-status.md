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

## Stage 1 result (boot 1, 2026-11-06): runs, 1440-wide fb correct
- S-Boot enters at **EL1**, MMU off, image at 0x80090000, DTB 0xa0000000. Boot CPU: MIDR 0x410fd462 (A510 r0p2),
  MPIDR 0x81000000 (MT=1, core in Aff1).
- MMFR0 0x0000022200101122: PARange 40 bit, ASID 16, **4K, 16K and 64K granules** (Apple-native 16K pages possible).
- ISAR1 0x0010111101211052: APA=5 (QARMA PAC, FPACCOMBINE), GPA=1, LRCPC2, I8MM, BF16, SB, SPECRES.
- SCTLR_EL1 0x30d8199a. **CNTFRQ_EL0 reads 26 MHz (0x18cba80)**; the DT forces 25.6 MHz. To be measured.
- initrd 0x85100000-0x8644c164 = vendor ramdisk (0x134ba9d) + boot ramdisk (unaligned, at 0x8644ba9d) + bootconfig.
  The stage 2 loader must find the payload by magic and copy it aligned.
- WTCON 0x1b7390 -> 0x1b7180 after kill_dog. DECON 0x30 reads 0x10010 after the 0x1281 write; fb writes show anyway.

## XNU boots (2026-11-06)
- Boot 2 (gts4l kernel 0369): up to "starting device-tree platform expert", hung reading the APSS WDT. 0370 fixed it.
- Boot 3 (0370): to the root wait (`Waiting on <dict>`), no root device.
- Boot 4 (0371, rd=ufsroot0): UFS transport works; TEST UNIT READY: ocs 0, upiu 0x21, response 0x1, status 0x2 (x3).
  0373 prints the sense data and wakes the device W-LUN with START STOP UNIT.
- Boot 5 (0372, cpus=8, PSCI over hvc): all 8 cores up.
- Root: iOS 16.7.16 iPhone X image on userdata. A raw adb exec-in stream on a slow port lost bytes (188/256 chunks bad);
  written again in verified 1 GiB parts over USB 3.
- Boot 6-8 (0373-0375): UFS sense 2/04/02 until START STOP UNIT (active) to the device W-LUN; root mounts, launchd
  starts. Boot 9 (cpus=1) the same. Boot 10 (sweetsample): launchd blocked in dyld's first vm_allocate trap.
- Cause: PAN. The TAIMEN config has no __ARM_PAN_AVAILABLE__ and SCTLR_EL1 SPAN=0, so every exception set
  PSTATE.PAN on the A510/A710/X2 and copyout faulted forever. 0377 sets SPAN.
- **Boot 11 (0375 + SPAN bit patched in the binary, cpus=8): iOS 16.7.16 boots to the GUI.** 1125x2436 canvas
  1:1 in the middle of the panel, console over it, no touch. Boot 12: canvas 1440x3084, sweetfb_native=1,
  sweetfb_noconsole=1.
- Builds: xnu-mini (self-hosted M1, Xcode 27 + 0376) 7m48s cold; hosted macos-15 3m54s with cache.
- Touch (0378-0384, bsd/dev/b0s_touch.c): rails on through ACPM (LDO28M 0x18->0xd8, LDO17S 0x30->0xf0), pins, USI
  SPI mode, SPI self-loopback OK. Real transfers stalled after one byte through boot 18: SPI2's ipclk is USI02
  (DT clock 0x771 = PERIC2 base + 5), not USI06, so it ran at 400 MHz / 4. 0384 sets USI02 / 16 (6.25 MHz SPI).
  **Boot 19 (0384): touch works.**
- Flash flow: tools/b0s-boot-xnu.sh KERNEL rd=ufsroot0 ufsrw=1 sweetlog=1 ... sweetfb_native=1 sweetfb_noconsole=1
  cpus=8 (ROOTFIX=~/gts4l-xnu/recon/rootfix.bin); dd to boot from TWRP; log = last 4100 KiB of userdata
  (tools/sweet-log.py). Check SPAN by pattern (movk #0x34d4 before msr sctlr_el1), not by offset.
