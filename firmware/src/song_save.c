/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* SAVE > PROJECT > SAVE SONG AS. Main loop, stopped only. song_keep and proj_wire are existing work
 * buffers: no new audio memory. The source stays open and untouched. Each destination section has the
 * storage layer's A/B protection; the four sections and index are NOT one atomic transaction. On an
 * interrupted overwrite the source remains available to retry the complete copy. */
#define SONG_MODE_TAG 0xA0u                      /* chain.rsv[2], previously zero: saved LOOP/SONG mode */

/* Normalize a source section into proj_wire, including current song-wide globals. 0 empty, 1 ready,
 * -1 cannot serialize. song_keep holds the unsaved live section for the duration of this operation. */
static int song_copy_section(uint32_t section, uint32_t current, const char *name)
{
    project_t *p = &proj_scratch;
    if (!proj_import(p, section == current ? &song_keep : &proj_slot[section], sizeof(project_store_t))) {
        if (section || !proj_import(p, &song_keep, sizeof song_keep)) return 0;
    }
    for (uint32_t i = 0; i < G_COUNT; i++)
        p->g[i] = mom_base(MOM_G, i, song.g[i]);
    p->g[G_DJF] = mom_base(MOM_G, G_DJF, motion_djf_value());
    p->chain = chain_config;
    p->chain.rsv[1] = 0;                         /* saved sections belong to the player, not GEN */
    p->chain.rsv[2] = SONG_MODE_TAG | (live.mode ? 1u : 0u);
    if (section == 0u && name) {
        memset(p->name, 0, sizeof p->name);
        memcpy(p->name, name, str_len(name) < PROJ_NAME_LEN ? str_len(name) : PROJ_NAME_LEN);
    }
    p->sum = proj_sum(p);
    return proj_pack(&proj_wire, p) ? 1 : -1;
}

static int song_save_as(uint32_t target, const char *name)
{
#if FELUCCA_FLASH
    uint32_t section, current, mask = 0;
    song_index_t *index = (song_index_t *)proj_wire_u.raw;
    _Static_assert(sizeof(song_index_t) <= sizeof proj_wire_u.raw, "song index fits existing staging RAM");
    if (transport_busy()) { ui_message("STOP TO SAVE"); return 1; }
    if (target >= NSONG || target == song_cur) { ui_message("CHOOSE ANOTHER PROJECT"); return 1; }
    if (!flash_ok) { ui_message("NO FLASH: NOT SAVED"); return 1; }
    current = live.cur >= 0 && live.cur < 4 ? (uint32_t)live.cur : proj_cur < 4u ? proj_cur : 0u;
    project_capture(&proj_scratch);
    if (!proj_pack(&song_keep, &proj_scratch)) {
        ui_message(proj_full ? "SECTION FULL" : "SAVE FORMAT ERROR");
        return 2;
    }
    /* Validate every section and every row's per-track sources BEFORE touching the destination. */
    for (section = 0; section < 4u; section++) {
        int rc = song_copy_section(section, current, name);
        if (rc < 0) { ui_message("SECTION FULL"); return 2; }
        if (rc) mask |= 1u << section;
    }
    if (!chain_valid(&chain_config)) { ui_message("INVALID ARRANGEMENT"); return 2; }
    for (uint32_t row = 0; row < chain_config.count; row++) {
        if (!(mask & (1u << chain_config.row[row].slot))) { ui_message("MISSING SECTION"); return 2; }
        for (uint32_t track = 0; track < NTRK; track++)
            if (!(mask & (1u << row_src(row, track, chain_config.row[row].slot)))) {
                ui_message("MISSING SECTION"); return 2;
            }
    }
    proj_wire_gen++;                             /* invalidate editor staging and the library, even on failure */
    for (section = 0; section < 4u; section++) {
        uint32_t obj = target ? OBJ_SONG1 + (target - 1u) * 4u + section : OBJ_PROJECT0 + section;
        int rc = song_copy_section(section, current, name);
        if (rc < 0) { ui_message("SAVE FORMAT ERROR"); return 2; }
        /* A zero-length committed object hides both older copies of an absent section. */
        if (st_save(obj, &proj_wire, rc ? sizeof proj_wire : 0u)) {
            ui_message("SAVE ERROR: RETRY"); return 2;
        }
    }
    memcpy(index, &song_idx, sizeof *index);
    index->rows[song_cur] = chain_config;
    index->rows[target] = chain_config;
    index->rows[target].rsv[1] = 0;
    index->rows[target].rsv[2] = SONG_MODE_TAG | (live.mode ? 1u : 0u);
    memcpy(index->scene[target], song_idx.scene[song_cur], sizeof index->scene[target]);
    memcpy(index->tsrc[target], song_idx.tsrc[song_cur], sizeof index->tsrc[target]);
    memcpy(index->tvar[target], song_idx.tvar[song_cur], sizeof index->tvar[target]);
    if (st_save(OBJ_SONGIDX, index, sizeof *index)) { ui_message("SAVE ERROR: RETRY"); return 2; }
    memcpy(&song_idx, index, sizeof song_idx);
    song_idx_dirty = 0;
    { char b[4]; fmt_int(b, (int32_t)target + 1); ui_say("SAVED SONG ", b); }
    ui.force = 1;
    return 0;
#else
    (void)target; (void)name;
    ui_message("NO FLASH: NOT SAVED");
    return 1;
#endif
}
