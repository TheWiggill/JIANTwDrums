/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * (JIANT 0.9.12: from Felucca 1.5's midi_learn.c, modified: six tracks, three bytes an entry, ten of them; p1 on seq-v2:
 * MIDI MAP FACTORY / CUSTOM ADD / CUSTOM, a CC fixed to a track or following the selected one; the map, its CCs and
 * its hook: midi_control.c) */
/* MIDI LEARN: a controller's CC set to any knob's parameter, on the device. MENU > CONTROL > LEARN SEL TRACK or FIXED
 * TRK arms it (menu_items.c; at the MENU's close); then a knob turned on any page that edits a track's parameter (HOME,
 * the EDIT and sound pages, ENV, LFO, FX, MIXER: ui_input.c knob_cap) picks that parameter; the next CC that comes
 * (midi_control.c midi_learned: any channel ROUT hears, USB or TRS) is set to it: on that track (FIXED TRK) or on
 * whichever track is selected when it comes (SEL TRACK), and stays until something else is learned on it. Then turn
 * the next knob. LEARN OFF ends it; LEARN CLEAR ALL clears every CC when the MENU closes. One CC sets one parameter, a
 * parameter has one CC: learning one again replaces both. A CC with a meaning of its own is refused ("CC64
 * RESERVED"); with ML_N learned, "LEARN FULL". Learning with MIDI MAP at FACTORY sets it to CUSTOM ADD (the factory map
 * and the learned CCs). Saved with the settings as each one changes (deferred while playing) */
static uint8_t *const ml_tab = &favorites.factory[ENGI_DIGITAL][1];
typedef char ml_row_fits[1u + 1u + 3u * ML_N <= sizeof favorites.factory[0] ? 1 : -1];
static const char *const MLMAP_N[MLM_N] __attribute__((section(".rodata.mlearn"))) = {"FACTORY", "CUSTOM ADD", "CUSTOM"};
static const char *const MLON_N[4] __attribute__((section(".rodata.mlearn"))) = {"OFF", "SEL TRACK", "FIXED TRK", "CLEAR ALL"};

static uint32_t ml_code(uint32_t id) { return id >= P_E0 ? ML_E0 + id - P_E0 : id + 1u; }
static uint8_t *ml_e(uint32_t i) { return ml_tab + 1u + 3u * i; }
static void ml_put(uint32_t i, uint32_t cc, uint32_t k, uint32_t code)   /* (the ISR reads the three: never part of one) */
{
    uint8_t *e = ml_e(i);
    fm1_irq_off();
    e[0] = (uint8_t)cc;
    e[1] = (uint8_t)k;
    e[2] = (uint8_t)code;
    fm1_irq_on();
}
static int ml_used(uint32_t i) { return (ml_e(i)[0] & 0x80u) != 0u; }
/* the CC of parameter id for track k (0..5, ML_SEL), + 1; 0 none */
static uint32_t ml_cc_of(uint32_t k, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < ML_N; i++)
        if (ml_used(i) && ml_e(i)[1] == k && ml_e(i)[2] == ml_code(id))
            return (ml_e(i)[0] & 127u) + 1u;
    return 0;
}
static uint32_t ml_count(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < ML_N; i++)
        n += (uint32_t)ml_used(i);
    return n;
}
/* cc -> parameter id of track k (ML_SEL: the selected one), in place of what either had; 1 learned, 0 no room (ML_N
 * others), 2 a CC never learned */
static uint32_t ml_learn(uint32_t cc, uint32_t k, uint32_t id)
{
    uint32_t i, f = ML_N;
    if (!ml_free_cc(cc) || cc > 127u || (k >= NTRK && k != ML_SEL) || id >= P_COUNT)
        return 2;
    for (i = 0; i < ML_N; i++)
        if (ml_used(i) && ((ml_e(i)[0] & 127u) == cc || (ml_e(i)[1] == k && ml_e(i)[2] == ml_code(id))))
            ml_put(i, 0, 0, 0);
    for (i = ML_N; i--;)
        if (!ml_used(i))
            f = i;
    if (f == ML_N)
        return 0;
    ml_put(f, 0x80u | cc, k, ml_code(id));
    if (ml_map() == MLM_FACTORY)
        ml_tab[0] = MLM_ADD;                         /* (learned with the map at FACTORY: the factory map and these) */
    return 1;
}
/* every learned CC cleared (MENU > CONTROL > LEARN CLEAR ALL), the settings saved; 1 = there were */
static int ml_clear_all(void)
{
    uint32_t i, n = ml_count();
    for (i = 0; i < ML_N; i++)
        ml_put(i, 0, 0, 0);
    if (n)
        settings_save();                             /* (deferred while playing) */
    return n != 0u;
}
/* MENU > CONTROL > LEARN (menu_put): OFF, SEL TRACK, FIXED TRK, CLEAR ALL; armed (or cleared) at the MENU's close */
static void ml_set(uint32_t v)
{
    ml.arm = ml.heard = 0;
    ml.pick = 0;
    ml.on = (uint8_t)(v & 3u);
}
static void ml_menu_closed(void)                     /* (ui_menu.c menu_close) */
{
    if (ml.on == 3u) {
        ml.on = 0;
        ui_message(ml_clear_all() ? "LEARN CLEARED" : "NOTHING LEARNED");
    } else if (ml.on && !ml.pick) {
        ui_say("LEARN: TURN A KNOB", ml.on == 1u ? ", SELECTED TRACK" : "");
    }
}
/* the picked parameter's name: "T2 CUT" / "SEL CUT" (b: 16 bytes) */
static void ml_name(char *b)
{
    if (ml.trk == ML_SEL) {
        str_cpy(b, "SEL ", 5);
    } else {
        b[0] = 'T';
        b[1] = (char)('1' + ml.trk % NTRK);
        b[2] = ' ';
        b[3] = 0;
    }
    str_cpy(b + str_len(b), track_desc(ml.trk == ML_SEL ? TSEL : &trk[ml.trk % NTRK], ml.id)->label, 12);
}
/* a knob turned parameter id of track t (ui_input.c knob_cap): picked while learning; "LEARN T2 CUT: SEND A CC" (its
 * CC if it has one) */
static __attribute__((noinline)) void ml_knob(const track_t *t, uint32_t id)
{
    char b[16], m[24];
    uint32_t cc, k;
    if (!ml.on || ml.on > 2u || id >= P_COUNT || ui.menu)
        return;
    k = ml.on == 1u ? ML_SEL : trk_index(t);
    if (ml.pick && ml.trk == k && ml.id == id)
        return;
    ml.trk = (uint8_t)k;
    ml.id = (uint8_t)id;
    ml.pick = 1;
    ml.heard = 0;
    ml.arm = 1;
    ml_name(b);
    str_cpy(m, "LEARN ", sizeof m);
    str_cpy(m + 6, b, sizeof m - 6u);
    cc = ml_cc_of(ml.trk, id);
    if (cc) {
        str_cpy(b, " = CC", sizeof b);
        fmt_int(b + 5, (int32_t)cc - 1);
    } else {
        str_cpy(b, ": SEND A CC", sizeof b);
    }
    ui_say(m, b);
}
/* each frame (ui_input): a CC heard for the picked parameter is learned, "CC21 > T2 CUT" */
static __attribute__((noinline)) void ml_poll(void)
{
    uint32_t cc = ml.heard, r;
    char a[12], b[16];
    if (!cc)
        return;
    ml.heard = 0;
    if (!ml.on || !ml.pick)
        return;
    cc--;
    a[0] = 'C';
    a[1] = 'C';
    fmt_int(a + 2, (int32_t)cc);
    r = ml_learn(cc, ml.trk, ml.id);
    if (r != 1u) {
        ui_say(a, r ? " RESERVED" : ": LEARN FULL");   /* (still waiting for one) */
        return;
    }
    str_cpy(a + str_len(a), " > ", 4);
    ml_name(b);
    ui_say(a, b);
    ml.pick = 0;
    ml.arm = 0;
    settings_save();                                 /* (deferred while playing) */
}
