/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* (0.9.9-snd1) The FILTER after the voices (src/voice.c track_filter, tf_run, track_comb) on the host, fed directly
 * (a saw, noise, a sine at the part's newest note; its voices faked: the comb follows the newest one):
 * 1. every TYPE (LP HP BP COMB COMB- MS20) at RES 0 / 100 / 127, CUT 0 / 64 / 127, from C1 to C8: bounded (no sample at
 *    full scale for more than a few in a row), no lockup (silence in: from 1.5 s on silent, or under 70 % of where its
 *    tail began: COMB's longest ring is ~9 s; about its mean: the SVF's fixed point holds a DC at CUT 0), the part's own-filter
 *    engines (ANALOG, LOFI) and the others;
 * 2. COMB tuned to the note (CUT 64): the autocorrelation of noise through it peaks at the note's period (+-1 %),
 *    across the keyboard; COMB- has its first positive peak there too and a negative one at half of it (hollow);
 * 3. no zipper: CUT swept over a held sine, the second difference at the blocks' first samples no larger than inside
 *    them (the line glided, was a step a block);
 * 4. RES on a curve: the ring after a burst longer as RES rises, near self-oscillation at the top (still ringing a
 *    second later) and bounded;
 * 5. MS20: the ring longer as RES rises; a full-scale saw at RES 127, every CUT, C1..C7: under full scale and no
 *    more than 6x RES 0's level (it screams, the clip holding it);
 * demos (argv[1], default build/filter_demo): comb_sweep.wav, comb_neg_sweep.wav, ms20_sweep.wav. */
#include <stdarg.h>
#define main hostsim_main
#include "hostsim.c"
#undef main

static int fails;
static void check(const char *what, int ok, const char *fmt, ...)
{
    printf("filter: %-70s %s", what, ok ? "ok" : "FAIL");
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        printf("  (");
        vprintf(fmt, ap);
        printf(")");
        va_end(ap);
    }
    printf("\n");
    fails += !ok;
}

#define MAXN (FS * 8u)
static int32_t buf[MAXN];
static uint32_t nz = 12345u;
static int32_t noise(void)
{
    nz = nz * 1664525u + 1013904223u;
    return (int32_t)(nz >> 16) - 32768;
}

/* part 1 as engine e, the FILTER at TYPE ft CUT RES (ANALOG's: E4 E5), a voice faked at note */
static track_t *part(uint32_t e, int ft, int cut, int res, int note)
{
    track_t *t = &trk[0];
    uint32_t i;
    host_tracks_init();
    host_preset(t, e, 0);
    for (i = 0; i < NVOICE; i++)
        t->v[i].active = 0;
    memset(tf_on, 0, sizeof tf_on); memset(tf_cmb, 0, sizeof tf_cmb); memset(tf_xb, 0, sizeof tf_xb);
    memset(tf_ft, 0, sizeof tf_ft);
    t->p[P_FTYPE] = (int16_t)ft;
    t->p[P_FCUT] = t->p[P_E4] = (int16_t)cut;
    t->p[P_FRES] = t->p[P_E5] = (int16_t)res;
    t->p[P_LD_FLT] = t->p[P_ED_FLT] = 0;
    t->v[0].active = 1;
    t->v[0].age = 1;
    t->v[0].stage = 0;
    t->v[0].pitch_cur = (int16_t)(note * 16);
    return t;
}
/* n samples of the input kind (0 silence, 1 saw at note, 2 noise, 3 sine at f Hz) through the filter into buf; a
 * per-block hook may move CUT */
static double sph;
static void run(track_t *t, uint32_t kind, int note, double f, uint32_t n, int32_t amp, void (*hook)(track_t *, uint32_t))
{
    uint32_t k, i;
    double hz = 440.0 * pow(2.0, (note - 69) / 12.0);
    for (k = 0; k < n; k += CTL) {
        int32_t o[CTL];
        if (hook)
            hook(t, k);
        hz = 440.0 * pow(2.0, (t->v[0].pitch_cur / 16.0 - 69) / 12.0);   /* (the saw at the voice's note) */
        for (i = 0; i < CTL; i++) {
            double x = 0;
            if (kind == 1) { sph += hz / FS; sph -= floor(sph); x = 2 * sph - 1; }
            if (kind == 2) x = noise() / 32768.0;
            if (kind == 3) { sph += f / FS; x = sin(2 * M_PI * sph); }
            o[i] = (int32_t)(x * amp);
        }
        track_post(t, ENGINES[t->engine], o, CTL);
        for (i = 0; i < CTL && k + i < MAXN; i++)
            buf[k + i] = o[i];
    }
}
/* the largest swing about the mean (a DC the SVF's fixed point holds at CUT 0 is no sound) */
static int32_t swing(uint32_t a, uint32_t b)
{
    double m = 0;
    int32_t p = 0;
    uint32_t i;
    for (i = a; i < b; i++)
        m += buf[i];
    m /= (b - a);
    for (i = a; i < b; i++)
        p = fabs(buf[i] - m) > p ? (int32_t)fabs(buf[i] - m) : p;
    return p;
}
static int32_t peak(uint32_t a, uint32_t b)
{
    int32_t p = 0;
    uint32_t i;
    for (i = a; i < b; i++)
        p = abs(buf[i]) > p ? abs(buf[i]) : p;
    return p;
}
static double rms(uint32_t a, uint32_t b)
{
    double s = 0;
    uint32_t i;
    for (i = a; i < b; i++)
        s += (double)buf[i] * buf[i];
    return sqrt(s / (b - a));
}
/* the longest run of samples at or past full scale */
static uint32_t pegged(uint32_t a, uint32_t b)
{
    uint32_t i, r = 0, m = 0;
    for (i = a; i < b; i++) {
        r = abs(buf[i]) >= 32767 ? r + 1 : 0;
        m = r > m ? r : m;
    }
    return m;
}
static double acorr(uint32_t a, uint32_t b, uint32_t lag)
{
    double s = 0, e = 0;
    uint32_t i;
    for (i = a; i + lag < b; i++) {
        s += (double)buf[i] * buf[i + lag];
        e += (double)buf[i] * buf[i];
    }
    return e > 0 ? s / e : 0;
}
static void save(const char *dir, const char *name, uint32_t n)
{
    char path[512];
    FILE *f;
    uint32_t i;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    if (!(f = fopen(path, "wb")))
        return;
    wav_hdr(f, n);
    for (i = 0; i < n; i++)
        wav_put(f, buf[i] / 2, buf[i] / 2);
    fclose(f);
}

static const char *const TN[] = {"LP", "HP", "BP", "COMB", "COMB-", "MS20", "LP24"};   /* (V10.22: LP24) */
#define NT ((int)(sizeof TN / sizeof TN[0]))
static int sw_from, sw_to;
static void sweep(track_t *t, uint32_t k)
{
    int32_t c = sw_from + (int32_t)((int64_t)(sw_to - sw_from) * (int32_t)k / (int32_t)(FS * 2u));
    t->p[P_FCUT] = t->p[P_E4] = (int16_t)clamp(c, 0, 127);
}
static void sweep_demo(track_t *t, uint32_t k)
{
    uint32_t seg = k / (FS * 2u);                       /* RES 40 / 90 / 120 / 127, CUT 40 .. 88 each */
    static const int16_t R[4] = {40, 90, 120, 127};
    t->p[P_FRES] = t->p[P_E5] = R[seg & 3u];
    t->p[P_FCUT] = t->p[P_E4] = (int16_t)(40 + (int32_t)((k % (FS * 2u)) * 48u / (FS * 2u)));
    if (k % (FS / 2u) == 0)                             /* a new note each half second (C2 G2 C3 D#3 ..) */
        t->v[0].pitch_cur = (int16_t)((36 + (int)((k / (FS / 2u)) * 7u % 24u)) * 16);
}

static void sweep_ms20(track_t *t, uint32_t k)
{
    static const int16_t R[4] = {40, 90, 120, 127};
    static const uint8_t RIFF[8] = {36, 36, 48, 36, 39, 36, 43, 46};
    uint32_t seg = k / (FS * 2u), ph = k % (FS * 2u);
    t->p[P_FRES] = t->p[P_E5] = R[seg & 3u];
    t->p[P_FCUT] = t->p[P_E4] = (int16_t)(30 + (int32_t)((ph < FS ? ph : FS * 2u - ph) * 80u / FS));
    t->v[0].pitch_cur = (int16_t)(RIFF[(k / (FS / 8u)) & 7u] * 16);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "build/filter_demo";
    static const uint32_t ENG[3] = {ENGI_PHASE, ENGI_ANALOG, ENGI_LOFI};
    static const int RES[3] = {0, 100, 127}, CUT[3] = {0, 64, 127};
    char what[160];
    int ft, ri, ci, note, ok, e;

    /* 1 */
    for (ft = 0; ft < NT; ft++) {
        int32_t pk = 0;
        uint32_t pg = 0, lock = 0;
        ok = 1;
        for (e = 0; e < 3; e++)
            for (ri = 0; ri < 3; ri++)
                for (ci = 0; ci < 3; ci++)
                    for (note = 24; note <= 108; note += 12) {
                        track_t *t = part(ENG[e], ft, CUT[ci], RES[ri], note);
                        int32_t p0;
                        run(t, 1, note, 0, FS / 4u, 12000, 0);
                        pk = peak(0, FS / 4u) > pk ? peak(0, FS / 4u) : pk;
                        pg = pegged(0, FS / 4u) > pg ? pegged(0, FS / 4u) : pg;
                        run(t, 0, note, 0, 2u * FS, 0, 0);
                        p0 = swing(0, FS / 4u);
                        pg = pegged(0, 2u * FS) > pg ? pegged(0, 2u * FS) : pg;
                        if (swing(FS * 3u / 2u, 2u * FS) > 64 && swing(FS * 3u / 2u, 2u * FS) * 10 > p0 * 7) {
                            lock++;
                            if (getenv("VERBOSE"))
                                printf("  %s e%u res %d cut %d note %d: %d after 1.5 s\n", TN[ft], ENG[e], RES[ri], CUT[ci], note, swing(FS * 3u / 2u, 2u * FS));
                        }
                    }
        snprintf(what, sizeof what, "%s at RES 0..127, CUT 0..127, C1..C8, 3 engines: bounded, dying out after", TN[ft]);
        check(what, pg <= 4u && !lock, "peak %d, run at full scale %u, %u still sounding after 2 s", pk, pg, lock);
    }

    /* 2 */
    for (ft = 3; ft <= 4; ft++) {
        double worst = 0, neg = 1;
        for (note = 36; note <= 84; note += 12) {
            track_t *t = part(ENGI_PHASE, ft, 64, 110, note);
            double per = FS / (440.0 * pow(2.0, (note - 69) / 12.0)), best = -2, err;
            uint32_t lag, bl = 0;
            run(t, 2, note, 0, FS, 8000, 0);
            for (lag = (uint32_t)(per * 0.8); lag <= (uint32_t)(per * 1.2) + 1u; lag++) {
                double c = acorr(FS / 4u, FS, lag);
                if (c > best) { best = c; bl = lag; }
            }
            err = fabs(bl - per) / per;
            worst = err > worst ? err : worst;
            if (ft == 4) {
                double h = acorr(FS / 4u, FS, (uint32_t)(per / 2 + 0.5));
                neg = h < neg ? h : neg;
            }
        }
        snprintf(what, sizeof what, "%s tuned to the note (noise in, CUT 64): the period's peak, C2..C6", TN[ft]);
        check(what, worst < 0.012 && (ft == 3 || neg < -0.2), "worst %.2f %%%s", worst * 100,
              ft == 4 ? ", the half period's correlation negative" : "");
    }

    /* 3 */
    for (ft = 3; ft <= 4; ft++) {
        track_t *t = part(ENGI_PHASE, ft, 40, 100, 48);
        double bd = 0, id = 0;
        uint32_t i, nb = 0, ni = 0;
        sw_from = 40; sw_to = 90;
        run(t, 3, 48, 220.0, FS * 2u, 12000, sweep);
        for (i = FS / 4u; i < FS * 2u; i++) {
            double d2 = fabs((double)buf[i] - 2.0 * buf[i - 1] + buf[i - 2]);
            if (i % CTL == 0) { bd += d2; nb++; } else { id += d2; ni++; }
        }
        bd /= nb; id /= ni;
        snprintf(what, sizeof what, "%s: CUT swept 40 -> 90 over a sine, no zipper at the blocks' edges", TN[ft]);
        check(what, bd < id * 1.5, "2nd difference at the edges %.1f, inside %.1f", bd, id);
    }

    /* 4 */
    {
        double prev = 0;
        int mono = 1, r;
        int32_t pk = 0;
        for (r = 0; r <= 127; r += 127 / 4) {
            track_t *t = part(ENGI_PHASE, 3, 64, r, 57);
            double tail;
            run(t, 2, 57, 0, FS / 20u, 12000, 0);
            run(t, 0, 57, 0, FS * 3u / 2u, 0, 0);
            tail = rms(FS / 4u, FS / 2u);
            pk = peak(0, FS * 3u / 2u) > pk ? peak(0, FS * 3u / 2u) : pk;
            if (tail < prev)
                mono = 0;
            if (getenv("VERBOSE"))
                printf("  RES %d: tail %.0f\n", r, tail);
            prev = tail;
        }
        check("COMB's ring after a burst longer as RES rises, still ringing at RES 127", mono && rms(FS, FS * 3u / 2u) > 30,
              "a second on at 127: rms %.0f, peak %d", rms(FS, FS * 3u / 2u), pk);
    }

    /* 5 */
    {
        double prev = 0, lo = 1e9, hi = 0;
        int mono = 1, r, c;
        int32_t pk = 0;
        for (r = 0; r <= 127; r += 127 / 4) {
            track_t *t = part(ENGI_PHASE, 5, 50, r, 57);
            double tail;
            run(t, 2, 57, 0, FS / 20u, 12000, 0);
            run(t, 0, 57, 0, FS / 2u, 0, 0);
            tail = rms(FS / 50u, FS / 10u);
            if (tail < prev)
                mono = 0;
            prev = tail;
        }
        check("MS20's ring after a burst longer as RES rises", mono, "at 127: rms %.0f 20 .. 100 ms on", prev);
        for (c = 20; c <= 120; c += 20)                 /* a full-scale saw at RES 127, against RES 0 */
            for (note = 24; note <= 96; note += 24) {
                track_t *t = part(ENGI_ANALOG, 5, c, 127, note);
                double a, b;
                run(t, 1, note, 0, FS / 4u, 32000, 0);
                pk = peak(0, FS / 4u) > pk ? peak(0, FS / 4u) : pk;
                b = rms(FS / 8u, FS / 4u);
                t = part(ENGI_ANALOG, 5, c, 0, note);
                run(t, 1, note, 0, FS / 4u, 32000, 0);
                a = rms(FS / 8u, FS / 4u);
                if (a > 100) {
                    lo = b / a < lo ? b / a : lo;
                    hi = b / a > hi ? b / a : hi;
                }
            }
        check("MS20 screams but controlled: a full-scale saw at RES 127, every CUT, C1..C7: under full scale",
              pk < 32767 && hi < 6.0, "peak %d, level against RES 0 x%.2f .. x%.2f", pk, lo, hi);
    }

    /* demos */
    {
        char cmd[600];
        snprintf(cmd, sizeof cmd, "mkdir -p '%s'", dir);
        if (system(cmd)) {}
        for (ft = 3; ft <= 4; ft++) {
            track_t *t = part(ENGI_PHASE, ft, 40, 40, 36);
            sph = 0;
            run(t, 1, 36, 0, FS * 8u, 9000, sweep_demo);
            save(dir, ft == 3 ? "comb_sweep.wav" : "comb_neg_sweep.wav", FS * 8u);
        }
        {   /* MS20: a saw riff, CUT swept up and down each 2 s, RES 40 / 90 / 120 / 127 */
            track_t *t = part(ENGI_PHASE, 5, 40, 40, 36);
            sph = 0;
            run(t, 1, 36, 0, FS * 8u, 14000, sweep_ms20);
            save(dir, "ms20_sweep.wav", FS * 8u);
        }
    }
    printf("filter: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
