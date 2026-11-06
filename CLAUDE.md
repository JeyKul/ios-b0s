# CLAUDE.md: ios-b0s

iOS 16 userland + open-source XNU on the Galaxy S22 Ultra (b0s, Exynos 2200, Xclipse 920). The owner tests on the phone; you write code and read logs.

## How to talk (strict)
- **Max 5 lines per reply** unless asked for detail. Format: what changed · what to test · what result proves/disproves it.
- No narration ("Let me…", "Now I'll…"), no recaps of earlier steps, no restating the owner's message.
- Ask the owner only what you can't get yourself, as **one concrete question with the expected answer** (e.g. "Did it stop on the text `b0s: no MSM8998 drivers`? yes/no").
- Don't explain a theory at length before testing it. One line of hypothesis, then the test.

## How to work (token discipline)
- Read with `grep -n` / `sed -n A,Bp`, never whole large files. Never `cat` logs: `grep` for `B0S|panic|step:|timeout` first.
- Don't re-read files already read this session. Don't re-run builds to "check" — CI tells you.
- One change per boot. Each change must log a line that proves it ran (`B0S <AREA>: …`).
- State goes in `docs/b0s-status.md` (short, dated, newest first), not in chat. Fresh sessions start by reading its top 40 lines.

## Hardware rules (bricking/hang safety)
- **Never read or write a block whose power domain may be off** — on Exynos it stalls the bus and freezes the SoC with no log. Before a power domain is on, only touch always-on blocks (PMU_ALIVE, the CMUs Samsung's sequence writes first).
- Before any risky step: print a `step:` marker and `sweet_log_kick()` so a hang shows where it stopped.
- New hardware code is **off by default** behind a boot-arg (`b0s<area>=1`), and every value carries its source (Samsung file + symbol).
- Never write EFS/sec_efs/modem/persist/bootloader partitions. `rd=ufsroot0` runs the Exynos-ported UFS driver (0371/0373). No MSM8998/Qualcomm-only paths on b0s.
- Power sequences: replay Samsung's cal-if tables exactly; on failure, run the matching off-sequence.

## Build / boot
- Kernel: push to `b0s` (xnu/patches/**) → xnu-mini (Mac mini, Xcode 27); xnu (GitHub, Xcode 16) is dispatch-only, as a fallback. Patches are numbered; next number = highest + 1.
- Boot image: `tools/b0s-boot-xnu.sh KERNEL [boot-args]` → `shim/out/b0s-xnu.img`.
- Log: the UFS log channel; pull it after the boot and **check which boot it belongs to** (kernel patch number / timestamp) before drawing conclusions.

## Map
- `xnu/patches/NNNN-*.patch` — all kernel changes; b0s drivers in `bsd/dev/b0s_*.c`, hooked from `sweet_ufs.c`.
- `shim/` — the boot shim (stage 2) and its UFS log. `tools/` — image/ADT/payload builders.
- `docs/b0s-status.md` — current state and next steps.
