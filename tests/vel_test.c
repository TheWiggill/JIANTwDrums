/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.6z6) Host test of the keys' velocity and pressure (firmware/src/vel.c, vel_diag.c's strike window, vel_kb.h)
 * through the key scan itself (hal/fm1_input.h fm1_input_tick, its debounce and its PA7 / PA8 sampling; hal/fm1_adc.h),
 * on modelled strikes built from the owner's two captures (the medians in docs/VELOCITY_DIAG.es.md "Calibración"): the
 * GPIO and the converter are memory; each modelled key has a pulled-up reading (880 at the touch down to its seat, ~200,
 * over its fall), a driven one (falling from 950 to its strike over the same fall: a sample that catches it still
 * falling reads high), its digital contact (closed under a threshold of the pulled-up reading) and, held, a pressure
 * (the driven reading going further down). Checked:
 *   1. the settings: an older setting (bytes 0) reads VELOCITY NORMAL, VEL MIN 20, AFTERTOUCH MID, VEL LEARN ON, the
 *      sampling on; VELOCITY OFF + AFTERTOUCH OFF: no converter access at all, every press 100 at its debounce (bit for
 *      bit as before); VELOCITY OFF, AFTERTOUCH on: 100, sampled;
 *   2. the map: continuous (no step over 2 between two readings a count apart), monotonic, through HARD 112 / NORMAL
 *      76 / SOFT 36 (MIN 1), intermediate readings intermediate velocities (>= 60 values), half the slope past the ends,
 *      1..127; SOFT >= NORMAL >= HARD; VEL MIN's floor;
 *   3. the owner's strikes replayed (G4, A#4, D4, D#4, F4, C4: soft / normal / hard as captured): each note's velocity
 *      the map of its window's lowest reading, which is the strike as captured where it landed within the window
 *      (normal, hard) or above it (a slow soft one still falling: softer); pooled, soft < normal < hard; per key where
 *      the capture's own medians are in order (G4, A#4) in order too;
 *   4. the latency: 400 modelled presses (falls 0.4 .. 10 ms, the digital contact at 600 or 400): the wait between the
 *      debounced press and the note let through (printed: mean, 90th percentile, max): the window closes at the key's
 *      next sample, driven, or the one after while the key is not seated and still falling fast (VD_EXT), so at most
 *      two sample periods (4.4 ms + a tick), never by VK_WAIT's limit; every press let through with a velocity;
 *   5. the calibration: a key whose strikes sit 50..60 counts off the seeds converges (its NORMAL within 12 of the
 *      strikes' median after 120 strikes, HARD / SOFT around the 15th / 85th percentiles), its soft / normal / hard
 *      then well apart; a graze learns nothing; VEL LEARN OFF nothing; saved (12 bits a key) and read back within the
 *      quantisation; no mark: the seeds; RESET: the seeds, saved; saved 30 s after the last strike learnt, not before;
 *   6. the top row (no converter): no wait; ~100 before any strike; it follows your dynamics (soft strikes: soft;
 *      three hard ones: most of the way up), a little humanised variation (within 7), no phantom press; OFF: 100;
 *   7. MASTER read in its slot while sensing (the knob followed), no phantom press over 3 s idle, the rows inputs;
 *   8. the pressure: a held key's reading under its strike, the dead zone (MID: 8 counts) none, 30 under ~44, smoothed
 *      (a quarter a driven sample), 0 at the release; LOW / HIGH; AFTERTOUCH OFF none;
 *   9. the jitter: 150 identical strikes a force (soft / normal / hard of G4's seeds), each with what a real one has
 *      (strike9: its touch anywhere in the scan's period, its fall +-25 %, noise sd 4, the pill settling, another
 *      column's interference on a third): each force's sd (printed; V10.6z9's vel.c gave 6.4 / 10.4 / 7.6 learning off,
 *      6.6 / 9.4 / 8.5 learning while playing) at most 8, 6 on average, soft..hard still >= 8 sd apart; eight forces
 *      softest to hardest: louder each (monotonic).
 *   cc -O1 -Ibuild/gen -Ifirmware/src tests/vel_test.c -o build/vel_test (-DVEL_BASELINE: 9.'s numbers alone, for
 *   an older vel.c) */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
static uint32_t host_now;                          /* TIMER4 ticks (24 MHz) */
#define FM1_INPUT_NOW() host_now
#include "../firmware/hal/fm1_time.h"
#include "../firmware/hal/fm1_gpio.h"
static volatile uint32_t host_reg[16][64];         /* the GPIO registers, as memory */
static uint32_t host_rows_bad;                     /* rows read while one of PA7 / PA8 was not an input */
static volatile uint32_t *host_pr(uint32_t p, uint32_t r);
#undef FM1_PR
#define FM1_PR(p, r) (*host_pr((p), (r)))
static uint32_t host_ticks(void) { return host_now; }
#define fm1_ticks host_ticks
static uint32_t adc_con, adc_res, adc_wla, adc_writes;
static uint32_t *adc_reg(void) { adc_writes++; return &adc_con; }
#define FM1_ADC_CON (*adc_reg())
#define FM1_ADC_RES adc_res
#define FM1_WLA_CON0 adc_wla
static void vd_sample(uint32_t col, uint32_t rowbit, uint32_t v, uint32_t driven);
static int vd_drive(uint32_t col, uint32_t rowbit);
static void host_note_on(uint32_t k);
#define FM1_INPUT_ANALOG(col, rowbit, v, driven) vd_sample(col, rowbit, v, driven)
#define FM1_INPUT_DRIVE(col, rowbit) vd_drive(col, rowbit)
#define FM1_INPUT_NOTE_ON(k) host_note_on(k)
#include "../firmware/hal/fm1_input.h"
static unsigned str_len(const char *s) { unsigned n = 0; while (s[n]) n++; return n; }
static void str_cpy(char *d, const char *s, unsigned max)
{
    unsigned i = 0;
    for (; max && i + 1u < max && s[i]; i++)
        d[i] = s[i];
    if (max)
        d[i] = 0;
}
static char host_msg[48];
static void ui_message(const char *s) { str_cpy(host_msg, s, sizeof host_msg); }
#include "../firmware/src/vel_diag.c"
static struct { uint8_t factory[16][32]; } favorites;   /* (the settings: vel_kb.h VEL_SET, the calibration's row 4) */
#define ENGI_SAMPLE 4u
static volatile uint32_t fm1_ms;
static uint32_t saves;
static void settings_save(void) { saves++; }
#include "../firmware/src/vel.c"

static int fails;
static int check(const char *what, int ok)
{
    printf("vel: %-104s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
    return ok;
}

/* ---- the model: each key's readings at time t (us) */
typedef struct {
    int on;
    uint32_t t0, fall, hold;                       /* touch, touch -> landed, the press's length (us) */
    int32_t pseat, d;                              /* the pulled-up seat, the driven strike */
    int32_t at_d;                                  /* held: the driven reading this far under the strike .. */
    uint32_t at_t0, at_ramp;                       /* .. from at_t0 (after the landing) over at_ramp us */
    int32_t graze;                                 /* a graze: the pulled-up reading never under this */
    int32_t nsd, alt, set;                         /* (9.) noise (sd, counts), another column's interference (each
                                                    * pulled-up and each driven reading +-alt in turn), landed: settling set under */
} mkey_t;
static mkey_t model[27];
static uint32_t pot = 700, sim_us, dig_th = 600;   /* the digital contact: closed under dig_th (pulled up) */
static int32_t model_v(uint32_t key, uint32_t t, int driven)
{
    const mkey_t *m = &model[key];
    uint32_t x;
    int32_t v;
    if (!m->on || t < m->t0 || t >= m->t0 + m->fall + m->hold)
        return 1023;
    x = t - m->t0;
    if (m->graze)
        return driven ? 1023 : m->graze;
    if (!driven)
        return x >= m->fall ? m->pseat : 880 - (880 - m->pseat) * (int32_t)x / (int32_t)m->fall;
    v = x >= m->fall ? m->d : 950 - (950 - m->d) * (int32_t)x / (int32_t)m->fall;
    if (m->set && x >= m->fall)                    /* (landed, the pill still settling: set under over ~3 ms) */
        v -= m->set * (int32_t)(x - m->fall) / (int32_t)(x - m->fall + 3000u);
    if (m->at_d && t > m->at_t0)
        v -= t >= m->at_t0 + m->at_ramp ? m->at_d : m->at_d * (int32_t)(t - m->at_t0) / (int32_t)m->at_ramp;
    return v;
}
static int dig_closed(uint32_t key) { return model[key].on && model_v(key, sim_us, 0) < (int32_t)dig_th; }
static volatile uint32_t *host_pr(uint32_t p, uint32_t r)
{
    static volatile uint32_t in;
    if (p == FM1_PA && r == FM1_IN) {
        uint32_t col = fm1__tick_col, rb, v = 0xFFFFFFFFu;
        if (~host_reg[FM1_PA][FM1_DIR / 4u] & ((1u << 7) | (1u << 8)))
            host_rows_bad++;
        for (rb = 2; rb <= 4u; rb++) {             /* PA6 PA7 PA8: the note keys' rows */
            int id = FM1_KEYMAP[rb][col];
            if (id >= 14 && dig_closed((uint32_t)id - 14u))
                v &= ~(1u << (rb + 4u));
        }
        in = v;
        return &in;
    }
    if (p == FM1_PB && r == FM1_IN) {
        in = 0xFFFFFFFFu;
        return &in;
    }
    return &host_reg[p & 15u][(r / 4u) & 63u];
}
static int32_t host_noise(int32_t sd);
static void adc_finish(void)
{
    uint32_t ch = (adc_con >> 8) & 15u, col = fm1_an.col, rb = fm1_an.rowbit, v;
    if (!(adc_con & 0x10u))
        return;
    if (ch == FM1_ADC_MASTER)
        v = pot;
    else if (ch == FM1_ADC_BATT)
        v = 620;
    else {
        uint32_t key = vd_note(col, rb), m = 1u << (rb == 3u ? 7 : 8);
        int driven = !(host_reg[FM1_PA][FM1_DIR / 4u] & m) && (host_reg[FM1_PA][FM1_OUT / 4u] & m);
        v = key < VD_NK ? (uint32_t)model_v(key, sim_us, driven) : 1023u;
        if (key < VD_NK && v < 1023u && (model[key].nsd || model[key].alt)) {
            int32_t w = (int32_t)v + host_noise(model[key].nsd) + ((sim_us / 4400u) & 1u ? model[key].alt : -model[key].alt);
            v = (uint32_t)(w < 0 ? 0 : w > 1023 ? 1023 : w);
        }
    }
    adc_res = v;
    adc_con |= 0x80u;
}
/* the presses as the keys' block (seq.c) sees them: let through (fm1_in.notes & ~vk_hold) */
static uint32_t t_press[27], t_vis[27], vel_vis[27], vis_prev, n_vis, n_press;
static void host_note_on(uint32_t k)
{
    t_press[k] = sim_us;
    n_press++;
    vel_note_on(k);
}
static void tick(void)
{
    uint32_t vis, k;
    adc_finish();
    sim_us += 100u;
    host_now = sim_us * 24u;
    fm1_ms = sim_us / 1000u;
    fm1_input_tick();
    vis = fm1_in.notes & ~vk_hold;
    for (k = 0; k < 27u; k++)
        if ((vis & ~vis_prev) >> k & 1u) {
            t_vis[k] = sim_us;
            vel_vis[k] = KB_VEL(k);
            n_vis++;
        }
    vis_prev = vis;
}
static void run_ms(uint32_t ms)
{
    uint32_t i;
    for (i = 0; i < ms * 10u; i++)
        tick();
}
static uint32_t rnd_s = 12345u;
static uint32_t rnd(uint32_t n) { rnd_s = rnd_s * 1103515245u + 12345u; return (rnd_s >> 8) % n; }
static int32_t gauss(int32_t mean, int32_t sd)    /* roughly normal: the sum of four uniforms */
{
    int32_t s = 0, i;
    for (i = 0; i < 4; i++)
        s += (int32_t)rnd(2001u) - 1000;
    return mean + s * sd / 1155;
}
/* one strike of key k: fall (us), strike d, held hold ms; then let go; returns the velocity the keys' block saw */
static uint32_t strike(uint32_t k, uint32_t fall, int32_t d, uint32_t hold)
{
    mkey_t *m = &model[k];
    memset(m, 0, sizeof *m);
    m->on = 1;
    m->t0 = sim_us + 300u;
    m->fall = fall < 100u ? 100u : fall;
    m->hold = hold * 1000u;
    m->pseat = 200;
    m->d = d;
    t_vis[k] = 0;
    run_ms(hold + fall / 1000u + 2u);
    m->on = 0;
    run_ms(25);
    return vel_vis[k];
}
static void settings(uint32_t curve, uint32_t min, uint32_t at, uint32_t learn)   /* places, as the menu's */
{
    VEL_SET = (uint8_t)(((curve ^ 2u) & 3u) | ((min ^ 2u) & 7u) << 2);
    VEL_SET2 = (uint8_t)(((at ^ 2u) & 3u) | learn << 2);
    vel_poll();
}
static uint32_t ns = 777u;                         /* (the noise's sequence) */
static int32_t host_noise(int32_t sd)
{
    int32_t a = 0, i;
    for (i = 0; i < 4; i++) {
        ns = ns * 1664525u + 1013904223u;
        a += (int32_t)((ns >> 16) % 2001u) - 1000;
    }
    return a * sd / 1155;
}
static int cmp_u(const void *a, const void *b) { return (int)(*(const uint32_t *)a) - (int)(*(const uint32_t *)b); }

/* (9.) one strike with the details a real one has: its touch anywhere in the scan's period, the fall's length as given,
 * the seat 190..240 pulled up, noise (sd 4 counts a reading), the pill settling 20 counts under its landing over a few
 * ms, and on one strike in three another column's interference (+-20 in turn, as the owner's 194 / 243) */
static uint32_t strike9(uint32_t k, uint32_t fall, int32_t d, int32_t alt)
{
    mkey_t *m = &model[k];
    memset(m, 0, sizeof *m);
    m->on = 1;
    m->t0 = sim_us + 300u + rnd(4400u);
    m->fall = fall;
    m->hold = 60000u;
    m->pseat = 190 + (int32_t)rnd(50u);
    m->d = d;
    m->nsd = 4;
    m->alt = alt;
    m->set = 20;
    t_vis[k] = 0;
    vel_vis[k] = 0;
    run_ms(70u + fall / 1000u);
    m->on = 0;
    run_ms(25);
    return vel_vis[k];
}
/* (9.) n strikes of each of three forces (identical landings: SOFT / NORMAL / HARD of key k's seeds, each its own fall
 * +-25 %): each force's mean and sd; mixed (lvl 3) or one force after another */
static const uint32_t FALL9[3] = {5000, 2500, 1000};
static double sqrt9(double x)                      /* (no libm: the workflow links none) */
{
    double r = x > 1 ? x : 1;
    int i;
    for (i = 0; i < 40; i++)
        r = (r + x / r) / 2;
    return x > 0 ? r : 0;
}
static void jitter(uint32_t k, const int32_t d[3], uint32_t n, uint32_t play, double mean[3], double sd[3])
{
    double s1[3] = {0}, s2[3] = {0};
    uint32_t i, c[3] = {0};
    for (i = 0; i < 3u * n; i++) {
        uint32_t f = rnd(3u), v;
        if (play) {                                /* (between them, playing: any force, as a player's spread) */
            uint32_t g = rnd(3u);
            strike9(k, FALL9[g] * (75u + rnd(51u)) / 100u, gauss(d[g], 25), rnd(3u) ? 0 : 20);
        }
        v = strike9(k, FALL9[f] * (75u + rnd(51u)) / 100u, d[f], rnd(3u) ? 0 : 20);
        s1[f] += v;
        s2[f] += (double)v * v;
        c[f]++;
    }
    for (i = 0; i < 3u; i++) {
        mean[i] = s1[i] / c[i];
        sd[i] = c[i] > 1u ? sqrt9((s2[i] - s1[i] * s1[i] / c[i]) / (c[i] - 1u)) : 0;
    }
}
int main(void)
{
    uint32_t i, j, k;
    host_reg[FM1_PA][FM1_DIR / 4u] = 0xFFFFFFFFu;
    /* ---- 1. the settings */
    vel_poll();
    check("1. an older setting (bytes 0): VELOCITY NORMAL, VEL MIN 20, AFTERTOUCH MID, VEL LEARN ON; the sampling on",
          vs.curve == 2u && vs.min == 20u && vs.atr == 2u && vs.learn && vk_on == 3u && fm1_an.want && vk_cal == 1u);
    check("   no calibration saved: the seeds (C4 594+15, D4 594+77, the others 560 / 594 / 675)",
          vs.ref[7][1] >> 4 == 609 && vs.ref[9][1] >> 4 == 671 && vs.ref[0][0] >> 4 == 560 && vs.ref[0][2] >> 4 == 675);
    settings(0, 2, 0, 0);
    tick();
    adc_writes = 0;
    n_vis = n_press = 0;
    for (j = 0; j < 6u; j++)
        vel_vis[j] = 0, strike(j, 1500u + 1000u * j, 560 + 20 * (int32_t)j, 60);
    {
        uint32_t ok = n_vis == 6u && n_press == 6u;
        for (j = 0; j < 6u; j++)
            ok &= vel_vis[j] == 100u && t_vis[j] == t_press[j];
        check("   VELOCITY OFF + AFTERTOUCH OFF: no converter access at all, six presses each 100 at its debounce (as before)",
              ok && adc_writes == 0 && !fm1_an.on && !vk_hold);
    }
    settings(0, 2, 2, 0);
    run_ms(5);
    vel_vis[3] = 0;
    strike(3, 1500u, 540, 60);
    check("   VELOCITY OFF, AFTERTOUCH on: sampled, the press 100 at its debounce", fm1_an.on && vel_vis[3] == 100u &&
          t_vis[3] == t_press[3]);

    /* ---- 2. the map */
    settings(2, 0, 2, 0);                          /* NORMAL, MIN 1 */
    {
        uint32_t key = 14, prev = 200, steps = 0, mono = 1, cont = 1, vals = 0, last = 999, d;
        int32_t h = vs.ref[key][0] >> 4, n = vs.ref[key][1] >> 4, s = vs.ref[key][2] >> 4;
        for (d = 300; d <= 1023u; d++) {
            uint32_t v = vel_map(key, d);
            if (d > 300u && v > prev) mono = 0;
            if (d > 300u && prev - v > 2u) cont = 0;
            if ((int32_t)d >= h - 40 && (int32_t)d <= s + 40 && v != last) vals++, last = v;
            prev = v;
            steps++;
        }
        check("2. the map (G4: HARD 594 / NORMAL 628 / SOFT 709): monotonic, continuous (<= 2 a count), >= 60 values",
              mono && cont && vals >= 55u);
        check("   HARD 104, NORMAL 76, SOFT 40 at the references; halfway between, halfway (90, 58)",
              vel_map(key, h) == 104u && vel_map(key, n) == 76u && vel_map(key, s) == 40u &&
              vel_map(key, (h + n) / 2) >= 89u && vel_map(key, (h + n) / 2) <= 91u &&
              vel_map(key, (n + s) / 2) >= 57u && vel_map(key, (n + s) / 2) <= 59u);
        check("   past the ends half the slope (SOFT + 40 counts: 31, HARD - 20: 112), clamped 1..127",
              vel_map(key, s + 40) >= 30u && vel_map(key, s + 40) <= 32u && vel_map(key, h - 20) >= 111u &&
              vel_map(key, h - 20) <= 113u && vel_map(key, 200) == 127u && vel_map(key, 1023) >= 1u);
        {
            uint32_t ok = 1, vs0, vn, vh;
            for (d = 300; d <= 1023u; d++) {
                vs.curve = 1; vs0 = vel_map(key, d);
                vs.curve = 2; vn = vel_map(key, d);
                vs.curve = 3; vh = vel_map(key, d);
                ok &= vs0 >= vn && vn >= vh;
            }
            vs.curve = 1; vs0 = vel_map(key, n);
            vs.curve = 3; vh = vel_map(key, n);
            vs.curve = 2;
            check("   the curves: SOFT >= NORMAL >= HARD everywhere (a NORMAL strike: 87 / 76 / 65)", ok && vs0 > 76u && vh < 76u);
        }
        settings(2, 2, 2, 0);                      /* MIN 20 */
        {
            uint32_t lo = 999, hi = 0;
            for (d = 300; d <= 1023u; d++) {
                uint32_t v = vel_map(key, d);
                lo = v < lo ? v : lo;
                hi = v > hi ? v : hi;
            }
            check("   VEL MIN 20: 20..127, the range scaled (no step at the floor)", lo == 20u && hi == 127u &&
                  vel_map(key, n) == 20u + 75u * 107u / 126u);
        }
    }

    /* ---- 3. the owner's strikes replayed, the seeds as they are */
    settings(2, 0, 2, 1);                          /* NORMAL, MIN 1, learning off (the seeds as seeded) */
    {
        static const struct { uint8_t key, lvl; int16_t d[5]; } CAP[] = {
            {14, 0, {629, 796, 655, 699, 696}}, {14, 1, {579, 690, 687}}, {14, 2, {560, 547, 545}},
            {17, 0, {572, 674, 655, 603}}, {17, 1, {567, 578}}, {17, 2, {532, 535, 535}},
            {9, 0, {859, 708}}, {9, 1, {606, 594}}, {9, 2, {689, 663}},
            {10, 0, {735, 659}}, {10, 1, {549, 538}}, {10, 2, {586}},
            {12, 0, {648, 588}}, {12, 1, {630, 638}}, {12, 2, {646, 535}},
            {7, 0, {696}}, {7, 1, {588, 537}}, {7, 2, {540, 693}},
        };
        static const uint32_t FALL[3] = {6000, 3000, 1000};   /* soft / normal / hard: touch -> landed */
        uint32_t sum[3] = {0}, cnt[3] = {0}, exact = 1, n = 0, med[2][3][5], mn[2][3] = {{0}};
        for (i = 0; i < sizeof CAP / sizeof CAP[0]; i++)
            for (j = 0; j < 5u && CAP[i].d[j]; j++) {
                uint32_t m0 = vk_mean ? vk_mean : 100u, v = strike(CAP[i].key, FALL[CAP[i].lvl] + rnd(800u), CAP[i].d[j], 120);
                exact &= v == (7u * vel_map(CAP[i].key, vs.vd[CAP[i].key]) + m0 + 4u) / 8u &&
                         vs.vd[CAP[i].key] >= (uint32_t)CAP[i].d[j] &&
                         (CAP[i].lvl == 0u || vs.vd[CAP[i].key] <= (uint32_t)CAP[i].d[j] + VD_NEAR / 2u);
                sum[CAP[i].lvl] += v;
                cnt[CAP[i].lvl]++;
                n++;
                if (CAP[i].key == 14u || CAP[i].key == 17u)
                    med[CAP[i].key == 17u][CAP[i].lvl][mn[CAP[i].key == 17u][CAP[i].lvl]++] = v;
            }
        check("3. 42 captured strikes replayed through the scan: each note's velocity the map of its window's reading (an "
              "eighth of the way to the recent mean), that reading the captured strike (normal, hard: landed within the "
              "window; at most VD_NEAR / 2 over it: the mean with a reading still falling) or above it (soft: 6 ms falls)",
              exact && n == 42u);
        printf("vel:    pooled mean velocity soft %u normal %u hard %u\n", sum[0] / cnt[0], sum[1] / cnt[1], sum[2] / cnt[2]);
        check("   pooled: soft < normal < hard", sum[0] / cnt[0] < sum[1] / cnt[1] && sum[1] / cnt[1] < sum[2] / cnt[2]);
        for (k = 0; k < 2u; k++)
            for (j = 0; j < 3u; j++)
                qsort(med[k][j], mn[k][j], sizeof med[k][j][0], cmp_u);
        check("   G4 and A#4 (their captures' medians in order): their notes' medians soft < normal < hard",
              med[0][0][mn[0][0] / 2] < med[0][1][mn[0][1] / 2] && med[0][1][mn[0][1] / 2] < med[0][2][mn[0][2] / 2] &&
              med[1][0][mn[1][0] / 2] < med[1][1][mn[1][1] / 2] && med[1][1][mn[1][1] / 2] < med[1][2][mn[1][2] / 2]);
    }

    /* ---- 4. the latency */
    {
        uint32_t w[400], nw = 0, timeouts = 0, zero = 0, ok = 1, sum = 0, ext = 0;
        for (i = 0; i < 400u; i++) {
            uint32_t key = rnd(20u), fall = 400u + rnd(9600u);
            dig_th = i & 1u ? 600u : 400u;
            vel_vis[key] = 0;
            strike(key, fall, 520 + (int32_t)rnd(300u), 40);
            if (!t_vis[key] || !vel_vis[key]) { ok = 0; continue; }
            w[nw] = t_vis[key] - t_press[key];
            sum += w[nw];
            zero += !w[nw];
            timeouts += vel_vis[key] != vk[key].vel || vk[key].pend;   /* (let through by VK_WAIT, not by its window) */
            ext += w[nw] > 2300u;
            nw++;
        }
        dig_th = 600;
        qsort(w, nw, sizeof w[0], cmp_u);
        printf("vel:    the wait after the debounced press: mean %.2f ms, 90%% %.1f ms, max %.1f ms; none for %u of %u; "
               "one more sample %u; past its time %u\n", sum / 1000.0 / nw, w[nw * 9u / 10u] / 1000.0, w[nw - 1u] / 1000.0, zero,
               nw, ext, timeouts);
        check("4. 400 presses (falls 0.4..10 ms, the contact at 600 or 400): every one let through, with a velocity", ok && nw == 400u);
        check("   the wait never over two sample periods (4.4 ms + a tick: one more while still falling), never past its "
              "time (each note the velocity its window closed with)", w[nw - 1u] <= 4500u && !timeouts);
    }

    /* ---- 5. the calibration */
    settings(2, 0, 2, 0);
    {
        uint32_t key = 12, s0 = saves, ds[150], nd = 0, sumv[3] = {0}, cv[3] = {0};
        static const int32_t MEAN[3] = {740, 655, 610}, SD[3] = {28, 18, 14};   /* the first capture's level, ~+50 */
        vk_cal = 2;
        vel_poll();
        check("5. RESET: the seeds, saved", vs.ref[key][1] >> 4 == 599 && saves == s0 + 1u && vs.n[key] == 0);
        for (i = 0; i < 150u; i++) {
            uint32_t lvl = rnd(3u);
            int32_t d = gauss(MEAN[lvl], SD[lvl]);
            ds[nd++] = (uint32_t)d;
            strike(key, 2000u + rnd(2000u), d, 50);
        }
        qsort(ds, nd, sizeof ds[0], cmp_u);
        printf("vel:    F4 after 150 strikes: HARD %d NORMAL %d SOFT %d (the strikes' 15th %u, median %u, 85th %u)\n",
               vs.ref[key][0] >> 4, vs.ref[key][1] >> 4, vs.ref[key][2] >> 4, ds[nd * 15 / 100], ds[nd / 2], ds[nd * 85 / 100]);
        check("   strikes ~55 counts off the seeds: NORMAL within 12 of their median, HARD / SOFT within 20 of the 15th / 85th",
              abs((int)(vs.ref[key][1] >> 4) - (int)ds[nd / 2]) <= 12 && abs((int)(vs.ref[key][0] >> 4) - (int)ds[nd * 15 / 100]) <= 20 &&
              abs((int)(vs.ref[key][2] >> 4) - (int)ds[nd * 85 / 100]) <= 20);
        for (i = 0; i < 60u; i++) {
            uint32_t lvl = i % 3u;
            sumv[lvl] += strike(key, 2500u, gauss(MEAN[lvl], SD[lvl]), 50);
            cv[lvl]++;
        }
        printf("vel:    then: mean velocity soft %u normal %u hard %u\n", sumv[0] / cv[0], sumv[1] / cv[1], sumv[2] / cv[2]);
        check("   then its soft / normal / hard well apart (soft < 60 < normal < 95 < hard)", sumv[0] / cv[0] < 60u &&
              sumv[1] / cv[1] > 60u && sumv[1] / cv[1] < 95u && sumv[2] / cv[2] > 95u);
        {
            uint16_t r0[3];
            memcpy(r0, vs.ref[key], sizeof r0);
            model[key].graze = 0;
            memset(&model[key], 0, sizeof model[key]);
            model[key].on = 1; model[key].t0 = sim_us + 300u; model[key].fall = 3000; model[key].hold = 40000;
            model[key].graze = 650;
            run_ms(80);
            model[key].on = 0;
            run_ms(20);
            check("   a graze learns nothing", !memcmp(r0, vs.ref[key], sizeof r0));
            settings(2, 0, 2, 1);
            strike(key, 2000u, 450, 50);
            check("   VEL LEARN OFF: nothing learnt", !memcmp(r0, vs.ref[key], sizeof r0));
            settings(2, 0, 2, 0);
        }
        {
            uint16_t r0[VK_NK][3];
            uint32_t ok = 1;
            s0 = saves;
            vs.dirty = 1;
            vs.t = fm1_ms;
            run_ms(29000);
            vel_poll();
            ok = saves == s0;
            run_ms(1100);
            vel_poll();
            check("   saved 30 s after the last strike learnt (not before), with the mark", ok && saves == s0 + 1u &&
                  VC_ROW[1] == VC_MARK && !vs.dirty);
            memcpy(r0, vs.ref, sizeof r0);
            memset(vs.ref, 0, sizeof vs.ref);
            vk_cal = 0;
            vel_poll();
            ok = 1;
            for (k = 0; k < VK_NK; k++) {
                int32_t n0 = r0[k][1] >> 4, n1 = vs.ref[k][1] >> 4;
                ok &= abs(n1 - n0) <= 4 && abs(((int32_t)vs.ref[k][2] >> 4) - n1 - (((int32_t)r0[k][2] >> 4) - n0)) <= 8 &&
                      abs((n1 - ((int32_t)vs.ref[k][0] >> 4)) - (n0 - ((int32_t)r0[k][0] >> 4))) <= 4;
            }
            check("   read back from the settings (a restore, power-on): within the quantisation (NORMAL 4, SOFT 8, HARD 4)", ok);
            VC_ROW[1] = 0;
            vk_cal = 0;
            vel_poll();
            check("   no mark (older settings): the seeds", vs.ref[key][1] >> 4 == 599 && vs.n[key] == 0);
        }
    }

    /* ---- 6. the top row: it follows your dynamics (your last few strikes, the upper keys' most, and the longer mean) */
    settings(2, 0, 2, 1);
    vk_mean = 0;
#ifndef VEL_BASELINE
    vs.fast = 0;
#endif
    for (k = 20; k < 27u; k++)
        vel_vis[k] = 0;
    n_press = n_vis = 0;
    strike(20, 2000u, 500, 50);
    {
        uint32_t v0 = vel_vis[20], w0 = t_vis[20] - t_press[20], soft = 0, vsoft, vhard, lo = 999, hi = 0, same = 1, ok = 1;
        for (i = 0; i < 8u; i++)                   /* soft ones (F4, G4, A#4), each its own */
            soft += strike(12u + 2u * (i % 3u), 5000u, 760 + (int32_t)rnd(30u), 50);
        strike(26, 2000u, 500, 50);
        vsoft = vel_vis[26];
        ok &= t_vis[26] == t_press[26];
        for (i = 0; i < 3u; i++)                   /* then three hard ones on C5 */
            strike(19, 800u, 520, 50);
        strike(23, 2000u, 500, 50);
        vhard = vel_vis[23];
        ok &= t_vis[23] == t_press[23];
        printf("vel:    the top row: 100 before any strike (%u); after 8 soft (mean %u) %u; after 3 hard (%u, the mean %u) %u\n",
               v0, soft / 8u, vsoft, vel_vis[19], vk_mean, vhard);
        check("6. the top row (C#5..G5, no converter): no wait; ~100 before any strike; after soft strikes soft (within 12 of "
              "theirs)", v0 >= 97u && v0 <= 104u && w0 == 0 && ok && abs((int)vsoft - (int)(soft / 8u)) <= 12);
        check("   after three hard ones: most of the way to them (over the longer mean, within 20 of theirs)",
              vhard > vk_mean + 8u && vhard + 20u >= vel_vis[19]);
        for (i = 0; i < 12u; i++) {                /* the same dynamic, twelve top notes: a little variation, no more */
            uint32_t v;
            strike(20u + i % 7u, 2000u, 500, 50);
            v = vel_vis[20u + i % 7u];
            same &= i == 0u || v == lo;
            lo = v < lo ? v : lo;
            hi = v > hi ? v : hi;
        }
        check("   twelve top notes with nothing between: a little humanised variation (not all equal, within 7)", !same &&
              hi - lo <= 7u);
        check("   every press one note, nothing else pressed (no phantom)", n_press == 26u && n_vis == 26u);
        settings(0, 2, 2, 1);                      /* VELOCITY OFF */
        strike(22, 2000u, 500, 50);
        check("   VELOCITY OFF: 100 exactly, at its debounce", vel_vis[22] == 100u && t_vis[22] == t_press[22]);
        settings(2, 0, 2, 1);
    }

    /* ---- 7. MASTER, no phantom */
    pot = 300;
    run_ms(6);
    check("7. MASTER read in its slot while sensing: the knob followed", fm1_adc_read(FM1_ADC_MASTER) == 300);
    n_press = 0;
    host_rows_bad = 0;
    run_ms(3000);
    check("   3 s idle: no press, nothing waiting, every row read with PA7 / PA8 inputs", !n_press && !fm1_in.notes &&
          !vk_hold && !host_rows_bad);

    /* ---- 8. the pressure */
    {
        mkey_t *m = &model[14];
        uint32_t peak = 0, smooth = 1, prev = 0, dz;
        settings(2, 0, 2, 1);
        memset(m, 0, sizeof *m);
        m->on = 1; m->t0 = sim_us + 300u; m->fall = 2000; m->hold = 400000; m->pseat = 200; m->d = 600;
        m->at_d = 6; m->at_t0 = m->t0 + 20000u; m->at_ramp = 10000u;   /* a finger resting: 6 under */
        run_ms(80);
        dz = vk[14].at;
        m->at_d = 30; m->at_t0 = sim_us; m->at_ramp = 40000u;   /* pressed on: 30 under over 40 ms */
        for (i = 0; i < 1200u; i++) {
            tick();
            if (vk[14].at > prev + 20u) smooth = 0;
            prev = vk[14].at;
            peak = vk[14].at > peak ? vk[14].at : peak;
        }
        check("8. pressure: the dead zone (6 counts under the strike at MID) none", dz == 0);
        check("   30 under: ~44 (MID: 2 a count - 16), smoothed (no jump over 20 a sample)", peak >= 42u && peak <= 44u && smooth);
        m->on = 0;
        run_ms(10);
        check("   let go: 0", vk[14].at == 0);
        settings(2, 0, 1, 1);                      /* LOW */
        m->on = 1; m->t0 = sim_us + 300u; m->at_d = 30; m->at_t0 = m->t0 + 15000u; m->at_ramp = 1000u;
        run_ms(120);
        peak = vk[14].at;
        settings(2, 0, 3, 1);                      /* HIGH */
        run_ms(120);
        prev = vk[14].at;
        settings(2, 0, 0, 1);                      /* OFF */
        m->on = 0; run_ms(20);
        m->on = 1; m->t0 = sim_us + 300u; m->at_t0 = m->t0 + 15000u;
        run_ms(120);
        check("   LOW 14, HIGH 104 for 30 under; AFTERTOUCH OFF none", peak >= 13u && peak <= 14u && prev >= 100u &&
              prev <= 104u && vk[14].at == 0);
        m->on = 0;
        run_ms(20);
    }
    /* ---- 9. the jitter: identical strikes, the details a real one has (strike9) */
    {
        static const int32_t D9[3] = {709, 628, 594};   /* G4's seeds: SOFT, NORMAL, HARD */
        double mean[3], sd[3], sdl[3], meanl[3], mono = 1, prev = 0, sep, sepl;
        uint32_t f;
        settings(2, 0, 2, 1);                      /* NORMAL, MIN 1, learning off: the reading alone */
        vk_mean = 0;
        rnd_s = 4242u;                             /* (the same strikes whatever ran before: comparable across builds) */
        ns = 777u;
        jitter(14, D9, 150, 0, mean, sd);
        sep = (mean[2] - mean[0]) / ((sd[0] + sd[1] + sd[2]) / 3);
        printf("vel:    jitter (G4, 150 identical strikes a force, learning off): soft %.1f +- %.1f, normal %.1f +- %.1f, "
               "hard %.1f +- %.1f; soft..hard %.1f sd apart\n", mean[0], sd[0], mean[1], sd[1], mean[2], sd[2], sep);
        vk_cal = 2;
        settings(2, 0, 2, 0);                      /* learning from the seeds, as on a fresh unit, while playing */
        vk_mean = 0;
        jitter(14, D9, 150, 1, meanl, sdl);
        sepl = (meanl[2] - meanl[0]) / ((sdl[0] + sdl[1] + sdl[2]) / 3);
        printf("vel:    jitter, learning from a reset, each between two strikes of any force (sd 25): soft %.1f +- %.1f, normal %.1f +- %.1f, hard %.1f +- %.1f; "
               "soft..hard %.1f sd apart\n", meanl[0], sdl[0], meanl[1], sdl[1], meanl[2], sdl[2], sepl);
        settings(2, 0, 2, 1);
        printf("vel:    8 forces, softest to hardest (each the mean of 30):");
        for (f = 0; f < 8u; f++) {                 /* harder and harder: louder and louder */
            int32_t dd = 720 - 20 * (int32_t)f;
            uint32_t i, sum = 0;
            for (i = 0; i < 30u; i++)
                sum += strike9(14, 5000u - 550u * f, dd, 0);
            printf(" %.0f", sum / 30.0);
            mono = mono && sum / 30.0 > prev;
            prev = sum / 30.0;
        }
        printf("\n");
#ifndef VEL_BASELINE
        check("9. jitter: identical strikes' sd at most 8 at any force, 6 on average (V10.6z9: 6.6 / 10.9 / 8.0, learning "
              "off), learning off and from a reset", sd[0] <= 8 && sd[1] <= 8 && sd[2] <= 8 && sd[0] + sd[1] + sd[2] <= 18 &&
              sdl[0] <= 8 && sdl[1] <= 8 && sdl[2] <= 8 && sdl[0] + sdl[1] + sdl[2] <= 18);
        check("   still expressive: soft..hard >= 8 sd apart, soft < normal < hard; 8 forces, harder louder each",
              sep >= 8 && sepl >= 8 && mean[0] < mean[1] && mean[1] < mean[2] && meanl[0] < meanl[1] && meanl[1] < meanl[2] && mono);
#endif
    }
    printf("vel: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
