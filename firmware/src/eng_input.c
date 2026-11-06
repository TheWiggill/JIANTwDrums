/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* (V10.54) INPUT: a track that plays the audio input instead of a synth (engine 15).
 * The input is the USB audio one: an interface on the USB host port (usb_host.c, V10.53) or the computer's USB IN+OUT
 * (usb.c uac_in_fill), a block of stereo Q15 in uin_l / uin_r (uin_on: this block holds it).
 *
 * SOURCE picks what the track takes: IN 1 (left), IN 2 (right), 1+2 (their sum, mono) or STEREO (both, the image kept
 * as the IMAGE of DRUM-X and the poly synths keep theirs: the part's mono sum through its chain, each side what it gives
 * up, eng_drum.c dx_sl / dx_sr). Two INPUT tracks can take IN 1 and IN 2 each. Then GAIN, GATE (a noise gate: closed
 * below its level), BITS and RATE (fewer bits, a lower rate: held samples), and the track's own chain as any part's:
 * FILTER, DIST, LEVEL, PAN, the sends, the mutes, the matrix. PLAY: THRU sounds it all the time; KEYS only while a
 * note plays, at the note's envelope (VOICE: ATK DEC SUS REL; the sequencer's steps gate it: a trance gate), RING then
 * multiplying it by a sine at the note's pitch. WIDTH: STEREO's width.
 *
 * While a track is INPUT the input goes to it alone (fx.c mix_part: the last live track's USB IN+OUT mix steps aside).
 * The FOLLOW source of the matrix (mod.c MS_FOLLOW) is the input's envelope, whatever track takes it. */
static int32_t uin_l[CTL], uin_r[CTL];                  /* (0.7.5) the input's block (usb.c uac_in_fill; was in fx.c) */
static volatile uint8_t uin_on;
enum { AIN_IN1, AIN_IN2, AIN_SUM, AIN_ST, AIN_N };
static const char *const N_AIN_SRC[] = {"IN 1", "IN 2", "1+2", "STEREO"};
static const char *const N_AIN_PLAY[] = {"THRU", "KEYS"};
static int16_t ain_gg[NTRK];                             /* GATE's gain now, Q15 */
static uint8_t ain_upd[NTRK];                            /* this block's GATE not yet stepped (ain_block) */
static uint8_t ain_any;                                  /* a track is INPUT (mod.c macro_master, once a block) */

/* the source of track t, block of n, into out (mono; STEREO: the mid, its sides into dx_sl / dx_sr at WIDTH when
 * side): GAIN, GATE (stepped once a block), BITS, RATE */
static __attribute__((noinline)) void ain_fill(const track_t *t, int32_t *out, uint32_t n, int side)
{
    const int16_t *p = t->p;
    uint32_t ti = (uint32_t)(t - trk) % NTRK, src = (uint32_t)clamp(p[P_E0], 0, AIN_N - 1), i;
    int32_t g = clamp(p[P_E1], 0, 127), thr = clamp(p[P_E2], 0, 127), sh = clamp(p[P_E6], 0, 127) * 14 / 127;
    uint32_t hold = (1u << (clamp(p[P_E7], 0, 127) * 5 / 127)) - 1u;   /* RATE: 1 .. 32 samples held */
    int32_t g0 = ain_gg[ti], g1 = g0, pk = 0, w = 0;
    g = g * g;                                           /* GAIN: Q12, 64 = 1.0, 127 = +12 dB */
    if (side && src == AIN_ST) {
        int32_t d = clamp(p[P_DIST], 0, 64);
        w = clamp(p[P_E4], 0, 127) * (d ? 256 - d * 3 : 256) >> 8;   /* (DIST shapes the sum only: narrower) */
        if (w)
            dx_xon |= 1u;
    }
    for (i = 0; i < n; i++) {
        uint32_t j = i & ~hold;
        int32_t l = uin_l[j], r = uin_r[j], x = src == AIN_IN1 ? l : src == AIN_IN2 ? r : src == AIN_SUM ? l + r : (l + r) >> 1;
        x = (x * g) >> 12;
        if (src == AIN_ST && w)
            dx_sl[i] = -(dx_sr[i] = ((((l - r) * g) >> 13) * w) >> 7);   /* L = sum - sl, R = sum - sr */
        out[i] = x;
        x = x < 0 ? -x : x;
        if (x > pk)
            pk = x;
    }
    if (thr) {                                           /* GATE: open over its level, closed under 3/4 of it */
        if (ain_upd[ti]) {
            int32_t lv = thr * thr * 2;
            g1 = pk > lv ? 32767 : pk < lv * 3 / 4 ? g0 - (g0 >> 4) - 1 : g0;
            g1 = clamp(g1, 0, 32767);
            ain_gg[ti] = (int16_t)g1;
            ain_upd[ti] = 0;
        }
        for (i = 0; i < n; i++) {
            int32_t k = g0 + (((g1 - g0) * (int32_t)i) >> CTL_LOG2);
            out[i] = mulq15(out[i], k);
            if (src == AIN_ST && w) {
                dx_sl[i] = mulq15(dx_sl[i], k);
                dx_sr[i] = -dx_sl[i];
            }
        }
    } else
        ain_gg[ti] = 32767;
    if (sh)
        for (i = 0; i < n; i++)
            out[i] &= ~((1 << sh) - 1);                  /* BITS */
}

/* (SWARM) the radio's surroundings as the input (the user's ask: the antenna and the clocks, their relations, a synth
 * that makes itself): with MENU > SYSTEM > BLUETOOTH AIR scanning and no USB input, every device heard around
 * (phones, watches, headphones, laptops, TVs: their advertisements, bt_seg.c BLE_HW_ADV_TAP) is a voice, eight at most.
 * Its address its note (held, in the first INPUT track's ROOT / SCALE); its payload's first 8 bytes its wave; its
 * RSSI (in the range seen) its level; each advertisement an accent, so its own clock (its advertising interval,
 * timed by the FM-1's crystal) its rhythm, and the devices' polyrhythm the swarm's; the ratio of its period to the
 * loudest device's a detune (-25 .. +25 cents: two devices beat at their clocks' relation); its interval's jitter a
 * vibrato; its channel its side (37 L, 38 both, 39 R). A device not heard fades (~6 s), its voice freed after 20 s:
 * a phone that comes in, a new note. Into uin_l / uin_r as an input block: SOURCE GAIN GATE KEYS RING BITS RATE and
 * FOLLOW (its envelope) as with USB. Only while a track is INPUT and no USB input filled the block (mod.c) */
#define SW_N 8u
#define SW_TPS 24000000u                                 /* TIMER4 ticks a second */
static struct sw_dev {
    uint32_t id, last, per, ph, inc;                     /* address hash (0 free); last packet; period (ticks); wave */
    int32_t acc, amp, jit;                               /* accent; level now; jitter (Q8 of the period) */
    uint16_t lv;                                         /* last RSSI level (0..126) */
    uint8_t hit, ch, note;
    int8_t w[8];                                         /* the wave: 8 points */
} sw[SW_N];
static int32_t sw_lmin = 127, sw_lmax;
static uint32_t sw_tick;
static uint8_t ain_tr;                                   /* the first INPUT track (mod.c macro_master) */
static int32_t scale_snap(const track_t *t, int32_t n);  /* seq.c */
#ifndef fm1_ticks
static uint32_t fm1_ticks(void);                         /* (hal/fm1_time.h; the host tests' own) */
#endif
void bt_air_tap(const uint8_t *p, uint32_t n, uint32_t w, uint32_t ch)   /* the BLE RX interrupt */
{
    uint32_t i, k = SW_N, o = 0, j = 0, now = fm1_ticks(), id;
    struct sw_dev *d;
    if (n < 8u)
        return;
    id = ((uint32_t)p[2] | (uint32_t)p[3] << 8 | (uint32_t)p[4] << 16 | (uint32_t)p[5] << 24) ^ ((uint32_t)p[6] << 5 | (uint32_t)p[7] << 13) ^ (p[0] & 15u) << 28;
    id |= 1u;
    for (i = 0; i < SW_N; i++) {
        if (sw[i].id == id) { k = i; break; }
        if (!sw[i].id ? o != 0xFFFFFFFFu : now - sw[i].last > o) { o = sw[i].id ? now - sw[i].last : 0xFFFFFFFFu; j = i; }   /* (a free
                                                         * one first, else the longest unheard) */
    }
    if (k == SW_N) {                                     /* a new device: the oldest's voice (or a free one) */
        d = &sw[j];
        d->id = id;
        d->per = d->jit = 0;
        d->note = 0;
        d->amp = 0;
        {                                                /* (no divide: the data bytes, or the address with none) */
            const uint8_t *q = n > 8u ? p + 8 : p + 2;
            uint32_t m = n > 8u ? n - 8u : 6u, c = 0;
            for (i = 0; i < 8u; i++) {
                d->w[i] = (int8_t)q[c];
                if (++c >= m)
                    c = 0;
            }
        }
    } else {
        uint32_t dt = now - (d = &sw[k])->last;
        if (dt < 4u * SW_TPS) {
            int32_t e = d->per ? (int32_t)dt - (int32_t)d->per : 0;
            d->per = d->per ? d->per + (uint32_t)((int32_t)(dt - d->per) / 8) : dt;
            e = e < 0 ? -e : e;
            d->jit += (((int32_t)((uint64_t)(uint32_t)e * 256u / (d->per | 1u))) - d->jit) / 4;
        }
        for (i = 0; i < 8u && 8u + i < n; i++)           /* the payload moves the wave a little */
            d->w[i] = (int8_t)((d->w[i] * 3 + (int8_t)p[8u + i]) / 4);
    }
    d->last = now;
    d->lv = (uint16_t)((w & 63u) + (w >> 8 & 63u));
    d->ch = (uint8_t)ch;
    d->hit = 1;
}
static __attribute__((noinline)) void ain_swarm(uint32_t n)
{
    uint32_t i, v, now = fm1_ticks(), ref = 0, rlv = 0, any = 0;
    int32_t gl[SW_N], gr[SW_N], a0[SW_N], da[SW_N];
    const track_t *t = &trk[ain_tr % NTRK];
    for (v = 0; v < SW_N; v++)                           /* the reference clock: the loudest device heard */
        if (sw[v].id && sw[v].per && sw[v].lv >= rlv && now - sw[v].last < 2u * SW_TPS)
            rlv = sw[v].lv, ref = sw[v].per;
    for (v = 0; v < SW_N; v++) {
        struct sw_dev *d = &sw[v];
        uint32_t age = now - d->last;
        int32_t lv, tgt, det = 0;
        a0[v] = da[v] = 0;
        if (!d->id)
            continue;
        if (age > 20u * SW_TPS && d->amp < 8) { d->id = 0; continue; }
        lv = d->lv;
        sw_lmin = lv < sw_lmin ? lv : sw_lmin;
        sw_lmax = lv > sw_lmax ? lv : sw_lmax;
        if (!d->note)                                    /* its note: its address, in the track's scale (C1 .. C5) */
            d->note = (uint8_t)clamp(scale_snap(t, 36 + (int32_t)(d->id >> 9) % 48), 24, 96);
        if (ref && d->per)                               /* its clock against the reference's: a detune */
            det = (int32_t)((((uint64_t)d->per << 8) / (ref | 1u)) & 255u) - 128;   /* (divisors never 0: #61) */
        det = det / 32 + (int32_t)((d->jit * (int32_t)((now >> 18) & 7u)) >> 9);   /* (+ the jitter's vibrato) */
        d->inc = pitch_inc((uint32_t)clamp((int32_t)d->note * 16 + det, 0, 2047));
        tgt = age > 6u * SW_TPS ? 0 : 1200 + 9000 * clamp(lv - sw_lmin, 0, 127) / (int32_t)(((uint32_t)(sw_lmax - sw_lmin) & 127u) | 1u);   /* the drone */
        if (d->hit) {                                    /* an advertisement: an accent */
            d->hit = 0;
            d->acc = tgt * 2;
        }
        d->acc -= (d->acc >> 4) + (d->acc > 0);       /* (down to 0) */
        a0[v] = d->amp;
        d->amp += (tgt + d->acc - d->amp) >> 3;
        da[v] = (d->amp - a0[v]) >> CTL_LOG2;           /* (n is CTL) */
        gl[v] = d->ch == 39u ? 14000 : 32767;
        gr[v] = d->ch == 37u ? 14000 : 32767;
        any |= (uint32_t)(d->amp > 8);
    }
    if (!(++sw_tick & 1023u) && sw_lmax - sw_lmin > 2)   /* (the range relaxes, ~0.7 s a step) */
        sw_lmin++, sw_lmax--;
    if (!any) {
        uin_on = 0;
        return;
    }
    for (i = 0; i < n; i++) {
        int32_t l = 0, r = 0;
        for (v = 0; v < SW_N; v++) {
            struct sw_dev *d = &sw[v];
            uint32_t k, f;
            int32_t x;
            if (!a0[v] && !da[v])
                continue;
            k = d->ph >> 29;
            f = (d->ph >> 13) & 0xFFFFu;
            x = (d->w[k] * (int32_t)(65536u - f) + d->w[(k + 1u) & 7u] * (int32_t)f) >> 8;   /* (Q15) */
            x = (x * (a0[v] + da[v] * (int32_t)i)) >> 15;
            l += (x * gl[v]) >> 15;
            r += (x * gr[v]) >> 15;
            d->ph += d->inc;
        }
        uin_l[i] = l;
        uin_r[i] = r;
    }
    uin_on = 1;
}
static void (*const volatile AIN_SWARM)(uint32_t) = ain_swarm;   /* (mod.c macro_master: RAM code) */

static void ain_block(struct track *t) { ain_upd[(uint32_t)(t - trk) % NTRK] = 1; }

/* THRU (voice.c track_render, after the voices): the input into the part, 1 = it sounds */
static __attribute__((noinline)) uint32_t ain_thru(track_t *t, int32_t *out, uint32_t n)
{
    if (t->p[P_E3] || !uin_on || n > CTL)
        return 0;
    ain_fill(t, out, n, 1);
    return 1;
}

/* (called through a pointer: track_render is inlined in RAM code, out of a direct call's reach of the image's end) */
static uint32_t (*const volatile AIN_THRU)(track_t *, int32_t *, uint32_t) = ain_thru;

static void ain_note_on(struct track *t, voice_t *v) { (void)t; v->ph[0] = 0; }
/* KEYS: a note's voice plays the input at its envelope, RING at its pitch */
static void ain_render(struct track *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    int32_t b[CTL], ring = clamp(t->p[P_E5], 0, 127) * 258;
    uint32_t i, ph = v->ph[0];
    if (!t->p[P_E3] || !uin_on || n > CTL)
        return;
    ain_fill(t, b, n, 0);
    for (i = 0; i < n; i++) {
        int32_t x = b[i];
        if (ring) {
            x += mulq15(mulq15(x, osc_sine(ph)) - x, ring);
            ph += m->inc;
        }
        out[i] += mulq15(x, amp_at(m, i));
    }
    v->ph[0] = ph;
}

static const preset_t INPUT_PRESETS[] = {
    {"IN 1", {AIN_IN1, 64, 0, 0, 127, 0, 0, 0}, {0, 64, 127, 20}, 0, TAG(FX) | 1, FX(0, 0, 0, 0)},
    {"IN 2", {AIN_IN2, 64, 0, 0, 127, 0, 0, 0}, {0, 64, 127, 20}, 0, TAG(FX) | 1, FX(0, 0, 0, 0)},
    {"IN 1+2", {AIN_SUM, 64, 0, 0, 127, 0, 0, 0}, {0, 64, 127, 20}, 0, TAG(FX) | 1, FX(0, 0, 0, 0)},
    {"STEREO IN", {AIN_ST, 64, 0, 0, 127, 0, 0, 0}, {0, 64, 127, 20}, 0, TAG(FX) | 1, FX(0, 0, 0, 0)},
    {"MIC GATE", {AIN_IN1, 80, 24, 0, 127, 0, 0, 0}, {0, 64, 127, 20}, 0, TAG(VOX) | 1, FX(0, 0, 15, 25)},
    {"SPACE IN", {AIN_ST, 64, 0, 0, 127, 0, 0, 0}, {0, 64, 127, 20}, 0, TAG(PAD) | 1, FX(0, 30, 35, 60)},
    {"TRANCE GATE", {AIN_ST, 64, 0, 1, 127, 0, 0, 0}, {0, 40, 0, 10}, 0, TAG(FX) | 1, FX(0, 0, 20, 20)},
    {"RING IN", {AIN_IN1, 64, 0, 1, 127, 90, 0, 0}, {0, 64, 127, 30}, 0, TAG(FX) | 1, FX(0, 0, 20, 30)},
    {"CRUSH IN", {AIN_SUM, 72, 0, 0, 127, 0, 90, 60}, {0, 64, 127, 20}, 0, TAG(FX) | 1, FX(10, 0, 0, 15)},
};

static const engine_t ENG_INPUT = {
    .name = "INPUT",
    .page_title = {"INPUT", "SHAPE"},
    .edit = {
        {"SOURCE", F_ENUM, 0, AIN_N - 1, AIN_IN1, N_AIN_SRC, 0},
        {"GAIN", F_INT, 0, 127, 64, 0, 0},
        {"GATE", F_PCT, 0, 127, 0, 0, 0},
        {"PLAY", F_ENUM, 0, 1, 0, N_AIN_PLAY, 0},
        {"WIDTH", F_PCT, 0, 127, 127, 0, 0},
        {"RING", F_PCT, 0, 127, 0, 0, 0},
        {"BITS", F_PCT, 0, 127, 0, 0, 0},
        {"RATE", F_PCT, 0, 127, 0, 0, 0},
    },
    .presets = INPUT_PRESETS,
    .npresets = NELEM(INPUT_PRESETS),
    .poly = 1,
    .note_on = ain_note_on,
    .render = ain_render,
    .block = ain_block,
    .knob = {P_E0, P_E1, P_E2, P_E3},
};
