/* SPDX-License-Identifier: GPL-3.0-only
 * JIANT V10.6: SEQ > EUCLID and MODES, a pattern generator in MUTATE's place. Included by ui_graph.c.
 * EUCLID: KNOB 1 HITS, 2 STEPS, 3 ROTATE spread the hits as evenly as the steps allow (the euclidean rhythm); KNOB 4 a
 * DRUM track's LANE (each of its eight sounds its own HITS / STEPS / ROTATE, the pattern their least common length up to
 * 64), a melodic track's VARY. MODES: four amounts that add to what is generated, 0 none:
 *   DRUM    ACCENT (a second euclid of accents), GHOST (soft hits between), RATCHET (x2 / x4 hits), VARY
 *   melodic MOTION (how far the melody moves), HARMONY (chords of the scale on strong beats), OCTAVE, SLIDE
 * VARY loosens the even grid: some hits move a step (towards the offbeat, more in the second half: it answers the
 * first), a pickup before a strong beat, a hit left out; on a melodic track notes held over the gap after them.
 * A melodic track's notes: its SCL (ROOT, SCALE) when one is set; else the key the other tracks' notes fit best (the
 * screen says FROM Tn); else a scale and root of the seed's (NEW). Turning a knob writes the whole pattern again (the
 * steps of the track; its automation stays); a new gesture after two seconds is an UNDO point. OCT+ a new variation
 * (the seed), OCT- the one before. The settings are the session's (RAM), the pattern is the track's as any other */
#define EU_M 4u
typedef struct { uint8_t h, n, r; } eul_t;
typedef struct { uint8_t on, lane, src, root, ctr, vary; uint16_t mask, prev, seed; uint8_t m[EU_M]; eul_t a, l[NLANE];
                 uint8_t mu[4], bar, flip; } eu_t;   /* lane 0 ALL (a: DENSITY, STEPS, ROTATE), 1.. a lane; (V10.16) mu: MUTATE's
                                                         * EVERY AMOUNT MODE WHAT, bar: the bar it last mutated on, flip: PULSE's */
#define EU_Z0 __builtin_offsetof(eu_t, vary)                     /* (V10.16) the settings a mutation changes: vary .. l[], the anchor */
#define EU_ZN (__builtin_offsetof(eu_t, mu) - EU_Z0)
static eu_t eu[NTRK];
static uint8_t eu_z[NTRK][EU_ZN];                       /* (V10.16) each track's anchor: what MUTATE orbits, OCT- returns to */
static uint32_t eu_s, eu_ms;
enum { EK_HITS, EK_STEPS, EK_ROT, EK_K4, EK_M0, EK_U0 = EK_M0 + EU_M, EK_N = EK_U0 + 4 };
static const uint8_t EU_MX[4] = {5, 100, 2, 3};          /* (V10.16) MUTATE's settings' tops */
static void eu_anchor(uint32_t k) { memcpy(eu_z[k % NTRK], (uint8_t *)&eu[k % NTRK] + EU_Z0, EU_ZN); }

static uint32_t eu_rnd(void) { eu_s ^= eu_s << 13; eu_s ^= eu_s >> 17; eu_s ^= eu_s << 5; return eu_s; }
static uint32_t eu_ch(uint32_t pct) { return eu_rnd() % 100u < pct; }
static uint32_t eu_hit(uint32_t i, const eul_t *l) { uint32_t n = l->n ? l->n : 1u, j = (i + n - l->r % n) % n; return l->h && (j * l->h) % n < l->h; }

/* (V10.7) ALL: the kit a DENSITY (1..8) makes, in sixteen (scaled to STEPS): the kick, the backbeat, the hats, the rest
 * sparse; ROTATE turns the kick and the percussion (the groove), the snare and the hats stay */
static void eu_kit(eu_t *e)
{
    static const uint8_t KT[8][NLANE] = {{2, 0, 0, 4, 0, 0, 0, 0}, {3, 2, 0, 4, 0, 0, 0, 0}, {4, 2, 0, 8, 0, 0, 0, 0},
        {4, 2, 1, 8, 2, 0, 0, 0}, {5, 2, 1, 8, 2, 0, 3, 0}, {5, 2, 1, 12, 2, 2, 3, 0}, {6, 3, 1, 12, 3, 2, 5, 2},
        {7, 4, 2, 16, 4, 3, 5, 3}};
    static const uint8_t KR[NLANE] = {0, 4, 12, 0, 6, 10, 3, 7};
    uint32_t i, n = e->a.n ? e->a.n : 16u, d = (e->a.h ? e->a.h - 1u : 0u) & 7u, h;
    for (i = 0; i < NLANE; i++) {
        h = (KT[d][i] * n + 8u) / 16u;
        e->l[i].h = (uint8_t)(h > n ? n : KT[d][i] && !h ? 1u : h);
        e->l[i].n = (uint8_t)n;
        e->l[i].r = (uint8_t)(((KR[i] * n + 8u) / 16u + ((0xE1u >> i) & 1u ? e->a.r : 0u)) % n);
    }
}

/* track k's settings, made the first time (a kit: ALL, DENSITY 4; a melody of five in sixteen) */
static eu_t *eu_of(uint32_t k)
{
    eu_t *e = &eu[k % NTRK];
    if (!e->on) {
        e->on = 1;
        e->seed = e->prev = (uint16_t)(1u + k * 7u);
        e->vary = 40;
        e->a.h = 4; e->a.n = 16;
        eu_kit(e);
        if (drum_track(&trk[k])) { e->m[0] = 40; e->m[3] = 30; }
        else { e->l[0].h = 5; e->m[0] = 45; e->m[1] = 20; e->m[2] = 10; e->m[3] = 15; }
        e->mu[1] = 50;
        eu_anchor(k);
    }
    return e;
}

/* VARY on hits on[] of n steps (v 0..100): hits moved one to three steps, never added or dropped (V10.7: HITS is what
 * sounds); len[] (0: none) the steps each note holds. (V10.16) The pattern is VARY's turns of the cycle (eu_turns), each
 * moved anew: the first near the grid, the last the most (A A' A'' B) */
static void eu_vary(uint8_t *on, uint8_t *len, uint32_t n, uint32_t v)
{
    uint32_t i, g;
    for (i = 1; i < n; i++) {
        if (on[i] != 1u || !eu_ch(v * (i >= n / 2u ? 10u : 6u) / 10u)) continue;
        g = 1u + eu_ch(v / 2u) + eu_ch(v / 4u);          /* a step on (to the offbeat mostly) or back, further the more */
        g = eu_ch((i & 1u) ? 50 : 60) ? i + g : i - g;
        if (g > 0u && g < n && !on[g]) { on[i] = 0; on[g] = 2; }
    }
    for (i = 0; len && i < n; i++) {
        len[i] = 1;
        if (!on[i]) continue;
        for (g = 1; i + g < n && !on[i + g] && g < 4u; g++) {}
        if (g > 1u && eu_ch(v * 7u / 10u)) len[i] = (uint8_t)(2u + eu_rnd() % (g - 1u));
    }
}

/* (V10.16) VARY's turns: as many cycles of n (up to four, 64 steps) as the pattern holds with VARY on; turn c of nc its
 * amount: the first a third, the last half as much again */
static uint32_t eu_turns(uint32_t n, uint32_t v) { return !v ? 1u : n * 4u <= 64u ? 4u : n * 2u <= 64u ? 2u : 1u; }
static uint32_t eu_vc(uint32_t v, uint32_t c, uint32_t nc)
{
    return nc < 2u ? v : !c ? v / 3u : c + 1u == nc ? (v * 3u / 2u > 100u ? 100u : v * 3u / 2u) : v;
}

/* the key of melodic track k: SCL's, else the one the other tracks' notes fit (src 1 + the track with the most), else
 * the seed's (NEW); e->root and e->mask */
static void eu_key(uint32_t k, eu_t *e)
{
    static const uint8_t TRY[7] = {1, 2, 3, 4, 8, 9, 7}, NEWS[6] = {2, 1, 3, 6, 8, 9};   /* MAJ MIN DOR MIX PHRY LYD HARM */
    const track_t *t = &trk[k];
    uint32_t hist[12] = {0}, best = 0, most = 0, tot = 0, j, i, r, c;
    int32_t sc, top = -0x7FFFFFFF;
    if (t->p[P_SCALE] > 0) { e->root = (uint8_t)(t->p[P_ROOT] % 12); e->mask = (uint16_t)scale_mask(t); e->src = 0; return; }
    for (j = 0; j < trk_live && j < NTRK; j++) {
        uint32_t n = 0, len = (uint32_t)clamp(trk[j].p[P_SLEN], 1, NSTEP);
        if (j == k || drum_track(&trk[j])) continue;
        for (i = 0; i < len; i++) {
            const step_t *s = &trk[j].step[i];
            if (s->time != ST_NOTE) continue;
            for (c = 0; c < s->n && c < 4u; c++, n++) hist[s->note[c] % 12u] += (i & 3u) ? 1u : 2u;
        }
        if (n > most) { most = n; best = j; }
        tot += n;
    }
    if (tot >= 4u) {
        for (r = 0; r < 12u; r++)
            for (i = 0; i < 7u; i++) {
                uint32_t m = SCALE_MASK[TRY[i]];
                for (sc = (int32_t)hist[r], c = 0; c < 12u; c++)
                    sc += (m >> ((c + 12u - r) % 12u)) & 1u ? 2 * (int32_t)hist[c] : -4 * (int32_t)hist[c];
                if (sc > top) { top = sc; e->root = (uint8_t)r; e->mask = (uint16_t)m; }
            }
        e->src = (uint8_t)(1u + best);
        return;
    }
    eu_s = e->seed * 2654435761u + 11u;
    e->root = (uint8_t)(eu_rnd() % 12u);
    e->mask = (uint16_t)SCALE_MASK[NEWS[eu_rnd() % 6u]];
    e->src = 7;
}

/* scale degree d (any integer) of the key from root pitch base: its note */
static int32_t eu_deg(const uint8_t *pc, uint32_t np, int32_t base, int32_t d)
{
    int32_t o = d >= 0 ? d / (int32_t)np : -((-d + (int32_t)np - 1) / (int32_t)np);
    return base + 12 * o + pc[d - o * (int32_t)np];
}

/* the kit (only 0: ALL, its lanes' least common length) or lane only-1 alone (the others as they are, the pattern
 * as long as both need, up to 64) */
static void eu_drums(track_t *t, eu_t *e, uint32_t only)
{
    uint8_t on[64];
    uint32_t l, i, j, n, old = (uint32_t)clamp(t->p[P_SLEN], 1, 64), len = only ? old : 1u;
    for (l = 0; l < NLANE; l++) {                        /* the pattern: the lanes' least common length, up to 64 */
        uint32_t a = len, b = e->l[l].n;
        if (!e->l[l].h || (only && l != only - 1u)) continue;
        while (b) { uint32_t x = a % b; a = b; b = x; }
        len = len / a * e->l[l].n > 64u ? 64u : len / a * e->l[l].n;
    }
    if (len < 2u) len = 16;
    if (!only) len *= eu_turns(len, e->m[3]);           /* (V10.16) VARY's turns */
    for (i = 0; i < NSTEP; i++)
        if (!only) step_clear(&t->step[i]);
        else {
            if (i >= old && i < len) t->step[i] = t->step[i % old];   /* (longer: the other lanes repeat) */
            grid_hit(t, i, only - 1u, 0);
        }
    t->p[P_SLEN] = (int16_t)len;
    for (l = only ? only - 1u : 0u; l < (only ? only : NLANE); l++) {
        const eul_t *L = &e->l[l];
        eul_t acc = {(uint8_t)(L->h * e->m[0] / 100u + (e->m[0] && L->h ? 1u : 0u)), L->n, (uint8_t)(L->r + 1u)};
        uint32_t c, nc, v = l ? e->m[3] : e->m[3] / 2u, fill = 0;
        if (!L->h || !(n = L->n)) continue;
        nc = (len + n - 1u) / n;
        for (i = 0; i < len; i++) {
            if (!(i % n)) {                              /* (V10.16) each turn of the cycle moved anew */
                c = i / n;
                eu_s = e->seed * 2654435761u + l * 97u + c * 7919u + 1u;
                for (j = 0; j < n; j++) on[j] = (uint8_t)eu_hit(j, L);
                eu_vary(on, 0, n, eu_vc(v, c, nc));
                fill = nc > 1u && c + 1u == nc && l ? v / 3u : 0u;   /* (the last turn's end: rolls) */
            }
            if (on[i % n]) {
                grid_hit(t, i, l, 1);
                if (e->m[0] && eu_hit(i % n, &acc)) grid_acc(t, i, l, 1);
                if (eu_ch(e->m[2] / 2u + (i % n >= n * 3u / 4u ? fill : 0u))) grid_hit_set(t, i, l, 1u + (eu_ch(70) ? LM_X2 : LM_X4));
            } else if (l && eu_ch(e->m[1] * 35u / 100u)) {
                grid_hit_set(t, i, l, 1u + LM_GHOST);
            }
        }
    }
    for (i = 0; !only && i < len; i++)                   /* (ALL: the open hat chokes the closed one) */
        if ((step_lanes(&t->step[i]) >> 4) & 1u) grid_hit(t, i, 3, 0);
}

static void eu_melody(uint32_t k, track_t *t, eu_t *e)
{
    uint8_t on[64], ln[64], pc[12];
    int8_t mem[64] = {0};                                   /* (V10.16) the first turn's degrees, hit by hit: the motif */
    uint32_t n = e->l[0].n ? e->l[0].n : 16u, i, c, np = 0, g = 0, cy, nc = eu_turns(n, e->vary), o, j, vc;
    int32_t ctr = 0, base, deg = 0, mo = e->m[0], ha = e->m[1];
    for (i = 0; !e->ctr && i < (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP); i++)   /* the register: the notes it had at the
                                                         * first turn (none: C4), kept */
        if (t->step[i].time == ST_NOTE && t->step[i].n) { ctr += t->step[i].note[0]; g++; }
    if (!e->ctr) e->ctr = (uint8_t)clamp(g ? ctr / (int32_t)g : 60, 36, 84);
    ctr = e->ctr;
    eu_key(k, e);
    for (c = 0; c < 12u; c++)
        if ((e->mask >> c) & 1u) pc[np++] = (uint8_t)c;
    if (!np) { pc[0] = 0; np = 1; }
    base = ctr - ((ctr - e->root + 120) % 12);           /* the root at or below the middle */
    for (i = 0; i < NSTEP; i++) step_clear(&t->step[i]);
    t->p[P_SLEN] = (int16_t)(n * nc);
    eu_s = e->seed * 2654435761u + k * 131u + 7u;
    for (cy = 0; cy < nc; cy++) {                        /* (V10.16) VARY's turns: A, then the motif answered */
        o = cy * n;
        vc = eu_vc(e->vary, cy, nc);
        for (i = 0; i < n; i++) on[i] = (uint8_t)eu_hit(i, &e->l[0]);
        eu_vary(on, ln, n, vc);
        for (i = 0, j = 0; i < n; i++) {
            step_t *s = &t->step[o + i];
            int32_t note, sh = 0;
            if (!on[i]) continue;
            if (cy && j < 64u && !eu_ch(vc * 6u / 10u)) deg = mem[j];   /* the motif's note, or a new one */
            else if (i) {
                int32_t span = mo * 5 / 100, d = span ? (int32_t)(eu_rnd() % (uint32_t)(2 * span + 1)) - span : 0;
                if (!(i & 3u) && eu_ch(50)) d = -deg + (eu_ch(50) ? 0 : (np >= 7u ? 4 : 3));   /* home or the fifth */
                deg = clamp(deg + d, -3, 9);
            } else if (!cy) deg = 0;
            if (!cy && j < 64u) mem[j] = (int8_t)deg;
            j++;
            if (eu_ch(e->m[2] * 6u / 10u)) sh = eu_ch(50) ? 12 : -12;
            note = clamp(eu_deg(pc, np, base, deg) + sh, 24, 108);
            s->time = ST_NOTE;
            s->vel = 100;
            s->note[0] = (uint8_t)note;
            s->n = 1;
            if (!(i & 3u) && eu_ch((uint32_t)ha)) {      /* a chord of the scale on a strong beat */
                s->note[s->n++] = (uint8_t)clamp(eu_deg(pc, np, base, deg + 2) + sh, 24, 108);
                if (ha > 50) s->note[s->n++] = (uint8_t)clamp(eu_deg(pc, np, base, deg + 4) + sh, 24, 108);
            }
            if (eu_ch(e->m[3] * 7u / 10u)) s->flags |= SF_SLIDE;
            for (c = 1; c < ln[i] && i + c < n; c++) {   /* held over the gap */
                t->step[o + i + c].time = ST_TIE;
                t->step[o + i + c].n = 0;
            }
        }
    }
}

/* the pattern of track k written again from its settings */
static void eu_gen(uint32_t k, uint32_t hand)
{
    track_t *t = &trk[k % NTRK];
    eu_t *e = eu_of(k);
    if (hand && fm1_ms - eu_ms > 2000u) undo_mark();     /* (a new gesture: an UNDO point; MUTATE's turns none) */
    eu_ms = hand ? fm1_ms : eu_ms;
    load_begin(t, LOAD_PAT);
    if (drum_track(t)) eu_drums(t, e, e->lane % (NLANE + 1u)); else eu_melody(k, t, e);
    load_end(t);
    pat_sig[k % NTRK] = ~steps_sig(t);                   /* (the user's steps now) */
    ui.force = 1;
}

/* the knob views (params.c page_desc: ids ID_EU0 + EK_*): their values and descriptions for the selected track */
static int16_t eu_v[EK_N];
static param_desc_t eu_d;
static const param_desc_t *eu_desc(uint32_t k, int16_t **valp)
{
    static const char *const LN[NLANE + 1] = {"ALL", "BD", "SD", "CP", "CH", "OH", "TM", "RS", "CB"};
    static const char *const MD[2][EU_M] = {{"MOTION", "HARMONY", "OCTAVE", "SLIDE"}, {"ACCENT", "GHOST", "RATCHET", "VARY"}};
    static const char *const MU[3][6] = {{"OFF", "1 BAR", "2 BARS", "4 BARS", "8 BARS", "16 BARS"}, {"ORBIT", "DRIFT", "PULSE"},
                                         {"GROOVE", "RHYTHM", "FEEL", "ALL"}};
    static const char *const ML[4] = {"EVERY", "AMOUNT", "MODE", "WHAT"};
    eu_t *e = eu_of(song.sel);
    uint32_t dr = drum_track(TSEL) != 0;
    uint32_t al = dr && !(e->lane % (NLANE + 1u));
    eul_t *L = al ? &e->a : &e->l[dr ? (e->lane + NLANE) % (NLANE + 1u) : 0];
    k %= EK_N;
    eu_d.fmt = F_INT; eu_d.min = 0; eu_d.def = 0; eu_d.names = 0; eu_d.unit = 0;
    if (k == EK_HITS && al) { eu_d.label = "DENSITY"; eu_d.min = 1; eu_d.max = 8; eu_v[k] = L->h; }
    else if (k == EK_HITS) { eu_d.label = "HITS"; eu_d.max = L->n; eu_v[k] = L->h; }
    else if (k == EK_STEPS) { eu_d.label = "STEPS"; eu_d.min = 2; eu_d.max = 64; eu_v[k] = L->n; }
    else if (k == EK_ROT) { eu_d.label = "ROTATE"; eu_d.max = L->n > 1u ? L->n - 1 : 0; eu_v[k] = L->r; }
    else if (k == EK_K4 && dr) { eu_d.label = "LANE"; eu_d.fmt = F_ENUM; eu_d.max = NLANE; eu_d.names = LN; eu_v[k] = e->lane; }
    else if (k == EK_K4) { eu_d.label = "VARY"; eu_d.max = 100; eu_v[k] = e->vary; }
    else if (k < EK_U0) { eu_d.label = MD[dr][k - EK_M0]; eu_d.max = 100; eu_v[k] = e->m[k - EK_M0]; }
    else {                                               /* (V10.16) MUTATE */
        k -= EK_U0;
        eu_d.label = ML[k]; eu_d.max = EU_MX[k]; eu_v[k + EK_U0] = e->mu[k];
        if (k != 1u) { eu_d.fmt = F_ENUM; eu_d.names = MU[k ? k - 1u : 0u]; }
        k += EK_U0;
    }
    *valp = &eu_v[k];
    return &eu_d;
}
/* knob k turned to v: the setting, then (not LANE) the pattern again */
static void eu_put(uint32_t k, int32_t v)
{
    eu_t *e = eu_of(song.sel);
    uint32_t dr = drum_track(TSEL) != 0;
    uint32_t al = dr && !(e->lane % (NLANE + 1u));
    eul_t *L = al ? &e->a : &e->l[dr ? (e->lane + NLANE) % (NLANE + 1u) : 0];
    if (k == EK_K4 && dr) { e->lane = (uint8_t)clamp(v, 0, NLANE); ui.force = 1; return; }
    if (k >= EK_U0) {                                    /* (V10.16) MUTATE: its settings (EVERY on: the anchor now) */
            if (k == EK_U0 && !e->mu[0] && v > 0) eu_anchor(song.sel);
        e->mu[k - EK_U0] = (uint8_t)clamp(v, 0, EU_MX[k - EK_U0]);
        ui.force = 1;
        return;
    }
    if (k == EK_HITS) L->h = (uint8_t)clamp(v, al, al ? 8 : L->n);
    else if (k == EK_STEPS) { L->n = (uint8_t)clamp(v, 2, 64); if (!al && L->h > L->n) L->h = L->n; if (L->r >= L->n) L->r = 0; }
    else if (k == EK_ROT) L->r = (uint8_t)clamp(v, 0, L->n - 1);
    else if (k == EK_K4) e->vary = (uint8_t)clamp(v, 0, 100);
    else e->m[(k - EK_M0) % EU_M] = (uint8_t)clamp(v, 0, 100);
    if (al && k < EK_K4) eu_kit(e);                      /* (ALL: the kit again, of DENSITY, STEPS, ROTATE) */
    if (chain_busy()) { ui_message("SONG PLAYS: STOP IT TO EDIT"); return; }
    eu_anchor(song.sel);                                 /* (V10.16) what is dialled: MUTATE's anchor */
    eu_gen(song.sel, 1);
}

/* (V10.16) MUTATE: track k's settings moved by AMOUNT (from the anchor: ORBIT; from where they are: DRIFT; PULSE: the
 * anchor and a mutation by turns): always a new seed (GROOVE: where the hits fall, the notes), RHYTHM HITS (DENSITY)
 * and ROTATE, FEEL the four MODES and VARY; a lane picked: that lane's, the others as they are */
static void eu_mutate(uint32_t k, eu_t *e)
{
    uint32_t p = e->mu[1], w = e->mu[3], dr = drum_track(&trk[k]) != 0, sl = e->lane % (NLANE + 1u), i, al = dr && !sl;
    eul_t *L = al ? &e->a : &e->l[dr ? sl - 1u : 0u];
    int32_t d = (int32_t)(1u + p / 40u);
    if (e->mu[2] != 1u) memcpy((uint8_t *)e + EU_Z0, eu_z[k], EU_ZN);
    if (e->mu[2] == 2u && (e->flip ^= 1u) == 0u) { eu_gen(k, 0); return; }   /* (PULSE: the anchor's turn) */
    eu_s ^= fm1_ms * 2654435761u + k;
    e->seed = (uint16_t)(eu_rnd() | 1u);
    if ((w == 1u || w == 3u) && L->n) {                  /* RHYTHM */
        if (eu_ch(p)) L->h = (uint8_t)clamp((int32_t)L->h + (eu_ch(50) ? d : -d), al || !dr ? 1 : 0, al ? 8 : L->n);
        if (eu_ch(p)) L->r = (uint8_t)((L->r + eu_rnd() % (1u + p * L->n / 200u)) % L->n);
        if (al) eu_kit(e);
    }
    if (w >= 2u)                                         /* FEEL */
        for (i = 0; i <= EU_M; i++) {
            uint8_t *m = i < EU_M ? &e->m[i] : &e->vary;
            if (eu_ch(p)) *m = (uint8_t)clamp((int32_t)*m + (int32_t)(eu_rnd() % (p + 1u)) - (int32_t)p / 2, 0, 100);
        }
    eu_gen(k, 0);
}
/* (V10.16) called every UI frame: playing (not a song's chain), each track with EVERY on mutates in the last beat of
 * every EVERY bars (the pattern new from the next bar) */
static void eu_tick(void)
{
    uint32_t k, b = bar_n;
    for (k = 0; !song.playing && k < NTRK; k++) eu[k].bar = 0;   /* (stopped: from the top again) */
    if (!song.playing || chain_busy() || beat_n != 3u || beat_pos < beat_samples() / 2u) return;
    for (k = 0; k < NTRK; k++) {
        eu_t *e = &eu[k];
        if (!e->on || !e->mu[0] || e->bar == (uint8_t)(b + 1u) || (b + 1u) % (1u << (e->mu[0] - 1u))) continue;
        e->bar = (uint8_t)(b + 1u);
        eu_mutate(k, e);
        if (k == song.sel) ui.force = 1;
    }
}

/* OCT+ a new variation, OCT- the one before; (V10.16) on MUTATE: OCT+ a mutation now, OCT- back to the anchor */
static void eu_oct(int up, int mu)
{
    eu_t *e = eu_of(song.sel);
    if (chain_busy()) { ui_message("SONG PLAYS: STOP IT TO EDIT"); return; }
    if (mu) {
        if (up) eu_mutate(song.sel, e);
        else { memcpy((uint8_t *)e + EU_Z0, eu_z[song.sel], EU_ZN); eu_gen(song.sel, 1); }
        ui_message(up ? "MUTATED" : "ANCHOR");
        return;
    }
    if (up) { e->prev = e->seed; e->seed = (uint16_t)(e->seed * 25173u + 13849u); }
    else { uint16_t s = e->seed; e->seed = e->prev; e->prev = s; }
    eu_anchor(song.sel);
    eu_gen(song.sel, 1);
    ui_message(up ? "NEW VARIATION" : "VARIATION BEFORE");
}

/* ------------------------------------------------------------- the graph --- */
/* the pattern as it is: a ring of its steps (DRUM: the lane picked) at the left; DRUM: the eight lanes as rows (HITS /
 * STEPS) at the right; melodic: the key in the ring and the notes as a roll at the right, where the key came from */
static void graph_euclid(const track_t *t)
{
    static const char *const LN[NLANE + 1] = {"ALL", "BD", "SD", "CP", "CH", "OH", "TM", "RS", "CB"};
    static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const eu_t *e = eu_of(song.sel);
    uint32_t dr = drum_track(t) != 0, len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), sl = e->lane % (NLANE + 1u), lane = (sl + NLANE) % (NLANE + 1u), i, n;
    int32_t h = (int32_t)graph_h(), cy = h / 2, cx = 64, rad = h / 2 - 12 < 54 ? h / 2 - 12 : 54;
    uint32_t now = song.playing ? t->seq_idx % len : ~0u, t_base = 0, t_now = now;
    char b[12];
    n = dr ? (sl ? e->l[lane].n : e->a.n) : e->l[0].n;   /* (V10.21) the ring: one cycle (the lane's, ALL's STEPS, the
                                                         * melody's), the turn playing (VARY's turns, a longer pattern) */
    n = n && n < len ? n : len;
    if (e->mu[0]) {                                      /* (V10.16) MUTATE on: the bars to the next one */
        uint32_t ev = 1u << (e->mu[0] - 1u);
        str_cpy(b, "MUT ", sizeof b);
        fmt_int(b + 4, (int32_t)(song.playing ? ev - bar_n % ev : ev));
        cv_text(2, 2, &AF_X, b, T_ACCENT);
    }
    {
        uint32_t base = now != ~0u ? now / n * n : 0u;
        if (base + n > len) base = len - n;             /* (a lane whose STEPS do not divide the pattern: its last n) */
        t_base = base;
        t_now = now;                                     /* (the roll: the whole pattern) */
        now = now != ~0u ? now - base : ~0u;
    }
    for (i = 0; i < n; i++) {                            /* the ring */
        uint32_t a = i * 65536u / n + 49152u;
        int32_t x = cx + ((rad * og_cos(a)) >> 15), y = cy + ((rad * og_sin(a)) >> 15), r;
        const step_t *s = &t->step[t_base + i];
        uint32_t hit = dr ? (step_lanes(s) >> lane) & (sl ? 1u : 0xFFu) : s->time == ST_NOTE && s->n;   /* (ALL: any) */
        uint32_t acc = dr ? (step_accents(s) >> lane) & (sl ? 1u : 0xFFu) : 0u, gh = dr && sl && hit && step_lmode(s, lane) == LM_GHOST;
        r = !hit ? 1 : gh ? 2 : acc ? 5 : n > 24u ? 3 : 4;
        cv_rrect(x - r, y - r, 2 * r + 1, 2 * r + 1, r, !hit ? T_DIM : gh ? T_MID : dr ? T_ACCENT : T_THEME, T_SURF);
        if (hit && dr && sl && (step_lmode(s, lane) == LM_X2 || step_lmode(s, lane) == LM_X4))
            cv_frame(x - r - 2, y - r - 2, 2 * r + 5, 2 * r + 5, T_ACCENT);
        if (i == now) cv_frame(x - 7, y - 7, 15, 15, T_TEXT);
    }
    if (dr) {
        cv_text_in(cx - 30, cy - 8, 60, &AF_M, LN[sl], T_ACCENT, T_SURF);
        for (i = 0; i < NLANE; i++) {                    /* the lanes */
            int32_t y = 6 + (int32_t)i * (h - 12) / NLANE, rh = (h - 12) / NLANE - 2;
            uint32_t on = sl ? i == lane : e->l[i].h != 0u;   /* (ALL: the lanes it plays) */
            uint16_t c = on ? T_TEXT : e->l[i].h ? T_MID : T_DIM;
            if (sl && on) cv_rect(136, y, 98, rh, T_RAISE);
            cv_rect(136, y, 3, rh, on ? T_ACCENT : T_DIM);
            cv_text(144, y + (rh - 8) / 2, &AF_X, LN[i + 1u], c);   /* (V10.21: the lanes' names; LN[0] is ALL) */
            b[0] = 0; fmt_int(b, e->l[i].h); str_cpy(b + str_len(b), "/", 2); fmt_int(b + str_len(b), e->l[i].n);
            cv_text_r(230, y + (rh - 8) / 2, &AF_X, b, c, sl && on ? T_RAISE : T_SURF);
        }
        return;
    }
    str_cpy(b, NN[e->root % 12u], sizeof b);             /* the key, in the ring: its root, its scale */
    cv_text_in(cx - 30, cy - 14, 60, &AF_M, b, T_TEXT, T_SURF);
    for (i = 1; i < sizeof SCALE_MASK / sizeof SCALE_MASK[0] && SCALE_MASK[i] != e->mask; i++) {}
    if (i < sizeof SCALE_MASK / sizeof SCALE_MASK[0])
        cv_text_in(cx - 30, cy + 6, 60, &AF_X, N_SCALE[i], T_MID, T_SURF);
    {                                                    /* the roll */
        int32_t x0 = 134, y0 = 8, w = 100, rh = h - 30, lo = 127, hi = 0, cw;
        uint32_t c;
        for (i = 0; i < len; i++)
            for (c = 0; t->step[i].time == ST_NOTE && c < t->step[i].n && c < 4u; c++) {
                lo = t->step[i].note[c] < lo ? t->step[i].note[c] : lo;
                hi = t->step[i].note[c] > hi ? t->step[i].note[c] : hi;
            }
        if (hi < lo) { lo = 60; hi = 72; }
        if (hi - lo < 12) { lo -= (12 - (hi - lo)) / 2; hi = lo + 12; }
        cw = w / (int32_t)(len ? len : 1u);
        cw = cw < 1 ? 1 : cw;
        cv_frame(x0 - 1, y0 - 1, w + 2, rh + 2, T_DIM);
        for (i = 0; i < len && (int32_t)i * cw < w; i++) {
            const step_t *s = &t->step[i];
            uint32_t g = 1;
            if (s->time != ST_NOTE) continue;
            while (i + g < len && t->step[i + g].time == ST_TIE) g++;
            for (c = 0; c < s->n && c < 4u; c++) {
                int32_t y = y0 + rh - 3 - (s->note[c] - lo) * (rh - 4) / (hi - lo);
                cv_rect(x0 + (int32_t)i * cw, y, (int32_t)g * cw - 1 > 1 ? (int32_t)g * cw - 1 : 1, 3, c ? T_ACCENT : T_THEME);
            }
        }
        if (t_now != ~0u) cv_rect(x0 + (int32_t)t_now * cw, y0, 1, rh, T_TEXT);
        if (e->src == 0u) str_cpy(b, "SCL", sizeof b);
        else if (e->src == 7u) str_cpy(b, "NEW KEY", sizeof b);
        else { str_cpy(b, "FROM T1", sizeof b); b[6] = (char)('0' + e->src); }
        cv_text_in(x0, y0 + rh + 6, w, &AF_X, b, e->src == 7u ? T_ACCENT : T_MID, T_SURF);
    }
}
