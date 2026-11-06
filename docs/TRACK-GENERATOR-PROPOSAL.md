# Per-track generator: implementation proposal

Requested alongside the DRUM/VOICE beta; not implemented in that release.

There is already reusable per-track generation in `gen.c:dice_seq(track_t *)`, reached by holding SEQ and turning SELECT. It calls `gen_role` for DRUM, BASS, PAD, LEAD, ARP or TEXTURE, uses existing genre/rhythm tables, and leaves the selected engine/preset in place. The user wants explicit controls, not just dice.

Proposed last SEQ page: **GENERATE**. Four knobs: GENRE, SEED, ROLE (tonal) / FILL (DRUM-X), ENERGY/DENSITY. OCT+ generates the selected track; OCT− restores its previous pattern through the existing dice/undo machinery. Keep HOME's full-song generator and all other tracks untouched. Use the track's current root, scale, length, DIV and FIT. A drum-and-bass rhythm on DRUM-X and a generated bass can accompany manually played lead/chords.

Required implementation details before release:

- Reuse pattern routines; never call `gen_make` or `gen_clean`, which rebuild the whole project.
- Isolate generator context. Current `dice_seq` mutates `gs`/`gmut`; the configurable wrapper must restore HOME's context instead of changing its song state. Existing main-loop scratch may hold the saved context if a compile-time size assertion proves it fits.
- Preserve other tracks, the selected sound, mix level, effects, automation, sections and arrangement. Current dice's `dlevel` and implicit length adjustments must not leak into this new operation.
- Keep generation atomic with the existing selected-track load guard; replace a phrase on a musical boundary while playback continues.
- Validate DRUM and tonal controls separately; don't offer meaningless roles to DRUM-X or change a tonal engine into another engine.
- Measure target flash and RAM after the fill changes. Generation adds no per-sample DSP or voices, but a page, controls and context isolation still consume firmware bytes. The previous complete candidate had only eight app bytes free; existing code reuse alone is not proof that a new page fits. Do not borrow loader space, the secondary-core safety reserve, audio buffers or the protected pool reserve.

Tests should snapshot all nonselected tracks and HOME generator state, generate each role/genre on the selected engine, verify those snapshots are unchanged, exercise live replacement/undo, and save/reload the resulting pattern. Normal target layout/flash/RAM checks are required.
