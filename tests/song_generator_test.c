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
    screen_save("build/song-check", "generator-song");
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
                 cur_page()->graph == GR_SONGSAVE && song_save_target == song_cur && !name_on());

    /* A saved/non-generated section is no longer a dead end. GEN owns only
     * the draft buffers and may replace them, even while the previous song plays. */
    make_song(1, 31);
    section_store(1); live.mode = 1;
    nav_put(SEC_HOME, "GEN 2"); frame();
    press(B_PLAY); events_block(CTL); song_poll();
    uint32_t before_seed = gp.seed; gp.sig = gen_sig();
    press(B_OCTUP); events_block(CTL); song_poll(); frame();
    bad += check("GEN 2 rerolls a playing song as a draft and resumes arrangement playback",
                 song_draft && !gform_kept && chain_config.count >= 4 && song.playing && chain.running && gp.seed != before_seed);
    screen_save("build/song-check", "generator-draft-page2");
    nav_put(SEC_HOME, "GEN 3"); frame();
    bad += check("GEN 3 keeps real parts visible while editing generator settings", gen_page()==2 && chain_config.count>=4);
    screen_save("build/song-check", "generator-draft-page3");
    seq_stop();song_poll();
    nav_put(SEC_SAVE,"SAVE SONG AS");frame();turn(EN_K2,1);frame();
    bad += check("SAVE scope selects LOOP without changing the live song", song_save_loop && chain_config.count>=4);
    song_save_loop=0;
    seq_stop(); song_poll();
    gp.form = 0; gen_go();
    bad += check("SONG to LOOP generation leaves only draft A and no old arrangement",
                 song_draft && !live.mode && !chain_config.count && dice_secs == 1 && project_used(0) &&
                 !project_used(1) && !project_used(2) && !project_used(3));
    gp.form = 2; gen_go();
    /* Draft arrangements intentionally do not overwrite the saved boot index. */
    song_draft=0;song_poll();
    live.mode=0;song_index_boot();
    bad += check("committed SONG mode survives boot", live.mode==1);
    live.mode=0;song_poll();live.mode=1;song_index_boot();
    bad += check("committed LOOP mode survives boot", live.mode==0);
    chain_config_t old = chain_config;
    old.rsv[2] = 0; song_idx.rows[song_cur] = old;
    live.mode = 0; song_index_boot();
    bad += check("legacy untagged projects retain default LOOP behavior", live.mode == 0);
    return bad != 0;
}
