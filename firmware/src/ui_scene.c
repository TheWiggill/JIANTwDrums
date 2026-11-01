/* SPDX-License-Identifier: GPL-3.0-only
 * JIANT V10.25: SCL > SCENE, the multitimbral mode's page (seq.c scene_key plays it). Included by ui_graph.c.
 * KNOB 1 SCENE OFF / ON (on: the keys play every track with a role), KNOB 2 the TRACK, KNOB 3 its ROLE (KEEP CHORD ARP
 * BASS LEAD), KNOB 4 its CHORD (the chord CHORD and ARP play; OFF: a triad of the scale). A role sets its track up:
 * ARP turns the track's arp on (UP) if it was off, CHORD BASS LEAD turn it off, CHORD makes the voice POLY. A kit
 * keeps its sequence (KEEP): the keys do not reach it. The graph: six columns, each track's role and the notes the
 * last key played makes on it */
static uint8_t sc_trk = 5;                               /* the track KNOB 2 picked (T6) */
static int16_t sc_v[4];
static param_desc_t sc_d;
static const char *const SC_ROLE[SC_N] = {"KEEP", "CHORD", "ARP", "BASS", "LEAD"};
static const param_desc_t *sc_desc(uint32_t k, int16_t **valp)
{
    static const char *const ONOFF[2] = {"OFF", "ON"};
    static const char *const TN[NTRK] = {"T1", "T2", "T3", "T4", "T5", "T6"};
    track_t *t = &trk[sc_trk % NTRK];
    k &= 3u;
    if (k == 3u) {                                       /* the picked track's own CHORD */
        *valp = &t->p[P_CHRD];
        return track_desc(t, P_CHRD);
    }
    sc_d.fmt = F_ENUM; sc_d.min = 0; sc_d.def = 0; sc_d.unit = 0;
    if (k == 0u) { sc_d.label = "SCENE"; sc_d.max = 1; sc_d.names = ONOFF; sc_v[0] = scn_on; }
    else if (k == 1u) { sc_d.label = "TRACK"; sc_d.max = NTRK - 1; sc_d.names = TN; sc_v[1] = sc_trk; }
    else { sc_d.label = "ROLE"; sc_d.max = drum_track(t) ? 0 : SC_N - 1; sc_d.names = SC_ROLE; sc_v[2] = drum_track(t) ? 0 : scn_role[sc_trk % NTRK]; }
    *valp = &sc_v[k];
    return &sc_d;
}
static void sc_setup(uint32_t i)                        /* track i set up for its role */
{
    track_t *t = &trk[i % NTRK];
    uint32_t r = scn_role[i % NTRK];
    if (drum_track(t) || r == SC_KEEP) return;
    if (r == SC_ARP) { if (!t->p[P_AMODE]) t->p[P_AMODE] = AM_UP; }
    else t->p[P_AMODE] = AM_OFF;
    if (r == SC_CHORD) t->p[P_VOICE] = V_POLY;
}
static void sc_put(uint32_t k, int32_t v)
{
    uint32_t i, need = 0;
    if (k == 0u) {
        scn_on = (uint8_t)(v != 0);
        for (i = 0; scn_on && i < NTRK; i++) {
            sc_setup(i);
            need |= (uint32_t)(i >= trk_live && scn_role[i] && !drum_track(&trk[i]));
        }
        ui_message(!scn_on ? "SCENE OFF: KEYS PLAY ONE TRACK" : need ? "SCENE: T5 T6 ASLEEP (MENU > 6 TRACKS)" : "SCENE: KEYS PLAY THE ROLES");
    } else if (k == 1u)
        sc_trk = (uint8_t)clamp(v, 0, NTRK - 1);
    else if (k == 2u && !drum_track(&trk[sc_trk % NTRK])) {
        scn_role[sc_trk % NTRK] = (uint8_t)clamp(v, 0, SC_N - 1);
        sc_setup(sc_trk);
    }
    ui.force = 1;
}
static void graph_scene(void)
{
    uint32_t i, j, n, h = graph_h();
    int32_t cw = 38, x0 = 6, y0 = 30, yh = (int32_t)h - y0 - 16;
    uint8_t o[KB_CH + 1];
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        int32_t x = x0 + (int32_t)i * cw;
        uint32_t r = drum_track(t) ? SC_KEEP : scn_role[i];
        uint16_t c = r == SC_KEEP ? T_DIM : r == SC_CHORD ? T_THEME : r == SC_ARP ? T_MID : r == SC_BASS ? T_ACCENT : T_TEXT;
        char b[3] = {'T', (char)('1' + i), 0};
        cv_text_in(x, 2, cw - 4, &AF_X, b, i == sc_trk ? T_TEXT : T_DIM, T_SURF);
        cv_text_in(x - 2, 14, cw, &AF_X, drum_track(t) ? "KIT" : SC_ROLE[r], c, T_SURF);
        if (i == sc_trk) cv_frame(x - 3, 0, cw - 1, 25, T_ACCENT);
        if (i >= trk_live) continue;
        if (r == SC_KEEP) {                              /* its own sequence: the steps, dim */
            for (j = 0; j < 16u; j++)
                if (t->step[j].time == ST_NOTE) cv_rect(x + 2, y0 + (int32_t)j * yh / 16, cw - 10, 2, T_DIM);
            continue;
        }
        n = scn_notes(t, r, last_note, o);               /* the last key's notes on it, by pitch */
        for (j = 0; j < n; j++) {
            int32_t y = y0 + yh - 4 - (clamp(o[j], 36, 96) - 36) * (yh - 6) / 60;
            cv_rect(x + 2 + (r == SC_ARP ? (int32_t)j * 7 : 0), y, r == SC_ARP ? 6 : cw - 10, 3, c);
        }
    }
    cv_text_in(4, (int32_t)h - 11, 232, &AF_X, scn_on ? "SCENE ON: A KEY PLAYS EVERY ROLE" : "SCENE OFF: KNOB 1 TURNS IT ON",
               scn_on ? T_ACCENT : T_DIM, T_SURF);
}
