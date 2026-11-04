/* hiss probe: one held note on a track alone, the master's output dumped (int32 L) */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static void take(const char *path, int tk, int fxoff, int lvl, int note)
{
    static int32_t o[2 * CTL];
    uint32_t b, i, k;
    FILE *f = fopen(path, "wb");
    ui_power_on();
    for (k = 0; k < NTRK; k++) trk[k].p[P_MUTE] = (int16_t)(k != (uint32_t)tk);
    song.master_q12 = 4096;
    if (getenv("ENG")) set_engine_of(&trk[tk], (uint32_t)atoi(getenv("ENG")));
    if (fxoff) trk[tk].p[P_REV] = trk[tk].p[P_DLY] = trk[tk].p[P_CHOR] = 0;
    if (lvl >= 0) trk[tk].p[P_LEVEL] = (int16_t)lvl;
    if (getenv("E3")) trk[tk].p[P_E3] = (int16_t)atoi(getenv("E3"));
    if (getenv("E0")) trk[tk].p[P_E0] = (int16_t)atoi(getenv("E0"));
    if (getenv("CUT")) trk[tk].p[P_FCUT] = (int16_t)atoi(getenv("CUT"));
    if (getenv("RES")) trk[tk].p[P_FRES] = (int16_t)atoi(getenv("RES"));
    printf("filter: type %d cut %d res %d\n", trk[tk].p[P_FTYPE], trk[tk].p[P_FCUT], trk[tk].p[P_FRES]);
    for (i = 0; i < 16u; i++) {
        step_t *s = &trk[tk].step[i];
        memset(s, 0, sizeof *s);
        s->time = i ? ST_TIE : ST_NOTE; s->vel = 100; s->n = 1; s->note[0] = (uint8_t)note;
    }
    trk[tk].p[P_SLEN] = 16;
    transport_req = 1; ui_input();
    for (b = 0; b < 1400u; b++) {
        if (b % 32u == 0u) { ui_input(); song_poll(); host_ticks += 20000u; fm1_ms += 20u; }
        memset(o, 0, sizeof o);
        mix_block(o, CTL);
        if (b >= 600u) for (i = 0; i < CTL; i++) fwrite(&o[2 * i], 4, 1, f);
    }
    fclose(f);
    { uint32_t a = 0, vv; for (vv = 0; vv < NVOICE; vv++) a += trk[tk].v[vv].active;
      printf("dbg: playing %d, active voices %u, mute %d, mix_l[0] %d, o0 %d, master %d\n", (int)song.playing, a, trk[tk].p[P_MUTE], mix_l[0], o[0], song.master_q12); }
    printf("%s: eng %u level %d rev %d dly %d\n", path, trk[tk].eng_req, trk[tk].p[P_LEVEL], trk[tk].p[P_REV], trk[tk].p[P_DLY]);
}
int main(int argc, char **argv)
{
    const char *d = argv[1];
    char p[512];
    int tk = argc > 2 ? atoi(argv[2]) : 4;
    snprintf(p, sizeof p, "%s/a_default.raw", d); take(p, tk, 0, -1, 57);
    snprintf(p, sizeof p, "%s/b_fxoff.raw", d); take(p, tk, 1, -1, 57);
    snprintf(p, sizeof p, "%s/c_fxoff_lvl40.raw", d); take(p, tk, 1, 40, 57);
    snprintf(p, sizeof p, "%s/d_fxoff_lvl1.raw", d); take(p, tk, 1, 1, 57);
    snprintf(p, sizeof p, "%s/e_fxon_lvl40.raw", d); take(p, tk, 0, 40, 57);
    return 0;
}
