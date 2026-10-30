/*
 * Galaxy Tab S4 (gts4llte, MSM8998) shim, stage 1: read-only diagnostic. Paints
 * on the S-Boot splash framebuffer before each register access, so the last line
 * on screen names whatever hung. Only writes: framebuffer, MDP frame kick, WDT
 * disable. No storage access. Stays up until a forced reset (Vol Down + Power).
 * Addresses: the tablet's live device tree, compared with docs/taimen-porting-map.md.
 */
#include <stdint.h>
#include "console.h"
#include "loader.h"

#define PSCI_SYSTEM_RESET	0x84000009UL
#define PS_HOLD		0x10ac000UL	/* restart@10ac000 */
#define APSS_WDT	0x17817000UL	/* qcom,wdt@17817000 */
#define WDT_EN		0x08
#define WDT_BARK	0x10
#define WDT_BITE	0x14
#define GCC_BASE	0x100000UL
#define MMSS_CC_BASE	0xc8c0000UL
#define MDSS_BASE	0xc900000UL
#define CTL0		(MDSS_BASE + 0x2000)
#define VIG0		(MDSS_BASE + 0x5000)
#define LM0		(MDSS_BASE + 0x45000)
#define INTF1		(MDSS_BASE + 0x6b800)
#define INTF2		(MDSS_BASE + 0x6c000)
#define PP0		(MDSS_BASE + 0x71000)

#define rsr(r) ({ uint64_t _v; __asm__ volatile("mrs %0, " r : "=r"(_v)); _v; })

static inline uint32_t rd32(uintptr_t a) { return *(volatile uint32_t *)a; }
static inline void wr32(uintptr_t a, uint32_t v) { *(volatile uint32_t *)a = v; }

/* Keep repainting a counter: if it moves, the CPU is alive and frame kicks still work. */
static void __attribute__((noreturn)) hang(void)
{
	for (uint64_t n = 0;; n++) {
		con_hex("BEAT", n);
		con_flush();
		con_up();
	}
}

/* Announce the access, do it, then show the value: a hang leaves "name .." as the last line. */
static uint32_t probe(const char *label, const char *name, uintptr_t a)
{
	con_hex2(label, name, (uint32_t)a, "..", 0);
	con_flush();
	con_up();
	__asm__ volatile("dsb sy" ::: "memory");
	uint32_t v = rd32(a);
	con_hex2(label, name, (uint32_t)a, "=", v);
	con_flush();
	return v;
}

#ifdef ALIVE
/* Proof of life that does not depend on the display: wait, then reboot the tablet. */
static void __attribute__((noreturn)) alive_reset(void)
{
	for (volatile uint32_t i = 0; i < 100000000; i++)
		;
	register uint64_t x0 __asm__("x0") = PSCI_SYSTEM_RESET;
	__asm__ volatile("dsb sy; smc #0" : "+r"(x0) : : "memory");
	/* PSCI did nothing on taimen from EL1; PS_HOLD low always resets. */
	wr32(PS_HOLD, 0);
	__asm__ volatile("dsb sy" ::: "memory");
	for (;;)
		__asm__ volatile("wfe");
}
#endif

void shim_main(uint64_t dtb)
{
	con_set_scale(2);
	con_clear(BLUE);
	con_str("SHIM", "gts4l stage 1 diag");
	con_hex("DTB", dtb);
	con_hex("MMFR0", rsr("id_aa64mmfr0_el1"));
	con_hex("ISAR0", rsr("id_aa64isar0_el1"));
	con_hex("MIDR", rsr("midr_el1"));
	con_hex("EL", rsr("CurrentEL"));
	con_hex("SCTLR", rsr("sctlr_el1"));
	con_hex("CNTFRQ", rsr("cntfrq_el0"));
	con_flush();
#ifdef STAGE2
	/* Act as iBoot: the SWEETXNU payload is the boot image's ramdisk. */
	const char *why = loader_boot(dtb);
	con_str("LOADER", why);
	hang();
#endif
	/* Do repeated kicks reach the panel at all? No register reads yet. */
	con_str("KICK", "1");
	con_flush();
	con_str("KICK", "2");
	con_flush();
	con_str("KICK", "3");
	con_flush();
	/* The generic timer: does it read, and does it count? */
	con_str("CNTPCT", "reading ..");
	con_flush();
	con_hex("CNT0", rsr("cntpct_el0"));
	con_flush();
	con_hex("CNT1", rsr("cntpct_el0"));
	con_flush();
	/* The kernel log needs the WDT off long enough to read the screen. */
	probe("WDT", "en", APSS_WDT + WDT_EN);
	probe("WDT", "bark", APSS_WDT + WDT_BARK);
	probe("WDT", "bite", APSS_WDT + WDT_BITE);
	wr32(APSS_WDT + WDT_EN, 0);
	__asm__ volatile("dsb sy" ::: "memory");
	probe("WDT", "en", APSS_WDT + WDT_EN);
	con_skip();
	probe("GCC", "mmss_axi", GCC_BASE + 0x9000);
	probe("GCC", "mmss_ahb", GCC_BASE + 0x9004);
	probe("MMSS", "mdss_ahb", MMSS_CC_BASE + 0x2308);
	probe("MMSS", "mdss_axi", MMSS_CC_BASE + 0x2310);
	probe("MMSS", "mdss_mdp", MMSS_CC_BASE + 0x231c);
	probe("MDSS", "hw_ver", MDSS_BASE + 0x0);
	probe("MDP", "hw_ver", MDSS_BASE + 0x1000);
	probe("MDP", "intr_st", MDSS_BASE + 0x1014);
	probe("VIG0", "src_size", VIG0 + 0x00);
	probe("VIG0", "src0_addr", VIG0 + 0x14);
	probe("VIG0", "ystride0", VIG0 + 0x24);
	probe("VIG0", "src_fmt", VIG0 + 0x30);
	probe("VIG1", "src0_addr", MDSS_BASE + 0x7000 + 0x14);
	probe("DMA0", "src0_addr", MDSS_BASE + 0x25000 + 0x14);
	probe("DMA1", "src0_addr", MDSS_BASE + 0x27000 + 0x14);
	probe("CTL0", "layer0", CTL0 + 0x0);
	probe("CTL0", "top", CTL0 + 0x14);
	probe("CTL0", "flush", CTL0 + 0x18);
	probe("CTL1", "layer0", CTL0 + 0x200 + 0x0);
	probe("CTL1", "top", CTL0 + 0x200 + 0x14);
	probe("LM0", "out_size", LM0 + 0x0);
	probe("INTF1", "timing_en", INTF1 + 0x0);
	probe("INTF2", "timing_en", INTF2 + 0x0);
	probe("PP0", "tear_en", PP0 + 0x0);
	probe("PP0", "autorefr", PP0 + 0x30);
	probe("PP1", "autorefr", PP0 + 0x800 + 0x30);
	con_skip();
#ifdef ALIVE
	con_str("DONE", "rebooting after a wait");
	alive_reset();
#else
	con_str("DONE", "hold VolDown+Power to reset");
	con_flush();
	hang();
#endif
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
	con_flush();
	hang();
}
