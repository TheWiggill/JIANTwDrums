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
