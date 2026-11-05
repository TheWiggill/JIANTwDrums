/* SPDX-License-Identifier: GPL-3.0-only */
/* GEN SONG: distinct parts, complete arranger playback, visible progress and
 * whole-song SAVE entry. Host UI model; persistence/NOR is tested separately. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static void make_song(unsigned genre, unsigned seed)
{
    ui_power_on();
    ui_rec_prefs |= PREF_SIX >> 8; rp_apply();
    gp.genre = genre; gp.seed = seed; gp.energy = 60; gp.form = 2;
    gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
    for (unsigned k = 0; k < GP_N; ++k) gp.amt[k] = GA_AUTO;
    gen_go();
}

int main(void)
{
    int bad = 0, ok;
    unsigned total = 0;
    uint32_t h[4], sig;
    for (unsigned genre = 0; genre <= NGENRE + 1u; ++genre) {
        make_song(genre > NGENRE ? GEN_EXPLORE : genre, 31);
        ok = live.mode && chain_config.count >= 4 && dice_secs == 15;
        for (unsigned s = 0; s < 4; ++s) {
            ok &= proj_import(&proj_scratch, &proj_slot[s], sizeof(project_store_t));
            h[s] = 2166136261u;
            for (unsigned t = 0; t < NTRK; ++t)
                h[s] = (h[s] ^ proj_hash(proj_scratch.t[t].step, sizeof proj_scratch.t[t].step) ^
                        proj_scratch.t[t].p[P_MUTE]) * 16777619u;
        }
        ok &= h[0] != h[1] || h[0] != h[2] || h[0] != h[3];
        if (!ok) { printf("GEN failed genre=%u\n", genre); ++bad; }
        ++total;
    }
    printf("GEN SONG: %u genre choices have complete, non-identical parts\n", total);
    make_song(1, 31);
    nav_put(SEC_HOME, "GEN 1"); frame();
    sig = gen_draw_sig();
    press(B_PLAY); events_block(CTL); song_poll(); frame();
    ok = chain.running && chain.row == 0 && live.cur == 1 && gen_draw_sig() != sig;
    bad += check("GEN PLAY starts actual arrangement B; progress invalidates the graph", ok);
    sig = gen_draw_sig();
    events_block(chain_bar()); song_poll(); frame();
    bad += check("GEN progress refreshes at next bar of the same section", chain.remaining == 15 && gen_draw_sig() != sig);
    unsigned transitions = 0, last = chain.row;
    for (unsigned bar = 1; bar < 129; ++bar) {
        events_block(chain_bar()); song_poll();
        if (chain.row != last) {
            last = chain.row; ++transitions;
            ok = live.cur == chain.config.row[last].slot;
            ok &= proj_import(&proj_scratch, &proj_slot[live.cur], sizeof(project_store_t));
            for (unsigned t = 0; t < NTRK; ++t)
                ok &= !memcmp(trk[t].step, proj_scratch.t[t].step, sizeof trk[t].step) &&
                      trk[t].p[P_MUTE] == proj_scratch.t[t].p[P_MUTE];
            bad += check("GEN next row loads that section's actual notes and mutes", ok);
        }
    }
    bad += check("GEN traverses six rows, including break, and wraps the song", transitions == 6 && chain.row == 0);
    seq_stop(); song_poll(); frame();
    press(B_SAVE); frame();
    bad += check("SAVE from GEN opens whole-song destination without changing source",
                 cur_page()->graph == GR_SONGSAVE && song_save_target != song_cur && !name_on());

    /* Saved user sections must never cause SONG generation to degrade silently
     * into a new loop while erasing the arrangement. */
    make_song(1, 31);
    section_store(1);
    chain_config_t rows = chain_config;
    uint32_t slots = proj_hash(proj_slot, sizeof proj_slot), music = dice_print();
    gp.seed = 32; gen_go();
    bad += check("protected A-D refuses before changing music, sections, mode or arrangement",
                 gform_kept && live.mode && !memcmp(&rows, &chain_config, sizeof rows) &&
                 music == dice_print() && slots == proj_hash(proj_slot, sizeof proj_slot) &&
                 str_eq(ui.msg2, "USE AN EMPTY PROJECT"));

    song_poll();
    live.mode = 0; song_index_boot();
    bad += check("saved SONG mode survives boot of the same project", live.mode == 1);
    live.mode = 0; song_poll();
    live.mode = 1; song_index_boot();
    bad += check("explicit LOOP mode also survives boot", live.mode == 0);
    chain_config_t old = chain_config;
    old.rsv[2] = 0; song_idx.rows[song_cur] = old;
    live.mode = 0; song_index_boot();
    bad += check("legacy untagged projects retain default LOOP behavior", live.mode == 0);
    return bad != 0;
}
