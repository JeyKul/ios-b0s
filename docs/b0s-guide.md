# iOS 16 on the Galaxy S22 Ultra: the complete guide (A to Z)

Everything needed to reproduce, understand and continue the b0s port without the original people. Short state and
dated log: `docs/b0s-status.md`. GPU/ACPM detail: `docs/b0s-gpu-handoff.md`. Working rules: `CLAUDE.md`.

---------------------------------------------------------------------------------------------------------------

## 1. What this is

- Phone: Samsung Galaxy S22 Ultra, SM-S908B (codename **b0s**), Exynos 2200 (**s5e9925**): 4x Cortex-A510,
  3x A710, 1x X2 (ARMv9, PAC, BTI, 16K granule), 12 GB LPDDR5, UFS 3.1, Xclipse 920 GPU (AMD RDNA2), 1440x3088 OLED.
- Software: Apple's open-source **XNU xnu-8796.141.3** (iOS 16 era), patched, running the **unmodified iOS 16.7.16
  userland of the iPhone X** (iPhone10,3 / D22AP, build 20H392) from an HFS+ root on the userdata partition.
- Lineage: Redmi Note 10 Pro "sweet" port (dipa-studio/ios-sweet) -> Pixel 1 (ios-sailfish) -> Pixel 2 XL
  (SourceQuality/ios-taimen) -> Galaxy Tab S4 (JeyKul/ios-taimen, branch gts4l) -> this repo (JeyKul/ios-b0s,
  branch b0s). Most kernel code comes from those; b0s adds Exynos support.
- Result (2026-11-06): boots to the iOS lock and home screen, 8 cores, 9 GiB RAM, 16 KiB pages, full panel, touch,
  root over the whole userdata. CPU rendering only. No GPU, Wi-Fi, audio, sensors, keys, battery, USB.

## 2. How it boots

```
S-Boot (Samsung bootloader, unlocked) loads boot.img (v4) + stock vendor_boot
  -> the "kernel" in boot.img is our shim (shim/start.S + shim/b0s.c + shim/loader.c), entered at EL1, MMU off,
     x0 = DTB (stage 1 measured CurrentEL = 1). Samsung's hypervisor is already resident at EL2 (PSCI via hvc);
     the shim and XNU both run at EL1 under it, nothing of ours runs at EL2.
  -> shim finds the "SWEETXNU" payload in the initrd (vendor ramdisk + our ramdisk + bootconfig, unaligned),
     copies it to 0xd0000000, loads the XNU Mach-O segments to the RAM base, places the Apple Device Tree (ADT)
     and boot_args, jumps.
  -> XNU (MACHINE_CONFIG B0S) -> Exynos UFS driver -> HFS+ root on userdata -> launchd -> SpringBoard
```

Nothing in the bootloader chain is replaced. Android comes back by flashing the stock boot.img.

## 3. Prerequisites

- Unlocked bootloader (OEM unlock), **TWRP** in the recovery partition (we used ExtremeXT's b0s TWRP,
  kernel 5.10.223), adb over **USB 3** (USB 2 corrupted adb streams, see 9.2).
- Linux PC: `aarch64-linux-gnu-gcc`, `mkbootimg`, `unpack_bootimg`, `avbtool`, `dtc`, `python3`, `7z`
  (reads HFS+), `llvm-objdump`/`llvm-nm`, `gh` (GitHub CLI).
- Kernel builds: GitHub Actions. `.github/workflows/xnu-mini.yml` runs on push on a **self-hosted Apple Silicon
  Mac mini** (labels `self-hosted, ios-b0s`, Xcode 27, brew: jq gum cmake ninja ccache, `/usr/local/bin/xcodes`
  stub). `.github/workflows/xnu.yml` (GitHub's macos-15, Xcode 16) is dispatch-only, the fallback.
  Input `machine_config`: **B0S** (16 KiB, current) or TAIMEN (4 KiB, old). Artifact: `kernel.development.b0s`.
- Xcode 27's iig needs xnu patch 0376 (IORPC.kernelContent).

## 4. From a stock phone to iOS, step by step

1. **Back up** (TWRP, phone on USB 3): `tools/b0s-backup.sh ~/s22u-ios/backup` -> every partition except
   userdata + LUNs sdb-sde, sizes and sha256 checked. Keep it out of git (EFS/IMEI inside).
2. **iOS root image** (on the PC). Needs the iPhone X IPSW `iPhone10,3_16.7.16_20H392_Restore.ipsw`:
   - unpack OS root `087-86698-044.dmg`, SystemOS cryptex `087-86593-047.dmg`, AppOS cryptex `087-86715-047.dmg`
   - `apfs-fuse` (built from sgan81/apfs-fuse) on PATH, then
     `tools/mkiosroot.sh OS.dmg SYSOS.dmg APPOS.dmg ios16-root.img 16G` (needs sudo for the loop mount).
   - Result used here: `~/gts4l-xnu/ipsw/ios16-root.img` (16 GiB HFSX, volume "iOSRoot").
3. **rootfix blob** (files the kernel writes into the root at boot: launchd.plist edits, Setup/migration skipped):
   `tools/taimen/mkrootfix.py ios16-root.img rootfix.bin` (used here: `~/gts4l-xnu/recon/rootfix.bin`).
4. **D22AP device tree** (MobileGestalt identity, `research/ipsw-x/d22ap.adt`, not in git):
   `tools/remotezip.py IPSW_URL DeviceTree.d22ap.im4p DIR` then
   `ipsw img4 im4p extract DIR/Firmware/all_flash/DeviceTree.d22ap.im4p -o research/ipsw-x/d22ap.adt`.
5. **Write the root** to userdata (erases Android data!):
   `tools/b0s-write-root.sh ~/gts4l-xnu/ipsw/ios16-root.img /tmp/parts $(seq 0 15)` (verified 1 GiB parts).
6. **Kernel**: push to `b0s` (or `gh workflow run xnu-mini.yml -R JeyKul/ios-b0s --ref b0s -f machine_config=B0S`),
   then `gh run download RUN -R JeyKul/ios-b0s -D research/kernels/X`.
7. **Boot image** and flash (TWRP):
   ```
   B0S_BASE=0x8bc000000 B0S_MEM=0x242000000 ROOTFIX=.../rootfix.bin tools/b0s-boot-xnu.sh kernel.development.b0s \
     rd=ufsroot0 ufsrw=1 sweetlog=1 cs_enforcement_disable=1 unentitled_ios_sim_launch=1 ignition_level=1 \
     launchd_unsecure_cache=1 wdt_test=no_user_checkin sweetprof=- sweetsample=- sweetfb_native=1 \
     sweetfb_noconsole=1 cpus=8
   adb push shim/out/b0s-xnu.img /tmp/b.img && adb shell 'dd if=/tmp/b.img of=/dev/block/by-name/boot bs=4M && sync'
   adb shell sha256sum /dev/block/by-name/boot      # compare with the local file
   adb reboot
   ```
8. **First boot once** with `b0sgrowroot=1` added: grows HFS+ over the whole partition (16 GiB -> 474124 MiB in
   0.7 s). Harmless to leave on.
9. **Logs**: the kernel mirrors its console to the last 4100 KiB of userdata. In TWRP:
   ```
   sz=$(adb shell blockdev --getsize64 /dev/block/by-name/userdata | tr -d '\r')
   adb exec-out "dd if=/dev/block/by-name/userdata bs=4096 skip=$((sz/4096-1025)) count=1025" > log.raw
   python3 -I tools/sweet-log.py log.raw log.txt
   ```
   The header's flush counter only moves when there is new console text: "one flush" is not proof of a hang.
   Always check which boot a log belongs to (patch strings, timestamps): a frozen boot leaves the previous log.
10. **Back to Android**: flash `backup/boot.img` to boot (and restore userdata if needed).

## 5. Boot-args (what each does)

| arg | meaning |
|---|---|
| `b0s=1` | the kernel is on b0s: no Qualcomm blocks, PSCI over hvc (0370) |
| `rd=ufsroot0 ufsrw=1` | root on the GPT partition "userdata" through the Exynos UFS driver, writable |
| `sweetlog=1` | console log ring to the end of userdata |
| `cpus=8` | all cores (PSCI CPU_ON over hvc, 0372) |
| `sweetfb_native=1 sweetfb_noconsole=1` | iOS draws 1440x3084 1:1; console hidden once the GUI runs |
| `b0sgrowroot=1` | grow the HFS+ root once (0385) |
| `b0stouch=0/1/2` | touch off / probe only / on (default 2) |
| `b0sgpu=1` | GPU/ACPM experiments (off by default; see 8) |
| `cs_enforcement_disable=1 ...` | iOS bring-up flags inherited from the Pixel/Tab S4 ports |
| `sweetsample=NAME,... sweetsampleint=S` | periodic user backtraces of named processes (find stuck daemons) |

## 6. Hardware facts (live DT of this phone + Samsung kernel source)

Source for everything below: ExtremeXT/android_kernel_samsung_s5e9925 (5.10) and the phone's
`/sys/firmware/fdt` (dumped to `~/s22u-ios/recon/live.dts`, not in git: it contains serials).

- RAM: 0x80000000-0xffffffff (many carve-outs), 0x880000000-0xaffffffff. Largest clean range for XNU:
  **0x8_bc000000-0xa_fe600000** (after sec_rdx_bootdev, before el2_earlymem). Shim payload buffer: 0xd0000000.
- S-Boot framebuffer `0xf6200000` (cmdline `s3cfb.bootloaderfb`), 1440x3088 ARGB, stride 5760. DECON0 0x19f00000;
  `HW_SW_TRIG_CONTROL` (0x30) = 0x1281 makes framebuffer writes reach the panel (from uniLoader's g0s board).
- Boot CPU A510, MPIDR 0x81000000 (MT bit; core in Aff1). MMFR0 0x0000022200101122: 4K/16K/64K granules, 40-bit PA.
  ISAR1: PAC (APA=5), FPAC. CNTFRQ_EL0 reads 26 MHz; the DT says 25.6 MHz (used; not measured).
- GICv3: GICD 0x10200000, GICR 0x10240000, stride 0x20000. Virtual timer PPI 11 = INTID 27.
- Watchdog 0x10050000 (shim turns it off). EL2 code at 0xc1400000, S2MPU tables 0xc1800000.
- PMIC: S2MPS25 (main) / S2MPS26 (sub) over SPMI, reached through the **ACPM mailbox 0x15900000** (simple inline
  protocol, channel 8, uniLoader drivers/samsung/exynos-acpm.c). LDO ctrl = 0xc0 enable | voltage code.
- Exynos 2200 silicon on this phone: **EVT 1.2** (last_kmsg).

## 7. Subsystems: what was done and why

### 7.1 Shim (shim/b0s.c, shim/loader.c, shim/build-b0s.sh)
- Stage 1 (no payload): colour bars, EL, ID registers, WDT off, DECON registers; OLED guard (dim, blank after 90 s).
- Stage 2: payload search by magic (byte loads: MMU off = Device memory, unaligned 8-byte loads fault), byte copy to
  0xd0000000, `B0S_GUI_H` 3084 as boot video height (3084/3 = 1028 points).

### 7.2 Kernel configuration (0386/0387)
- **B0S** = `-DARM64_BOARD_CONFIG_SWEET -DARM64_BOARD_CONFIG_TAIMEN -DARM64_BOARD_CONFIG_B0S -mcpu=cortex-a76`:
  every TAIMEN driver with SWEET's 16 KiB ARM_LARGE_MEMORY layout. The 4 KiB TAIMEN kernel only fits 4-5 GiB:
  `kmem_range_init: vm_map_locate_space failing for kmem_ptr_range` at 6 GiB (39-bit KVA).
- Shared region without code-signing enforcement (no trust caches): otherwise "Text page corruption detected".
- **SCTLR_EL1.SPAN = 1 (0377)**: the config assumes no PAN (Kryo); the Exynos cores have PAN, so every exception
  set PSTATE.PAN and copyout faulted forever: launchd hung in its first `vm_allocate` trap. Found with
  `sweetsample` + symbolicating dyld from the root image.

### 7.3 Board isolation (0370, 0374, 0375)
- `sweet_is_b0s()` (boot-arg b0s=1) gates every Qualcomm access: APSS WDT (its read hung the first boot), PS_HOLD,
  all MSM8998/MSM8996 drivers (touch, keys, SPMI, perf, GPU, modem, audio, sensors, USB, Wi-Fi, battery).
- Reset: PSCI SYSTEM_RESET over hvc. CPU_ON over hvc (0372): 8/8 cores, SGIs by Aff1.

### 7.4 Storage (0371, 0373)
- Exynos UFS is UFSHCI plus vendor registers at +0x1100. Linux (ufs-exynos*.c) needs: FMP descriptor type 3 set by
  SMC (`SMC_CMD_SMU` 0x82001850, then `SMC_CMD_FMP_SECURITY` 0x8C2001810 with desctype 3): **128-byte PRDT entries**,
  crypto fields zero; `HCI_TXPRDT_ENTRY_SIZE` = 12 (4 KiB per entry); `fixed-prdt-req_list-ocs` (dword PRDT counts).
- After S-Boot every command fails NOT READY (sense 2/04/02) until **START STOP UNIT (ACTIVE) to the UFS Device W-LUN
  0xd0** (as ufshcd_set_dev_pwr_mode).

### 7.5 Display
- No display driver: S-Boot leaves DECON scanning 0xf6200000; AppleCLCD (SweetFramebuffer) copies frames there.
- Full panel: ADT canvas 1440x3084 + `sweetfb_native=1`.

### 7.6 Touch (bsd/dev/b0s_touch.c, 0378-0384)
- STM FTS (fts2ba61y) on **SPI2 @ 0x11d40000** (USI). Polled on the IRQ line level (gpa1-2, active low).
- Rails through ACPM SPMI, only if off: tsp_io = LDO28M 1.8 V (ctrl 0xd8), tsp_avdd = LDO17S 3.0 V (ctrl 0xf0).
- **SPI2's clock is USI02** (DT clock 0x771 = PERIC2 base 1900 + 5), not USI06: mux 0x11c0101c = 1 (400 MHz),
  div 0x11c0181c = 15 (/16) -> 6.25 MHz SPI; gates 0x11c02080/20cc/20d0; QCH 0x11c03080 CLOCK_REQ.
  Six boots were lost configuring USI06 (the SPI ran at 100 MHz: loopback fine, real transfers stalled).
- Also needed: `SLAVE_SEL` 0 during a transfer even with a GPIO CS; FIFO access 32-bit per byte; FB_CLK 0;
  sysreg 0x11c22010 = 2 (USI SPI mode); pins gpp2-0..2 function 2, gpp2-3 GPIO CS.
- Protocol (stm_core.c): write = [C0] reg data; read 0x87 = event FIFO (16-byte events); reset FA 20 00 00 24 81;
  sense on C0 10; scan A0 00 01. Coordinates 0-4095 scaled to 1440x3088.

### 7.7 Root filesystem
- HFS+ (open-source XNU has no APFS). The kernel's taimen_rootfix applies the ROOTFIX blob once launchd remounts
  the root read-write. 0385 grows it with `HFSIOC_RESIZE_VOLUME` (hfs_extendfs).

## 8. GPU (Xclipse 920): research and state

- Samsung's driver: `drivers/gpu/drm/samsung/sgpu` = amdgpu fork, `CHIP_VANGOGH_LITE` (gfx 10.3). Windows (DT):
  rmmio 0x16e00000 (1 MiB), doorbell 0x16f00000, 0x16dc0000, BG3D_PWRCTL/DVFS 0x16cd0000, sysreg 0x16c00000.
  IRQ SPI 0x1d3. GC registers: rmmio + 4 x (GC_BASE {0x1260, 0xa000}[seg] + reg).
- Firmware: one signed blob in the kernel source (`unified_firmware_sign.h`, v2.15.1, 225280 B), copied to sgpu_rmem
  0x8ff00000 (0xe0000), verified by EL2 (`exynos_verify_subsystem_fw("G3D_TMR")`), RLC backdoor autoload, no PSP.
- Power (cal-if/s5e9925/pmucal, EVT1.1+ = `g3dcore_on_evt1_1`): PLL_G3D (CMU_G3D 0x16c30000), V_PWREN set-bit atomic
  (PMU 0x15863ef0 | 0xc000, value 0xa), wait VGPIO_TX (0x15860a10) bit 7, BG3D_PWRCTL_CTL2 cond-writes, wait
  PWRCTL_STATUS bit 1, then the PD driver's DTZPC restore SMC (0x82000410, 1, need_smc 0x16c10204, 2) + S2MPU.
- Measured (boots 24-26): rail on and acked, but G3DCORE_STATUS only after the EVT1.0 `G3DCORE_CONFIGURATION` write,
  and the first BG3D_PWRCTL read **freezes the SoC**. Linux first calls `cal_dfs_set_rate(0xb04000d, boot_freq)`
  (sgpu_governor.c:1103) = **ACPM DVFS**: APM sets the G3D voltage and PLL.
- ACPM DVFS transport (acpm/acpm_ipc.c, fw_header/framework.h): APM SRAM 0x02060000, `acpm_framework` at +0xf800,
  12 plugins (2 fvp owns ch 5 = DT acpm_dvfs; 9 plugin_DVFS owns ch 3), 13 AP channels. Queue words are SRAM
  offsets of the indices (0392). **ch 5 is TYPE_BUFFER, q_len 1**; Samsung's send only proceeds when APM's
  rx_rear word (SRAM 0xF308) != 0; we read 0 after S-Boot. Android logs show DVFS works there. **This is the
  blocker.** 0397 (read-only dump of 0xF2E0-0xF31F + mailbox status) is flashed, its log not read yet.
- Never: follow pointers in APM SRAM (froze, 0394), send test commands on other ACPM channels, read Linux debugfs
  clk/exynos-pd on the phone (froze TWRP twice).
- After DVFS: GPU power-on, firmware, page tables, rings; userland = Mesa RADV (a community RADV already runs on
  this GPU over Samsung's sgpu on Android: github.com/JimVulkan/radv-xclipse) behind an sgpu-compatible XNU service,
  then the existing sweetgl/sweetmtl Metal layers.

## 9. Pitfalls (each cost real time)

1. **Reading a powered-off block freezes the whole SoC with no log.** Before any domain is on, touch only PMU_ALIVE
   and the CMUs Samsung writes first. Print `step:` + `sweet_log_kick()` before risky accesses.
2. **adb streams over USB 2 lost bytes** (data shifted by 128 B): use USB 3, verify every write by sha256.
3. A frozen boot leaves the **previous** boot's log on disk: check which boot a log is from.
4. Clock IDs: compute `CLK_<BLOCK>_BASE + n` from include/dt-bindings/clock/s5e9925.h, don't count by eye.
5. Mac mini builds lay the kernel out differently: find SCTLR's `movk #0x34d4` by pattern, not by offset.
6. Shell: zsh does not word-split `$var` lists; use arrays or `bash -c`.

## 10. Not done / ideas

- GPU (above). Buttons: PMIC power key / volume via ACPM SPMI. Battery/charger (S2MPS + PD). USB (DWC3 + Exynos PHY)
  for USB networking like the Tab S4. Brightness: DSI DCS through DECON. Wi-Fi: BCM4389 on PCIe. Audio: ABOX.
- iOS 26 (Liquid Glass): xnu-12377 is public; the CPU has PAC; would need the patch stack rebased, generic PAC, an
  APFS/cryptex root and the GPU.
- More RAM: >9 GiB needs multi-range RAM support in XNU.
