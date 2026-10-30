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
