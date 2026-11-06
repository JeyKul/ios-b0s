/*
 * Galaxy S22 Ultra (b0s, Exynos 2200 / s5e9925) shim, stage 1: read-only diagnostic
 * on the S-Boot framebuffer (s3cfb.bootloaderfb=0xf6200000). Writes only: framebuffer,
 * DECON HW_SW_TRIG_CONTROL (as uniLoader g0s, so fb writes reach the panel) and the
 * watchdog off. Text for 90 s, then black (OLED). Forced reset: Vol Down + Power, 7 s.
 */
#include <stdint.h>
#include "console.h"
#include "loader.h"
#include "fdt.h"

#define DECON0_BASE		0x19f00000UL	/* drmdecon@19f00000 */
#define HW_SW_TRIG_CONTROL	0x30
#define WDT_BASE		0x10050000UL
#define WDT_WTCON		(WDT_BASE + 0x0)
#define WDT_WTCLRINT		(WDT_BASE + 0xc)
#define CL0_NONCPU_INT_EN	(0x15860000UL + 0x1244)
#define nWDTRESET_EN		(1u << 2)

#define rsr(r) ({ uint64_t _v; __asm__ volatile("mrs %0, " r : "=r"(_v)); _v; })

static inline uint32_t rd32(uintptr_t a) { return *(volatile uint32_t *)a; }
static inline void wr32(uintptr_t a, uint32_t v) { *(volatile uint32_t *)a = v; }

/*
 * OLED burn-in guard: the text stays SHOW_S seconds (time for a photo), then the
 * screen goes black and only a 6x6 dot wanders across it, so no pixel stays lit.
 */
#define SHOW_S	90

static void dot(int x, int y, uint32_t c)
{
	for (int j = 0; j < 6; j++)
		for (int i = 0; i < 6; i++)
			con_fb[(y + j) * FB_W + x + i] = c;
}

static void blank(void)
{
	for (uint32_t i = 0; i < FB_W * FB_H; i++)
		con_fb[i] = 0xff000000U;
}

static void __attribute__((noreturn)) hang(void)
{
	uint64_t f = rsr("cntfrq_el0"), t0 = rsr("cntpct_el0");
	for (uint64_t n = 0; rsr("cntpct_el0") - t0 < SHOW_S * f; n++) {
		con_hex("BEAT", n);
		con_up();
		for (volatile uint32_t i = 0; i < 20000000; i++)
			;
	}
	blank();
	for (uint32_t n = 0;; n++) {
		int x = (n * 37) % (FB_W - 6), y = (n * 101) % (FB_H - 6);
		dot(x, y, 0xff303030U);
		for (volatile uint32_t i = 0; i < 50000000; i++)
			;
		dot(x, y, 0xff000000U);
	}
}

static uint32_t probe(const char *label, const char *name, uintptr_t a)
{
	con_hex2(label, name, (uint32_t)a, "..", 0);
	con_up();
	__asm__ volatile("dsb sy" ::: "memory");
	uint32_t v = rd32(a);
	con_hex2(label, name, (uint32_t)a, "=", v);
	return v;
}

static void kill_dog(void)
{
	wr32(CL0_NONCPU_INT_EN, rd32(CL0_NONCPU_INT_EN) & ~nWDTRESET_EN);
	wr32(WDT_WTCON, rd32(WDT_WTCON) & ~((1u << 5) | (1u << 0)));
	wr32(WDT_WTCLRINT, 1);
	__asm__ volatile("dsb sy" ::: "memory");
}

/* Colour bars across the top: a wrong FB_W shows as slanted bars. */
static void bars(void)
{
	static const uint32_t c[] = { RED, GREEN, 0xff000060U, WHITE };
	for (int y = 0; y < 64; y++)
		for (int x = 0; x < FB_W; x++)
			con_fb[y * FB_W + x] = c[x * 4 / FB_W];
}

void shim_main(uint64_t dtb)
{
	wr32(DECON0_BASE + HW_SW_TRIG_CONTROL, 0x1281);
	con_set_scale(3);
	con_clear(BLUE);
	bars();
	con_skip(); con_skip();
	con_str("SHIM", "b0s stage 1 diag");
	uint64_t el = rsr("CurrentEL") >> 2;
	con_hex("EL", el);
	con_hex("DTB", dtb);
	con_hex("PC", (uint64_t)__builtin_return_address(0));
	con_hex("MIDR", rsr("midr_el1"));
	con_hex("MPIDR", rsr("mpidr_el1"));
	con_hex("MMFR0", rsr("id_aa64mmfr0_el1"));
	con_hex("MMFR1", rsr("id_aa64mmfr1_el1"));
	con_hex("PFR0", rsr("id_aa64pfr0_el1"));
	con_hex("ISAR0", rsr("id_aa64isar0_el1"));
	con_hex("ISAR1", rsr("id_aa64isar1_el1"));	/* PAC: APA/API/GPA */
	con_hex("SCTLR1", rsr("sctlr_el1"));
	if (el == 2) {
		con_hex("HCR", rsr("hcr_el2"));
		con_hex("SCTLR2", rsr("sctlr_el2"));
	}
	con_hex("CNTFRQ", rsr("cntfrq_el0"));
	con_hex("CNT", rsr("cntpct_el0"));
	uint64_t a, b;
	if (fdt_chosen_u64(dtb, "linux,initrd-start", &a) && fdt_chosen_u64(dtb, "linux,initrd-end", &b))
		con_hex2("INITRD", "lo=", a, "hi=", b);
	con_skip();
	probe("WDT", "con", WDT_WTCON);
	kill_dog();
	probe("WDT", "con", WDT_WTCON);
	for (int i = 0; i < 0x40; i += 4)
		probe("DECON", "r", DECON0_BASE + i);
#ifdef STAGE2
	con_str("LOADER", loader_boot(dtb));
#endif
	con_str("DONE", "VolDown+Power 7s to reset");
	hang();
}

void secondary_main(uint64_t idx)
{
	(void)idx;
}

void exc_handler(uint64_t slot, uint64_t *regs)
{
	(void)regs;
	con_skip();
	con_hex("EXC", slot);
	con_hex("ESR", rsr("esr_el1"));
	con_hex("FAR", rsr("far_el1"));
	con_hex("ELR", rsr("elr_el1"));
	hang();
}
