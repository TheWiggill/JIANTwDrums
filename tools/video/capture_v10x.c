/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * (V10.36) "NIGHT SHIFT": the film of everything since V10 (V10 .. V10.36), on an original industrial darkwave song in F#
 * (126 BPM, six tracks on six engines: TECHNO RAW, a galloping REESE, a WIRE arp on a euclidean 7, the DARK PAD breathing on
 * its DRIFT LFO, a choir, a MOOG LEAD line over F#m9 Gmaj7 Em/F# C#m). Built as capture_v10i.c (below its older notes).
 * (V10.28) the industrial techno cut (capture_v10.c's script, its own song and sends). (V10.27) the V10 news film's footage: the v5 films' darkwave song (E minor, 132 BPM, six tracks on six engines, as
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
#define BPM 126u
#define BARS 72u
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
/* NIGHT SHIFT: F# minor with the Phrygian G; the chords F#m9, Gmaj7, Em/F#, C#m (four bars, 64 steps) */
static const uint8_t PADC[4][4] = {{54, 57, 61, 68}, {55, 59, 62, 66}, {54, 59, 64, 67}, {49, 56, 61, 64}};
static const uint8_t ROOT[4] = {30, 31, 30, 25};
/* the lead: a darkwave line, sparse, ending on the leading tone (step, note) */
static const uint8_t LEAD[][2] = {{0, 73}, {6, 71}, {8, 69}, {12, 68}, {16, 67}, {22, 66}, {26, 74}, {32, 76}, {36, 74},
                                  {40, 71}, {44, 69}, {48, 73}, {54, 68}, {58, 69}, {60, 71}};
static void section(void)
{
    static const uint8_t ARPN[4][3] = {{78, 81, 85}, {79, 83, 86}, {78, 83, 88}, {73, 80, 85}};
    static const uint8_t CHO[4] = {69, 71, 71, 68};
    uint32_t p, j, c = 0;
    for (p = 0; p < 64u; p++) {
        uint32_t b4 = p / 16u, i16 = p % 16u, g = p % 4u;
        if (g != 1u) {                                   /* the gallop: 1 . 1 1, the octave on the last */
            uint8_t v = (uint8_t)(ROOT[b4] + (g == 3u ? 12u : 0u));
            put(T_BASS, p, 1, &v, ST_NOTE, g == 2u ? SF_ACCENT : 0u);
        } else put(T_BASS, p, 0, 0, ST_REST, 0);
        if (((i16 * 7u) % 16u) < 7u) {                   /* a euclidean 7 of 16: the wire */
            uint8_t a = ARPN[b4][c++ % 3u];
            put(T_ARP, p, 1, &a, ST_NOTE, 0);
        } else put(T_ARP, p, 0, 0, ST_REST, 0);
        put(T_PAD, p, !(p % 16u) ? 4u : 0u, PADC[b4], p % 16u == 15u ? ST_REST : !(p % 16u) ? ST_NOTE : ST_TIE, 0);
        put(T_CHOIR, p, !(p % 16u) ? 1u : 0u, &CHO[b4], p % 16u >= 12u ? ST_REST : !(p % 16u) ? ST_NOTE : ST_TIE, 0);
        put(T_LEAD, p, 0, 0, ST_REST, 0);
        for (j = 0; j < sizeof LEAD / 2u; j++)
            if (LEAD[j][0] == p) put(T_LEAD, p, 1, &LEAD[j][1], ST_NOTE, j % 4u == 0u ? SF_ACCENT : 0u);
        if (p && trk[T_LEAD].step[p].n == 0u && trk[T_LEAD].step[p - 1u].n && p % 16u != 0u && (p % 4u) != 3u)
            trk[T_LEAD].step[p].time = ST_TIE;           /* (each note held to the next) */
    }
}
static step_t keep_steps[NTRK][NSTEP];

static void song_build(void)
{
    uint32_t k;
    ui_power_on();
    settings.palette = UI_JIANT_INDEX; palette_set(UI_JIANT_INDEX);
    ui_skin = SKIN_AIR; ui_style = ST_FLAT; style_apply();
    trks_put(1);
    song.g[G_BPM] = BPM;
    song.master_q12 = 4096;
    sound(T_LEAD, "ANALOG", "MOOG LEAD");
    sound(T_BASS, "ANALOG", "REESE");
    sound(T_ARP, "PHASE", "WIRE");
    sound(T_DRUM, "DRUM", "TECHNO RAW");
    sound(T_PAD, "ANALOG", "DARK PAD");
    sound(T_CHOIR, "VOICE", "CHOIR AAH");
    for (k = 0; k < NTRK; k++) {
        track_defaults_steps(&trk[k]);
        trk[k].p[P_ROOT] = 6; trk[k].p[P_SCALE] = 2;  /* F# minor */
        trk[k].p[P_SLEN] = 64;
        trk[k].p[P_MUTE] = 1;
    }
    section();
    trk[T_DRUM].p[P_SLEN] = 16;
    trk[T_LEAD].p[P_LEVEL] = 96; trk[T_LEAD].p[P_DLY] = 90; trk[T_LEAD].p[P_REV] = 70; trk[T_LEAD].p[P_PAN] = 8;
    trk[T_LEAD].p[P_GLIDE] = 30;
    trk[T_BASS].p[P_LEVEL] = 92; trk[T_BASS].p[P_CHOR] = 0; trk[T_BASS].p[P_DLY] = 0; trk[T_BASS].p[P_REV] = 0;
    trk[T_BASS].p[P_FCUT] = 62; trk[T_BASS].p[P_FRES] = 50; trk[T_BASS].p[P_DIST] = 30;
    trk[T_ARP].p[P_LEVEL] = 84; trk[T_ARP].p[P_CHOR] = 0; trk[T_ARP].p[P_DLY] = 100; trk[T_ARP].p[P_REV] = 60;
    trk[T_ARP].p[P_PAN] = -40;
    trk[T_PAD].p[P_LEVEL] = 84; trk[T_PAD].p[P_REV] = 127; trk[T_PAD].p[P_CHOR] = 50;
    trk[T_CHOIR].p[P_LEVEL] = 76; trk[T_CHOIR].p[P_REV] = 120; trk[T_CHOIR].p[P_DLY] = 40; trk[T_CHOIR].p[P_PAN] = 34;
    trk[T_DRUM].p[P_LEVEL] = 106; trk[T_DRUM].p[P_REV] = 30; trk[T_DRUM].p[P_DIST] = 24;
    song.g[G_RTYPE] = 0; song.g[G_RSIZE] = 124; song.g[G_DTIME] = DT_DOT; song.g[G_DFDBK] = 66;
    song.g[G_CDEPTH] = 60; song.g[G_CRATE] = 18;
    song.sel = T_DRUM;
    eu_put(EK_HITS, 5);                                /* (the kit from EUCLID: DENSITY 5) */
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
    /* ---- the drone, then the machine */
    if (q == B(0, 0)) { mute(T_PAD, 0); mute(T_CHOIR, 0); }
    if (q == B(2, 0)) {
        mute(T_DRUM, 0); mute(T_BASS, 0); home(T_BASS);
        caption(q, "JIANT FM", "EVERYTHING|SINCE V10", "V10 to V10.36|thirty-six updates in one song", 0,
                "NIGHT SHIFT · F# minor · 126 BPM|six tracks, six engines|played by the firmware itself");
    }
    /* ---- V10: the song as lanes */
    if (q == B(5, 0)) {
        chain_config.count = 4;
        chain_config.row[0] = (chain_row_t){0, 4}; chain_config.row[1] = (chain_row_t){1, 8};
        chain_config.row[2] = (chain_row_t){2, 4}; chain_config.row[3] = (chain_row_t){0, 8};
        page(T_LEAD, "TIMELINE");
        caption(q, "V10 · THE NEW BASE", "TIMELINE", "the song as lanes, left to right|a row can borrow a track from another section|and vary it there", 0,
                "MUTE · THIN · FILL · HALF · REV|SHIFT · OCT± · VAR 1–4|installs over official V16");
    }
    /* ---- EUCLID */
    if (q == B(8, 0)) {
        mute(T_ARP, 0);
        page(T_DRUM, "EUCLID");
        caption(q, "SEQUENCER", "EUCLID", "a euclidean generator, each lane a ring|DENSITY builds a whole kit|melodies in the key on a synth", 0,
                "HITS · STEPS · ROTATE · LANE|starts at 0: built from nothing");
    }
    if (q == B(9, 0)) enc_turn(EN_K1, 1);
    if (q == B(10, 0)) {
        eu_put(EK_K4, 0); eu_put(EK_M0 + 3u, 35);
        caption(q, "SEQUENCER", "VARY|IN TWO HALVES", "0–50: the cycle varied, every value its own|51–100: four turns, A A' A'' B|further apart the higher", 0, "");
    }
    if (q == B(11, 2)) eu_put(EK_M0 + 3u, 85);
    /* ---- EUCLID on your own sequence, and back */
    if (q == B(13, 0)) {
        page(T_ARP, "EUCLID");
        eu_put(EK_HITS, 5);
        caption(q, "SEQUENCER", "YOUR SEQUENCE,|TRANSFORMED", "EUCLID bends what you recorded|with your own notes|and gives it back at 0", 0,
                "HITS / DENSITY 0:|YOUR SEQUENCE BACK");
    }
    if (q == B(14, 0)) eu_put(EK_HITS, 9);
    if (q == B(15, 0)) eu_put(EK_HITS, 0);
    /* ---- MUTATE */
    if (q == B(16, 0)) {
        page(T_DRUM, "MUTATE");
        eu[T_DRUM].mu[0] = 1; eu[T_DRUM].mu[1] = 60; eu[T_DRUM].mu[3] = 3;
        caption(q, "SEQUENCER", "MUTATE", "the pattern mutates on the bar|the anchor above, what plays below|ORBIT · DRIFT · PULSE", 0, "");
    }
    if (q == B(19, 0)) eu[T_DRUM].mu[0] = 0;
    /* ---- SCENE */
    if (q == B(19, 0)) {
        static const uint8_t R[NTRK] = {SC_LEAD, SC_BASS, SC_ARP, SC_KEEP, SC_CHORD, SC_KEEP};
        for (k = 0; k < NTRK; k++) if (k != T_DRUM && k != T_CHOIR) {
            memcpy(keep_steps[k], trk[k].step, sizeof keep_steps[k]);
            for (uint32_t i = 0; i < NSTEP; i++) step_clear(&trk[k].step[i]);
        }
        memcpy(scn_role, R, sizeof R);
        page(T_PAD, "SCENE");
        sc_put(0, 1);
        trk[T_ARP].p[P_ARATE] = 2;
        caption(q, "PERFORMANCE", "SCENE", "one key plays the whole band|CHORD · ARP · BASS · LEAD|saved with the project", 0,
                "OCT+ dices the roles|REC writes every role at once");
    }
    if (q == B(19, 2)) scene_play(18);                 /* F# */
    if (q == B(21, 0)) scene_play(19);                 /* G */
    if (q == B(23, 0)) scene_play(16);                 /* E */
    if (q == B(24, 2)) scene_play(13);                 /* C# */
    if (q == B(25, 3)) {
        scene_play(99);
        sc_put(0, 0);
        memset(scn_role, 0, sizeof scn_role);
        for (k = 0; k < NTRK; k++) if (k != T_DRUM && k != T_CHOIR) memcpy(trk[k].step, keep_steps[k], sizeof keep_steps[k]);
    }
    /* ---- the lead, NUDGE */
    if (q == B(26, 0)) {
        mute(T_LEAD, 0);
        page(T_LEAD, "STRETCH");
        ndg_step = 1; ui.cursor = 0; ui.force = 1;
        caption(q, "SEQUENCER", "MICRO TIMING", "NUDGE a single step, 1/64 of a step|STEP: velocity per step, ACC · SLD|REC held alone: UNDO", 0, "");
    }
    if (q >= B(26, 2) && q < B(28, 0)) enc_turn(EN_K3, 3);
    /* ---- the sound */
    if (q == B(28, 0)) {
        ndg_step = 0;
        trk[T_BASS].p[P_E2ATK] = 0; trk[T_BASS].p[P_E2DEC] = 46; trk[T_BASS].p[P_E2SUS] = 0; trk[T_BASS].p[P_E2REL] = 30;
        trk[T_BASS].p[P_M1SRC] = MS_ENV2; trk[T_BASS].p[P_M1DST] = MD_CUT; trk[T_BASS].p[P_M1AMT] = 44;
        page(T_BASS, "ENV 2");
        caption(q, "SOUND", "ENV 2", "a second envelope on every voice|to any destination", 0, "");
    }
    if (q == B(30, 0)) {
        trk[T_BASS].p[P_FTYPE] = FT_LP24; trk[T_BASS].p[P_FRES] = 72;
        page(T_BASS, "FILTER");
        caption(q, "SOUND", "LP24", "a 24 dB/oct low-pass|on every engine", 0, "");
    }
    if (q >= B(30, 2) && q < B(32, 0)) trk[T_BASS].p[P_FCUT] = (int16_t)(38 + (q - B(30, 2)) * 7u);
    if (q == B(32, 0)) {
        trk[T_LEAD].p[P_E0] = AW_PLS3;
        page(T_LEAD, "EDIT 1");
        caption(q, "SOUND", "PLS3", "ANALOG: three pulses, MORPH the width", 0, "");
    }
    if (q >= B(32, 2) && q < B(34, 0)) trk[T_LEAD].p[P_MORPH] = (int16_t)((q - B(32, 2)) * 14u);
    /* ---- DRIFT */
    if (q == B(34, 0)) {
        trk[T_LEAD].p[P_MORPH] = 0;
        trk[T_PAD].p[P_LWAVE] = 5; trk[T_PAD].p[P_LRATE] = -2; trk[T_PAD].p[P_LD_FLT] = 50; trk[T_PAD].p[P_LD_PIT] = 2;
        page(T_PAD, "LFO");
        caption(q, "MODULATION", "DRIFT", "an LFO that wanders, smooth, never the same|drones and pads that breathe|LFO RATE down to 16 bars", 0,
                "the factory pads now|breathe by themselves");
    }
    /* ---- STRING FDBK */
    if (q == B(37, 0)) {
        trk[T_PAD].p[P_LD_FLT] = 22; trk[T_PAD].p[P_LRATE] = -14;
        sound(T_CHOIR, "STRING", "FEEDBACK");
        trk[T_CHOIR].p[P_LEVEL] = 84; trk[T_CHOIR].p[P_REV] = 100;
        page(T_CHOIR, "EDIT 2");
        caption(q, "SOUND", "STRING FDBK", "a guitar's feedback|an amp that hears the string", 0, "GAIN · DIST · SPEAKR · BLOOM");
    }
    /* ---- TAPE */
    if (q == B(39, 0)) {
        tape_v[1] = 80; tape_v[0] = 25;
        page(T_LEAD, "TAPE");
        caption(q, "MASTER", "TAPE WOBBLE", "WOW warps like an old record|a random, smooth pitch drift", 0, "");
    }
    if (q == B(41, 0)) { tape_v[1] = 0; tape_v[0] = 0; }
    /* ---- PRESETS: DRUM-X's new kit, heard and given back */
    if (q == B(41, 0)) {
        page(T_DRUM, "PRESETS");
        caption(q, "SOUNDS", "PRESETS", "two columns, each sound plays as you pass|DRUM-X: the new kit HIP HOP|+ the pattern BOOM BAP", 0,
                "OCT+ keep · OCT- the one before");
    }
    if (q > B(41, 0) && q < B(42, 3) && q % 2u == 0u) enc_turn(EN_K1, 1);
    if (q == B(43, 2)) press(B_OCTDN);
    if (q == B(44, 0)) {
        page(T_LEAD, "TOOLS"); ui.act = 1; ui.force = 1;
        caption(q, "SAVE", "TOOLS", "by scope: TRACK · SOUND · SONG · SYSTEM|hold OCT+ a second to do it", 0, "");
    }
    if (q == B(45, 0)) { ui.act = 2; ui.force = 1; }
    /* ---- Bluetooth */
    if (q == B(46, 0)) {
        ui.act = 0;
        hold(B_HOME);
        bt_up = BT_RUN;
        favorites.factory[6][24] = BT_MARK; favorites.factory[6][25] = 1;
        ui.menu = 3; ui.mlv = 1; ui.menu_sel = MI_BT;
        bp.conn = bp.have = 1; bp.lvl = 122 * 256 + 200; bp.raw = 0xBC3F; bp.w4[0] = 0xBC3F; bp.w4[1] = 0x8888; bp.w4[2] = 0xFFFF; bp.w4[3] = 0x0A0B; bp.src = 0;
        ui.force = 1;
        caption(q, "MIDI", "BLUETOOTH|MIDI", "in and out, notes, CC, clock|pairs with no PIN, remembered|PROX: a phone as a knob", 0,
                "a steady MIDI clock in and out|USB · TRS · Bluetooth");
    }
    if (q == B(48, 0)) {
        favorites.factory[6][25] = 3;
        mod_prox = 9000;
        caption(q, "EXPERIMENT", "AIR", "the radio listens to the devices around|your hand near the antenna|becomes a modulation source", 0,
                "MENU › SYSTEM › BT › AIR|no phone in your hand");
    }
    if (q >= B(48, 1) && q < B(50, 0)) { mod_prox = (int16_t)(9000 + 6000 * (int32_t)((q % 8u) < 4u ? q % 4u : 4u - q % 4u)); bp.lvl += (q & 1u) ? 300 : -260; ui.force = 1; }
    if (q == B(50, 0)) {
        ui.menu = 0; ui.force = 1; bt_up = 0; memset(&favorites.factory[6][24], 0, 2);
        home(T_LEAD);
    }
    /* ---- the list, the outro */
    if (q == B(52, 0)) {
        home(T_DRUM);
        caption(q, "NEWS", "SINCE V10", "", 3, "");
        item("TIMELINE", "the song as lanes, rows that vary", B(5, 0));
        item("EUCLID", "rings, DENSITY, VARY in two halves", B(8, 0));
        item("REVERSIBLE", "your sequence transformed, and back", B(13, 0));
        item("MUTATE", "the pattern mutates on the bar", B(16, 0));
        item("SCENE", "one key, the whole band", B(19, 0));
        item("MICRO TIMING", "NUDGE a step, VEL per step, UNDO", B(26, 0));
        item("ENV 2 · LP24 · PLS3", "a second envelope, 24 dB, three pulses", B(28, 0));
        item("DRIFT", "LFOs that wander, pads that breathe", B(34, 0));
        item("STRING FDBK · TAPE", "guitar feedback, a warped record", B(37, 0));
        item("HIP HOP · PRESETS", "a new kit, two-column browsing", B(41, 0));
        item("BLUETOOTH MIDI", "pairing, PROX, a steady clock", B(46, 0));
        item("AIR", "the antenna as a sensor", B(48, 0));
    }
    if (q == B(64, 0)) caption(q, "", "", "", 0, "");
    if (q == B(66, 0)) { mute(T_DRUM, 1); mute(T_BASS, 1); }
    if (q == B(68, 0)) for (k = 0; k < NTRK; k++) if (k != T_PAD && k != T_CHOIR) mute(k, 1);
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
