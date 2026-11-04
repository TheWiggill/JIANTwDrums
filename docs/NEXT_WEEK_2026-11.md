# Next week (the owner, 2026-10-26)

Public and beta: v0.9.9-par8 (ee63cee.. line: port-p2 / seq-v2). Ultra stable: gen2 (fe81db8).

1. **V16 -> JIANT install**: not supported yet. On a recoverable unit: flash the official V16, dump the flash
   (scratchpad v16tool/ scripts were the comparison tools), confirm the update record near 0xD9F00 and the 320 MHz
   boot, make JIANT's installer / loader clear it and boot right, test on the owner's unit before publishing.
   Until then users say: "JIANT installs from the official firmware up to V15".
2. **The hiss** (docs/HISS_2026-10-26.md, tests/hiss_probe.c): with every engine, falls with the note, before the
   track's fader; a pure sine 76 dB SNR (~12.5 bits). Measure the shared chain stage by stage (voice_amp, the voices'
   sum into the part, the part's scaling), fix where the floor jumps; then ADD / WHEEL's uninterpolated tables.
3. **Live REC latency** (par8 REC_LAT ~5.8 ms, FM-1 only): the owner to confirm notes land where played (early ->
   lower it).
4. The list from 2026-10-25: generator (rhythm by genre: the owner's patterns or drum loops; the other genres after
   references), ADD / STRING more, ANALOG fatter, FM6 presets with an endless release, SAVE / SETTINGS pages, song
   mode redesign (SONG in HOME, ARRANGE timeline, M1 menu, section actions), the piano roll's x2 drawing, the old
   reference tests (djf, micro, cpu_vg mono4) updated, voices in an inaudible release cut (the +22 % of gen6e).
