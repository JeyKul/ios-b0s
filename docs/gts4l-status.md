# Galaxy Tab S4 (SM-T835) status

iOS 16.7.16 (iPhone X root) reaches the lock screen and home screen on the taimen kernel with patches 0358-0359.

Build the boot image:

    ROOTFIX=rootfix.bin TAIMEN_MEM=0x7e400000 tools/gts4l-boot-xnu.sh KERNEL DTB_TAIL \
      rd=ufsroot0 ufsrw=1 cs_enforcement_disable=1 unentitled_ios_sim_launch=1 ignition_level=1 \
      launchd_unsecure_cache=1 wdt_test=no_user_checkin sweetprof=- sweetsample=- sweetfb_noconsole=1 \
      gts4ltouch=2 gts4lrot=3 cpus=8 sweetperf=1 sweetkeys=1 gts4lkeys=1

`rootfix.bin` is `tools/taimen/mkrootfix.py ios16-root.img rootfix.bin`: migration and Setup skipped.

Measured:
- 8/8 CPUs up. Bootloader clocks: silver 1555 MHz, gold 1132 MHz (not on GPLL0, so `taimengold` refuses).
- S10 and S13 at 828 mV, OSM off.
- Rotated present: 41 ms per frame in rows of single pixels, 18 ms in blocks; best 10 s window 35.9 swaps/s.
- Touch works, gts4lflip=0 with gts4lrot=3. Keys: power, volume up (PM8998 GPIO 6), volume down as Home.

Not done: GPU (CPU renderer only), brightness, faster gold clocks, Wi-Fi, audio, sensors, battery, iPadOS.

GPU, first boot (patch 0359 kernel): `a540_zap` accepted by TrustZone, CP works, Mesa drives CoreAnimation as FD540.
S1 read back 0x0407 after a write of 775 mV (0x0307); the high byte matches PON's 0x04 from the unlocked key poll, so
this looks like a crossed read, not a real 1031 mV. The clock set stopped there and gfx3d stayed where it was. Patch 0360
locks the poll and adds `gts4lgpuclk=MHZ`.

GPU at 342 MHz with patch 0360: S1 reads back 775 mV, gfx3d measured 341 MHz, full-screen quad 17 ms.

Metal: `mtltest` 0 FAIL lines (device through Metal.framework, no shaders). `mtlbridgetest` result 0: compute bad 0/1000,
render bad 0/64, import bad 0/4096; QuartzCore's library 3 kernels ok, 4 failed, 13 pipeline pairs ok, 8 failed.

RAM: TAIMEN_BASE=0xb1000000 TAIMEN_MEM=0xbd400000 boots (max_mem 0xbd400000). S-Boot loads the ramdisk at 0xb0000000, so the shim moves the payload to 0xa1000000 first. The range includes Android's secure camera area (0xfd600000-0xffffffff); not yet shown to be touched under load.

USB: S-Boot leaves the DWC3 powered and clocked with the USB3 PHY in reset. sweetusb=2, at SuperSpeed and with sweetusbss=0, times out on the first endpoint command. Patch 0362 adds gts4lusbutmi=1, the high-speed-only clock switch from Samsung's dwc3-msm.c.

Gold: S-Boot leaves gold on its PLL at 1132.8 MHz (GFMUX cfg 0x300). Patch 0361 adds gts4lgold=1. The chip's open-loop fuses differ from the Pixel's, so `taimenosm` refuses and its tables must not be reused.

Gold at 1728 MHz (gts4lgold=1 taimengold=1728 taimenamp=1): every step's self-test passed on cpu4-7, no panic in the first short run, hottest sensor 58.7 C.

Apps: the 32 stock apps installed from `mkrootfix --apps-archive` on the `cache` partition (`--apps-part cache`), with the LaunchServices database removed once. They appear on the home screen; later boots must not remove the database again.

USB: patches 0363-0364 (QUSB2 rails, clamp, init sequence, reset) with the UTMI switch: the core runs, link state goes to suspend, the host sees nothing. taimen connected at high speed with the USB3 PHY running (boot 133), so 0365 adds gts4lusbss=1 to reset and start the USB3 PHY as Samsung's driver does.

USB networking works (0366): redriver set up as TWRP does, USB3 lane from GPIO 38. Boot args `sweetusb=2 gts4lusbss=1` (no gts4lusbutmi or sweetusbss). The PC sees "Sweet iOS (USB Ethernet)" through cdc_ncm at 5000M, tablet at 192.168.7.2, ping about 1 ms. lockdownd's port 62078 answers.

S Pen (0367): gts4lpen=1 works as a finger; needs gts4lpenflip=2 (top and bottom were mirrored). Screen off works: black memory on an AMOLED when iOS sleeps the display. Fingers unaffected.

iPadOS 16.6.1 (iPad 6, 20G81) boots to the home screen: GTS4L_IPAD=1, gts4lipad=1, gts4lrot=3, mkrootfix --ipad. sweetgl's QuartzCore patches install (dsc-ipad6), mtltest passes, migration runs once after fairplayd.A2 is removed. gts4lipad=2 (1600x2560) also boots.

Open: home screen widgets and icons overlap at both resolutions. The dock is correct. artwork-device-subtype 2048 did not fix it; next, read the MobileGestalt cache and IconState from a copy of the partition. Built-in apps are not yet installed on iPadOS. The iPhone stylus test did nothing; retry gts4lpen=2 on iPadOS. Brightness, audio, sensors and Wi-Fi are not started.
