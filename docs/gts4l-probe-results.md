# Tab S4 (gts4llte) probe results, 2026-10-30
Read from photo_2026-10-30_20-34-11.jpg. Build: -DALIVE -DNOKICK. Each register value on screen has a
stale trailing 0 (leftover from the ".." line), removed below. Photo is slightly blurred.

CPU: EL1, MMU/caches off (SCTLR 0xC00800), MMFR0 0x1122 (no 16K), ISAR0 0x11120 (no LSE),
MIDR 0x51AF8014, CNTFRQ 19.2 MHz, DTB in x0 = 0x84800000
CNT0 0xBFEF3B6, CNT1 0xD9BC811: 1.41 s per painted line + counted 2M spin (caches off)
WDT: en 0 at entry, bark 0x7FFFF, bite 0xFFFFF
Clocks: mmss_axi/mmss_ahb/mdss_ahb 0x20008001, mdss_axi/mdss_mdp 0x00004221
MDSS hw 0x30000001, MDP hw 0x30000000, intr_st 0x04333300
VIG0: src_size 0x06400500 (1280x1600), src0 0x9D400000, ystride 0x2800, fmt 0x000237FF
VIG1: src0 0x9D400000. DMA0/1 src0: 0
CTL0: layer0 0x01000005, top 0x00020020, flush 0x60000000. CTL1: layer0 0, top 0x00020030
LM0 out_size 0. INTF1/INTF2 timing_en 1
PP0: tear_en 1, autorefresh 0x80000002. PP1: autorefresh 0x00000002
