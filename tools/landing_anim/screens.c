/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * The front page's screens (web/landing/screens/): the firmware's own pages, recorded whole and moving. The browser
 * emulator's sources (web/emu/felucca_web.c: the whole firmware on a simulated FM-1, nothing patched) built for the
 * host; a song from the generator plays on all six tracks while each scene is reached with the panel's own buttons
 * and knobs, as a person would, and its 240 x 240 screen is written out frame after frame.
 *   screens OUT.raw   ->  per scene: "SCN name frames\n" then frames x 240 x 240 RGB565 (little endian)
 * tools/landing_anim/screens.py packs them into animated WebP (tools/landing_anim/build_screens.sh). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define EXPORT
#include "web/emu/felucca_web.c"

static FILE *out;
enum { FPS = 15 };
static void ms(uint32_t n) { while (n--) web_ms(); }
static uint32_t btn_of(const char *n)
{
    static const char *const N[] = {"FX", "SCL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC",
                                    "OCT-", "OCT+"};
    uint32_t i;
    for (i = 0; i < sizeof N / sizeof N[0]; i++)
        if (!strcmp(N[i], n)) return 1u << i;
    fprintf(stderr, "screens: no button %s\n", n);
    return 0;
}
static void tap(const char *b) { web_buttons(btn_of(b)); ms(80); web_buttons(0); ms(260); }
static void hold(const char *b) { web_buttons(btn_of(b)); ms(700); }
static void release(void) { web_buttons(0); ms(260); }
static void turn(uint32_t role, int32_t n)
{
    int32_t s = n < 0 ? -1 : 1;
    while (n) { web_enc(role, s); ms(70); n -= s; }
    ms(200);
}
/* SELECT to the section's first page, then n detents on */
static void page(uint32_t n) { turn(EN_SELECT, -30); turn(EN_SELECT, (int32_t)n); }
static void track(uint32_t t) { turn(EN_ALGO, -8); turn(EN_ALGO, (int32_t)t); }
enum { N = 30 };                                     /* frames a scene: 2 s */
/* track t on engine e, its preset p, selected (as PRESETS would load it) */
static void engine(uint32_t t, uint32_t e, uint32_t p)
{
    set_engine_of(&trk[t], e);
    apply_preset_to(&trk[t], p);
    trk[t].p[P_MUTE] = 0;
    track(t);
    ms(400);
}
static void rec(const char *name, uint32_t frames)
{
    uint32_t f, i;
    fprintf(out, "SCN %s %u\n", name, frames);
    for (f = 0; f < frames; f++) {
        ms(1000u / FPS);
        for (i = 0; i < 240u * 240u; i++) {
            uint16_t v = web_fb[i];
            v = (uint16_t)((v >> 8) | (v << 8));
            fwrite(&v, 2, 1, out);
        }
    }
}
int main(int argc, char **argv)
{
    out = fopen(argc > 1 ? argv[1] : "screens.raw", "wb");
    web_nor_erase();
    web_boot();
    ms(9000);                                         /* the splash */
    trks_put(1);                                      /* GLO > SYSTEM > TRACKS 6: every being awake */
    web_gen(0, 11, 70, 1);                            /* EXPLORE, a song (FORM SONG: TIMELINE has its rows) */
    song.g[G_BPM] = 118;
    if (!song.playing) transport_req = 1;
    ms(1500);
    /* HOME: the ecosystem, then the drum track's being (its kit of little beings), then GEN */
    rec("home", N);
    tap("HOME"); rec("being", N);
    tap("HOME"); rec("gen", N);
    tap("HOME");
    /* the engines' own pages (EDIT 1: each engine draws its own creature, alive with its sound), on T5; DRUM-X on T1 */
    engine(4, ENGI_ANALOG, 0); tap("EDIT"); page(0); rec("e_analog", N);
    engine(4, ENGI_STRING, 0); tap("EDIT"); page(0); rec("e_string", N);
    engine(4, ENGI_ADD, 0); tap("EDIT"); page(0); rec("e_add", N);
    engine(4, ENGI_PHASE, 0); tap("EDIT"); page(0); rec("e_phase", N);
    engine(4, ENGI_LOFI, 0); tap("EDIT"); page(0); rec("e_lofi", N);
    engine(4, ENGI_WHEEL, 0); tap("EDIT"); page(0); rec("e_wheel", N);
    engine(4, ENGI_NOISE, 0); tap("EDIT"); page(0); rec("e_noise", N);
    engine(4, ENGI_ANIMAL, 0); tap("EDIT"); page(0); rec("e_animal", N);
    engine(4, ENGI_VOICE, 0); tap("EDIT"); page(0); rec("e_voice", N);
    engine(5, ENGI_FM6, 0); tap("EDIT"); page(0); rec("e_fm6", N);
    page(5); rec("e_operator", N);
    track(0); tap("EDIT"); page(0); rec("e_drumx", N);
    /* the sequencer: EUCLID on the drums */
    track(0); tap("SEQ"); page(3); rec("euclid", N);
    /* the mix, LIVE, the master's EQ, the song */
    tap("GLO"); page(2); rec("mixer", N);
    page(0); rec("mano", N);
    tap("FX"); page(15); rec("master", N);
    tap("SAVE"); page(0); rec("timeline", N);
    fclose(out);
    return 0;
}
