/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* (V10.6v) SWARM (eng_input.c ain_swarm) with fake advertisements (bt_air_tap, as the radio's interrupt calls it):
 * CHAOS 0 with the FM-1's own voice bit for bit as V10.6u (SW_GOLD), OCTAVE's scale walk in the track's scale and round,
 * a new device's re-randomize glide, CHAOS 127 bounded (no overflow), the block's cost against CHAOS 0.
 *   cc -O1 -w -Wno-implicit-function-declaration -Ibuild/gen -Ifirmware/src tests/swarm_test.c -lm -o build/swarm
 * SWARM_WAV=dir: demos (3 -> 6 devices arriving at CHAOS 0 / 64 / 127, the scale walk) as WAVs into dir */
#define main hostsim_main
#define HOST_OWN_TICKS 1
static unsigned host_clock;
#define fm1_ticks() (host_clock)
#include "hostsim.c"
#undef main

static int bad;
static void check(const char *what, int ok)
{
    printf("swarm: %-78s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}
static void pkt(uint32_t dev, uint32_t lv, uint32_t ch)
{
    uint8_t p[30];
    uint32_t i;
    p[0] = 0x00; p[1] = 28;
    for (i = 2; i < 30u; i++) p[i] = (uint8_t)(dev * 41u + i * (dev + 3u) * 17u);
    bt_air_tap(p, 30, lv | lv << 8, ch);
}
static void sw_fresh(uint32_t chaos)
{
    host_tracks_init();
    host_preset(&trk[1], ENGI_INPUT, 9u);                /* SWARM */
    trk[1].p[P_E7] = (int16_t)chaos;
    memset(sw, 0, sizeof sw);
    sw_lmin = 127; sw_lmax = 0; sw_prev = 0; sw_tick = 0; sw_rnd = 0x9E3779B9u; sw_key = 0;
#ifdef SW_SELF
    memset(sw_ko, 0, sizeof sw_ko); memset(sw_kt, 0, sizeof sw_kt);
#endif
    ain_tr = 1; ain_sw = 1; host_clock = 0;
}
static uint32_t fnv(uint32_t h, int32_t v) { return (h ^ (uint32_t)v) * 16777619u; }
/* blocks of SWARM; devs: packets of that many devices at their own intervals (0: the FM-1's own voice only); from[i]:
 * the block device i first speaks at (0: all from the start) */
static const uint32_t *run_from;
static uint32_t run(uint32_t blocks, uint32_t devs, FILE *w, double *rms, int32_t *peak)
{
    uint32_t b, i, h = 2166136261u;
    double e = 0;
    int32_t pk = 0;
    for (b = 0; b < blocks; b++) {
        host_clock += CTL * 24000000u / 44100u;
        for (i = 0; i < devs; i++)
            if (b % (60u + 23u * i) == i * 5u && (!run_from || b >= run_from[i])) pkt(i + 1u, 10u + 9u * (i % 4u), 37u + i % 3u);
        ain_swarm(CTL);
        for (i = 0; i < CTL; i++) {
            int32_t l = sw_on ? sw_l[i] : 0, r = sw_on ? sw_r[i] : 0;
            h = fnv(fnv(h, l), r);
            e += (double)l * l + (double)r * r;
            pk = l > pk ? l : -l > pk ? -l : r > pk ? r : -r > pk ? -r : pk;
            if (w) { int16_t o[2] = {(int16_t)clamp(l >> 1, -32767, 32767), (int16_t)clamp(r >> 1, -32767, 32767)}; fwrite(o, 2, 2, w); }
        }
    }
    if (rms) *rms = sqrt(e / (2.0 * blocks * CTL));
    if (peak) *peak = pk;
    return h;
}
static struct sw_dev *dev_of(uint32_t dev)              /* the voice of pkt's device dev */
{
    uint8_t p[30];
    uint32_t i, id;
    p[0] = 0x00;
    for (i = 2; i < 8u; i++) p[i] = (uint8_t)(dev * 41u + i * (dev + 3u) * 17u);
    id = (((uint32_t)p[2] | (uint32_t)p[3] << 8 | (uint32_t)p[4] << 16 | (uint32_t)p[5] << 24) ^ ((uint32_t)p[6] << 5 | (uint32_t)p[7] << 13)) | 1u;
    for (i = 0; i < SW_N; i++) if (sw[i].id == id) return &sw[i];
    return 0;
}
static void blocks(uint32_t n) { while (n--) { host_clock += CTL * 24000000u / 44100u; ain_swarm(CTL); } }
static void tests(void)
{
    uint32_t i, k, h0;
    {   /* CHAOS 0, the FM-1's own voice: bit for bit V10.6u (its hash then, tests/swarm_test.c on 1647422) */
        sw_fresh(0);
        h0 = run(3000, 0, 0, 0, 0);
        check("CHAOS 0 (and OCTAVE 0), no device around: the FM-1's own voice bit for bit as V10.6u", h0 == 0x734fbf03u);
    }
    {   /* OCTAVE's second half: each hit the next degree of the scale (D minor), over 2..8 degrees, round */
        static const uint8_t OC[3] = {64, 80, 127}, RN[3] = {2, 3, 8};
        int ok = 1;
        for (k = 0; k < 3u; k++) {
            uint8_t nt[20];
            struct sw_dev *d;
            sw_fresh(0);
            trk[1].p[P_ROOT] = 2; trk[1].p[P_SCALE] = 2; trk[1].p[P_E6] = OC[k];
            for (i = 0; i < 20u; i++) { pkt(1, 30, 38); blocks(1); d = dev_of(1); nt[i] = d ? d->nt : 0; }
            for (i = 0; i < 20u; i++) {
                ok &= scale_snap(&trk[1], nt[i]) == nt[i] && nt[i] >= d->note && nt[i] <= d->note + 14u;
                if (i + RN[k] < 20u) ok &= nt[i + RN[k]] == nt[i];
                if (i && (i + 1u) % RN[k]) ok &= nt[i] > nt[i - 1u];
                if (!((i + 1u) % RN[k])) ok &= nt[i] == d->note;   /* (round: its own note again) */
            }
            if (k == 2u) { printf("swarm:   OCTAVE 127 (8 degrees) by hit:"); for (i = 0; i < 12u; i++) printf(" %u", nt[i]); printf("\n"); }
        }
        trk[1].p[P_E6] = 40;                            /* (the first half: octaves, 0..3 more: 40 / 16 = 2, 3 places) */
        {
            uint8_t o[6];
            for (i = 0; i < 6u; i++) { pkt(1, 30, 38); blocks(1); o[i] = dev_of(1)->oct; }
            ok &= o[1] == (o[0] + 1u) % 3u && o[2] == (o[1] + 1u) % 3u && o[3] == o[0];
        }
        check("OCTAVE's second half: a walk of the track's scale (2, 3, 8 degrees) on each hit, in the scale, round; its first half octaves", ok);
    }
    {   /* a new device: the knobs glide to new places (~1.5 s), the old one's packets do not move them */
        int16_t kt[4], k1[4];
        int ok, moved = 0, far = 0;
        sw_fresh(0);
        pkt(1, 30, 38); blocks(1);
        memcpy(kt, sw_kt, sizeof kt); memcpy(k1, sw_ko, sizeof k1);
        for (i = 0; i < 4u; i++) { moved |= kt[i] != 0; far |= abs(k1[i]) > abs(kt[i]) / 8 + 256; }
        ok = moved && !far;
        blocks(1030);                                   /* (~1.5 s) */
        for (i = 0; i < 4u; i++) ok &= abs(sw_ko[i] - kt[i]) <= abs(kt[i]) * 3 / 10 + 512 && abs(sw_ko[i]) >= abs(kt[i]) / 2 - 256;
        for (i = 0; i < 20u; i++) { pkt(1, 30, 38); blocks(20); }
        ok &= !memcmp(kt, sw_kt, sizeof kt);
        pkt(2, 20, 37); blocks(1);
        ok &= memcmp(kt, sw_kt, sizeof kt) != 0;
        printf("swarm:   knob offsets after a new device: PLUCK %d FM %d IMAGE %d DRIFT %d (of the knob's 127)\n",
               kt[0] / 256, kt[1] / 256, kt[2] / 256, kt[3] / 256);
        check("a new device: PLUCK FM IMAGE DRIFT glide to new places over ~1.5 s (not a jump); a known one's packets do not", ok);
    }
    {   /* each device its own timbre; its family from its advertisement (Apple, Microsoft, appearance) */
        static const uint8_t APL[16] = {0, 14, 1, 2, 3, 4, 5, 6, 2, 1, 6, 4, 0xFF, 0x4C, 0x00, 0x10};
        static const uint8_t MS[13] = {0, 11, 1, 2, 3, 4, 5, 6, 4, 0xFF, 0x06, 0x00, 0x01};
        static const uint8_t WT[12] = {0, 10, 1, 2, 3, 4, 5, 6, 3, 0x19, 0xC1, 0x00};   /* (appearance 0x00C1: a watch) */
        int ok = sw_family(APL, 16) == 1u && sw_family(MS, 13) == 2u && sw_family(WT, 12) == 3u;
        uint8_t z[30] = {0, 28, 9, 9, 9, 9, 9, 9};
        ok &= sw_family(z, 30) == 0u;
        sw_fresh(0);
        for (i = 1; i <= 6u; i++) pkt(i, 30, 38);
        blocks(1);
        {
            uint32_t sh = 0, rt = 0;
            for (i = 1; i <= 6u; i++) { struct sw_dev *d = dev_of(i); ok &= d && d->tn != 255u; sh |= 1u << (d->sh >> 5); rt |= 1u << d->rt; }
            ok &= __builtin_popcount(sh) >= 3 && __builtin_popcount(rt) >= 2;
            for (i = 1; i <= 6u; i++) { struct sw_dev *d = dev_of(i); if (d) printf("swarm:   device %u: shape %u ratio %u depth %u tone %u\n", i, d->sh, d->rt, d->fi, d->tn); }
        }
        printf("swarm:   families %u %u %u %u\n", sw_family(APL, 16), sw_family(MS, 13), sw_family(WT, 12), sw_family(z, 30));
        check("each device its own timbre (shape, FM ratio, depth, tone); a family by manufacturer / appearance", ok);
    }
    {   /* a swell in, a long fade out, a ninth device takes the oldest's voice without a click */
        struct sw_dev *d;
        int32_t a10, a300, a_before, a_after, m = 0;
        int ok;
        sw_fresh(0);
        pkt(1, 30, 38); blocks(10); a10 = dev_of(1)->amp;
        blocks(290); a300 = dev_of(1)->amp;
        ok = a10 < a300 / 2 && a300 > 500;
        blocks(6u * 690u - 300u);                       /* (6 s unheard: it leaves) */
        d = dev_of(1);
        a300 = d->amp;
        blocks(345); ok &= d->amp > a300 * 9 / 20 && d->amp < a300 * 9 / 10;   /* (half a second into its fade: most of it) */
        blocks(16u * 690u); ok &= !dev_of(1);
        sw_fresh(0);
        for (i = 1; i <= 8u; i++) { pkt(i, 30, 38); blocks(200); }
        a_before = 0;
        for (i = 0; i < SW_N; i++) if (sw[i].id == dev_of(1)->id) { k = i; a_before = sw[i].amp; }
        pkt(9, 30, 38); blocks(1);
        a_after = sw[k].amp;
        ok &= !dev_of(1) && dev_of(9) == &sw[k] && a_after > a_before / 2;
        for (i = 0; i < CTL; i++) m = abs(sw_l[i]) > m ? abs(sw_l[i]) : m;
        printf("swarm:   swell %d -> %d; fade %d; ninth %d -> %d\n", a10, a300, d ? d->amp : -1, a_before, a_after);
        check("arrival swells (~0.4 s), departure fades long (~1.5 s), a ninth device takes the oldest's voice, its level kept", ok);
    }
    {   /* CHAOS: 0 / 64 / 127 with six devices: bounded, louder in timbre, never silent, no overflow */
        static const uint8_t C[3] = {0, 64, 127};
        double r[3];
        int32_t pk[3];
        uint32_t hh[3];
        int ok = 1;
        for (k = 0; k < 3u; k++) {
            sw_fresh(C[k]);
            hh[k] = run(3000, 6, 0, &r[k], &pk[k]);
            ok &= r[k] > 500.0 && pk[k] < (1 << 20) && r[k] == r[k];
        }
        printf("swarm:   six devices: CHAOS 0 rms %.0f peak %d, 64 rms %.0f peak %d, 127 rms %.0f peak %d\n",
               r[0], pk[0], r[1], pk[1], r[2], pk[2]);
        ok &= hh[0] != hh[1] && hh[1] != hh[2];
        check("CHAOS 0 / 64 / 127 with six devices: each another sound, bounded (no overflow), never silent", ok);
    }
}
/* (V10.6v) the RAMP demo: an ANALOG bass pattern (1/16), a 4-bar EXP RAMP on CUT from OPP (0) to the step's lock (120) on
 * step 1, 6 bars: the riser, then the knob's 40 */
static void wav_open(FILE *f, uint32_t frames)
{
    uint32_t v;
    fwrite("RIFF", 1, 4, f); v = 36 + frames * 4; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f); v = 1 | 2u << 16; fwrite(&v, 4, 1, f); v = 44100; fwrite(&v, 4, 1, f);
    v = 44100 * 4; fwrite(&v, 4, 1, f); v = 4 | 16u << 16; fwrite(&v, 4, 1, f); fwrite("data", 1, 4, f);
    v = frames * 4; fwrite(&v, 4, 1, f);
}
static void ramp_demo(const char *path)
{
    FILE *f = fopen(path, "wb");
    uint32_t i, b, blocks;
    int32_t o[2 * CTL];
    track_t *t = &trk[0];
    if (!f) return;
    host_tracks_init();
    song.g[G_BPM] = 124;
    host_preset(t, ENGI_ANALOG, 0);
    for (i = 1; i < NTRK; i++) trk[i].p[P_LEVEL] = 0;
    t->p[P_SLEN] = 16; t->p[P_SDIV] = 2; t->p[P_SGATE] = 70;
    for (i = 0; i < 16u; i++) {
        uint8_t n = (uint8_t)(36 + (i % 8u == 3u ? 12 : i % 8u == 6u ? 7 : 0));
        put_step(t, i, 1, &n, ST_NOTE, i % 4u == 0u ? SF_ACCENT : 0u);
    }
    t->p[P_E4] = 40; t->p[P_E5] = 80;
    t->step[0].comp = CO_MAKE(CA_RAMP, CC_ALWAYS); t->step[0].carg = CV_RAMP_BARS | 7u; t->step[0].clen[0] = 3;
    motion_set_lock(t, 0, P_E4, 120);
    transport_req = 1;
    blocks = 6u * 16u * div_samples(2) / CTL;
    wav_open(f, blocks * CTL);
    for (b = 0; b < blocks; b++) {
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++) { int16_t q[2] = {(int16_t)clamp(o[2u * i], -32767, 32767), (int16_t)clamp(o[2u * i + 1u], -32767, 32767)}; fwrite(q, 2, 2, f); }
    }
    transport_req = 2;
    mix_block(o, CTL);
    fclose(f);
    printf("swarm: demo %s\n", path);
}
static void swarm_demo(const char *dir, const char *name, uint32_t chaos, uint32_t oct)
{
    static const uint32_t FROM[6] = {0, 0, 0, 2100, 3400, 4800};   /* (3 devices, then 3 more at ~3 s, 5 s, 7 s) */
    char path[512];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (!(f = fopen(path, "wb"))) return;
    sw_fresh(chaos);
    trk[1].p[P_E6] = (int16_t)oct;
    trk[1].p[P_ROOT] = 2; trk[1].p[P_SCALE] = 2;        /* (D minor) */
    wav_open(f, 9000u * CTL);
    run_from = FROM;
    run(9000, 6, f, 0, 0);
    run_from = 0;
    fclose(f);
    printf("swarm: demo %s\n", path);
}
int main(void)
{
    if (getenv("SWARM_DBG")) { uint32_t b,v; sw_fresh(0); pkt(1,30,38); for(b=0;b<6000;b++){ blocks(1); if(b%250==0){ printf("b%u on%u ",b,sw_on); for(v=0;v<SW_N;v++) if(sw[v].id) printf("[%u %x amp%d acc%d in%u]",v,sw[v].id,sw[v].amp,sw[v].acc,sw[v].in); printf("\n");} } return 0; }
    if (getenv("SWARM_BENCH")) {                         /* (valgrind: ain_swarm's instructions, 6 devices at CHAOS c) */
        sw_fresh((uint32_t)atoi(getenv("SWARM_BENCH")));
        run(2000, 6, 0, 0, 0);
        return 0;
    }
    tests();
    {
        double r; int32_t pk; struct timespec a, z; uint32_t k;
        for (k = 0; k < 2u; k++) {
            double best = 1e18, ns; uint32_t rep;
            for (rep = 0; rep < 5u; rep++) {
                sw_fresh(k ? 127u : 0u); run(500, 6, 0, 0, 0);
                clock_gettime(CLOCK_MONOTONIC, &a);
                run(6000, 6, 0, 0, 0);
                clock_gettime(CLOCK_MONOTONIC, &z);
                ns = ((z.tv_sec - a.tv_sec) * 1e9 + (z.tv_nsec - a.tv_nsec)) / 6000.0;
                best = ns < best ? ns : best;
            }
            printf("swarm: cost %.0f ns a block (6 devices, CHAOS %u)\n", best, k ? 127u : 0u);
        }
    }
    if (getenv("SWARM_WAV")) {
        char path[512];
        snprintf(path, sizeof path, "%s/ramp_analog_cut_4bar_exp_opp.wav", getenv("SWARM_WAV"));
        ramp_demo(path);
        swarm_demo(getenv("SWARM_WAV"), "swarm_3to6_chaos0.wav", 0, 0);
        swarm_demo(getenv("SWARM_WAV"), "swarm_3to6_chaos64.wav", 64, 0);
        swarm_demo(getenv("SWARM_WAV"), "swarm_3to6_chaos127.wav", 127, 0);
        swarm_demo(getenv("SWARM_WAV"), "swarm_3to6_scalewalk.wav", 10, 127);
    }
    printf(bad ? "swarm: %d FAILED\n" : "swarm: all ok\n", bad);
    return bad != 0;
}
