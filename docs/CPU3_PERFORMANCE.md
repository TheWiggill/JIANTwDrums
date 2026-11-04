# 0.9.9 cpu3: performance investigation and four-track allocation

## Reproduced cross-track stealing without chords

Four FM6/VOICE tracks each play one note at a time, with overlapping releases. At capacity, the old POLY allocator could fade another track's oldest released voice even though the requesting track had a released slot of its own. The new note then rendered alongside the fading victim, briefly increasing actual render work beyond the admission count. `voice_fairness_test` reproduces the old result (one cross-track fade) and checks zero unnecessary cross-track fades after the fix, with budgets of both 8 and 12. Each other track retains its held lead and tail. Normal global fallback remains available when the requesting track has no released slot. This is a concrete allocator issue; it does not establish the cause of every hardware dropout.

At capacity, reuse the requesting track's oldest released slot in place before looking elsewhere. The engine's existing retrigger path retains its phases and envelope continuity. Held notes and the original global overload protection retain their protection. Two mixed-song reference hashes intentionally change because tails are kept differently: `song/4track_mix` and `slicer/song_gate_stut`. Only those two entries were revised; 155 existing references are retained.

## Lossless DSP changes

* Chorus: after a full 2048-sample ring of zero input (about 46 ms), all three interpolated taps are provably zero. Skip their calculations while preserving buffer writes, LFO/write positions, parameter glides and all other FX. Resume on any nonzero input; never discard a chorus tail. Four bytes of state. The reference test forces the old taps, moves parameters and LOAD, drains/restarts repeatedly, and compares every output sample and the complete FX state over 12000 blocks.
* VOICE vowels: when TALK's interpolation weight is exactly zero, the second vowel does not contribute. Avoid its formant preparation and interpolation. No reduced sample rate, omitted resonator or change to WORDS synthesis. The WORDS path remains a priority for further work.

## Host measurements

These are GCC 14.2 `-O2` x86-64 diagnostic timings, using thread CPU time and medians of nine interleaved before/after runs. They are NOT FM1 CPU percentages. Binary layout, host scheduling/frequency and scene details affect timings, including unchanged engines. Small differences should not be treated as proven gains. The comparison isolates the DSP edits; fairness is established by separate allocation tests. Every benchmark scene has the same before/after audio checksum. Full audio/state checks are separate tests.

The FX figures are for the whole bus chain with the named input/model, not individual reverb algorithms in isolation. The current chain still processes delay/reverb with silent sends; more aggressive sleeping needs state/tail/freeze/self-oscillation tests and is not included here.

| Scene | Before ns/sample | After ns/sample | Host delta |
|---|---:|---:|---:|
| FX no input | 34.9 | 24.5 | -29.9% |
| FX chorus input | 32.1 | 31.6 | -1.5% |
| FX delay input | 31.5 | 20.5 | -34.8% |
| FX ROOM | 31.6 | 21.4 | -32.2% |
| FX SPRING | 46.1 | 34.0 | -26.2% |
| FX SHIMMER | 41.1 | 30.0 | -26.9% |
| FX RESO | 37.5 | 25.3 | -32.4% |
| FX CLOUD | 47.8 | 35.1 | -26.6% |
| mixer idle | 60.9 | 48.5 | -20.4% |
| six FM6 tracks dry | 339.2 | 313.5 | -7.6% |
| six VOICE tracks dry | 334.7 | 303.7 | -9.3% |

The no-chorus FX shortcut shows a clear reduction of about one third in its isolated chain scenario. Active chorus remains fully processed. A 139-preset survey (three runs each, four requested notes, dry track rendering with periodic retriggers) gives the following most expensive preset observed per engine. This is a controlled pattern, not a worst-case guarantee; drums may have fewer live voices due to decay/choke.

| Engine index | Most expensive preset observed | ns/sample | Live at end |
|---|---|---:|---:|
| 0 | CELLO | 57.3 | 4 |
| 2 | RESO | 54.4 | 4 |
| 3 | FLOAT PAD | 65.2 | 4 |
| 5 | CANTOR ES | 87.3 | 4 |
| 7 | FULL ORGAN | 68.0 | 4 |
| 10 | 808 BOOM | 39.4 | 3 |
| 11 | TAPE AIR | 59.8 | 4 |
| 12 | TINE EP | 84.7 | 4 |
| 14 | ROAR | 49.6 | 4 |

FM6 and VOICE are the leading engine costs in these scenes; some WHEEL/LOFI presets are also significant. SPRING/CLOUD bus-chain scenes cost more than ROOM here. Idle event processing measured about 3.8 ns/sample; full idle mixing was about 61 ns/sample before these edits. This does not measure note-on bursts, UI/IRQ load, USB copies/resampling or flash stalls on the actual FM1. Do not remove MIDI, sequencer, safety or ownership work based on idle timings. Existing measured per-track/FX splitting and deadline/late/shed diagnostics remain essential for hardware diagnosis.

## Four FM operators

The controlled FM6 test uses algorithm 32, identical sustained envelopes, four held notes, and six versus four non-silent operators. Two operator sample loops stop running, but the six-operator envelopes/state/control code remain. It saves roughly a quarter of the measured track render time in this scene, not a quarter of the whole firmware. Outputs/timbres intentionally differ. This is an activity-count experiment, not a completed six-to-four patch converter or a native FM4 mode.

FM6 already avoids sample loops for operators below its amplitude threshold. The survey found presets with one, four and five active operators as well as six, so a blanket four-operator limit would not help every sound. Arbitrarily deleting two operators can remove carriers or whole modulation paths and make patches silent. The current six-operator state remains 6528 bytes even when only four operators sound.

The repository also retains DIGITAL, a separate four-operator engine behind FELUCCA_FM4=1. Its two extra envelope arrays alone require 960 bytes when it coexists with FM6; re-enabling it is not automatically a RAM saving. The audit compiles that flag for feasibility after uploading the default FM6 package. It is research only: the beta continues using FM6 and preserves existing patches. A practical next option is a separately selectable FM4 engine/preset bank with explicit project compatibility, or curated reduced patches, rather than silently replacing every FM6 voice.

## Reproduction

Compile `tests/performance_probe.c` against each source revision using `cc -O2 -w -Ibuild/gen -Ifirmware/src ... -lm`. Copy this new probe to the cpu2 reference checkout first so both builds use the same harness. `tools/profile_performance.py BEFORE AFTER 9` prints the scene comparison. Raw results are in `docs/performance/cpu3-host.jsonl` and `cpu3-presets.jsonl`. No timing thresholds or automatic FM1 utilization conversions are imposed.

## 0.9.9 ctl64: blocks of 64 samples

The control block (core.h `CTL`, `tools/gen_tables.py`) went from 32 to 64 samples, with the half buffer kept at 128
frames: two `mix_block` passes per half instead of four, so the fixed cost of a block (`events_block`, `mc_split`, the
core-1 handshake, the buffers cleared and summed, the sends and the effect buses, the master) is paid half as often.
Everything that counted blocks of 32 now counts in `CTL` (envelope and LFO tables, glides, fades, the steal fade, the
leveler's and DUCK's followers, DRUM-X's FLAM and PUNCH, the FX gate, WORDS' phoneme timing, the FX layer's LPF / HPF
sweep back to open: `perform.c` `PF_CSTEP`, 192 a block of 32, so the layer is idle again ~45 ms after a key is let go
as before, not 95). The host tests that counted blocks (`reverb_test` DUCK's 3 / 20 ms, `voice_test`'s glide: 163
blocks of 32, 1/8 st a block, `perform_test`'s return to the live signal) count in `CTL` now. Two DRUM-X checks compared
a strike against another with the noise generator left where the last hit had taken it (a clap's first ms, a snare's
first 5 ms under PUNCH: the window's energy varied up to x5 strike to strike on the 32-sample build as well); they now
reset the kit's generator before each compared strike (`drumx_test.c` `same_noise`), so they compare the same noise.

Timing: the sequencer runs once a block as before, but a note falling inside the block carries its sample (core.h
`ev_off`, voice.c `voice_begin` / `voice_off_at`, `track_render`): the voice sounding is rendered up to that sample, the
new note from it (its amplitude ramp keeps the block's slope), a release likewise. `tests/ctl64_timing_test.c`: on a
MICRO-shifted pattern, every rendered onset is within 2 samples of the sequencer's sample-at-a-time position (POLY, MONO
cuts, kit hits); the 32-sample build placed them within −22 .. +5 samples. FM6 renders whole blocks (`engine_t.whole`):
its notes start at the nearer block edge, as before. Keys and MIDI are unchanged (the next block, as before).

Host CPU, exact instruction counts under valgrind (`tests/cpu_vg.py`, instructions / sample, x86-64 gcc -O2; the base
is `codex/fm1-v099-cpu5usb1` with the same sections added):

| Section | 32 | 64 | Delta |
|---|---:|---:|---:|
| mix/idle (nothing sounding: the fixed cost) | 516 | 426 | -17.5 % |
| seq/mono4 (four MONO tracks cutting each other, 1/16, dry) | 1514 | 1188 | -21.5 % |
| eng/DRUM (8 lanes) | 1477 | 1156 | -21.7 % |
| eng/FM6 (8 notes) | 1710 | 1507 | -11.9 % |
| eng/VOICE / VOICE.EN | 1500 / 1783 | 1337 / 1619 | -10.9 % / -9.2 % |
| eng/ANALOG / PHASE / LOFI / NOISE / WHEEL | 2210 / 2033 / 1899 / 1847 / 2071 | 2072 / 1896 / 1748 / 1694 / 1892 | -6 .. -9 % |
| rev/ROOM .. CLOUD, dly/full .. quarter | 1416 .. 1597 | 1302 .. 1492 | -7 .. -8 % |
| mix/full (six parts, every FX) | 2671 | 2505 | -6.2 % |
| seq/plain / seq/heavy | 2381 / 2783 | 2158 / 2485 | -9.4 % / -10.7 % |
| song/cputest (.djf) | 3016 (3085) | 2742 (2812) | -9.1 % (-8.8 %) |
| peak/VOICE.EN6 (the worst block, per sample) | 3304 | 2937 | -11.1 % |

The fixed cost of a block fell from about 516 to 426 instructions a sample in the idle mixer (90 fewer: on the FM-1,
roughly 3 % of a half); the per-voice work is unchanged (`tests/target_budget.py`: the render loops' instructions as
before). Static RAM grew 9 KB (the CTL-long buffers; `.data+.bss` 88 KB of 96), the image 1.4 KB. The one audible side
effect is the effect buses' one-block lag, now 1.45 ms (see the README's dual-core section).
