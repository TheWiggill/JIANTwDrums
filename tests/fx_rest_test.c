/* SPDX-License-Identifier: GPL-3.0-only
 * (cpu5) the buses at rest (fx.c fx_rest): a chord into CLOUD, the delay (FEEDBK 60) and the chorus, then silence.
 * Against the same run built with FX_REST_OFF (the reference, fx_rest_test.py builds both): the output is the same
 * sample for sample until the gate has shut and the rest begins; through the rest both give exact zeros; the rest
 * engages in the block after the gate shuts and leaves the certificates held (room_used, dly_used clear, cho_zeros full); a
 * send after the rest wakes the buses in the same block, and from there the two runs differ at most by the residue
 * the gate was hiding (under its floor, 64 at the wet's scale: a limit cycle of the feedback, now gone), never by a
 * repeat or a tail. Prints a hash of the pre-rest output (the two must match: fx_rest_test.py) */
#define main hostsim_main
#include "hostsim.c"
#undef main
static uint64_t hash_bytes(uint64_t h, const void *p, size_t n)
{
    const uint8_t *s = p;
    while (n--) h = (h ^ *s++) * 1099511628211ull;
    return h;
}
#define NB_SEND 300u                                     /* blocks of the chord (218 ms) */
#define NB_QUIET ((uint32_t)FS * 5u / CTL)              /* then 5 s of silence */
#define NB_WAKE 2000u                                    /* a second chord, and its tail */
int main(int argc, char **argv)
{
    uint64_t h = 1469598103934665603ull, hp = h;
    uint32_t b, i, rng = 777, rest_at = 0, zeros = 0, worst = 0, first_rest = 0, leak = 0;
    int reference = argc > 1 && !strcmp(argv[1], "reference");
    int32_t out[2 * CTL];
    host_tracks_init();
    song.g[G_RTYPE] = 4; song.g[G_DFDBK] = 60; song.g[G_DTIME] = 6;
    for (b = 0; b < NB_SEND + NB_QUIET + NB_WAKE; b++) {
        int send = b < NB_SEND || (b >= NB_SEND + NB_QUIET && b < NB_SEND + NB_QUIET + 200u);
        for (i = 0; i < CTL; i++) {
            rng = rng * 1664525u + 1013904223u;
            fxq_c[i] = fxq_d[i] = fxq_r[i] = send ? (int32_t)(rng >> 18) - 8192 : 0;
        }
        fx_run(CTL);
        if (b < NB_SEND + NB_QUIET) {
#ifndef FX_REST_OFF
            if (!first_rest && rest_ph) first_rest = b;
#endif
            if (!rest_at) {                              /* until the gate has shut: the output hashed (the same both ways) */
                if (fxg_g == 0 && b >= NB_SEND) rest_at = b;
                else h = hash_bytes(h, fx_wet, sizeof fx_wet), h = hash_bytes(h, fx_sl, sizeof fx_sl);
            } else {                                     /* the gate shut: exact zeros with the rest. The reference lets
                                                         * the delay's residue reopen it now and then (fx_gate's dly:
                                                         * a flicker under its floor), and the reverb's WIDE side was
                                                         * never gated (its residue for ever): the rest silences both */
                for (i = 0; i < CTL; i++) {
                    uint32_t a = (uint32_t)(fx_wet[i] < 0 ? -fx_wet[i] : fx_wet[i]);
                    zeros += fx_wet[i] != 0;
                    if (a > leak) leak = a;
                }
            }
        } else if (b >= NB_SEND + NB_QUIET) {
            for (i = 0; i < CTL; i++) {                  /* after the wake: printed (the residue apart, the same) */
                uint32_t a = (uint32_t)(fx_wet[i] < 0 ? -fx_wet[i] : fx_wet[i]);
                if (a > worst) worst = a;
            }
            hp = hash_bytes(hp, fx_wet, sizeof fx_wet);
        }
    }
    (void)out;
    printf("fx-rest: mode=%s pre_hash=%016llx post_hash=%016llx gate_shut_at=%u rest_from=%u nonzero_through_rest=%u "
           "leak_peak=%u wake_peak=%u room_used=%u dly_used=%u cho_zeros=%u rest_ph=%u\n",
           reference ? "reference" : "rest", (unsigned long long)h, (unsigned long long)hp, rest_at, first_rest, zeros,
           leak, worst, room_used, dly_used, cho_zeros, rest_ph);
    if (!rest_at || leak >= FXG_FLOOR2) return 2;        /* the gate never shut, or more than a residue through it */
#ifndef FX_REST_OFF
    if (zeros) return 2;                                 /* the rest: exact zeros */
    if (!first_rest || first_rest > rest_at + 2u) return 3;   /* (the rest follows the gate at once; the gate's own
                                                         * timing: ~4 s here, the delay's FEEDBK tail holds its second floor) */
#endif
    if (!room_used || !dly_used) return 4;               /* the wake: the second chord excited them again */
    return 0;
}
