/* SPDX-License-Identifier: GPL-3.0-only
 * JIANT (0.6.26 .. 0.6.83) the song-idea generator's helpers, GEN's now: the roles, the engines' and the scales' numbers,
 * a tie, a held note, an euclidean rhythm, a name's word, a high part's level, a track's loop and what sounds in it,
 * SAVE > TOOLS's CPU TEST song. Included by ui_input.c (main loop only), before gen.c.
 * (p2, as main 0.9.18) The old generator itself (its 21 genres DSTYLE, their drum bars, dice_drums / bass / pads / lead /
 * arp / texture / sound) is gone: SEQ + SELECT dices with GEN's generator (gen.c dice_seq) */

enum { RL_DRUM, RL_BASS, RL_PAD, RL_LEAD, RL_ARP, RL_TEX, RL_N };
#define EG_A 0u                                         /* ANALOG */
#define EG_PH 2u                                        /* PHASE */
#define EG_LO 3u                                        /* LOFI */
#define EG_VO 5u                                        /* VOICE */
#define EG_NO 11u                                       /* NOISE */
#define EG_FM ENGI_FM6
#define SC_MAJ 1u
#define SC_MIN 2u
#define SC_DOR 3u
#define SC_MPEN 6u
#define SC_HARM 7u
#define SC_PHRY 8u
#define SC_MEL 11u
#define SC_MIX 4u
#define SC_LYD 9u

static void dstep_tie(step_t *s) { s->n = 0; s->time = ST_TIE; s->flags &= SF_MFINE; s->vel = 0; }
/* holds the note of step i over the next k steps (as far as the loop goes and the next note) */
static void dhold(track_t *t, uint32_t i, uint32_t k, uint32_t len)
{
    uint32_t j;
    for (j = 1; j <= k && i + j < len; j++) {
        if (t->step[i + j].time == ST_NOTE)
            break;
        dstep_tie(&t->step[i + j]);
    }
}
/* an euclidean rhythm: k hits spread over n steps, turned by rot */
static int euclid(uint32_t i, uint32_t k, uint32_t n, uint32_t rot)
{
    if (!n || !k) return 0;
    i = (i + rot) % n;
    return (i * k) % n < k;
}

static int dname_has(const char *s, const char *w)
{
    uint32_t i, j;
    for (i = 0; s[i]; i++) {
        for (j = 0; w[j] && s[i + j] == w[j]; j++) {}
        if (!w[j]) return 1;
    }
    return 0;
}
/* a part that sits high plays quieter: its level down with its average note above E4 (and its highest) */
static void dlevel(track_t *t, uint32_t len)
{
    uint32_t i, j, cnt = 0;
    int32_t sum = 0, top = 0, m;
    for (i = 0; i < len; i++)
        if (t->step[i].time == ST_NOTE)
            for (j = 0; j < t->step[i].n; j++) { sum += t->step[i].note[j]; cnt++; if (t->step[i].note[j] > top) top = t->step[i].note[j]; }
    if (!cnt) return;
    m = sum / (int32_t)cnt;
    if (m > 64) t->p[P_LEVEL] = (int16_t)clamp(t->p[P_LEVEL] - (m - 64) * 2, 50, 127);
    if (top > 79) t->p[P_LEVEL] = (int16_t)clamp(t->p[P_LEVEL] - (top - 79), 50, 127);
}


/* (0.6.81) a track's loop, a step that starts a note, the note sounding at a step (gen.c reads them too) */
static uint32_t dlen(const track_t *t) { uint32_t l = (uint32_t)t->p[P_SLEN]; return l >= 1u && l <= NSTEP ? l : 16u; }
static int dstarts(const track_t *t, uint32_t i) { const step_t *s = &t->step[i % dlen(t)]; return s->time == ST_NOTE && s->n; }
static int32_t dsounding(const track_t *t, uint32_t i)  /* the note sounding at step i of t's loop (held), -1 none */
{
    uint32_t len = dlen(t), j;
    for (j = 0; j < len; j++) {
        const step_t *s = &t->step[(i + len - j) % len];
        if (s->time == ST_NOTE) return s->n ? s->note[0] : -1;
        if (s->time != ST_TIE) return -1;
    }
    return -1;
}
/* (0.6.67) SEQ + SELECT: the selected track's sequence diced as DICE ALL writes a part (a genre, its progression and
 * energy; the track's own scale and root): the drums on a DRUM track; else the role its steps sound like (low: the
 * bass, chords: a pad, else a lead or an arp). The sound, the mix and the automation stay. Its message */
/* (0.6.83) SAVE > TOOLS 6 CPU TEST: the heaviest song the instrument should play cleanly, to measure on the device
 * (USB console status: audio_max_us, audio_late, voices_shed) and to listen for clicks. Every live track: chords of four
 * on every step at 160 BPM, gliding into each other (GLIDE, POLY), two step components on each (ARP SEQ / ECHO x8 and
 * RAND / OCT, DIV, LEN); the last live track the DRUM kit, every lane on every step, OCT / ARP on its steps; every FX
 * on (DIST, CHORUS, DLY with PITCH, the CLOUD reverb). UNDO (GLO + G4) brings the music back */
static void cpu_test_song(void)
{
    uint32_t k, i, n = trk_live;
    if (song.playing) transport_req = 2;
    song.g[G_BPM] = 160;
    song.g[G_RTYPE] = 4;                                /* (CLOUD: the heaviest reverb) */
    song.g[G_DPIT] = 12;                                /* (the delay's pitch shifter on) */
    for (k = 0; k < n; k++) {
        track_t *t = &trk[k];
        int drum = k + 1u == n;
        set_engine_of(t, drum ? ENGI_DRUM : k == 1u ? ENGI_FM6 : k == 2u ? 2u /* PHASE */ : ENGI_ANALOG);
        apply_preset_to(t, 0);
        load_begin(t, LOAD_PAT);
        track_defaults_steps(t);
        t->p[P_MUTE] = 0;
        t->p[P_VOICE] = V_POLY; t->p[P_SLEN] = 32; t->p[P_SGATE] = 120;
        t->p[P_GLIDE] = drum ? 0 : 90;
        t->p[P_DIST] = 30; t->p[P_CHOR] = 50; t->p[P_DLY] = 60; t->p[P_REV] = 70;
        for (i = 0; i < 32u; i++) {
            step_t *s = &t->step[i];
            memset(s, 0, sizeof *s);
            s->time = ST_NOTE; s->vel = 100;
            if (drum) {
                s->hit = 0xFF;
                comp_legacy(s, 0, i % 4u ? CP_OCT : CP_ARPN, i % 4u ? 4u : CP_ARG(0u, 2u, 0u));
            } else {
                uint32_t r = 48u + (i * 5u) % 12u;
                s->n = 4; s->note[0] = (uint8_t)r; s->note[1] = (uint8_t)(r + 4u); s->note[2] = (uint8_t)(r + 7u);
                s->note[3] = (uint8_t)(r + 11u);
                comp_legacy(s, 0, i % 3u == 0u ? CP_ARPQ : CP_ECHO, i % 3u == 0u ? CP_ARG(3u, 4u, 3u) : 6u);
                comp_legacy(s, 1, i % 2u ? CP_RAND : CP_OCT, i % 2u ? 7u : 5u);
                s->ctime = 5u | (6u << 3);
            }
        }
        comp_migrate(t->step, 32u);                     /* (seq2d: as components v2) */
        load_end(t);
    }
}
