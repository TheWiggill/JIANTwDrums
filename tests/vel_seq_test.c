/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.6z6) The keys' velocity and pressure where a played note goes, on the real sources (ui_test.c's): the key scan's
 * side (vel.c: what it hands the keys' block, vel_kb.h) is set by hand here; tests/vel_test.c runs that side itself.
 *   cc -O1 -w -Wno-implicit-function-declaration -Ibuild/gen -Ifirmware/src tests/vel_seq_test.c -lm
 * 1. VELOCITY OFF (the scan's state zeroed, as at power-on with it OFF): a key plays 100 to the voice, MIDI OUT and the
 *    recording, as before V10.6z6; a MIDI channel aftertouch is left alone;
 * 2. a key's velocity (37) reaches the voice (the matrix's VEL), MIDI OUT's note-on, the recorded step, a chord's every
 *    note, a strum's later notes and the arp's notes (MIDI OUT ALL);
 * 3. a press waiting for its strike (vk_hold) is not seen until let through, then plays its velocity;
 * 4. the pressure: a held key's (80) into its track's AT, slewed (no step over 1/8 of the way + 1 a block), MIDI OUT
 *    ALL's channel aftertouch to 80 and back to 0 at the release; recorded (P_XPR bits 2..4) and played back without the
 *    keys; stopped: the sound's value back;
 * 5. the default routing: with AFTERTOUCH on, INIT SOUND and a new project's sounds AT -> CUT 20 in the matrix's free
 *    slot 4 (not on a DRUM track, not over a slot in use; AFTERTOUCH OFF: none, as before); a preset load adds none;
 * 6. the MENU: CONTROL's rows VELOCITY, VEL MIN, AFTERTOUCH, VEL LEARN (an older setting: NORMAL, 20, MID, ON); RESET
 *    asks for the seeds (vel.c) once picked, and reads RESET. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static void ck(const char *what, int ok)
{
    printf("vel_seq: %-100s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}
static void blocks(uint32_t n) { while (n--) events_block(CTL); }
static void ms_run(uint32_t ms)
{
    uint32_t k;
    for (k = 0; k < ms / 16u; k++) {
        frame();
        blocks(16u * (FS / 1000u) / CTL);
    }
}
static uint32_t on_vel[8], n_on, at_last = 999, n_at;   /* MIDI OUT: note-ons' velocities, the last aftertouch */
static void drain(void)
{
    while (mo_r != mo_w) {
        uint32_t pkt = midi_out_q[mo_r++ % MQ], st = (pkt >> 8) & 0xF0u, d1 = (pkt >> 16) & 127u, d2 = pkt >> 24;
        if (st == 0x90u && d2 && n_on < 8u)
            on_vel[n_on++] = d2;
        if (st == 0xD0u)
            at_last = d1, n_at++;
    }
}
static void setup(void)
{
    ui_power_on();
    usb.config = 1;
    mo_r = mo_w = 0;
    mo_lvl = MO_KEYS;
    song.sel = 0;
    memset(&vs, 0, sizeof vs);
    frame();
    n_on = 0;
    n_at = 0;
    at_last = 999;
}
static uint32_t KEY = 12u;                            /* F4: a key with a converter */

int main(void)
{
    char what[160];
    uint32_t i, k;
    /* ---- 1. OFF */
    setup();
    trk[0].at = 55;                                   /* (as a MIDI channel aftertouch left it) */
    key_down(KEY);
    blocks(4);
    drain();
    ck("1. VELOCITY OFF: a key plays 100 (the voice's VEL, MIDI OUT's note-on)", trk[0].m_vel == 100u && n_on == 1u &&
       on_vel[0] == 100u);
    ck("   nothing pressed harder: a MIDI channel aftertouch on the track left alone", trk[0].at == 55u);
    key_up(KEY);
    blocks(4);
    /* ---- 2. a velocity */
    setup();
    vs.on = 1;
    kb_velx[KEY] = 37u ^ 100u;
    key_down(KEY);
    blocks(4);
    drain();
    ck("2. a key's velocity 37: the voice's VEL, MIDI OUT's note-on", trk[0].m_vel == 37u && n_on == 1u && on_vel[0] == 37u);
    key_up(KEY);
    blocks(4);
    trk[0].p[P_CHRD] = 1;                             /* (a chord: TRIAD, POLY) */
    trk[0].p[P_VOICE] = V_POLY;
    trk[0].p[P_CSTR] = 20;                            /* (strummed) */
    n_on = 0;
    key_down(KEY);
    blocks(200);
    drain();
    {
        uint32_t ok = n_on >= 3u;
        for (i = 0; i < n_on; i++)
            ok &= on_vel[i] == 37u;
        snprintf(what, sizeof what, "   a strummed chord: its %u notes all 37 (the later ones too)", n_on);
        ck(what, ok);
    }
    key_up(KEY);
    blocks(8);
    trk[0].p[P_CHRD] = 0;
    trk[0].p[P_CSTR] = 0;
    midi_out_mode = MO_ALL;
    frame();
    trk[0].p[P_AMODE] = 1;                            /* (ARP UP) */
    transport_req = 1;
    ms_run(32);
    drain();
    n_on = 0;
    key_down(KEY);
    ms_run(400);
    drain();
    {
        uint32_t ok = n_on >= 2u;
        for (i = 0; i < n_on; i++)
            ok &= on_vel[i] == 37u;
        snprintf(what, sizeof what, "   the arp (MIDI OUT ALL): its %u notes 37", n_on);
        ck(what, ok);
    }
    key_up(KEY);
    transport_req = 2;
    ms_run(32);
    trk[0].p[P_AMODE] = 0;
    midi_out_mode = MO_KEYS;
    frame();
    /* recording */
    setup();
    vs.on = 1;
    kb_velx[KEY] = 37u ^ 100u;
    trk[0].p[P_SLEN] = 16; trk[0].p[P_SDIV] = 2; song.g[G_BPM] = 120;
    song.rec = 1;
    transport_req = 1;
    ms_run(32);
    key_down(KEY);
    ms_run(160);
    key_up(KEY);
    ms_run(64);
    {
        uint32_t v = 0;
        for (i = 0; i < 16u; i++)
            if (trk[0].step[i].vel)
                v = trk[0].step[i].vel;
        ck("   recorded: the step keeps the key's velocity (37)", v == 37u);
    }
    song.rec = 0;
    transport_req = 2;
    ms_run(32);
    /* ---- 3. a press waiting */
    setup();
    vs.on = 1;
    kb_velx[KEY] = 0;
    vk_hold = 1u << KEY;
    key_down(KEY);
    blocks(3);
    drain();
    k = n_on;
    kb_velx[KEY] = 90u ^ 100u;
    vk_hold = 0;
    blocks(2);
    drain();
    ck("3. a press waiting for its strike: unseen until let through, then its velocity (90)", k == 0 && n_on == 1u &&
       on_vel[0] == 90u);
    key_up(KEY);
    blocks(4);
    /* ---- 4. the pressure */
    setup();
    vs.on = 3;
    midi_out_mode = MO_ALL;
    frame();
    key_down(KEY);
    blocks(4);
    vk[KEY].at = 80;
    {
        uint32_t step_ok = 1, prev = trk[0].at;
        for (i = 0; i < 100u; i++) {
            int32_t want = 80 - (int32_t)prev;
            events_block(CTL);
            if ((int32_t)trk[0].at - (int32_t)prev > want / 8 + 1) step_ok = 0;
            prev = trk[0].at;
        }
        drain();
        snprintf(what, sizeof what, "4. a held key's pressure 80: the track's AT %u, slewed; MIDI OUT ALL's channel aftertouch %u",
                 trk[0].at, at_last);
        ck(what, trk[0].at == 80u && step_ok && at_last == 80u && n_at <= 45u);
    }
    vk[KEY].at = 0;
    key_up(KEY);
    blocks(100);
    drain();
    ck("   let go: back to 0, and MIDI OUT's aftertouch 0", trk[0].at == 0 && at_last == 0);
    /* recorded */
    setup();
    vs.on = 3;
    trk[0].p[P_SLEN] = 16; trk[0].p[P_SDIV] = 2; song.g[G_BPM] = 120;
    motion.on = 0;
    song.rec = 1;
    transport_req = 1;
    ms_run(32);
    key_down(KEY);
    ms_run(160);
    vk[KEY].at = 90;
    ms_run(480);
    vk[KEY].at = 0;
    ms_run(48);
    key_up(KEY);
    ms_run(48);
    {
        uint32_t n = 0, mx = 0;
        for (i = 0; i < motion.count; i++) {
            const motion_event_t *e = &motion.event[i];
            if (MOTION_TRK(e) == 0u && MOTION_ID(e) == P_XPR && MOTION_VAL(e) == 127 - (90 >> 4 << 2))
                n++;
        }
        song.rec = 0;
        frame();
        for (i = 0; i < 2u * 16u * 125u / 16u; i++) {
            ms_run(16);
            if (trk[0].at > mx) mx = trk[0].at;
        }
        snprintf(what, sizeof what, "   recorded (P_XPR %d, %u events), played back without the keys: the track's AT to %u",
                 127 - (90 >> 4 << 2), n, mx);
        ck(what, n >= 1u && mx == 5u * 18u);
    }
    transport_req = 2;
    ms_run(64);
    blocks(200);
    ck("   stopped: the sound's value back (P_XPR 127), the pressure 0", trk[0].p[P_XPR] == 127 && trk[0].at == 0);
    /* ---- 5. the default routing */
    setup();
    VEL_SET2 = 2;                                     /* (AFTERTOUCH OFF: its place 0, stored ^ 2) */
    power_on_parts();
    ck("5. AFTERTOUCH OFF: a new project's sounds as before (no routing added)", trk[1].p[P_M1SRC + 9] != MS_AT &&
       trk[2].p[P_M1SRC + 9] != MS_AT);
    VEL_SET2 = 0;                                     /* (AFTERTOUCH MID, the default) */
    load_depth++;                                     /* (as ui_test.c's ui_power_on_099: the power-on parts) */
    fm6_init();
    power_on_parts();
    load_depth--;
    {
        uint32_t ok = 1, drums = 0;
        for (i = 0; i < trk_live; i++) {
            const track_t *t = &trk[i];
            if (drum_track(t)) {
                drums++;
                ok &= t->p[P_M1SRC + 9] != MS_AT;
            } else {
                ok &= t->p[P_M1SRC + 9] == MS_AT && t->p[P_M1SRC + 10] == MD_CUT && t->p[P_M1SRC + 11] == 20;
            }
        }
        ck("   AFTERTOUCH on: a new project's sounds AT -> CUT 20 in slot 4 (a DRUM track none)", ok && drums >= 1u);
        apply_preset_to(&trk[0], 1);
        ck("   a preset loaded: no routing added (its slots its own)", trk[0].p[P_M1SRC + 9] != MS_AT ||
           trk[0].p[P_M1SRC + 11] != 20);
        trk[0].p[P_M1SRC + 9] = MS_LFO; trk[0].p[P_M1SRC + 10] = MD_PAN; trk[0].p[P_M1SRC + 11] = 5;
        vel_at_route(&trk[0]);
        ck("   slot 4 in use: kept", trk[0].p[P_M1SRC + 9] == MS_LFO && trk[0].p[P_M1SRC + 11] == 5);
        init_sound_of(&trk[1], trk[1].eng_req);
        ck("   INIT SOUND: AT -> CUT 20", trk[1].p[P_M1SRC + 9] == MS_AT && trk[1].p[P_M1SRC + 10] == MD_CUT);
    }
    /* ---- 6. the MENU */
    setup();
    VEL_SET = VEL_SET2 = 0;
    ck("6. MENU > CONTROL: VELOCITY, VEL MIN, AFTERTOUCH, VEL LEARN; an older setting NORMAL, 20, MID, ON",
       MI_TAB[MI_VEL] == MTAB_CONTROL && MI_TAB[MI_VLEARN] == MTAB_CONTROL && str_eq(MI_NAME[MI_VEL], "VELOCITY") &&
       str_eq(menu_vname(MI_VEL, menu_get(MI_VEL)), "NORMAL") && str_eq(menu_vname(MI_VELMIN, menu_get(MI_VELMIN)), "20") &&
       str_eq(menu_vname(MI_AT, menu_get(MI_AT)), "MID") && str_eq(menu_vname(MI_VLEARN, menu_get(MI_VLEARN)), "ON") &&
       menu_n(MI_VEL) == 4u && menu_n(MI_VELMIN) == 8u && menu_n(MI_AT) == 4u && menu_n(MI_VLEARN) == 3u);
    menu_put(MI_VEL, 0);
    menu_put(MI_AT, 3);
    menu_put(MI_VELMIN, 0);
    ck("   VELOCITY OFF, AFTERTOUCH HIGH, VEL MIN 1: stored", menu_get(MI_VEL) == 0u && menu_get(MI_AT) == 3u &&
       str_eq(menu_vname(MI_VELMIN, menu_get(MI_VELMIN)), "1") && (VEL_SET & 3u) == 2u);
    vk_cal = 1;
    menu_put(MI_VLEARN, 2);
    k = vk_cal;
    vk_cal = 1;
    menu_put(MI_VLEARN, 2);
    ck("   VEL LEARN RESET: the seeds asked for once picked (vel.c), it reads RESET", k == 2u && vk_cal == 1u &&
       menu_get(MI_VLEARN) == 2u);
    printf("vel_seq: %s\n", bad ? "FAILED" : "all ok");
    return bad != 0;
}
