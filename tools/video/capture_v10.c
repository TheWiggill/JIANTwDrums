/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * (V10.27) the V10 news film's footage: the v5 films' darkwave song (E minor, 132 BPM, six tracks on six engines, as
 * capture_dark.c builds it) played by the real engines while the script goes through what came in V10.10 .. V10.27:
 * SONG mode (TIMELINE, SECTIONS ERASE), EUCLID's rings and VARY's turns, MUTATE, SCENE (one key, the whole band),
 * NUDGE's micro timing, ENV 2, LP24, PLS3, STRING FDBK, the TAPE's wobble, PRESETS and TOOLS redesigned, Bluetooth MIDI
 * and PROX, the slower LFO; then the list and the outro. Writes audio.wav, screen.rgb, frames.txt, scenes.txt and
 * list.txt (tools/video/film_prep.py, then tools/video/film.mjs ... update).
 *   cc -O2 -w -Ibuild/gen -Ifirmware/src -Itests -o build/video5/capture_v10 tools/video/capture_v10.c -lm
 *   build/video5/capture_v10 build/video5/v10 */
#define UI_TEST_NO_MAIN 1
#define PF_MS 900u
#define UI_NO_DRIFT 0
#include "ui_test.c"

#define FPS 30u
#define BPM 132u
#define BARS 64u
#define SPLASH_BEATS 8u
#define B(bar, q) ((bar) * 4u + (q))
static const double BEAT_S = 60.0 / BPM;
enum { T_LEAD, T_BASS, T_ARP, T_DRUM, T_PAD, T_CHOIR };
static FILE *wav, *rgb, *cap, *frm, *lst;
static uint32_t nsamp;

static uint32_t eng_of(const char *name)
{
    uint32_t e;
    for (e = 0; e < NENGINES; e++) if (str_eq(ENGINES[e]->name, name)) return e;
    return 0;
}
static uint32_t pre_of(uint32_t e, const char *name)
{
    uint32_t i;
    for (i = 0; i < ENGINES[e]->npresets; i++) if (str_eq(ENGINES[e]->presets[i].name, name)) return i;
    return 0;
}
static void sound(uint32_t k, const char *eng, const char *pre)
{
    uint32_t e = eng_of(eng);
    set_engine_of(&trk[k], e);
    apply_preset_to(&trk[k], pre_of(e, pre));
    trk[k].engine = trk[k].eng_req;
}
static void put(uint32_t k, uint32_t i, uint32_t n, const uint8_t *notes, uint32_t time, uint32_t flags)
{
    step_t *s = &trk[k].step[i];
    uint32_t j;
    s->n = (uint8_t)n;
    for (j = 0; j < 4u; j++) s->note[j] = j < n ? notes[j] : 0;
    s->time = (uint8_t)time;
    s->flags = (uint8_t)flags;
    s->vel = n ? 100 : 0;
    s->comp = s->carg = 0;
}
static const uint8_t ROOT_V[4] = {40, 36, 43, 38};
static const uint8_t CH_V[4][4] = {{52, 55, 59, 64}, {48, 55, 60, 64}, {50, 55, 59, 62}, {50, 54, 57, 62}};
static const uint8_t ARPV[4][8] = {{64, 67, 71, 76, 71, 67, 71, 74}, {64, 67, 72, 76, 72, 67, 72, 74},
                                   {62, 67, 71, 74, 71, 67, 71, 74}, {62, 66, 69, 74, 69, 66, 69, 73}};
static void section(void)
{
    uint32_t b, i;
    for (b = 0; b < 4u; b++) {
        for (i = 0; i < 16u; i++) {
            uint8_t n = (uint8_t)(ROOT_V[b] + (i == 14u ? 12u : 0u));
            put(T_BASS, b * 16u + i, i % 2u ? 0u : 1u, &n, i % 2u ? ST_REST : ST_NOTE, i == 0u ? SF_ACCENT : 0u);
        }
        for (i = 0; i < 16u; i++) {
            uint8_t n = ARPV[b][i / 2u];
            put(T_ARP, b * 16u + i, i % 2u ? 0u : 1u, &n, i % 2u ? ST_REST : ST_NOTE, 0);
        }
        for (i = 0; i < 16u; i++)
            put(T_PAD, b * 16u + i, i ? 0u : 4u, CH_V[b], i ? (i < 15u ? ST_TIE : ST_REST) : ST_NOTE, 0);
        for (i = 0; i < 16u; i++) {
            uint8_t v[2] = {(uint8_t)(CH_V[b][2] + 12u), (uint8_t)(CH_V[b][3] + 12u)};
            put(T_CHOIR, b * 16u + i, i ? 0u : 2u, v, i ? (i < 15u ? ST_TIE : ST_REST) : ST_NOTE, 0);
        }
    }
    {
        static const uint8_t LV[][2] = {{71, 6}, {69, 2}, {67, 4}, {66, 4}, {64, 8}, {67, 4}, {64, 4},
                                        {74, 6}, {71, 2}, {69, 4}, {67, 4}, {66, 12}, {0, 4}};
        uint32_t j, p = 0;
        for (j = 0; j < 13u; j++) {
            uint8_t n = LV[j][0];
            for (i = 0; i < LV[j][1] && p < 64u; i++, p++)
                put(T_LEAD, p, n && !i ? 1u : 0u, &n, !n ? ST_REST : i ? ST_TIE : ST_NOTE, 0);
        }
    }
}
static step_t keep_steps[NTRK][NSTEP];

static void song_build(void)
{
    uint32_t k;
    ui_power_on();
    settings.palette = UI_JIANT_INDEX; palette_set(UI_JIANT_INDEX);
    ui_skin = SKIN_AIR; ui_style = ST_FLAT; style_apply();   /* (the device's default look) */
    trks_put(1);
    song.g[G_BPM] = BPM;
    song.master_q12 = 4096;
    sound(T_LEAD, "ANALOG", "SAW LEAD");
    sound(T_BASS, "ANALOG", "FAT BASS");
    sound(T_ARP, "FM6", "PLUCK");
    sound(T_DRUM, "DRUM", "DRUM-X");
    sound(T_PAD, "ANALOG", "STRINGS");
    sound(T_CHOIR, "VOICE", "CHOIR AAH");
    for (k = 0; k < NTRK; k++) {
        track_defaults_steps(&trk[k]);
        trk[k].p[P_ROOT] = 4; trk[k].p[P_SCALE] = 2;  /* E minor */
        trk[k].p[P_SLEN] = 64;
        trk[k].p[P_MUTE] = 1;
    }
    section();
    trk[T_DRUM].p[P_SLEN] = 16;
    trk[T_LEAD].p[P_LEVEL] = 70; trk[T_LEAD].p[P_DLY] = 70; trk[T_LEAD].p[P_REV] = 70; trk[T_LEAD].p[P_CHOR] = 60;
    trk[T_LEAD].p[P_PAN] = 10; trk[T_LEAD].p[P_GLIDE] = 20;
    trk[T_BASS].p[P_LEVEL] = 100; trk[T_BASS].p[P_CHOR] = 90; trk[T_BASS].p[P_FCUT] = 70;
    trk[T_ARP].p[P_LEVEL] = 100; trk[T_ARP].p[P_CHOR] = 110; trk[T_ARP].p[P_DLY] = 70; trk[T_ARP].p[P_REV] = 50;
    trk[T_ARP].p[P_PAN] = -22;
    trk[T_PAD].p[P_LEVEL] = 66; trk[T_PAD].p[P_REV] = 100; trk[T_PAD].p[P_CHOR] = 70; trk[T_PAD].p[P_FCUT] = 66;
    trk[T_CHOIR].p[P_LEVEL] = 58; trk[T_CHOIR].p[P_REV] = 120; trk[T_CHOIR].p[P_PAN] = 18;
    trk[T_DRUM].p[P_LEVEL] = 92; trk[T_DRUM].p[P_REV] = 40;
    song.g[G_RTYPE] = 0; song.g[G_RSIZE] = 116; song.g[G_DTIME] = 3; song.g[G_DFDBK] = 40;
    song.g[G_CDEPTH] = 90; song.g[G_CRATE] = 30;
    song.sel = T_DRUM;
    eu_put(EK_HITS, 4);                                /* (the kit from EUCLID: DENSITY 4, as the film shows it) */
    ui.msg_t = 0; ui.msg[0] = ui.msg2[0] = 0;
    transport_req = 1;
}

static void caption(uint32_t q, const char *tag, const char *title, const char *sub, int kind, const char *data)
{
    fprintf(cap, "%.4f\t%s\t%s\t%s\t%d\t%s\n", q * BEAT_S, tag, title, sub, kind, data);
}
static void item(const char *title, const char *words, uint32_t q)
{
    fprintf(lst, "%s\t%s\t%.4f\n", title, words, (q + 1u) * BEAT_S + 0.3);
}
static void page(uint32_t sel, const char *title) { song.sel = (uint8_t)sel; go_title(title); }
static void home(uint32_t sel) { song.sel = (uint8_t)sel; go_home(); }
static void enc_turn(uint32_t role, int32_t s) { host_enc[panel.enc[role]] += s * panel.dir[role]; }
static void mute(uint32_t k, int m) { trk[k].p[P_MUTE] = (int16_t)m; }
static uint32_t scn_key = 99;
static void scene_play(uint32_t k)                     /* SCENE: one key down (the last one up) */
{
    if (scn_key < 27u) { fm1_in.notes &= ~(1u << scn_key); }
    scn_key = k;
    if (k < 27u) { fm1_in.notes |= 1u << k; host_notes |= 1u << k; }
}

static void at_beat(uint32_t q)
{
    uint32_t k;
    /* ---- the band comes in */
    if (q == B(0, 0)) { mute(T_PAD, 0); mute(T_CHOIR, 0); mute(T_DRUM, 0); }
    if (q == B(2, 0)) {
        mute(T_BASS, 0); mute(T_ARP, 0); home(T_BASS);
        caption(q, "JIANT FM", "WHAT'S NEW|IN V10", "from V10.10 to V10.27|everything since the last film", 0,
                "six tracks, six engines|a darkwave song in E minor|played by the firmware itself");
    }
    /* ---- EUCLID */
    if (q == B(6, 0)) {
        page(T_DRUM, "EUCLID");
        caption(q, "SEQUENCER", "EUCLID", "a euclidean rhythm generator|each lane a ring, in its heat colour|DENSITY builds a whole kit that plays", 0,
                "HITS · STEPS · ROTATE|LANE: ALL, BD .. CB|melodies in the key on a synth");
    }
    if (q == B(7, 0)) enc_turn(EN_K1, 1);
    if (q == B(8, 0)) enc_turn(EN_K1, 1);
    if (q == B(9, 0)) {
        eu_put(EK_M0 + 3u, 70);                        /* VARY 70: four turns */
        caption(q, "SEQUENCER", "VARY|A A' A'' B", "the pattern becomes a phrase of four turns|each moved anew, the last the most|HITS stays exact in every turn", 0,
                "VARY 0: the pure euclid|B ends with a fill");
    }
    /* ---- MUTATE */
    if (q == B(12, 0)) {
        mute(T_LEAD, 0);
        page(T_DRUM, "MUTATE");
        eu[T_DRUM].mu[0] = 1; eu[T_DRUM].mu[1] = 60; eu[T_DRUM].mu[3] = 3;
        caption(q, "SEQUENCER", "MUTATE", "the pattern mutates on the bar while it plays|above the anchor, below what sounds|the arc counts to the next one", 0,
                "EVERY · AMOUNT · ORBIT DRIFT PULSE|OCT+ mutate now · OCT- the anchor");
    }
    if (q == B(16, 0)) eu[T_DRUM].mu[0] = 0;
    /* ---- SCENE: the melodic sequences set aside, one key plays the band */
    if (q == B(17, 0)) {
        static const uint8_t R[NTRK] = {SC_LEAD, SC_BASS, SC_ARP, SC_KEEP, SC_CHORD, SC_KEEP};
        for (k = 0; k < NTRK; k++) if (k != T_DRUM && k != T_CHOIR) {
            memcpy(keep_steps[k], trk[k].step, sizeof keep_steps[k]);
            for (uint32_t i = 0; i < NSTEP; i++) step_clear(&trk[k].step[i]);
        }
        memcpy(scn_role, R, sizeof R);
        page(T_PAD, "SCENE");
        sc_put(0, 1);
        trk[T_ARP].p[P_ARATE] = 2;
        caption(q, "PERFORMANCE", "SCENE", "one key plays the whole band|CHORD · ARP · BASS · LEAD from one note|the drums keep their sequence", 0,
                "OCT+ dices the roles|never two tracks alike|REC writes every role's roll");
    }
    if (q == B(17, 2)) scene_play(11);                 /* E */
    if (q == B(19, 0)) scene_play(7);                  /* C */
    if (q == B(21, 0)) scene_play(14);                 /* G */
    if (q == B(23, 0)) scene_play(9);                  /* D */
    if (q == B(24, 3)) {
        scene_play(99);
        sc_put(0, 0);
        memset(scn_role, 0, sizeof scn_role);
        for (k = 0; k < NTRK; k++) if (k != T_DRUM && k != T_CHOIR) memcpy(trk[k].step, keep_steps[k], sizeof keep_steps[k]);
    }
    /* ---- NUDGE */
    if (q == B(25, 0)) {
        page(T_LEAD, "STRETCH");
        ndg_step = 1; ui.cursor = 0; ui.force = 1;
        caption(q, "SEQUENCER", "MICRO TIMING", "NUDGE blue: the sequence by steps|NUDGE red: the step itself, 1/64 of a step|early or late, by hand", 0,
                "OCT- the sequence · OCT+ the step|PRESETS picks the step");
    }
    if (q >= B(25, 2) && q < B(27, 0)) enc_turn(EN_K3, 3);
    /* ---- the sound */
    if (q == B(27, 0)) {
        ndg_step = 0;
        trk[T_BASS].p[P_E2ATK] = 0; trk[T_BASS].p[P_E2DEC] = 50; trk[T_BASS].p[P_E2SUS] = 0; trk[T_BASS].p[P_E2REL] = 30;
        trk[T_BASS].p[P_M1SRC] = MS_ENV2; trk[T_BASS].p[P_M1DST] = MD_CUT; trk[T_BASS].p[P_M1AMT] = 40;
        page(T_BASS, "ENV 2");
        caption(q, "SOUND", "ENV 2", "a second envelope on every voice|routed like the LFO, to any destination|here: a pluck on the bass's cutoff", 0,
                "ATK · DEC · SUS · REL|MOD: SRC ENV2");
    }
    if (q == B(29, 0)) {
        trk[T_BASS].p[P_FTYPE] = FT_LP24; trk[T_BASS].p[P_FRES] = 70;
        page(T_BASS, "FILTER");
        caption(q, "SOUND", "LP24", "a 24 dB/oct low-pass|on every engine", 0, "two poles in series|RES on the second");
    }
    if (q >= B(29, 2) && q < B(31, 0)) trk[T_BASS].p[P_FCUT] = (int16_t)(40 + (q - B(29, 2)) * 8u);
    if (q == B(31, 0)) {
        trk[T_LEAD].p[P_E0] = AW_PLS3;
        page(T_LEAD, "EDIT 1");
        caption(q, "SOUND", "PLS3", "ANALOG: three pulse oscillators|MORPH sets the width", 0, "the third detuned the other way");
    }
    if (q >= B(31, 2) && q < B(33, 0)) trk[T_LEAD].p[P_MORPH] = (int16_t)((q - B(31, 2)) * 15u);
    if (q == B(33, 0)) {
        trk[T_LEAD].p[P_MORPH] = 0;
        sound(T_CHOIR, "STRING", "FEEDBACK");
        trk[T_CHOIR].p[P_LEVEL] = 70; trk[T_CHOIR].p[P_REV] = 90;
        page(T_CHOIR, "EDIT 2");
        caption(q, "SOUND", "STRING FDBK", "a guitar's feedback, an amp that hears the string|GAIN · DIST · SPEAKR · BLOOM|heard at once now", 0,
                "Karplus-Strong strings|PLUCK PICK HAMMR STICK FDBK");
    }
    /* ---- the master's TAPE */
    if (q == B(35, 0)) {
        tape_v[1] = 85; tape_v[0] = 20;
        page(T_LEAD, "TAPE");
        caption(q, "MASTER", "TAPE WOBBLE", "WOW now warps like an old record|a random, smooth pitch drift|DRIVE: the tape's saturation", 0,
                "WOW 0 .. 100|up to ±45 cents");
    }
    if (q == B(37, 0)) { tape_v[1] = 0; tape_v[0] = 0; }
    /* ---- PRESETS, TOOLS, the song */
    if (q == B(37, 0)) {
        page(T_LEAD, "PRESETS");
        caption(q, "SOUNDS", "PRESETS", "engines and sounds in two columns|each sound plays as you pass it|OCT+ keep · OCT- the one before", 0, "");
    }
    if (q > B(37, 0) && q < B(39, 2) && q % 2u == 0u) enc_turn(EN_K1, 1);
    if (q == B(39, 2)) press(B_OCTDN);
    if (q == B(40, 0)) {
        page(T_LEAD, "TOOLS"); ui.act = 1; ui.force = 1;
        caption(q, "SAVE", "TOOLS", "by scope: TRACK · SOUND · SONG · SYSTEM|a picture of what it clears|hold OCT+ a second to do it", 0, "");
    }
    if (q == B(41, 0)) { ui.act = 2; ui.force = 1; }
    if (q == B(41, 2)) { ui.act = 4; ui.force = 1; }
    if (q == B(42, 0)) {
        ui.act = 0;
        chain_config.count = 4;                         /* (the song's rows, shown; the section plays on) */
        chain_config.row[0] = (chain_row_t){0, 4}; chain_config.row[1] = (chain_row_t){1, 8};
        chain_config.row[2] = (chain_row_t){2, 4}; chain_config.row[3] = (chain_row_t){0, 8};
        page(T_LEAD, "TIMELINE");
        caption(q, "SONG", "SONG MODE", "TIMELINE: the song left to right|SECTIONS: ERASE a section to zero|ARRANGE: rows, bars, scenes", 0, "");
    }
    /* ---- Bluetooth, the LFO */
    if (q == B(44, 0)) {
        hold(B_HOME);
        bt_up = BT_RUN;
        ui.menu = 3; ui.mlv = 1; ui.menu_sel = MI_BT;
        bp.conn = bp.have = 1; bp.lvl = 125 * 256 + 230; bp.raw = 0xBC3F; bp.w4[0] = 0xBC3F; bp.w4[1] = 0x8888; bp.w4[2] = 0xFFFF; bp.w4[3] = 0x0A0B; bp.src = 11;
        ui.force = 1;
        caption(q, "MIDI", "BLUETOOTH MIDI|+ PROX", "MIDI by Bluetooth, notes, CC and clock|PROX: how close the phone is|a modulation source", 0,
                "MENU > SYSTEM > BT ON|FAR · NEAR, then SRC PROX");
    }
    if (q == B(46, 0)) {
        ui.menu = 0; ui.force = 1; bt_up = 0;
        trk[T_PAD].p[P_LRATE] = -14; trk[T_PAD].p[P_LD_FLT] = 60;
        page(T_PAD, "LFO");
        caption(q, "MODULATION", "SLOWER", "LFO in time up to 16 BARS|PLAYHEADS at 1/8 and 1/16|STRUM up to a second", 0, "");
    }
    /* ---- the list, the outro */
    if (q == B(48, 0)) {
        home(T_DRUM);
        caption(q, "NEWS", "NEW IN V10", "", 3, "");
        item("EUCLID", "rings per lane, VARY in four turns", B(6, 0));
        item("MUTATE", "the pattern mutates on the bar", B(12, 0));
        item("SCENE", "one key plays the whole band", B(17, 0));
        item("MICRO TIMING", "NUDGE a single step, 1/64 of a step", B(25, 0));
        item("ENV 2 · LP24 · PLS3", "a second envelope, 24 dB, three pulses", B(27, 0));
        item("STRING FDBK", "guitar feedback, heard at once", B(33, 0));
        item("TAPE WOBBLE", "a warped record on the master", B(35, 0));
        item("PRESETS · TOOLS", "redesigned: two columns, by scope", B(37, 0));
        item("SONG MODE", "TIMELINE, ERASE a section", B(42, 0));
        item("BLUETOOTH MIDI", "and PROX, the phone as a knob", B(44, 0));
        item("SLOWER", "16-bar LFOs, 1/16 playheads, long strums", B(46, 0));
    }
    if (q == B(58, 0)) caption(q, "", "", "", 0, "");
    if (q == B(60, 0)) mute(T_DRUM, 1);
    if (q == B(62, 0)) for (k = 0; k < NTRK; k++) if (k != T_PAD) mute(k, 1);
}

static void put_screen(void)
{
    static uint8_t buf[240u * 240u * 3u];
    uint32_t i;
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = swap16(host_screen[i]);
        buf[3u * i] = (uint8_t)((c >> 11) * 255u / 31u);
        buf[3u * i + 1u] = (uint8_t)(((c >> 5) & 63u) * 255u / 63u);
        buf[3u * i + 2u] = (uint8_t)((c & 31u) * 255u / 31u);
    }
    fwrite(buf, 1, sizeof buf, rgb);
}
int main(int argc, char **argv)
{
    char p[512];
    const char *dir = argc > 1 ? argv[1] : "build/video5/v10";
    const double SECS = BARS * 4u * BEAT_S;
    uint32_t f, total, beat_done = 0xFFFFFFFFu, nwav, k;
    snprintf(p, sizeof p, "%s/audio.wav", dir); wav = fopen(p, "wb");
    snprintf(p, sizeof p, "%s/screen.rgb", dir); rgb = fopen(p, "wb");
    snprintf(p, sizeof p, "%s/scenes.txt", dir); cap = fopen(p, "w");
    snprintf(p, sizeof p, "%s/frames.txt", dir); frm = fopen(p, "w");
    snprintf(p, sizeof p, "%s/list.txt", dir); lst = fopen(p, "w");
    if (!wav || !rgb || !cap || !frm || !lst) { fprintf(stderr, "cannot write in %s\n", dir); return 1; }
    song_build();
    total = (uint32_t)(FPS * SECS); nwav = (uint32_t)(FS * SECS);
    wav_hdr(wav, nwav);
    fprintf(cap, "# %u fps, %d BPM, %u bars, splash %.4f s, end %.4f s, beat %.6f s\n", FPS, BPM, BARS,
            SPLASH_BEATS * BEAT_S, SECS, BEAT_S);
    for (f = 0; f < total; f++) {
        double t = (double)f / FPS;
        uint32_t want = (uint32_t)((uint64_t)(f + 1u) * FS / FPS), beat = (uint32_t)(t / BEAT_S);
        if (beat != beat_done) { beat_done = beat; at_beat(beat); }
        while (nsamp < want && nsamp < nwav) {
            int32_t o[2 * CTL];
            int16_t s[2 * CTL];
            uint32_t i, n = CTL;
            keyboard_block();
            mix_block(o, CTL);
            for (i = 1; i < CTL; i += 2u)
                scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)clamp(o[2u * i], -32768, 32767);
            if (nsamp + n > nwav) n = nwav - nsamp;
            for (i = 0; i < 2u * n; i++) s[i] = (int16_t)clamp(o[i], -32768, 32767);
            fwrite(s, 2, 2u * n, wav);
            nsamp += n;
        }
        fprintf(frm, "%d", t >= SPLASH_BEATS * BEAT_S && ui.home && !ui.menu && !ui.confirm);
        for (k = 0; k < NTRK; k++) fprintf(frm, " %d", trk[k].p[P_MUTE] ? 0 : (int)trk[k].peak);
        fprintf(frm, "\n");
        host_ticks += 1000000u / FPS; fm1_ms += 1000u / FPS;
        if (t < SPLASH_BEATS * BEAT_S) {
            if (f == 0) { spl_anim = 1; draw_splash(); }
            splash_write((uint32_t)(t * 1000.0));
        } else {
            static int first = 1;
            if (first) { ui.force = 1; first = 0; }
            ui_input();
            ui_draw();
        }
        for (k = 0; k < NTRK; k++) trk[k].peak = 0;
        put_screen();
    }
    fclose(wav); fclose(rgb); fclose(cap); fclose(frm); fclose(lst);
    printf("%u frames, %u samples\n", total, nsamp);
    return 0;
}
