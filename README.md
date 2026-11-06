# ios-b0s

**Apple's iOS 16 userland on Apple's open-source XNU kernel, on a Samsung Galaxy S22 Ultra (SM-S908B, `b0s`,
Exynos 2200).**

A bare-metal shim that S-Boot loads as the "kernel" acts as iBoot. A patched XNU (xnu-8796, the iOS 16 generation)
runs on the phone, and the **unmodified iOS 16.7.16 (20H392) userland of the iPhone X** runs on top of it, from an
HFS+ root on the userdata partition. Nothing in the boot chain is replaced. Android comes back by flashing the stock
`boot.img`.

> Experimental. iOS boots to the lock screen and home screen with touch, but everything is drawn by the CPU and
> most hardware is not driven yet. The project is paused (2026-11-06).

## Status

| Area | State |
|---|---|
| Boot | S-Boot (unlocked) → shim (`shim/b0s.c`) → XNU at EL1 under Samsung's EL2 hypervisor |
| Kernel | `MACHINE_CONFIG` **B0S**: 16 KiB pages, ARM_LARGE_MEMORY layout, `-mcpu=cortex-a76` |
| CPU | 8/8 cores (PSCI over hvc), PAN handled (SCTLR.SPAN) |
| RAM | 9.03 GiB of 12 (largest contiguous range, 0x8_bc000000–0xa_fe600000) |
| Storage | Exynos UFS (128-byte PRDT/FMP, START STOP UNIT wake), HFS+ root grown over all of userdata |
| Display | S-Boot's framebuffer, 1440x3084, iOS drawn 1:1 by the CPU |
| Touch | STM FTS on SPI2 (`bsd/dev/b0s_touch.c`) |
| GPU (Xclipse 920, RDNA2) | Not working. Rail and power domain come up; blocked on APM's DVFS channel (see the guide) |
| Not started | Wi-Fi, audio, sensors, buttons, battery/charging, USB, brightness, modem |

## Documentation

- **[docs/b0s-guide.md](docs/b0s-guide.md): the complete guide.** Prerequisites, how to go from a stock phone to iOS
  step by step, boot-args, hardware facts, how each subsystem was made to work, the GPU research, pitfalls.
- [docs/b0s-status.md](docs/b0s-status.md): dated state and log of boots, newest first.
- [docs/b0s-gpu-handoff.md](docs/b0s-gpu-handoff.md): ACPM/GPU detail (APM SRAM tables, channel 5, plugins).
- [CLAUDE.md](CLAUDE.md): the working rules (hang safety, one change per boot, logging).
- Earlier ports this tree comes from: [docs/README-taimen.md](docs/README-taimen.md) (Pixel 2 XL),
  [docs/gts4l-status.md](docs/gts4l-status.md) (Galaxy Tab S4).

## Quick start

You need an unlocked S22 Ultra (Exynos) with TWRP, a Linux PC, USB 3, the iPhone X 16.7.16 IPSW and a GitHub
account (kernels build on Actions). Full commands are in [the guide](docs/b0s-guide.md#4-from-a-stock-phone-to-ios-step-by-step).

1. Back up the phone: `tools/b0s-backup.sh BACKUPDIR` (TWRP; keep it private, it holds the IMEI).
2. Build the iOS root (`tools/mkiosroot.sh`), the rootfix blob (`tools/taimen/mkrootfix.py`) and extract `d22ap.adt`.
3. Write the root to userdata: `tools/b0s-write-root.sh ROOT.img WORKDIR $(seq 0 15)` (erases Android's data).
4. Build the kernel: push to `b0s`, or run the `xnu-mini` / `xnu` workflow with `machine_config=B0S`.
5. Make the boot image with `tools/b0s-boot-xnu.sh` and write it to `boot` from TWRP. First boot with `b0sgrowroot=1`.

## Repository layout (b0s)

| Path | Contents |
|---|---|
| `shim/b0s.c`, `shim/loader.c`, `shim/build-b0s.sh` | The shim: stage-1 diagnostics, payload loader |
| `tools/b0s-boot-xnu.sh`, `tools/mkadt.py --b0s` | Boot image and Apple Device Tree for b0s (`B0S_BASE`, `B0S_MEM`) |
| `tools/b0s-backup.sh`, `tools/b0s-write-root.sh` | Verified backup and root writing over adb |
| `tools/sweet-log.py` | Decodes the kernel log kept at the end of userdata |
| `xnu/patches/0370-0397` | b0s kernel changes (board isolation, UFS, PSCI, PAN, touch, B0S config, root grow, GPU probes) |
| `.github/workflows/xnu-mini.yml`, `xnu.yml` | Kernel builds (self-hosted Mac mini on push; GitHub runner on demand) |

## Credits

- [SourceQuality/ios-taimen](https://github.com/SourceQuality/ios-taimen): the Pixel 2 XL port this tree is forked
  from, via the Galaxy Tab S4 branch (`gts4l`).
- [joranv1/ios-sailfish](https://github.com/joranv1/ios-sailfish) and [dipa-studio/ios-sweet](https://github.com/dipa-studio/ios-sweet):
  the Pixel 1 and Redmi Note 10 Pro ports underneath it, including the 16 KiB SWEET layout B0S reuses.
- [NWylynko/ios-flame](https://github.com/NWylynko/ios-flame), [Floppikrob/ios-galaxy-s8](https://github.com/Floppikrob/ios-galaxy-s8),
  [orhunnx/ios-a5xelte](https://github.com/orhunnx/ios-a5xelte): shim UFS/log code and the 4 KiB XNU work.
- [ExtremeXT/android_kernel_samsung_s5e9925](https://github.com/ExtremeXT/android_kernel_samsung_s5e9925): Samsung's
  Exynos 2200 kernel source, the reference for every register value here.
- [ivoszbg/uniLoader](https://github.com/ivoszbg/uniLoader): the S22+ (g0s) board code (framebuffer, ACPM SPMI).
- Apple's open-source releases (xnu, IOHIDFamily, hfs), [blacktop/darwin-xnu-build](https://github.com/blacktop/darwin-xnu-build),
  [blacktop/ipsw](https://github.com/blacktop/ipsw).

iOS belongs to Apple and is not distributed here: download the IPSW yourself.

## License

The XNU patches are derived from XNU and fall under the [APSL 2.0](https://opensource.apple.com/apsl/). The shim and
tools follow the upstream repositories' licenses (MIT).
