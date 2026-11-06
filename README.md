Warning: truncated output (original token count: 30050)
Total output lines: 1409

Warning: truncated output (original token count: 30219)
Total output lines: 1414

Warning: truncated output (original token count: 79905)
Total output lines: 3440

# JIANT FM

**Bio-synthetic operating system for the M-VAVE FM-1** — synthesis, sequencing, mutation, performance.

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)
[![Emulator](https://img.shields.io/badge/try%20it-in%20the%20browser-ff7a00.svg)](https://juanjiant-bit.github.io/JIANT/emu/)

![JIANT FM: the signature, the screens in the COLORS skin and what's new through 0.9.2: six navigation rules, three skins, EXPLORE, VOICE words, ANIMAL, DRUM-X octaves, REC in 64ths, the DJ FILTER and USB stems](docs/jiant-hero.png)

JIANT FM is an alternative firmware for the **M-VAVE FM-1**, made for playing live and building whole songs without
a computer. It is a fork of [Felucca](https://github.com/hugelton/Felucca) 1.1.5.1 by Hügelton Instruments (GPL-3.0)
and takes its song system from [SLOOP](https://github.com/isod89/sloop-fm1).

**▶ [Try it in the browser](https://juanjiant-bit.github.io/JIANT/emu/)** · **[Interactive manual](https://juanjiant-bit.github.io/JIANT/manual/)** · **[Web installer](https://juanjiant-bit.github.io/JIANT/webapp/installer/)** · **[Web editor](https://juanjiant-bit.github.io/JIANT/webapp/editor/)**: the emulator runs the same code as
the FM-1, with sound, and is played with your computer keyboard or the mouse. The emulator, the installer and the
editor update themselves with every change merged into `main`.

## JIANT FM V10 — 2026-10-30

**V10 is the new base of JIANT FM**: one version, in both installers, tested on the FM-1. Earlier versions are no
longer offered; from here on every update starts from V10.

[Install](https://juanjiant-bit.github.io/JIANT/webapp/installer/) · [Emulator](https://juanjiant-bit.github.io/JIANT/emu/) · [Editor](https://juanjiant-bit.github.io/JIANT/webapp/editor/) · [Beta installer](https://juanjiant-bit.github.io/JIANT/preview/webapp/installer/)

### Installing

- **Official V16** (M-VAVE, October 2026; what a new FM-1 ships with): press **Install**. V16's own boot stage is
  kept; JIANT runs on it.
- **Official V15**: press **Install**.
- **JIANT, any version**: press **Install**; your projects stay.
- **Felucca, Skiff or another custom firmware**: open **Return to official** first (it saves that firmware's backup),
  then Install.
- **Back to official**: **Return to official** takes M-VAVE's own `FM-1.fwsc` of **V16** (718,196 bytes) or **V15**
  (699,956 bytes), checked byte for byte. It saves a complete JIANT backup first; the editor restores it later.

Chrome or Edge on a computer, or Chrome on Android with a USB OTG cable (iPhone and iPad have no Web MIDI). Use a
data USB cable straight into the computer, and keep it connected until the page says **Done**.

### What's in V10

**Songs**
- **SAVE › TIMELINE**: the song left to right, its bars the width. A lane for the rows (their section and bars) and
  one per track. Each row can take any track from another of the song's four sections (**FROM**) and vary it in that
  row only (**VAR**: MUTE, THIN, FILL, HALF, REV, SHIFT, OCT+, OCT−, VAR 1–4, the same row always the same). A track
  that follows its row is dim, one from another section bright with its letter, a muted one an empty frame.
  KNOB 1 the row, KNOB 2 the lane, KNOB 3 / 4 the lane's two values.
- 8 songs × 4 sections (A–D), up to 16 rows each, scenes per row, SONG REC, quick chains, the song's END.

**Sound**
- Engines: DRUM-X, VOICE, ANIMAL, LOFI, ANALOG, FM6, PHASE, WHEEL, NOISE, **STRING** (Karplus-Strong, with **FDBK**:
  guitar feedback, MOVE, bloom, fuzz, the envelope's attack and release) and **ADD** (additive, ten spectra).
- DRUM-X **EDIT › KIT 4 › ACOUST**: a synthesized acoustic voicing of the kit, 0–100 %.
- Master: leveler, CLIP, TAPE (drive and wow), a 4-band EQ; FX: delay, reverb (SHIMMER, RESO, CLOUD), chorus,
  slicer, punch-in effects.

**Sequencer and recording**
- Piano roll on every SEQ page; step components (SHIFT, CHORD, RATCHET, ARP…); EUCLID and MODES; PLAYHEADS; STRETCH.
- **SEQ › EUCLID and MODES** (V10.6, in MUTATE's place; MUTATE is gone): HITS, STEPS (2–64) and ROTATE write a
  euclidean pattern, live; on DRUM-X each of the eight sounds its own (LANE; V10.7: LANE ALL, KNOB 1 DENSITY, writes
  a whole kit), on a melodic track the notes in the
  track's SCL, or the key the other tracks fit, or a new one. MODES adds ACCENT, GHOST, RATCHET and VARY (DRUM-X) or
  MOTION, HARMONY, OCTAVE and SLIDE (melodic). OCT+ a new variation, OCT− the one before; GLO + G4 UNDO.
- **REC + A3** undo / redo, **REC + B3** quantize on/off, **REC + knob 2** REC Q; REC and PLAY independent.
- **SEQ + E5 / F5** copy / paste a whole sequence.
- GEN (HOME + SELECT): the song generator by genre (V10.8: a part set OFF stays off, the GENRE is kept between
  visits); GLO › LIVE: the six sequences as a live board.
- **SEQ › STEP KNOB 4 VEL** (V10.8): the step's velocity 1–127, then ACC, SLD, A+S.

**Connection**
- USB name **FM-1**, so the apps made for the FM-1 find it; USB STEMS (8 channels: T1–T6 + FX) for recording in a DAW.

The manual ([EN](docs/MANUAL.md) · [ES](docs/MANUAL.es.md)) and the cheat sheets below describe V10 (V10.61).

## Reporting a crash

If the FM-1 crashes, the red screen now stays **10 seconds** (it was 4) and names the core, the VEC, PC and SP, and the bus
master that wrote or read where it should not. **Take a photo of it**, then power on and open **MENU > ABOUT**, which keeps
the CRASH line (VEC / PC / SP / bus) until power-off. Report both, with the version you installed, in a
[GitHub issue](https://github.com/juanjiant-bit/JIANT/issues/new/choose) (the bug report template asks for the rest).

> **Status of `main`: the 0.9.18 development line (the text below).** The public and beta installers use separately pinned 0.9.9-par revisions: see *Current versions* above. It builds and passes every test (Felucca's and its own). Back up the flash before installing it.
  **RATCH and ARP spill over the steps** (0.9.18) *(in the 0.9.1x line, installer: 0.9.17; not yet in 0.9.9-beta4a)*: RATCH x1–x8 (ECHO folded in: FLAT, FADE, RISE, a pitch step) and
  ARP one note at a time at its RATE play on past their step; OCT an octave a lap; SEQ › COMP frames the step it edits;
  the SEQ › BAR page is gone.
  **Every SEQ page is the piano roll** (0.9.17) *(in the 0.9.1x line, installer: 0.9.17; not yet in 0.9.9-beta4a)*: the same bars' strip and roll (a kit: the grid) on all the pages,
  the values on top, each page marking what it does on the notes: PATTERN their GATE and SWING, TRACK the notes moved
  by PITCH, PLAYHEADS the extra heads as they play, COMP the steps with a component, AUTOMATION and AUTO LIST every
  record (V10.6: EUCLID and MODES their pattern; MUTATE is gone). The BOTTOM skin is retired (a saved BOTTOM opens as AIR).
  **DRUM-X, VOICE and ANIMAL first** (0.9.9): the FM-1 powers on with T1 DRUM-X, T2 VOICE singing words, T3 ANIMAL's
  bird, T4 LOFI's bytebeat, T5 ANALOG's bass and T6 FM6's electric piano, and PRESETS lists the engines in that order.
  **A new skin, AIR** (0.9.7, the default of new settings): HUD's light look on every page with the knob row on top,
  the middle of the screen free; HOME's beings over the whole screen. **MENU › SCREEN › SCOPE** (OFF by default): an
  engine's EDIT pages give its creature the whole panel. **HOME's beings have faces**: three isotherms coloured by their
  own level, eyes that shut on a hit, a mouth that opens with the sound (V10.6: the look up went with MUTATE). 15 KB of
  flash given back, nothing removed, the sound bit for bit.
  **Every engine draws its own creature** (0.9.6): ANALOG a cell whose membrane is its wave, FM6 its operators as
  little beings wired as the algorithm (also on OPERATOR and OP ENV), WHEEL nine tube worms, one a drawbar, NOISE a
  swarm, PHASE a snail whose shell is the bent phase, LOFI a caterpillar whose skin is its output and segments its bits
  (0.9.10: CHIP flattens it into a DIP chip, 1BIT all the way; every knob shows on it).
  **Messages no longer darken the screen** (0.9.5): a message is a small pill over the page, which keeps moving behind
  it, clear of the page's words; a page's hint comes only once you stay on it (never while SELECT walks past), at most
  twice ever; a layer button's tap no longer says "HOLD [X] QUICK". On JIANT each card's colour follows its value
  again, as in 0.6.
  **Three engines draw their own creature** (0.9.4): VOICE its vocal tract singing, a glowing tube in a ghost of a
  head (0.9.9; the tongue, jaw, lips and velum move with the phoneme sung, glottal pulses travel up it, waves leave the
  mouth), ANIMAL a creature that metamorphoses between bird, insect and mammal, DRUM-X its kit as
  little beings that squash, shut their eyes and shout with every hit.
  **MUTATE can go back** (0.9.3): turning MUTATE off asks to return the track to its anchor; SEQ + D5 REVERT, GLO + B4 REVERT ALL, undoable. (V10.6: MUTATE, REVERT and REVERT ALL are gone; SEQ › EUCLID takes its place.)
  **0.9.2 is driven by six rules** (see [Navigation](#navigation)): SELECT walks a section's pages as one chain, the
  section's button jumps group to group, PRESETS always loads presets, KNOB 1 picks, a button held is its layer, and
  the tabs and the footer say where you are. Every page, knob and function is where it was; only the way between them
  changed.
  **The dice starts in EXPLORE** (0.8.6): HOME + SELECT opens GEN on a choice that follows no genre and breaks every
  rule (any tempo, odd metres and polyrhythm, exotic modes, any engine on any role, DRUM-X octaves, PLAYHEADS),
  each seed a very different piece, still in the key its SCALE and CHORD pages show. RANDOM and the 43 genres follow.
  **DRUM-X octaves** (0.8.5): the kit stays on its keys at every OCT; above the centre it repeats in time (0.9.13: once a step, x2, x4),
  below it plays SHORT, GHOST or FLAM, each sound its own on each step, and live recording keeps exactly what you
  played. MIDI drum notes outside 35–81 now trigger these variations: see the changelog before installing.

> ⚡ **New in 0.8.3 – 0.9.2: a new look, nine engines, finer timing and a map you can learn in a minute.**
> - **Flat navigation** (0.9.2): one chain of pages per section, the groups as tabs on top (a dot a page), the
>   breadcrumb **SECTION · GROUP · page n/N** at the foot; SCL and ARP two sections; GEN three HOME pages. Clearing a
>   step is **SEQ + G5**, renaming **SAVE + C5**: EDIT is always EDIT.
> - **Three skins, MENU > SCREEN > SKIN** (0.8.9; HUD faster in 0.9.0; 0.9.7: a fourth, **AIR**, the default of new
>   settings; 0.9.17: BOTTOM retired): **COLORS** (rounded cards, each
>   card its colour, on JIANT its value's heat as in 0.6), **BOTTOM** (the picture edge to edge, a slim knob row) and **HUD**
>   (HOME's beings under everything). The **LENTE** look (0.8.5): tabs, cards and a breadcrumb in a crisp 5 × 7 pixel
>   face, values in Inter Tight. **Held layers** show the keyboard itself: MAP, KNOBS and STATE (ALGORITHM while held).
> - **VOICE sings words** (0.8.5): ES, EN or JA, each note the next syllable of a lyric you type (or the dice's).
>   **ANIMAL** (0.8.3), the ninth engine: birds, insects, frogs and beasts from physical models.
> - **DRUM-X in every octave** (0.8.5): the kit stays on its keys; above the centre it ratchets (x2, x4, x8), below it
>   plays SHORT, GHOST or FLAM, each sound its own on each step, recorded as you played.
> - **The dice starts in EXPLORE** (0.8.6): HOME + SELECT opens GEN on a choice that follows no genre and breaks every
>   rule (odd metres, exotic modes, any engine on any role, PLAYHEADS); RANDOM and 43 genres follow.
> - **A DJ FILTER on the master** (0.9.1, GLO › MIXER KNOB 4): low-pass to the left, high-pass to the right, out of
>   the chain in the middle; automatable, saved with the project. **REC off the grid in 64ths** of a step (0.9.1):
>   SEQ › NOTE's MICRO, −32/64 … +31/64.
> - **USB audio that keeps its time** (0.8.9): no lost half buffers (the clicks under load), no dropped packets, the
>   48 kHz resampler on the second core. **Stems**: every track to the computer (MENU > USB > AUDIO STEMS), one channel each, into
>   your DAW.
> - Every version's details: the **[Changelog](#changelog)**.

> 🫶 If you resonate with my work, consider supporting me on **[Ko-fi](https://ko-fi.com/juanjiant)** (in Argentina:
> **[Cafecito](https://cafecito.app/juanjiant)**). Thanks for all the good energy, hope you enjoy this little beast ✨🎹

## Navigation

Ten buttons, ten sections, and the same six rules in every one of them (0.9.2). The map shows every section, its
groups (the tabs on the screen) and its pages in the order SELECT walks them, with the held layers and the combos.
Click it to open it full size ([SVG](docs/navigation-map.svg), zoomable · [en español](docs/navigation-map.es.svg)).

[![JIANT FM navigation map: every section, its groups and its pages in chain order, the six rules, the held layers and the combos](docs/navigation-map.png)](docs/navigation-map.svg)

1. **SELECT walks the pages**: one chain per section, a page a detent, stopping at the first and the last.
2. **The section's button: in, or on**: from elsewhere, the page you left there; inside, the first page of the next
   group (round). FX › DLY 2 → REVERB; EDIT › FILTER → FOCUS.
3. **PRESETS loads presets**: the track's sounds everywhere, the patterns in SEQ, the genre in GEN; never a page, a
   track or a module.
4. **KNOB 1 picks**: the step, the bar, the slot, the stage, the operator, the kit's sound. The track is always
   **ALGORITHM**'s.
5. **Hold = the layer**: a button held puts its shortcuts on the keys and knobs (SEQ + G5 CLR STEP, SAVE + C5 NAME,
   GLO + G4 UNDO …). Held + SELECT: **HOME** GEN, **EDIT / SEQ / LFO / FX** their dice, **GLO** the tempo,
   **ENV / SCL / ARP / SAVE** walk their chain.
6. **The screen says where**: on top the section's groups as tabs (a dot a page), at the foot
   **SECTION · GROUP · page** and **n/N**: **FX · DELAY · DLY 2  6/16**.

The whole chapter: [the manual, § 3](docs/MANUAL.md#3-pages-and-layers) ·
[el manual, § 3](docs/MANUAL.es.md#3-páginas-y-capas). The map is generated from the firmware's own chain tables
(`firmware/src/ui_lens.c`) by `tools/gen_nav_map.py`.

## Manual

What changed in each version: the **[Changelog](#changelog)**, at the end of this page.

The full user manual, every page, layer and setting, with a careful chapter on saving: **[English](docs/MANUAL.md)** ·
**[Español](docs/MANUAL.es.md)**.

**[The interactive manual](https://juanjiant-bit.github.io/JIANT/manual/)** (Spanish and English): the FM-1 drawn
with every control (touch one: what it does alone, held and in combination, with the firmware's real screen), every
command searchable, step-by-step recipes, the navigation map and what's new by version. Its source is
[web/manual](web/manual) (the chains and screens from the firmware, `tools/gen_manual.py`).

## Cheat sheets

Two printable sheets with everything on the device. Click an image to open it full size, or download the PDF.

| Controls | Sound · FX · Mod |
| --- | --- |
| [![JIANT FM cheat sheet: controls](docs/cheatsheet-controls.png)](docs/cheatsheet-controls.png) | [![JIANT FM cheat sheet: sound, effects and modulation](docs/cheatsheet-sound.png)](docs/cheatsheet-sound.png) |
| Every knob and button, the layers with their key maps (FX, GLO, SAVE, EDIT, LFO, SCL, SEQ, REC), the combos and every page's four knobs. **[PDF](docs/cheatsheet-controls.pdf)** | The eleven engines' EDIT pages (STRING and ADD included), FILTER, VOICE, ENV, LFO, the MOD matrix, MSEQ, macros, the sequencer, scales and ARP, songs, and every effect: DIST, the SLICER bus, delay, the five reverbs, the master, TAPE and the 4-band EQ. **[PDF](docs/cheatsheet-sound.pdf)** |

## The idea

A pocket synthesizer shouldn't feel like a menu. JIANT starts from a simple question: what if the device behaved
less like a tool and more like an organism?

That's why everything in JIANT is alive and visible. The screen works like a **thermal camera**: colour isn't
decoration, it's intensity. Cold is cyan and violet, hot is red, orange, yellow and white. A high parameter burns; a
sounding voice glows; silence cools down. Each engine is a **being**, a protozoan drawn in lines whose shape comes
from what you're playing: open the filter and it grows spines, raise the resonance and its membrane vibrates, detune
the oscillators and its nuclei drift apart. HOME is an **ecosystem**: six creatures, one per track, breathing with
their own audio. You don't need to read numbers to know what's going on; you can see it.

At the same time, what is technical is shown as technical. The LFO runs at its rate, the envelope shows where the
voice is, the distortion draws its transfer curve, the delay its echoes, the reverb its response. The organic invites
you to play; the technical lets you understand what you did.

The second idea is that **randomness is an invitation, not an accident**. Every session starts with macros rolled
towards what your sounds have; HOME + SELECT writes a whole piece, EDIT + SELECT dices the sound; macro dice only land on what is playing.
The system pushes you to move things and always leaves you near something that works: the dice start from factory
presets and vary around them, never from nothing.

The third is that **everything is playable and everything is recordable**. Effects are played on the keys and land
in an automation lane; knobs record their movement; songs are built from bars and scenes, and recorded while you
play them. The synthesis engines run in real time on a chip designed for something else.

And the last one is a discipline: **lightweight**. Every graphic is a handful of lines, every effect fits in the
audio CPU budget, the signature shown at power-on is 428 bytes. An instrument that feels big inside a small device.

## The system

### Both cores (dual core)

*Since 0.6.71, experimental.* The FM-1's chip (JieLi AC791N) has **two processor cores**; Felucca and JIANT up to
0.6.70 used one, which ran everything: the sound, the screen, the keys. Now the second one helps with the sound:

- **How the work is shared.** Every 0.7 ms block, the tracks are split between the two cores by how long each took to
  render over the last blocks (timed as they play, since 0.6.73). While the first core renders its tracks, the second
  renders the others into buffers of its own; then they are added together and go through the master on the first
  core. The drum tracks go together, on one core or the other. Since 0.6.75 the second core also runs the effect
  buses (chorus, delay, reverb), on the sends of the block before (0.7 ms later, inaudible), so they no longer wait.
- **The same sound.** The sum is exactly the one-core sum: a test plays a whole six-track song both ways and checks
  that the output is identical, bit for bit.
- **More room.** With the second core doing part of the synth work, the load meter stays lower: more tracks, voices,
  FM6 and heavy reverbs before the clicks and the cut notes start. LOAD LITE / ECO are still there for the heaviest
  songs. **12 voices** (0.9.12, after [Skiff](https://github.com/illuminus5/skiff)): while the second core plays, the
  tracks share 12 voices instead of 8 (each still at most 8; ECO 6); with one core, 8 as before.
- **Units that cannot run it.** On some FM-1s the second core now and then reads a word of memory wrong, more often
  under load (found by the Melodee and fm1-x0x developers). JIANT guards against it: every job carries a check word,
  and if the second core reads a wrong one, faults or does not answer in time, it is **stopped at once and the first
  core plays every track again**, with no crash (at most one 0.7 ms block of those tracks is lost). Since 0.6.76 the
  effects' output is checked too: far out of range, the block is muted, their lines cleared and the second core stopped
  the same way. Such a unit
  then stays on one core **until it is switched off**, and so does any unit after a crash while both cores ran.
- **What it says.** MENU > SYSTEM > ABOUT, under AUDIO LOAD: **CORES 2**, or **CORES 1** with the reason: REC AT POWER-ON,
  BAD READ, NO ANSWER, FAULT, SINCE A FAULT (an earlier one this power-on), NO START.
- **One core on purpose.** Hold **REC** while switching the FM-1 on: that session plays on one core, as before.
- **Credits.** How to start the second core safely on the FM-1 and the guard against misreads come from
  [Melodee](https://github.com/keremimo/melodee) (keremimo) and [fm1-x0x](https://github.com/charlesvestal/fm1-x0x)
  (Charles Vestal), both GPL-3.0; JIANT's version is in `firmware/hal/fm1_multicore.h`.

### A living screen

![HOME, ARRANGE, SEQ › NOTE off the grid, FX › DELAY, the DRUM-X kit and VOICE's WORDS in the COLORS skin](docs/jiant-screens.png)

*HOME with its six beings, the song in ARRANGE, SEQ › NOTE with a note 13/64 late, the delay, the DRUM-X colony and
VOICE singing its lyric, in the COLORS skin, exactly as the firmware draws them (the renders of `tests/ui_render.c`).*

- **Thermal palette**: values, curves, steps, hits and notes are coloured by intensity (cyan → blue → violet → red →
  orange → yellow → white). Selection is violet and active is orange; on JIANT each card's value and gauge are its
  amount's heat (0.9.5, as 0.6), the other palettes give each knob its own colour.
- **Skins** (MENU > SCREEN > SKIN, 0.8.9): **AIR** (0.9.7, the default of new settings: HUD's floating words on
  every page, the knob row on top under the tabs, the middle free), **COLORS** (near-black rounded cards on top, a rounded
  panel) and **HUD** (HOME's beings swim under everything, the words float over them, the knob row at the foot; on the
  SEQ pages on top, as AIR's). (0.9.17: BOTTOM, the knob row at the foot on every page, is retired; a saved BOTTOM
  opens as AIR.) Every skin wears the **LENTE** layout (0.8.5): the
  section's groups as tabs on top (a dot a page), the breadcrumb **SECTION · GROUP · page n/N** at the foot. Values
  in [Inter Tight](https://github.com/rsms/inter-tight) SemiBold, names, tabs and the breadcrumb in a crisp 5 × 7
  pixel face, small captions in [Chakra Petch](https://github.com/m4rc1e/Chakra-Petch).
- **HOME, the ecosystem**: one being per track (its species from its engine, its shape from its four knobs). Each one
  heats up, swells and moves with its own audio; the selected track is drawn bigger; muted ones go dark; in silence
  they all stay still. (0.9.7) Each wears a little face: its eyes shut on a hit, its mouth opens with its level;
  muted, it sleeps.
- **Engine pages**: each engine draws its own creature (0.9.4 / 0.9.6, [below](#v096)): VOICE a singing head, ANIMAL
  a bird / insect / mammal, DRUM-X its kit of little beings, ANALOG a cell, FM6 a colony of operators, WHEEL tube worms,
  NOISE a swarm, PHASE a snail, LOFI a caterpillar. (Before: the sound's being drawn in isotherms, four contours of
  the same body, cold edge, hot core, as HOME's ecosystem still shows them.) The being's knobs read as what they are: **CUT** grows spines (more harmonics, spikier), **RES** makes the membrane vibrate,
  **DTN/SPRD** splits two nuclei, **NOIS/RAND/CRSH** frays the edge, **WAVE** changes the lobes.
- **DRUM-X, the colony**: one being per kit sound, each with its own shape (round kick, spiky snare, fringed hats,
  clustered clap, star-shaped cowbell) that swells and heats up when it hits (KIT 2, KIT 3, X-MOD); on KIT, SOUND 1–4
  and HOME (0.9.4) **the kit as little beings** with eyes and mouths that squash and shout with every hit.
- **Live technical graphs**: LFO (the wave runs at its rate, with playhead and current value), ENV (a dot travels the
  curve with the sounding voice), DIST (transfer curve by TYPE and TONE response), DLY and DLY 2 (the echoes, L and
  R), REVERB and REVERB 2 (the impulse response, stereo, drawn per model), CHORUS (the three modulated lines), AUTOPAN (the swing across the stereo field), MASTER
  (clipper curve, PUNCH shape, how much DUCK pulls down), ENV/LFO DEST (bipolar columns under each knob, lit by what
  they send), VOICE (one cell per voice and the glide curve), GLOBAL (the bar in thermal columns with swing, clock and
  tuning).
- **Load meter**: five small bars next to the battery showing the current audio load.
- **Power-on**: the "Jiant FM1" signature is written stroke by stroke in thermal colours (MENU > SCREEN > ANIMATION OFF: it shows
  up whole).

### Engines
V10 has eleven synthesis engines: **DRUM (DRUM-X), VOICE, ANIMAL, LOFI, ANALOG, FM6, PHASE, WHEEL, NOISE, STRING
and ADD**. The synthesis engines are listed in the order PRESETS shows them (0.9.9-par; the first six are the tracks at power-on, T1–T6).
- **ANALOG**: two oscillators with **INT** (an interval of ±24 semitones: fifths, octaves) and DTN; besides SAW, SQR,
  TRI, SIN and PWM it has **SYNC**, **RING** and **SAW3** (three saws). Its filter is per voice.
- **FM6**: Dexed's engine (imports .syx), with its algorithms drawn. Fully editable on the device (from Felucca 1.5):
  **EDIT > OPERATOR** (KNOB 1 the operator 1–6, then RATIO or FIXED frequency, FINE, LEVEL), **OP ENV** (the stage
  ATK DEC SUS REL, its RATE and LEVEL, drawn as the envelope) and **OPERATOR 2** (MODE ratio/fixed, DETUNE, VEL), the
  operator lit in the algorithm's colony of operators (0.9.6: each a little being wired as the chart was) with its
  frequency; OP ENV's operator rides its envelope. Editing makes the patch the track's own (SLOT OWN) and it is
  saved with the project; the macros (EDIT 1 / 2) still act on top. The web editor edits every byte of it too.
  **Voice bank** (from Felucca 1.4.1): import a 32-voice DX7 cartridge (.syx) in the web editor's 6-OP FM tab and press
  **Send to the device's voice bank**; on an FM6 track **SLOT** (EDIT 2, KNOB 4) then goes F1–F8, OWN, **B1–B32**
  (empty slots skipped), the voice's name on the chart. A voice loaded is copied into the track: projects and user
  presets keep it even after you send another bank. The bank lives in its own flash sector, written power-cut safe;
  the full backup does not include it (keep the .syx).
- **PHASE**: phase distortion with **FB** (the output feeds back into the phase: from hard edge to growl).
- **LOFI**: 1, 4 and 8-bit chip with **BYTE** (bytebeat: 32 formulas picked with ALGO and mutated with VAR). In
  BYTE, **MOTN** sets the speed of the formula's rhythm, **GRIT** crushes its rate, **BEND** folds time and **LOOP**
  repeats a short stretch of the formula: the noise becomes a tone tuned to the note. **FLOAT** is a floatbeat
  generator: 32 formulas of `t` made of sines (rhythm, melody and timbre at once), in the key played. **0.9.9-par6**:
  48 formulas each (ALGO 0–191), and MOTN past half is **SYNC**: the formula's rhythm follows the bar, so a note only
  transposes it.
- **STRING** (0.9.9-snd5): a Karplus-Strong string. EXCITE (NOISE, PLUCK, PICK, HAMMR, STICK), DECAY, BRIGHT, POS (where
  it is struck); BODY page: STRETCH (a stiff, metallic string), BODY, SIZE and **BOW** (a bow's noise fed into the loop).
  RELEASE is its damper: the string rings past the step and dies out when let go.
- **ADD** (0.9.9-snd5): additive. MODE picks the spectrum (HARM ODD WIDE BELL BAR DRUM PRIME FIFTH SUB PLATE), MORPH
  moves between spectra, BRIGHT and PARTIAL how many partials, SHIFT stretches them, COMB carves notches, DECAY lets
  the highs die first and MOTION sweeps the spectrum.
- **VOICE** (formants, after klattsch), **WHEEL** (drawbar organ) and **NOISE** (coloured and metallic noise).
  **VOICE sings words** (0.8.5): LANG ES / EN / JA, each note the next syllable of a lyric you type (on the device or
  from the web editor) or of phrases the dice puts together from each language's word bank; POS and LOOP automate it.
- **ANIMAL** (0.8.3): animal calls from physical models: a source that oscillates or strikes exciting a resonator,
  with a call's rhythm on top. **CMOS** is the owner's 40106 / 4093 breadboard "bird machine" (Schmitt-trigger RC
  oscillators: a slow one gates and sweeps a fast one, the exponential charge is the chirp's hook), **PULSE** an
  escapement into a resonator (crickets, katydid, cicada, frogs, toad), **WHSTL** a Hopf oscillator whose tone is born
  as the pressure crosses a threshold (canary, cardinal, dove, owl, peeper, bat), **WING** flight tones with drift and
  Doppler fly-bys (mosquito, bee, fly), **GLOTT** a glottal pulse into two formants with subharmonics and chaos (wolf,
  bark, roar, purr). GROUP makes chirps and trills, TEMP (°C) speeds insects up as warmth does (Dolbear's law); a note
  is a call. 24 presets.
- **FILTER on every engine** (EDIT > FILTER): **TYPE** LP, HP, BP or **COMB** (a comb tuned to the note you're
  playing; CUT moves it ±32 semitones and RES is how much it rings), **CUT** and **RES**. On ANALOG it's its per-voice
  filter, with DRV; on LOFI it's per voice too (its CUT and RES are the ones on this page); on the others it's a filter
  on the track that follows the LFO and the envelope (LFO/ENV DEST FLT), and its DRV is the track's DIST.

### DRUM-X: synthesized drums with morph
An 8-sound kit (BD SD CP CH OH TM RS CB) generated in real time, Microtonic style.
- Each sound has two sides, **A** and **B**; **MORPH** (KNOB 1) moves between them.
- **FOLD** runs each sound's oscillator through a wavefolder after its envelope: the hit starts bright and the tail
  returns to the clean wave. **FM** changes the timbre with harmonic FM in 8 bands; plus TUNE, TONE, DECAY, NOISE and
  DRV.
- **EDIT > SOUND 1–4** (one sound each, KNOB 1 picks it): SIDE, PITCH, PMOD; DECAY, NOISE, COLOR; **MODE**,
  **PM.TYPE** (how the pitch moves: DECAY, LONG like the 808, NOISE, SINE), DRIVE; **LAYER**, **FEEDBK** and a
  per-hit **SEND**.
- **The kit in every octave** (0.8.5): the keys play the kit on the same keys at every OCT; above the centre a key
  **ratchets** in time (x2, x4, x8), below it plays **SHORT**, **GHOST** or **FLAM**, each sound its own on each step,
  and live recording keeps exactly what you played. MIDI drum notes outside 35–81 trigger the same variations.
- **Group mutes** (KICK, SNARE, HAT, PERC) with GLO held and **per-sound mutes** with EDIT held.
- **EDIT > X-MOD** (0.5.1): a tempo-synced LFO on MORPH and RAND, which gives every hit its own MORPH and tune.
- Nine kits as the DRUM engine's presets (DRUM-X, 909 CLUB, 808 BOOM, LOFI DUST …); EDIT + SELECT rolls a new kit.

### Dice: the system invites you to move everything
- A button held + **SELECT** rolls that part's dice (0.9.2: SELECT alone walks the pages). **EDIT + SELECT rolls the
  track's sound dice**: a synth loads one of its factory presets at random and moves each engine parameter up to a
  quarter of its range; DRUM-X rolls the A and B sides of each sound around the factory kit. **SEQ / LFO / FX +
  SELECT** dice the sequence, the LFO, the effects, each shown **BEFORE** and **NOW** to keep or cancel; **GLO +
  SELECT** is the tempo. **HOME + SELECT** opens
  **GEN**, the song generator (since 0.6.91; HOME's last three pages since 0.9.2): it starts on **EXPLORE** (0.8.6: no
  genre, every rule broken: odd metres, exotic modes, any engine on any role; at power-on and NEW PROJECT, V10.8:
  after that it keeps the genre you picked), then RANDOM or a genre among 43 (house,
  techno, dub, UK garage, funk, post punk, jazz, ambient …), a seed, an energy, a form (a loop, parts, a club song); how much drums,
  bass, chords, arp and lead; complexity, swing, how human. **OCT+** writes the piece from them (OCT+ again: a new
  seed; a knob turned and OCT+: the same piece with that one thing changed), **OCT−** is done. Every piece has one
  key, written to the SCALE page (and the CHORD page set to its chords: the keys play along in key), drums from the
  genre's priority map, a bass archetype written against the kick, a functional or modal progression with cadences
  voiced and led voice by voice, a lead composed from a motif in periods and sentences, a sound recipe a role, the
  genre's kit and space (see v0.6.91, v0.8.3).
- **UNDO: GLO held + G4** takes back the last big change (DICE ALL, a sound diced, a clear, INIT, RECALL, a pattern or
  project loaded); pressing it again goes forward (redo). G4 breathes when there is something to undo.
- **Macros M1–M4** (LFO held; they stay latched when released until you press LFO again): every session starts with
  two random routes per track towards what its sound has. With the macros open, **F3** rolls new routes, only on
  the tracks that are playing; G3 clears them (LFO + SELECT rolls the track's LFO). They can be assigned by hand in MOD (SRC M1…M4) and are saved
  with the project.

### Punch-in FX: effects you play and record

> *In the 0.9.1x line (installer: 0.9.17), not yet in 0.9.9-beta4a:* the sweeps, TAPE STOP, the performance macros and recorded FX lanes below (0.9.15-0.9.17).
With FX held the keys become effects that act while hel…8050 tokens truncated…er a pattern of another
  length they drift across the notes (polymeter, as the OP-Z's). The COMP page marks the lane and where it is.
  Automatable. Costs nothing on the audio: a copy of the step once per step.

### Making a song (sections, TIMELINE, ARRANGE)

- **Sections**: make a loop, hold **SAVE** + **C4** to store it as section **A** (D4–F4: B–D; it works while playing
  and goes to flash when you stop). Change it and store B, C, D. SAVE + F3–B3 plays a section on the next bar.
- **TIMELINE** (SAVE opens on it at power-on, SAVE's first page): **OCT+** adds a row; KNOB 1 the row, KNOB 2 the lane.
  On **SEC**: KNOB 3 the section, KNOB 4 the bars. On a track lane (T1–T6): KNOB 3 **FROM** (another section's part),
  KNOB 4 **VAR** (MUTE, THIN, FILL, HALF, REV, SHIFT, OCT±, VAR 1–4), in that row only.
- **ARRANGE** (SELECT right past SECTIONS and PROJECTS, or SAVE + G5): KNOB 2 on **END** picks STOP, x2–x8 or LOOP;
  KNOB 4 right stores a **scene** (mutes, macros) in the row.
- **Play**: PLAY on ARRANGE or TIMELINE plays the song from the top; elsewhere SAVE + D5 (SONG mode) first.
  **OCT−** on a row asks **DELETE SONG ROW?** (OCT+ deletes). **SAVE + E5 (SONG REC)** writes the sections you play
  live into the rows instead.
- Step by step: [the manual, § 7](docs/MANUAL.md#your-first-song-step-by-step) ·
  [el manual, § 7](docs/MANUAL.es.md#tu-primer-tema-paso-a-paso).

### EUCLID

- **SEQ › EUCLID** (V10.6) writes the selected track's pattern as a euclidean rhythm, live; **MODES** (the next page)
  adds to it.
- **Drums**: LANE (KNOB 4) starts on **ALL**, where KNOB 1 **DENSITY** (1–8) writes a whole kit, with KNOB 2 STEPS and
  KNOB 3 ROTATE. Pick a lane (BD SD CP CH OH TM RS CB) to change that sound alone. MODES: ACCENT, GHOST, RATCHET, VARY.
- **Melodic**: KNOB 1 HITS (exactly that many notes), STEPS, ROTATE, KNOB 4 VARY; the notes stay in the SCL key, or in
  the key the other tracks play (else a new one). MODES: MOTION, HARMONY, OCTAVE, SLIDE.
- **VARY** (V10.16): above 0 the pattern is four turns of the cycle, A A' A'' B, each varied anew (HITS exact in
  each); a melody answers its own motif.
- **SEQ › MUTATE** (V10.16, the page after MODES): EVERY (OFF, 1–16 bars), AMOUNT, MODE (ORBIT round the anchor,
  DRIFT walking on, PULSE mutation and anchor by turns), WHAT (GROOVE, RHYTHM, FEEL, ALL). It mutates on the bar while
  playing; OCT+ mutates now, OCT− returns to the anchor (what you dialled).
- **OCT+** a new variation, **OCT−** the one before; **GLO + G4** (or REC held alone and let go) undoes.
- In one minute: [the manual](docs/MANUAL.md#euclid-and-modes-v106) ·
  [el manual](docs/MANUAL.es.md#euclid-y-modes-v106).

### Bluetooth

- **MIDI by Bluetooth** (V10.13): MENU › SYSTEM › BT ON, connect from an app (**JIANT FM-1**): notes, CC, clock in;
  MIDI OUT to the app. **PROX** (V10.14–V10.20): how close the **phone** is, as a modulation source (BT row › OCT+:
  phone away OCT− FAR, phone on the FM-1 OCT+ NEAR; then SRC PROX on a MOD slot). See the manual, § 14.

## Changelog

### V10.61 (beta)
- **PROX thresholds** (MENU › SYSTEM › BLUETOOTH, OCT+ for the PROX page): KNOB 1 **MIN** (under it PROX is 0: the
  phone lets go without taking it far away), KNOB 2 **MAX** (over it PROX is full), KNOB 3 **SPD** (the response, 1
  as before .. 5 quickest). Over the FAR / NEAR calibration, saved with it.

### V10.60 — public (2026-11-03)
- Stable installer: everything from V10.47 to V10.60: the INPUT engine (USB audio into a track, IN 1 / IN 2 / 1+2 /
  STEREO, GATE, KEYS, RING) and MOD SRC FOLLOW; USB HOST audio interfaces at 44.1 and 48 kHz (an experiment); USB
  IN+OUT without feedback; STEREO IMAGE per voice; DLY R TIME; the roll follows the playhead; GEN's sounds fixed; the
  master EQ no longer resets; menu and AIR fixes; FX > RPT at slow tempos; more free memory.

### V10.60 (beta)
- **More room in the firmware** (~4.3 KB free, was 0.8 KB): the conversion of old DIGITAL sounds and the reading of
  JIANT 0.1-0.5 projects moved into the Bluetooth segment. Nothing changes in use.

### V10.59 (beta)
- **FX › RPT at slow tempos**: 1/8 works from 41 BPM (was 81) and 1/16 from 21, recorded at half the quality
  (11 kHz, a lo-fi repeat). Faster repeats sound as before.

### V10.58 (beta)
- **The master EQ (and TAPE) no longer go back to flat on their own** (a user's report since V10.44): every read of a
  stored project or section, as some GLO pages do, set the EQ playing to the stored one. They change only when a
  project is loaded now.
- **FX › EQ with MENU › DISPLAY › SCOPE ON**: the master's scope live behind the EQ curve (OFF: the curve alone).
- **MENU in BOTTOM and HUD**: the four cards show at the foot (they were hidden under the panel, a blank band above).
- **AIR's HOME**: the beings swim under the cards (they crossed the tabs and the values).

### V10.57 (beta)
- **USB audio interfaces at 48 kHz on HOST** (OP-Z and the many interfaces without 44.1 kHz): set to 48 kHz and
  converted as they are read. The refusal now reads "NOT USB MIDI OR AUDIO 16BIT 44.1-48K".

### V10.56 (beta)
- **USB IN+OUT without feedback**: the computer's or the phone's audio no longer plays through the last track by
  itself (recording into an app that monitors its input, such as Koala, looped into a digital feedback). The input is
  heard on a track with the INPUT engine; STEMS+IN keeps its own stem as before.

### V10.55 (beta)
- **GEN's sounds fixed**: since V10.42 GEN put the wrong factory sounds on some roles (a sub bass on the bells, leads
  on the electric pianos and plucks, a pedal bass on the organs). Each role takes the right kind of sound again.
- The INPUT engine's meters keep their middle column drawn with no signal coming in.

### V10.54 (beta)
- **INPUT engine**: the USB audio input on a track of its own (a USB audio interface on HOST behind a powered hub, or the
  computer's audio with USB IN+OUT). EDIT › INPUT: **SOURCE** IN 1, IN 2, 1+2 (summed) or STEREO (the image kept), so
  two tracks can take IN 1 and IN 2 each; **GAIN**, **GATE**, **PLAY** THRU (always) or KEYS (only while a note plays,
  at its envelope: steps gate the input); EDIT › SHAPE: **WIDTH**, **RING** (KEYS: a sine at the note), **BITS**, **RATE**.
  The track's FILTER, DIST, LEVEL, PAN and sends work as on any part; the page shows both inputs' meters. While a track
  is INPUT, the input no longer also comes in through the last track. Presets: IN 1, IN 2, IN 1+2, STEREO IN, MIC GATE,
  SPACE IN, TRANCE GATE, RING IN, CRUSH IN.
- **MOD SRC FOLLOW**: the input's envelope follower as a modulation source, on any track and destination (an input
  opening a filter, ducking a pad, driving a send).

### V10.53 (beta)
- **USB audio interface input** (experimental, a user's ask): with MENU › SYSTEM › USB › HOST, a class compliant USB
  audio interface on a **powered hub** (the FM-1 does not power the port) is recognised besides MIDI keyboards: its
  input (16-bit, mono or stereo, 44.1 kHz among its rates — Behringer UM2 / UCA and the like) plays into the FM-1 through
  the last track, like USB IN+OUT's computer audio (its LEVEL, PAN and effect sends). Not yet tried on hardware;
  interfaces that need USB high speed or only do 24-bit / 48 kHz are not supported.

### V10.52 (beta)
- **IMAGE you can see**: a synth's EDIT › STEREO page draws the stereo field from L to R. Each voice sounding is a
  blob at its place, its size following its level, its note above it; silent voices are a small tick where they
  would sit; a bar shows how wide IMAGE opens.

### V10.51 (beta)
- **Fix**: the roll following the playhead (V10.47) did nothing on the FM-1: the panel was never redrawn while the
  playhead was outside the 16 steps shown. Now it follows.

### V10.50 (beta)
- **Fix**: on VOICE, V10.49's STEREO page showed and moved LYRIC instead of IMAGE (they shared a slot). A synth's
  IMAGE is now its own parameter on its own page (old projects: 0 %).
- **EDIT order**: the sound's pages first (FILTER, MORPH, RETRIG, ROTOR, WORDS, STEREO), then FOCUS, then **VOICE and
  VOICE 2 last**, in a tab of their own (FM6: FILTER, STEREO, the operators, then VOICE).

### V10.49 (beta)
- **Delay R TIME** (users' ask): FX › DLY 3, KNOB 4. The right echo gets its own time, as a ratio of TIME: =L (as
  before), x1/2, x2/3, x3/4, x4/3, x3/2, x2. TIME 1/8 with R TIME x3/2 puts the right echo at 3/16; it stays in time
  when TIME changes or is modulated. Not with PING-PONG or the grains. Saved with the project.
- **IMAGE on synths** (users' ask): EDIT › STEREO now also shows on synth tracks. In POLY, IMAGE spreads the voices
  across the stereo field (each voice its own place, like DRUM-X's sounds); the page shows where the eight voices sit
  and lights the ones sounding. DIST narrows it, as on DRUM-X.

### V10.48 (beta)
- **Room for what comes next**: the LOFI, ANIMAL and STRING animations moved to the Bluetooth part of the flash, freeing
  ~6 KB in the main firmware (it had ~100 bytes left). Nothing changes on screen.

### V10.47 (beta)
- **The roll follows the playhead** (a user's ask): with a sequence longer than 16 steps, while it plays and nothing
  was touched for 3 seconds, SEQ › STEP (the piano roll and the drum grid) shows the 16 steps playing. Touching a key
  or a knob holds the view where it is for 3 seconds, so what you see is what the keys edit.

### V10.46 — public (2026-11-02)
- Stable installer: everything from V10.36 to V10.46: the SAFE START (a start that hangs in the power setup is retried
  without it), LFO DRIFT and breathing pads, in-tune random intervals, ANALOG SAW3 / PLS3 with an interval per
  oscillator, GLO pages reordered with BPM on MIXER, USB HOST (an experiment), presets less alike with more basses and
  leads, pages that show what they do (LEVELS, MASTER, EQ, PUNCH, SYSTEM 2, MORPH), the LIVE DJ filter fix.

### V10.46 (beta)
- **Safe start**: if a start hangs between the power-supply setup (added in V10.11) and the UI, the watchdog resets it
  and the next start skips that setup (as JIANT did up to V10.10), showing SAFE START on screen, until the FM-1 is
  switched off. Prompted by reports of FM-1s that stayed dark after installing (cause not yet known).

### V10.45 (beta)
- **Fix**: GLO › LIVE › MANO KNOB 4 is the DJ FILTER again (since V10.37 it changed the tempo; a user's report).
- **MORPH draws the wave**: on ANALOG's PWM, SYNC, RING, SAW3 and PLS3 the MORPH page shows the wave itself as MORPH
  bends it (it was a ramp).

### V10.44 (beta)
- **More pages that show what they do**: FX › MASTER › EQ draws the four bands' curve (LOW / HIGH shelves, the two
  mids as bells, a dot per band); GLO › SYSTEM 2 draws USB, TRS and BT as boxes with an IN / OUT light that flashes
  as MIDI passes, and PROX's bar when Bluetooth is linked or in AIR. These pictures (and LEVELS') live in the
  Bluetooth segment, the app's flash being full.

### V10.43 (beta)
- **Pages that show what they do**: EDIT › LEVELS / LEVELS 2 (DRUM-X) draw a column per sound, its level the height,
  lit as the sound plays, the page's four framed. FX › MASTER draws CLIP's curve (in → out, the level now a dot),
  PUNCH's push as a bar and the duck's gain moving over its curve. FX › PUNCH shows the lane 16 steps a row, every
  step a box, each recorded punch in its colour and the playhead walking it (an empty lane used to draw nothing).

### V10.42 (beta)
- **Factory presets less alike, the basics there**: FM6's seven near copies (ICE BELL, DX KEYS, FM PLUCK, SOFT BELL,
  FM STRINGS, WOOD: the first eight patches bent a little) are now other instruments: **FM LEAD, SQR LEAD, SAW LEAD,
  SLAP BASS, SUB BASS, BELL PAD** (HARD BASS stays). WHEEL gets **PEDAL BASS** and **DRIVE LEAD** where DUB ORGAN and
  SOFT KEYS were; LOFI gets a third bass, **PULSE BASS**. Program-change numbers stay where they were (a project keeps
  its sounds: they are stored in it, not looked up by name).

### V10.41 (beta)
- **USB HOST** (experimental, a user's ask): MENU › SYSTEM › USB › HOST makes the FM-1's USB port the host of a class
  compliant **USB-MIDI keyboard or controller**, plugged straight in (or behind a hub), no computer needed. Notes, CC,
  program change, bend, clock and start / stop come in as USB-MIDI does. The FM-1 does not power the port: use a
  keyboard with its own power or a powered hub. Never saved (the FM-1 always powers on as a USB device). Input only.
  From Melodee (Kerem Kilic; the USB host enumeration by recursivefieldinstruments).

### V10.40 (beta)
- **Web editor follows the knobs that change with the mode**: with ANALOG on SAW3 / PLS3 it shows **INT 3** (−24…24
  semitones, it used to show NOISE 0…127 and could not send a negative interval), and STRING FDBK's or NOISE LFSR's
  knobs as the panel names them; changing WAVE / EXCITE from the editor resets the knobs that change meaning, as on the
  FM-1.
- **HOME's screen repairs itself faster**: the screen only resends what changed, and very rarely a small 8 × 4 pixel
  block could stay old for up to ~1.5 minutes on HOME. Now any such block is redrawn within ~10 seconds.

### V10.39 (beta)
- **ANALOG SAW3 / PLS3: each oscillator its own interval**. In the three-oscillator WAVEs the NOISE knob (EDIT 1, KNOB 4)
  becomes **INT 3**, oscillator 3's interval (±24 semitones); INT stays oscillator 2's. No noise in those two WAVEs (the
  factory SAW3 sounds had none: they sound the same). Changing WAVE into or out of them sets that knob to 0.

### V10.38 (beta)
- **Random intervals stay in tune**: GEN, EXPLORE and the sound dice (EDIT + SELECT) used to give ANALOG's **INT**
  (oscillator 2's interval) any value, so most generated songs had a dissonant oscillator. Now an interval chosen by
  chance is **0, a fourth, a fifth or an octave** (5, 7, 12, up or down; also VOICE's SHIFT), and the random macros
  never aim at it. Turning INT by hand still reaches any value.

### V10.37 (beta)
- **GLO › MIXER KNOB 4 is BPM** (the tempo at hand; the DJ FILTER stays on LIVE › MANO's KNOB 4), and GLO's pages
  go **LIVE · MIX · COMPS · GLOBAL · SYSTEM**: the performing ones first, the settings last.
- **Bluetooth named where MIDI is**: GLOBAL › **CLOCK** reads **USB +BT** (that source follows a clock sent over
  Bluetooth too), and GLO › **SYSTEM 2** shows Bluetooth's state on KNOB 3 (OFF, ADV, CONN, AIR) next to USB and TRS IN.

### V10.36 (beta)
- **LFO WAVE DRIFT** (a user's ask, for ambient): a smooth random wander, no steps; with RATE 4BAR … 16BAR drones and
  pads drift and breathe. The factory pads now **breathe by themselves** (DRIFT on the filter, some a little on pitch):
  ANALOG SOFT PAD, STRINGS, JUNO PAD, DARK PAD; FM6 PAD, FM STRINGS; PHASE GLASS PAD, VOX PAD; NOISE HISS PAD; LOFI CHIP PAD.

### V10.35 — public (2026-11-02)
- Stable installer: everything from V10.27 to V10.35: testers' fixes, EUCLID reversible on your own sequence (VARY in
  two halves), SCENE saved with the project, Bluetooth MIDI pairing (Just Works), a steady MIDI clock in and out, the
  DRUM-X kit HIP HOP and the pattern BOOM BAP, BLUETOOTH AIR (an experiment).

### V10.35 (beta)
- **BLUETOOTH AIR** (an experiment): MENU › SYSTEM › BT gets a third value. The radio only listens to the Bluetooth
  devices around (nothing sent, no MIDI) and PROX follows how strongly they are heard: a hand or a body near the
  antenna shadows them, no phone in your hand. Calibrate on the PROX page (OCT+ on the BT row) as with a phone.

### V10.34 (beta)
- **DRUM-X kit HIP HOP** (a user's ask): boom bap: a round kick with a knock, a fat cracking snare, a tight clap,
  dusty hats, a woody rim, a shaker. It suggests the new pattern **BOOM BAP** (SEQ › PATTERNS 14).

### V10.33 (beta)
- **MIDI clock steady**. Out (MIDI OUT CLOCK / ALL): each of the 24 pulses a beat leaves at its own time (USB and
  Bluetooth), not in bursts: a follower reads the tempo you set (104 read 105.1 and wandered before). In (CLK USB /
  TRS / Bluetooth): the tempo measured over two beats and held as a whole BPM; no more 103 / 105 / 104 jumping.

### V10.32 (beta)
- **Bluetooth MIDI pairs** with no PIN (Just Works) when the computer or phone asks for it: Windows and the Macs /
  Androids that refused before now connect. The pairing survives a power-off (nothing to pair again).

### V10.31 (beta)
- **EUCLID is reversible on your sequence**: it keeps what you had recorded (whole: notes, chords, velocities,
  components, micro timing) and puts it back when its settings are at rest again (VARY and MODES 0, your own rhythm)
  or **HITS / DENSITY go to 0** (YOUR SEQUENCE BACK). The whole copy is the last track EUCLID took; for another track
  taken earlier, its rhythm and notes come back.
- **SCL › SCENE is saved with the project** (ON and the six roles); an older project loads with SCENE OFF.

### V10.30 (beta)
- Optimisation by hand: repeated drawing code in one place; every screen exactly as before (checked on 9250
  renders). About 0.6 KB more flash free (4.4 KB in all).

### V10.29 (beta)
- Optimisation: more of the code built for size (EUCLID, SCENE, LIVE, MIDI LEARN, the song player, the engines'
  menus and presets); about 1.8 KB more flash free for what comes next. The sound is unchanged.

### V10.28 (beta)
- **EUCLID starts at 0** (no HITS, DENSITY 0, MODES and VARY 0): built from nothing.
- **A sequence the track already has** (recorded, edited, a generator song) is **transformed with its own notes**:
  VARY moves its hits and keeps its held notes; HITS / STEPS / ROTATE switch to the euclidean rhythm, still with your
  notes (**YOUR SEQ** / **YOUR NOTES**). DRUM-X: only the lanes it had.
- **VARY in two halves**: 0–50 varies the one cycle (no bars added, every value its own variation); 51–100 adds the
  four parts A A' A'' B, further apart the higher.

### V10.27 (beta)
- Testers' fixes: the **CHORD** page on screen no longer changes the next key's chord (with VOICING LEAD a key could
  play another inversion after the page was shown).
- **MENU** (HOME held): KNOB 1..4 each edit their own card's row (all four edited the selected row).
- **TAPE WOW** wobbles like a warped record: a random, smoothed pitch drift, up to ~±45 cents at 100 (it was ~4 cents).
- Slower: **LFO RATE** 8BAR and 16BAR (between the slowest Hz and 4BAR); **PLAYHEADS RATE** 1/8 and 1/16; **STRUM**
  as before up to 64, then up to ~1 s.

### V10.26 — public (2026-11-02)
- Stable installer: everything from V10.21 to V10.26, tested on hardware: SCENE, the redesigned EUCLID / MUTATE /
  PRESETS / TOOLS, NUDGE's micro timing, ENV 2, LP24, PLS3, STRING FDBK.

### V10.26 (beta)
- **SCENE** starts OFF with every track KEEP. **OCT+** dices the roles (each role one track at most: never two tracks
  doing the same, never two playing chords), **OCT−** all KEEP; turned ON with none set it dices; a role given by hand
  takes it from the track that had it.

### V10.25 (beta)
- **SEQ › STRETCH › NUDGE** in two modes: **OCT−** (blue) moves the whole sequence a step, as before; **OCT+** (red)
  moves the **cursor step in micro timing**, 1/64 of a step a detent (a tester's ask: per-step micro timing back on a
  knob). PRESETS picks the step.

### V10.24 (beta)
- **SCL › SCENE**, the multitimbral mode: give each track a role (KEEP, CHORD, ARP, BASS, LEAD); with SCENE ON one key
  plays them all at once from that note and the key (the kit keeps its sequence). REC records every role's track into
  its own steps. See the manual, "SCENE".
- **SEQ › EUCLID** redrawn: each DRUM-X lane a concentric ring in its heat colour, the lane picked bright, VARY's turns
  (A A′ A″ B) under the ring.
- **SEQ › MUTATE** redrawn: the anchor's pattern above, the pattern playing below with the changed steps lit, an arc
  filling to the next mutation; OCT+ MUTATE / OCT− ANCHOR on screen.
- **SAVE › PRESETS**: two columns (the engines with their beings and counts, the presets); turning plays each sound,
  **OCT+** keeps it, **OCT−** brings back the sound from before you browsed.
- **SAVE › TOOLS**: KNOB 1 the scope (TRACK, SOUND, SONG, SYSTEM), KNOB 2 the action, a picture of what it clears;
  **hold OCT+ a second** to do it (a bar fills; no dialog, UNDO as before).

### V10.23 (beta)
- **STRING FDBK** is heard: the loop took BLOOM's level twice (about −15 dB); now about four times louder.
- **EXCITE into FDBK** sounds at once: EDIT 2's knobs become GAIN DIST SPEAKR BLOOM, and GAIN used to inherit
  STRETCH's 0 (so FDBK sounded like PLUCK until GAIN went up). Each side now keeps its own four values; FDBK starts at
  GAIN 50 %, BLOOM 31 %.

### V10.22 (beta)
- **ENV 2** (ENV › after ENV DEST): a second envelope per voice, ATK DEC SUS REL, routed like the LFO: **SRC ENV2** on
  a MOD slot, to any DEST.
- **LP24** filter type: a 24 dB/oct low-pass (two poles in series, RES on the second), on every engine.
- **ANALOG WAVE PLS3**: three pulse oscillators (2 and 3 detuned each way), MORPH sets the pulse width.
- Tester report "STRUM drops an octave": not reproduced; a test now covers OCT −1 with STRUM, every VOICING and QUANT.

### V10.21 (beta)
- **SEQ › EUCLID**: the lane rows at the right are named correctly (they were one off, so the lane picked looked like
  another); the ring shows one cycle (the lane's, ALL's or the melody's) at the turn playing, its playhead going round,
  also with lanes of different STEPS and VARY's turns.
- **PROX page**: the instructions say the phone (FAR: phone away, NEAR: phone on the FM-1, top left).

### V10.20 — public (2026-11-01)
- Stable installer: everything from V10.10 to V10.20 (Bluetooth MIDI, PROX, EUCLID VARY turns, SEQ › MUTATE,
  SAVE › SECTIONS ERASE), tested on hardware. PROX follows the **phone's** distance (its Bluetooth signal), not a hand.

### V10.20 (beta)
- **PROX smooth**: one fixed signal reading (the radio's gain and raw value together), the median of nine packets and
  ~0.3 s of smoothing (V10.18's "the reading that moves most" picked a noisy byte: a random sample and hold). FAR and
  NEAR closer than 4 levels are refused (FAR AND NEAR TOO CLOSE).

### V10.19 (beta)
- The PROX page redraws as soon as a reading arrives (it could stay on NOT CONNECTED while connected) and shows the
  link as seen at the right of LEVEL: **L1 / L0** (the radio's link), the packets PROX read / the good packets received.

### V10.18 (beta)
- **PROX moves**: the FM-1 now reads all four signal-strength (RSSI) registers of each Bluetooth packet and uses the
  one that changes with distance (V10.14 read one that, on the hardware, did not move). The PROX page shows the four
  raw words and which one it uses; once FAR or NEAR is stored that choice stays.

### V10.17 (beta)
- **SAVE › SECTIONS: ERASE** (KNOB 2): pick a section with KNOB 1, KNOB 2 right, OCT+ asks **ERASE SECT B?**, OCT+
  erases it: the section is EMPTY again (flash too). The music playing stays. Stopped only.

### V10.16 (beta)
- **SEQ › EUCLID's VARY varies much more**: with VARY above 0 the pattern is **four turns** of the cycle (STEPS 16:
  64 steps, as many as fit), **A A' A'' B**: the first close to the grid, the last the most, with rolls at the end on
  DRUM-X. Hits move one to three steps; HITS stays exact in every turn. A melody's later turns answer the first one's
  motif (some notes kept, some new). VARY 0: one turn, as before.
- **SEQ › MUTATE** (EUCLID · MODES · MUTATE): mutations of the EUCLID pattern on the bar, while playing, around the
  **anchor** (what you dial). KNOB 1 **EVERY** OFF / 1 / 2 / 4 / 8 / 16 bars, KNOB 2 **AMOUNT**, KNOB 3 **MODE**
  ORBIT (from the anchor) / DRIFT (from the last one) / PULSE (mutation and anchor by turns), KNOB 4 **WHAT** GROOVE
  (a new variation) / RHYTHM (+ HITS or DENSITY, ROTATE) / FEEL (+ the MODES and VARY) / ALL. On MUTATE, OCT+ mutates
  now, OCT− goes back to the anchor. A DRUM-X lane picked mutates alone. Not while a song plays; no UNDO points.

### V10.15 (beta)
- VOICE's English dictionary moved into the Bluetooth area of the flash (14 KB freed in the app for what comes next).
  Without that area (a broken install) English words are read by the letter rules.

### V10.14 (beta)
- **PROX**, a modulation source from Bluetooth: how close the connected phone is (its signal strength, filtered).
  MENU › SYSTEM › BLUETOOTH (ON, connected) › OCT+ opens the PROX page: a meter, OCT− stores FAR, OCT+ NEAR. Then
  pick **PROX** as a MOD slot's SRC (any destination). It fades to 0 when the phone disconnects.

### V10.13 (beta)
- **Bluetooth MIDI in and out**: what a connected app sends plays the FM-1 as USB-MIDI does (notes, CC, program change,
  bend, clock with CLK USB, MIDI LEARN and MAP); what MENU › MIDI OUT lets out also goes to the app. No SysEx (the
  editor and updates stay on USB).
- ABOUT's credits and licences moved into the Bluetooth area (flash for the app).

### V10.12 (beta: the first radio test)
- **MENU › SYSTEM › BLUETOOTH** (OFF by default): ON, with the music stopped, calibrates the radio by itself and
  advertises **JIANT FM-1** as a Bluetooth MIDI device. The row shows ADV (advertising) or CONN (connected) and the
  connections since power-on; NO SEG if the Bluetooth code did not install, FAILED if the start failed. No MIDI goes
  through it yet: this beta tests the radio. If a start hangs, the next boot turns BLUETOOTH OFF by itself.
- The Bluetooth stack (Optimist's, open source) lives in its own flash area at 0xEA000, written by the installer with
  the rest of the package and checked before use.

### V10.11
- **Power supplies set up as the stock firmware does** (the DC-DC converter on; ported from Melodee, GPL-3.0): the
  radio's prerequisite for Bluetooth. MENU › ABOUT shows **DC** after the CPU and rail line when it took. Nothing
  else changes: the radio stays off.
- 0xEA000–0xFBFFF (72 KB) reserved for the Bluetooth code.

### V10.10
- **The Bluetooth radio's calibration is kept on update.** The stock firmware keeps the radio's RF trims and
  Bluetooth address at 0xE8000, with its update record at the end of that sector; JIANT's update loader erased the
  whole sector to clear the record. It now keeps the sector and clears only the record's 256 bytes. (The first step
  towards Bluetooth MIDI and a proximity sensor: see the BT check page, `webapp/btcheck/`.)
- **BT check page** (read-only): over MIDI (Android Chrome too) or the serial console, it reads the 12 KB at
  0xE7000–0xE9FFF and says whether the radio's calibration is there; the dump can be saved.

### V10.9
- **MENU › LARGE shows the animations** (testers: with LARGE on they were hidden): HOME's strip under the tall
  cards shows the six tracks' creatures in a row (the scope only if none has one); an EDIT page's strip shows the
  engine's creature, alive, instead of the page's title.

### V10.8
- **GEN: a part set OFF stays off**. BASS, CHORDS, ARP or LEAD set to OFF are never turned back on: before, with
  fewer than two melodic parts on, BASS and CHORDS were forced on. That rule now only fills in parts left on **AUTO**.
- **GEN keeps the GENRE** you picked between visits (it went back to EXPLORE every time GEN opened). Power-on and
  NEW PROJECT still start on EXPLORE.
- **SEQ › STEP (melodic): KNOB 4 is VEL** (was FLAG): 1–127 is the step's velocity (a step with none recorded shows
  96, what it plays at); a turn stops at 127, and past it, one detent each: **ACC** (accent, plays at 127), **SLD**
  (slide, keeps the velocity), **A+S**. Turning back down returns to VEL 127. The DRUM grid is unchanged (STEP LANE
  HIT ACC).
- **FX › DLY TIME: dotted divisions 1/4D, 1/8D, 1/16D**, placed by length among the others (… 1/2, 1/4D, 1/4, 1/8D,
  1/8, 1/16D, 8T, 1/16 …). Saved projects keep their times.
- **Less pitch wobble from the generators** (a tester: GEN sounded like "a passing train"): the random macro routes
  (GEN, the dice, the power-on macros) never aim at PITCH; GEN's EXPLORE uses a pitched delay seldom (one piece in
  ten, octaves only) and less glide, vibrato and detune; the FX dice's pitched delay is one in ten, octaves only (was
  one in five, with fifths and fourths).
- **"STOP TO EDIT" now reads "SONG PLAYS: STOP IT TO EDIT"**: shown when a song (SONG mode, SAVE + D5, or ARRANGE /
  TIMELINE PLAY) or a quick chain is playing; editing steps, GEN and EUCLID wait until it stops (or SONG mode is
  turned off with SAVE + D5, so PLAY plays the loop).

### V10.7
- **SEQ › EUCLID on DRUM-X: LANE ALL**. KNOB 4 LANE has nine values, **ALL** (the default) then BD SD CP CH OH TM RS
  CB. With ALL, KNOB 1 is **DENSITY** (1–8, default 4) instead of HITS, and DENSITY, STEPS and ROTATE write a whole
  kit: a kick, the snare on the backbeat (steps 5 and 13 of 16), closed hats (the open hat chokes the closed one), and
  sparser clap, open hat, tom, rim and bell as density rises; ROTATE turns the kick and the percussion (the groove),
  the snare and hats stay. Each density sets every lane's HITS / STEPS / ROTATE, so picking a lane afterwards shows
  them. With one lane picked, HITS / STEPS / ROTATE (and MODES, OCT+ / OCT−) write **only that lane**; the others stay
  as they were (a longer lane, least common multiple up to 64, makes them repeat). Turning LANE writes nothing.
- **EDIT held + PRESETS**: jumps to the first sound of the next / previous engine (its name on screen) instead of
  scrolling through every preset. Not while a momentary tweak (EDIT held + a knob turned) is in progress.
- **Fix**: SEQ + SELECT (the sequence dice) on a DRUM-X track gave an empty or almost empty pattern when the
  generator's genre had no drums (NEO CLASSIC) or with AMBIENT's sparse map; it now always plays a beat (a kit
  pattern that comes out with almost no hits is thrown again).
- **EUCLID's VARY only moves hits** (a step); it no longer adds a pickup or drops hits, so **HITS is exactly the number
  of hits / notes that sound** (HITS 3: three hits).
- **REC held alone = UNDO**: hold REC past HOLD (~0.5 s) and let go without touching anything else (no key, knob,
  button or OCT): UNDO of the last big change; hold it alone again: redo. Anything used while REC is held keeps it the
  REC layer as before (CLICK, REC Q, A3 …); a REC tap still arms recording; GLO + G4 still works.

### V10.6
- **SEQ › EUCLID and MODES replace MUTATE**: the SEQ chain is STEP · LENGTH · TRACK · EUCLID · MODES · PLAYHEADS ·
  STRETCH · COMP · COMP 2 · AUTOMATION · AUTO LIST. EUCLID: HITS, STEPS (2–64), ROTATE and LANE (DRUM-X) or VARY
  (melodic); MODES: ACCENT GHOST RATCHET VARY (DRUM-X) or MOTION HARMONY OCTAVE SLIDE (melodic). A knob turned writes
  the track's pattern again, live; OCT+ a new variation, OCT− the one before; GLO + G4 UNDO.
- **Gone**: MUTATE and MUTATE 2 (ORBIT / DRIFT, KEEP, REVERT, the BACK TO BEFORE MUTATE? dialog), GLO + A4 FREEZE,
  GLO + B4 REVERT ALL, SEQ + D5 REVERT, HOME's mutation look up and ring; GEN no longer turns MUTATE on. SEQ TOOLS
  loses C4 RANDOM, D4 COOK, E4 BEAT and C5 (the lane's RANDOM); SEQ + SELECT still dices with GEN's generator.

### V10 — 2026-10-30
- **The new base**: JIANT FM V10 replaces every earlier line in both installers (no earlier versions are offered).
- **SAVE › TIMELINE**: the song as lanes; each row can take a track from another section (FROM) and vary it in that
  row only (VAR: MUTE THIN FILL HALF REV SHIFT OCT+ OCT− VAR 1–4).
- **Official V16**: JIANT installs over M-VAVE's V16 and returns to it (Return to official takes V16 or V15).
- **USB name FM-1**, so the apps made for the FM-1 find it.
- From the add15 line: REC undo / redo and REC Q shortcuts, independent REC and PLAY, COPY / PASTE SEQ, the SHIFT and
  CHORD components, STRING FDBK (guitar feedback with MOVE), DRUM-X ACOUST, ADD kept as engine 13.

### v0.6.46
Feedback from users (YouTube, Reddit):
- **INIT SOUND is dry**: every effect send (DIST, CHORUS, DELAY, REVERB) at 0, so the effects are yours to add. On
  **FM6** INIT is the init voice (OP1 alone at full level, the rates at 99: a plain sine) instead of a factory patch.
- **NEW PROJECT** (it was INIT SYSTEM: SAVE > TOOLS, 5th row): a clean start, every track its engine's sound, dry, FM6
  the init voice, no steps or automation.
- **NOISE DUST** never silent: at DENS 0 it still crackles softly (DUST is only the crackle).
- **FM6 EDIT 1**: the chart says the algorithm's number also when ALG follows the patch ("ALG 05 PAT").
- **Web installer**: pick an earlier version to go back to (the last few stay there), and links to the manual
  (English, Español).

### v0.6.45
- **MORPH** (EDIT > MORPH) on ANALOG, PHASE and LOFI: the wave slides continuously into the next ones; on ANALOG and
  LOFI the SHAPE modulation sweeps it too. The page draws the morphed cycle.
- **Fix (reported on Reddit): "the sequence pitch drops by itself"**. It was SCL 2's STRN, a global transpose of
  every sequence that is easy to turn by mistake next to ROOT. Renamed **ALL TR**; when it is not 0 the piano roll
  shows an **ALL TR** badge and PLAY warns. 0 = off.

### v0.6.44
- **Punch-in FX latched by default** (asked by many): **FX LATCH** is now ON (turned on once for everyone; turn it OFF
  in MENU > CONTROL if you prefer holding). **Lock**: hold FX until its map opens and let go without touching
  anything: the FX layer stays open (**[FX] LOCKED**), as the macros latch with LFO; keys toggle effects, knobs are
  FX's macros, OCT− all off. Press FX again to unlock.

### v0.6.43
- **MUTATE** (SEQ > MUTATE, a suggestion from the community): each track's **SOUND** and **PATTERN** change by
  themselves every 1 to 32 bars, on the bar, in time with the clock; **AMOUNT** how much; **ORBIT** varies around an
  anchor (KEEP: OCT+ on the page) so the music stays itself for minutes, **DRIFT** wanders on. Never LEVEL, PAN or
  MUTE, never the kick. **FREEZE** (OCT− on the page, or GLO + A4) stops it all. The page draws the next 32 bars of
  every track; on HOME each being gets a ring filling up to its next change. (V10.6: MUTATE is gone, FREEZE and the
  ring with it; SEQ › EUCLID and MODES take its place.)

### v0.6.42
- **Step components, the second set** (SEQ > COMP): **RAND** (the note at random in the scale), **OCT** (an octave,
  every loop or every other one, by turns, at random), **TONAL** (the rest of the loop moved along the scale),
  **BEND** (the note glides over the step), **ECHO** (repeats, each softer), **PULSE** (again on the empty steps
  after it), **JUMP** (then another step), **REV** (back to step 1: ping-pong), **FILL** (only while FX is held) and
  **KILL** (cuts what the track sounds). Each with its picture on the page.

### v0.6.41
- **Smooth engine and waveform changes**: another engine fades the old one out (~12 ms) and what is held (keys, the
  sequencer's notes) goes on in the new engine, fading in, instead of being cut. A list value changed while notes
  sound (WAVE, CHIP, a type) dips for ~3 ms instead of clicking; the notes go on.

### v0.6.40
- **INIT SYSTEM** (SAVE > TOOLS, 5th row, with a dialog; UNDO brings the music back): everything to zero at once:
  every track its engine's default sound, no steps, automation or locks, the mix, effects, tempo and SLICER at their
  defaults, the song empty, macros at 0, the factory DRUM-X kit. Saved projects, user presets, MENU, CLOCK, TUNE and
  MIDI stay.
- **Power-on at zero**: the device starts as INIT SYSTEM leaves it. **RESTORE LAST** (MENU > SYSTEM) is now **OFF** by
  default; turn it ON to get the last session back at power-on (if you had set it OFF before, it reads ON once: set
  it again).

### v0.6.39
- **Step components** (OP-Z style), on a new page **SEQ > COMP** right after STEP: one per step, picked with KNOB 2.
  **ARP NOTES / SEQ / SCALE** turn a step into an arpeggio (its own notes, every note of the pattern, or the scale up
  from it; UP, DOWN, UP-DN, RND; x2 to x8 in the step; 1 to 4 octaves or 3 to 15 degrees) that goes on over the ties
  after it. **TRIG** plays a step only on some loops (1:2 .. 4:4, 1:8, FIRST, NOT FIRST). The step's own **CHANCE**
  and **RATCH** are back here too. The page draws the arpeggio's ladder (the note playing lit) and TRIG's loops.

### v0.6.38
- **Step recording, chords with a length and arpeggios**: on STEP with REC (stopped), hold a note or a chord and turn
  **KNOB 1**: it lasts that many steps, written when you let go. Press **ARP** while holding it: the chord goes into
  the roll as an arpeggio, KNOB 1 its resolution (1/16 .. 1/4), KNOB 2 the order (UP, DOWN, UP-DN, RND), KNOB 3 the
  octaves.
- **DICE ALL**: the lead never goes past G5 on any of its paths (a FRANTIC cell could reach C#6).

### v0.6.37
- **SEQ pages, lighter**: PHRASES and the step CHANCE page are out of the cycle; **SHIFT** is now the last SEQ page
  and has a third knob, **CHANCE**: the whole track's probability (100 % plays every step as written, lower thins it
  out), automatable like OFFSET and PITCH. Old projects and presets keep playing as they were.
- **Films**: the presentation video in the landing page's look, with a dark minimal techno track (low FM chords, acid,
  a cyber line).

### v0.6.36
- **DRUM-X: FM becomes FM ENV.** The old FM knob put a fixed FM over every hit and muddied the kit (the macros' dice
  liked it a lot). **FM ENV** strikes the FM with each hit and lets it go with the pitch envelope (a few ms): a beater,
  a click, a metal edge on the attack, the tail as the kit has it. Projects keep the value; its effect is the new one.
- **DRUM-X: EDIT > TONE**, a new page after X-MOD: **BP**, a band-pass on the hats, the rim and the cowbell (OFF or 15
  centres, about 1.5 to 9 kHz), and **FEEDBK**, every sound's oscillator fed back into itself (warm to growl to noise).
  Saved with the section's kit in a byte it already reserved: older projects open with both OFF, sections no bigger.
- **Films**: the features film's music is new and darker (E harmonic minor, Em(add9) Cmaj7 Am7 B7, a rolling sub, a
  low arp, the robot low and spaced) and the camera cuts to HOME's beings every two bars of a long scene; the DICE ALL
  film shows HOME's beings instead of the mixer.

### v0.6.35
- **DICE ALL: the drums as before, darker.** The chords, the variety and the guard rails of 0.6.34 stay; the drums go
  back to 0.6.33's, which sounded better: the DRUM-X kit rolled around the factory one through a soft low-pass, the
  patterns written whole (the energy no longer thins or fills them), and in the night's genres they almost always play.
  The night's genres come up four throws in five (IDM and DARK IDM still the most).
- **Two new films**: the DICE ALL film (one minute: a throw every few seconds, an animated die rolling between them,
  no names) and the features film updated; thumbnails for both (`tools/video/thumbnails.py`).

### v0.6.34
- **DICE ALL: a generator of pieces, not of conditions.** Each throw makes three choices and guard rails keep it
  musical inside the chaos (see the manual, chapter 8):
  - **A genre**: the dark ones, plus five new worlds: **SOUNDTRACK** (enveloping pads, no drums or a heartbeat),
    **HARD TECHNO**, **INDIE POP**, **ELECTRO PUNK** and **NEO CLASSICAL** (Reich's pulses and phases, Glass's additive
    arps, Pärt's tintinnabuli).
  - **An energy**: **SPACE** (two or three things, long tails: minimalism with cared-for timbres), MID, **FRANTIC**.
  - **An orchestration**: which roles play; **the drums may not play at all** (about a third of the throws).
  - Guard rails: **chords always of four notes**, **deep basses** (from E1), **arps in the middle octaves** (C3–C5:
    Alberti, Glass groups, a Reich pulse, a walk), **coherent melodies** (a motif in steps developed over four bars,
    ending on a cadence), **space between notes**, **high notes softer** and high parts quieter, the lead under G5.
  - **Seven drum kits** built on DRUM-X: 808, HARD, CLICK, INDUS, SOFT, GLITCH, ELECTRO.
  - Sounds always fit their role (never a bass sound off the bass); envelopes shaped for the role and the energy.
  - **A twist** in one throw in five: two loops phasing, a comb, the grain delay down, a polymetric lead, a drone.

### v0.6.33
- **DICE ALL: more IDM, darker, never flat.** IDM and a new **DARK IDM** (slow, phrygian, a drone, in-scale fourths,
  a few long notes) come up about 4 throws in 10; IDM's scales and progressions darker (PHRY, HARM; the flat second).
  - **A filter envelope on every sound** (strongest on the bass and the arps, their envelope made a pluck), so each
    note opens and closes instead of sitting still.
  - **The filter TYPE changes**: LP mostly, BP on chords, arps and textures, the COMB tuned to the note on arps and
    textures; a slow LFO of its own shape (SIN, TRI, S&H for the IDM arps) on the cutoff; a little drive (SOFT, FOLD)
    on some basses and leads; the drums through a soft low-pass (the hats not glassy).
  - **Delay and reverb vary**: the delay time (1/4, 1/8, 8T, 16T, 1/16), the GRAIN delay pitched an octave or a fourth
    down or a fifth up (often on IDM), the reverb type among ROOM, SPRING, RESO and CLOUD, its size, modulation and
    width; each track its own space, now and then one soaked and one dry, chorus on some chords.
  - Measured over 90 throws: brightness 1714 Hz (0.6.31) → **722 Hz**, the same loudness.

### v0.6.32
- **DICE ALL goes dark.** "Out of 30 sequences one is OK": the song ideas were shrill, cheesy, too bright and happy.
  Now they are night music: **DARK MINIMAL**, **DUB TECHNO**, **IDM**, **DARK ELECTRO**, **ACID NIGHT**, **COLD WAVE**,
  **BROKEN**, **HYPNOTIC**, **DEEP BREAKS**, in minor modes only (MIN, DOR, PHRY, HARM, MEL, MPEN; no major,
  mixolydian, lydian or blues), no pop progressions.
  - **A harmony bug fixed**: the chords were built from the scale's fifth and the lead from its third, so a v chord
    sounded over a i bass. Every part now starts from the same tonic.
  - **Chords with colour**: voiced as 7ths, 9ths without the 5th, in-scale fourths or suspended, and led to the
    nearest voicing in a middle register (no leaps); dub stabs keep one voicing, offbeat after offbeat, into the delay.
  - **Filtered**: every synth's cutoff goes down by role (chords and arps darkest), with some resonance; a slow LFO
    makes the chords breathe, the envelope plucks the arps. Bright factory sounds (bells, brass, marimbas, organs,
    chiptune) are never picked.
  - **Melodies lower and stranger**: the lead around C3–C4 in small steps, or a hypnotic euclidean ostinato that turns
    on itself (Marc Houle, The Hacker), or a few long notes; arps an octave lower without octave jumps.
  - **Grooves**: a rolling offbeat sub and a dub bass, minimal and deep-techno beats with ghosts, rims and chance,
    velocities a little human; darker delay and reverb (lower tone, more damping, no SHIMMER, CLOUD pitched down).
  - Measured over 90 throws: the mix's brightness (its spectral centroid) **1714 Hz → 1376 Hz**, the average note
    **an octave lower** (65 → 55).

### v0.6.31
- **The full manual, in English and Spanish**: [docs/MANUAL.md](docs/MANUAL.md) · [docs/MANUAL.es.md](docs/MANUAL.es.md).
  Every page and its four knobs, every layer's keys, the engines, DRUM-X, the sequencer, ARRANGE, the dice and UNDO,
  effects, modulation, performance, the MENU, MIDI, the web tools and troubleshooting, and a careful chapter on
  **saving**: what is saved where, how the autosave really works (10 s after the last change while stopped, never at
  power-off), quick stores, copying, switching projects, names and user presets.
- **SELECT on the MIXER sets the tempo**, as on GLO's other pages (GLO's button opens it); it said DICE: HOME / EDIT.
- **New README images** and **presentation video v3**: the screens as they look now, taken from the video's footage
  (song ideas, ARRANGE, UNDO, TRACKS and LOAD, one scale), with the support links at the end.
- README and cheat sheets corrected against the firmware: the autosave, ARRANGE instead of the SONG page, what the
  punch-in lane records, UNDO on GLO's G4, HOME held for MENU, the reverbs' AIR knob, DRUM-X's labels.

### v0.6.30
- **LOAD, next to TRACKS** (GLO > SYSTEM: KNOB 1 **TRACKS 4 / 6**, KNOB 2 **LOAD ECO / LITE / FULL**; also MENU > SYSTEM >
  LOAD). TRACKS says how many tracks play, LOAD how heavy the sound may get, so you choose: a lean, ultra-optimised
  system to push to the limit, or a slow one full of heavy effects. Everything is swapped only while it sounds; the
  project keeps its settings and sounds as saved back on FULL (or on another unit).
  - **FULL** (the default): everything as set.
  - **LITE**: the heavy parts swapped for their nearest light kin: SHIMMER and CLOUD reverbs as ROOM, RESO as SPRING,
    the GRAIN delay as a plain delay, FM6 two voices.
  - **ECO**: LITE, and every reverb a light SPRING (0.6.70: a shorter chain), **no SLICER** (its tracks go straight
    to the mix) and **no chorus**: their pages disappear until you leave ECO. Every synth track plays at most
    **4 voices** (the drums as ever), **6** between all tracks (8 otherwise), and the load budget steps in sooner.
  - Since 0.6.71 the second core takes part of the synth work ([both cores](#both-cores-dual-core)): FULL goes much
    further before LOAD is needed.
  - The cards of what's swapped say **LITE** or **ECO** under their value.
  - Why CPU and not RAM: the firmware reserves all its memory at boot, so nothing can be freed while it runs. What runs
    out is the audio time (the load meter, the CPU % card next to LOAD), and that's what LOAD gives back.
- 0.6.29's **6 LITE** comes up as TRACKS 6 + LOAD LITE. GLO > SYSTEM's USB card is gone (the header shows USB).
- **Ko-fi / Cafecito**: support links in the README, the web installer, the web editor and the emulator.

### v0.6.29
- **TRACKS has a middle option: 6 LITE.** GLO > SYSTEM > TRACKS (and MENU > AUDIO > TRACKS) now reads **4 FULL** (the
  default), **6 LITE** or **6 FULL** (the old 6 HIGH). With 6 LITE all six tracks play and the heaviest parts are
  swapped for their nearest light kin while they sound: the **SHIMMER** and **CLOUD** reverbs play as **ROOM**,
  **RESO** as **SPRING**, the **GRAIN** delay (DLY 2 PITCH / SPRAY) as a plain delay, and **FM6** plays two voices
  instead of four. Nothing in the project changes: back on 6 FULL (or on another unit) it sounds as saved. The cards
  that are swapped say **LITE** under their value.
- **Fixed**: when a message covered the header while the BPM was rolling, the old BPM could stay half-rolled.

### v0.6.28
- **"After clearing a sequence it still triggers notes on all tracks" — fixed.** The punch-in FX lane (effects
  recorded by holding FX keys while REC is armed) kept repeating the last notes of the cleared steps with STUTTER and
  ARP, and ARP played random fills on a DRUM track with no steps. Now a clear ends those repeats and forgets the notes,
  a DRUM track with no steps gets no fill, and an effect **latched** with FX LATCH no longer records itself into the lane
  (only one held by hand does). A section jump or quick chain waiting for the next bar is called off by a clear (it
  brought the steps back). To erase the lane itself: hold REC and press FX.
- **Clears say which track**: "T3 CLEARED", and below it "SEQ+REC: ALL TRACKS" (the others play on; SEQ held + REC
  clears all six).
- **T5 and T6 asleep (TRACKS 4) can't go unnoticed**: when a note is sent to them (a key, MIDI, their steps) or PLAY
  starts with steps on them, the screen says T5-T6 ASLEEP · GLO>SYSTEM>TRACKS 6.

### v0.6.27
- **DICE ALL never repeats itself**: the moment you press (the timer's ticks) stirs the random numbers on every throw,
  and the last 32 pieces are remembered by a fingerprint (every track's steps, engines and the tempo): a piece that
  matched one would be thrown again. **The tempo moves on every throw** (at least 3 BPM, inside the style's range),
  unless MENU > BPM LOCK is on.

### v0.6.26
- **HOME + SELECT is a song-idea generator.** Each throw picks a **style** with its own tempo, swing, scales, chord
  progressions, drums, bass, how much silence it leaves, the engines it likes and its delay and reverb: **ROBOT POP**,
  **ELECTRO**, **BIG BEAT**, **ORGANIC**, **ART ROCK**, **DUB PUNK**, **MINIMAL**, **SYNTH POP**, **IDM** (the screen
  says which). Every track gets a **role** (DRUMS, BASS, CHORDS, LEAD, ARP, TEXTURE; with TRACKS 4 the first four), a
  **new engine** the style likes for it (FM6 on two tracks at most: its CPU), a factory sound whose name fits the role,
  moved a little, its voice mode, gate, level, pan and sends, and a part written from **one harmony**: the bass on the
  chords' roots (octaves, pumping, acid with slides, walking, drones, blips), chords voiced in the scale (7ths in some
  styles; held, stabbed, pulsed or sparse), the arp over the chord's notes with holes, the lead a motif repeated with a
  variation or answered, and whole bars left silent. ORGANIC, MINIMAL and IDM put loops of odd lengths against each
  other. Drums from each style's bars with fills, ghosts and accents; IDM's euclidean, ratcheted and by chance.
  No automation and no modulation beyond the macros' routes (nothing that takes RAM). UNDO (GLO + G4) as before.

### v0.6.25
- **Interface audit, second round.**
  - **SAVE > TOOLS is a list**: KNOB 1 (or PRESETS, or any knob) moves the pick down and up, OCT+ does it (its dialog
    first). It was one knob per action.
  - **The footer on the song's and the instrument's pages** (PROJECT, ARRANGE, TOOLS, GLOBAL, SYSTEM) shows the
    project and its name, not the track's engine and sound.
  - **Clearer messages**: SAVED (NO FLASH) instead of SAVED (RAM); an empty favourites list says NONE (was --/0).
  - **Layers**: on GLO, a track asleep (TRACKS 4) has its SOLO and MUTE dim and its key quiet.

### v0.6.24
- **Interface audit, first round.**
  - **The song in one place**: SAVE > ARRANGE (SEQ > SONG was the same page twice and is gone from SEQ).
  - **Readable names**: LEVEL, SWING, CLOCK, DEPTH, STEREO, FEEDBK, SPRAY, FOLLOW, PUNCH, TRACKS, ROUTE, SCALE, QUANT,
    TRANS, OFFSET, PITCH, FILTER, SHAPE, PHASE, ORDER, VOICE, GLIDE, DEST, DETUNE, NOISE, CRUSH, DRIFT, DECAY, COLOR,
    VOWEL, BREATH, MORPH, MOTION, DRIVE, ROTOR, RATIO... instead of LVL, SWG, CLK, CDP, STER, FDBK, SPRY, FOLW...
    Library files saved by the web editor with the old names load as before.
  - **Mixer (6 columns)**: each sound's name cut to fit (no "..."); a muted track keeps its name, in the accent colour.
  - **FX in sections** (TRACK, SLICER, DELAY, REVERB, CHORUS, MASTER): a tap the next page, two quick taps the next
    section; the footer counts within the section.

### v0.6.23
- **ARRANGE: delete a row right there**: KNOB 2 (SECT) turned left of A asks **DELETE SONG ROW?** (OCT+ yes).

### v0.6.22
- **ARRANGE (SEQ > SONG) redrawn, after SLOOP's arranger**: the song as a column of rows, each a coloured section chip
  (A, B, C, D each its own heat), its bars as blocks that fill as they play, the count on the right and a dot for its
  scene; a section with nothing stored says EMPTY. Under them **+ ADD** (how many rows of the 16 are used) and **END**.
  On the right the whole song as one strip (each row as long as its share of the bars, the play head on it) and the bars
  in all. KNOB 1 the row, 2 its section (left of A: DELETE SONG ROW?), 3 its bars, 4 its scene.
- **END: STOP, x2 .. x8 or LOOP**: the song plays once, that many times, or round and round (KNOB 2 on END).
- **Tracks asleep (TRACKS 4) in grey**: on HOME as a muted being, on the mixer greyed with OFF.

### v0.6.21
- **HOME + SELECT dices everything** (after **DICE EVERYTHING?**, OCT+ yes / OCT− no): one musical scale for every
  engine and the effects, each track a new engine, sound and sequence in that scale (T1 a bass; the others a lead, pad
  or arpeggio; DRUM a beat), the macros' routes and the delay, reverb and chorus. Tempo, levels and mutes stay.
- **SELECT by page**: the tempo on GLO's pages (GLOBAL, SYSTEM, MIXER), the sound's dice on EDIT only, nothing on the
  others (it says where).
- **UNDO: GLO held + G4**: the last big change taken back (DICE ALL, a sound diced, a clear, INIT, RECALL, a pattern or
  project loaded); again: forward. The transport keeps playing.
- **Installer**: a warning to return to official V15 first when coming from another custom firmware (a brick was
  reported going straight from one), with the button right there; Install is enabled once you confirm the FM-1 runs
  official V15 or JIANT.

### v0.6.20
- **SCALE is global, strictly**: ROOT, SCALE and QNT are one for the whole song. Before, a project, MIDI or
  automation could leave a track in its own scale; now every melodic track follows the selected one's at once.

### v0.6.19
- **TRACKS on GLO > SYSTEM** (KNOB 2: TRKS 4 LOW / 6 HIGH), next to the tracks: the same setting as MENU > AUDIO >
  TRACKS, saved. Going to 4, T5 and T6 now fade out (they were cut: a click).
- **Less stutter under load** (a full check of the last updates):
  - the overload guard sheds a voice again only after two overloaded blocks in a row (since 0.6.17 one spike, e.g. a
    key scan, was enough: a voice cut every 3 ms, clicks and cut tails);
  - the load budget is measured on the sound alone, and it only takes tails under -50 dB (was -30: audible);
  - a track let go from a black-key mute or a solo no longer keeps the FX layer working every block.
- **GRAIN delay without clicks**: with a long TIME, a grain's new spray moved the other grain in the middle of its
  window (a click every grain); and the FX buses' quiet gate now opens and reopens with a short fade (it jumped).
- **DRUM-X DRV**: several loud hits at once with DRV high could wrap into a full-scale click; held now.
- **LATCH**: turning HOLD off or ARP on with a key down, SEQ TOOLS CLEAR and REC CLEAR ALL now let the latched notes
  go (they kept droning and stacked up); a panic also ends them on MIDI OUT.
- MENU > ABOUT's AUDIO LOAD updates live.

### v0.6.18
- **MENU > AUDIO > TRACKS: 4 LOW (the default) or 6 HIGH.** With 4, tracks 5 and 6 sleep: they keep their sound,
  steps and mix but don't play or cost anything, and the whole CPU goes to T1-T4 (selecting T5 or T6 says so). With
  6, all six play and you know you're near the limit: watch the load meter. The setting is kept with the others.
- **Release tails ring out again with a sequence running.** Since 0.6.16 the load budget took the oldest released
  voice for every new note once the audio passed 65 %, so with the sequencer playing the tails were cut. Now it only
  takes a tail that is nearly gone (under -30 dB); otherwise the note is added and the tails ring. The guard at 80 %
  stays the last line.

### v0.6.17
- **DRUM-X at half the work**: each hit computed at 22 kHz (it is low-passed at ~8 kHz anyway) and brought back to
  full rate, its noise kept at the same level: the kit costs about 45 % less, the heaviest part of a busy song.
- **The guard acts sooner**: the load budget from 65 % (was 70), and over 80 % a voice fades after a single
  overloaded block (was two in a row).
- **MENU > ABOUT shows the audio's load** (scroll with PRESETS): NOW and PEAK (% of the time the audio took), LATE
  (blocks that came too late: each one a click) and SHED (voices the guard faded), since power-on. Send a photo of
  it after a click to tell what happened.

### v0.6.16
- **Stable under load: the load budget.** The device measures how much of its time the audio takes. Past 70 %, the
  sound may not grow: a new note takes a voice (the oldest released first) instead of adding one, and CLOUD spawns
  half its grains; under 60 % for a while, back to normal. The overload guard (80 %) stays as the last line. The
  load stops climbing before the screen lags or the sound clicks, whatever you turn on.
- **Mutes and solos cost nothing**: GLO's solo and the FX layer's black-key mutes were the sound silenced after
  being made; now they are MIDI mutes: the track gets no new notes and, once faded, its voices end.
- **FM6 plays 4 voices a track** (was 6): six operators a voice, the heaviest engine.
- **DRUM-X's DRV and knee on the kit's sum** (were each hit's): the same glue on the bus, half the work.

### v0.6.15
- **No hiss at rest with CLIP up**: the clipper rounded a tail's tiny negative residue down (-1) and drove it x4: a
  constant offset with a faint hiss on one side, even with every track off. Now rounded; and the FX buses (delay,
  chorus, reverbs) fade their own last residue to silence once nothing has been sent to them for a while (1.5 s at
  -55 dB, 4 s at -36 dB, 8 s at -30 dB), so the tails' rounding never leaves a hum. CLOUD's FREEZE is never gated.
- **CLOUD always dies out**: past SIZE ~80 its loop rang on for ever (its tail fed back into its own line); now only
  the grains feed it: a long tail that ends (SIZE 126 stays a faint wash for long, close to FREEZE).
- **More headroom**: DRUM-X stops computing a hit at -66 dB (was -84), FM6 ends a released note once its carriers
  are under ~-66 dB (its six operators no longer run for seconds on an inaudible tail), and the overload guard steps
  in at 80 % of the audio's time (was 85 %): it fades a voice before the sound clicks and the screen starves.

### v0.6.14
- **No more sharp highs when changing presets**: the notes still sounding used to take the new sound's values at once
  (its filter, its envelope amounts), jumping bright and loud. Now they fade out in ~3 ms with the sound they had
  (all of it, kept the moment before the change), and the new notes start clean. Also on user presets and sections.
- **SEQ TOOLS** (SEQ held): each cell shows its key with the octave (F3 CLEAR, F4 CLR AUTO: both read "F" before),
  and the footer shows **REC: CLEAR ALL** (all six tracks' steps and automation, OCT+ confirms), which was nowhere
  on screen.
- **Key LATCH**: with ARP OFF, **ARP 2 > HOLD ON** latches the keys: the notes keep sounding when you let go, the
  next chord replaces them, HOLD OFF lets them go.
- **CLOUD costs about a third less CPU**: its grains computed every other sample (the line they read is written at
  half rate anyway), the same sound. First step against the lag and clicks with six tracks busy.

### v0.6.13
- **AIR on SHIMMER and CLOUD** (REVERB's fourth knob, PRE on ROOM / SPRING): ±24 semitones. Down, the reverb goes lower
  and darker; up, higher and airier, in pitch and in filter at once. SHIMMER's loop climbs octave + AIR on each pass
  (an octave down to two up); CLOUD's grains move with it. 0 sounds as before.
- **CLOUD's graph** is a swarm of moving dots: AIR lifts or sinks the cloud, MOD splits it into octave layers, the
  grains playing now drawn bigger.
- **AUTOPAN's graph** fixed (SIN came out wrong) and drawn like LFO's: two cycles across, the playhead and a dot on
  where the pan is now, L up, R down.
- **CHORUS STER** moves its graph: the three taps fan apart with it, and fold into one line at mono.
- **SAVE > TOOLS** says what each knob does (CLEAR SEQ, INIT SOUND, DELETE ROW n, CLEAR SONG) instead of "TURN TO PICK";
  the page name in the footer is lit, so you always see where you are.
- **LOFI BYTE / FLOAT: RETRIG** (EDIT > RETRIG, after EDIT 2): **RTRG ON** starts the formula from the top on every
  note, instead of carrying on where it was.

### v0.6.12
- **Projects in SAVE**: SAVE > PROJECT now shows the project (1–8, each a whole song) and its four **sections A–D**,
  each with its own sequences and sounds for the six tracks. KNOB 1 opens another project (stopped; the music you are
  playing stays, ready to be stored into one of its sections), KNOB 2 or PRESETS picks the section, LOAD makes it the
  music, SAVE stores the music into it. Each section shows whether it's saved, **NOW** on the one playing and the bars
  it plays in the song; the arrangement sits below in a line (A4>B8>D4>B8).
- **ARRANGE**, right after PROJECT: the project's song, sections chained by bars as in SLOOP (the same rows as
  SEQ > SONG; OCT+ plays it). To copy a section, LOAD it and SAVE it into another letter.

### v0.6.11
- **FM6 voice bank**, from Felucca 1.5.1: a whole 32-voice DX7 cartridge (.syx) sent from the web editor (6-OP FM tab,
  **Send to the device's voice bank**), picked on the device with **SLOT B1–B32** after F1–F8 and OWN (empty slots
  skipped; the voice's name above the algorithm). Loading a voice copies it into the track, so projects and presets
  keep it whatever the bank holds later. Its own flash sector (0xE7000), written so a power cut leaves the old bank or
  the new one, never half; editor commands 74–77 as Felucca's. The full backup doesn't include it: keep the .syx.

### v0.6.10
- **FM6 fully editable on the device**, brought from Felucca 1.5.1: EDIT > **OPERATOR** (operator 1–6, RATIO / FIXED,
  FINE, LEVEL), **OP ENV** (each stage's RATE and LEVEL, drawn) and **OPERATOR 2** (MODE, DTUN, VEL), on an FM6 track
  only. The patch becomes the track's own (SLOT OWN), saved as always; no new format, a few bytes of RAM. An edit keeps
  the patch's volume trim (0.6.8), so the level doesn't jump while you turn.
- **Fix**: a card could keep a stale gauge colour or width (or lose its tail with a long label) until the page was
  redrawn; its cache now holds the whole card.

### v0.6.9
- **AUTOPAN** per track (FX > AUTOPAN, after DIST): DPTH, RATE, SYNC (BEAT: 4 BARS .. 1/32 in time with the bar;
  FREE: Hz) and WAVE (SIN TRI SAW SQR S&H), through the pan's glide so even SQR doesn't click; its own graph. It
  lives in the slots of the old per-track slicer (a bus since 0.6.3): no more memory, the project format unchanged,
  every old project and preset loads with it off.
- **CHORUS > STER**: the chorus's stereo amount, 0 as it always sounded, down to mono, up to about twice as wide
  (in a stored slot that was unused).
- Cheat sheets and manual updated.

### v0.6.8
Fixes from the first users' reports:
- **No more crackles turning knobs.** Measured on every engine, a knob turned fast against the same move made smooth:
  - **FILTER** (every engine but ANALOG): going in and out of the chain (CUT at the top with RES 0, the LP open) and
    changing TYPE now crossfade, the filter starting from the sound instead of a stale state. It clicked each time CUT
    or RES crossed the open point (FM6's CUT 17x the high-frequency energy, now as smooth as a slow turn).
  - **LEVEL and PAN** glide (~3 ms) instead of stepping at each knob move.
  - **DIST** comes in smoothly from 0: DST 1..8 blend the dry sound into the drive (at 1 it was the full drive, with
    its low cut, at once).
- **FM6: SysEx voices no longer distort.** DX7 voices with several carriers at full came out up to 5x louder than the
  factory patches, and their chords kept the master's limiter about 10 dB down (that was the glitchy, noisy sound). A
  voice above the level every voice is made for is now trimmed to it, per patch and automatically: its chords
  sit where the factory patches' do (the limiter about 2 dB, as theirs). Quieter voices and the 8 factory patches are
  untouched; nothing is ever boosted. Tested with 71 classic DX7 voices.
- **SEQ TOOLS: F4 CLR AUTO** clears the selected track's automation and parameter locks (every step), the steps stay.
  It breathes when there is automation to clear. Next to F3 CLEAR (the sequence).
- **Video v2** (`tools/video/`): every effect, DRUM-X's pages, the modulators, sequencers and automation, a beat from
  soft to industrial at 132 BPM, fast montage cuts; the captions come from the capture's own script.

### v0.6.7
- **Cheat sheets** in the README: controls, and sound · effects · modulation, as images and printable PDFs.
- **Page jump fix**: once you turn PRESETS with a page button held, that button's layer (its map) stays off until you
  let the button go, even if you keep holding past the HOLD time or the first turn came at the very moment you pressed
  it. Hold it again afterwards and the layer works as always.
- **Presentation video tools** (`tools/video/`): `capture.c` plays a 60 s demo song on the real engines in the host
  build and records the FM-1's screen and audio; `compose.py` makes a 1080p video from them in the project's look.

### v0.6.6
- **Jump straight to a page**: hold a page button (ENV, LFO, FX, SCL, EDIT, GLO, SAVE, ARP, SEQ) and turn **PRESETS**
  to go through that button's pages forwards or back (FX has 13, EDIT up to 16), the title on screen; let go and you
  stay on that page. It doesn't take anything away: with a button held, PRESETS did nothing before; a tap still goes to
  the next page, and once you used a layer's knobs or keys while holding, PRESETS stays out of it.

### v0.6.5
- **RESO (reverb) changes chords now.** On RESO, the REVERB page's PRE knob becomes **PITCH** (the strings' chord
  0–14 steps up the scale) and DAMP becomes **FOLW**: OFF, or T1–T6, the chord built on the lowest note that track
  plays, so the strings follow your bass line or your chords, gliding from one to the next (the chord stays when the
  track goes quiet). The pre-delay and the damping were barely used on RESO; ROOM, SPRING, SHIMMER and CLOUD keep them.

### v0.6.4
- **LOFI FLOAT: DRV is gone, FBK takes its place.** DRV had a scaling bug that silenced the engine as soon as it was
  turned up, and it duplicated the DIST on the FILTER page anyway. **FBK** (feedback) sends the output back into the
  formula's time: the floatbeat modulates itself like FM, from a slight shimmer to chaos, a different deformation on
  every ALGO. Projects saved before keep their values (FBK starts where DRV was).

### v0.6.3
- **The SLICER is now one slicer on a bus, like a sampler's chop.** Its settings (mode, pattern, rate, depth) are the
  section's, and on **SL BUS** / **SL BUS 2** you choose with seven ON/OFF switches what goes in: **T1–T6** and
  **FX** (the delay, reverb and chorus returns). Everything ON turns it into a **stutter sequencer for the whole mix,
  after the effects**: the reverb and delay tails get cut too. With FX OFF the tails ring free over the cut.
  The routed tracks enter after their DIST, filter, level and pan; their sends still go to the effects.
- **SL PITCH**: the repeats (STUT) follow a **16-step pitch sequence** (±24 semitones, with the mod sequencer's page
  layout: KNOB 1 the step, KNOB 2 its pitch, LEN, QNT). Played like a tape (higher = faster), the loop keeps the
  step's rhythm; **QNT SCL** snaps the steps to the song's scale. One sample turns into an arpeggio.
- No limit of tracks any more (STUT on four was the 0.6.2 compromise), the same memory as before (32 KB, now a
  stereo recording of 371 ms), and less CPU: one slicer instead of one per track.
- Each section of a song keeps its own SLICER settings. Projects from before load with their slicer settings turned
  into the bus's: the first track that had it on gives the mode, pattern, rate and depth, the tracks that had it on
  go in, FX stays out.

### v0.6.2
- **SLICER memory shared**: GATE works on all six tracks as before; **STUT** (the one that records and repeats) can
  run on **up to 4 tracks at once**. A fifth STUT track plays live without repeats and the screen says
  **STUT: 4 TRACKS MAX**; when one of the four leaves STUT, the waiting one takes its place on the next step. Each
  track keeps its own pattern, rate and depth, and its sends still follow the chopped sound. This frees 16 KB of
  the big-buffer memory (pool now at 92 %, 25 KB free) for future effects.

### v0.6.1
- **MIXER: the six tracks at once** in the JIANT style, as six thermal columns (number, sound, LEVEL as the heat
  column with the meter over it, its dB, PAN and REV as two bars underneath). The other styles keep four cards that
  follow the selected track.

### v0.6.0
- **Six tracks instead of four.** T5 starts on PHASE and T6 on WHEEL (any engine can go on any track, as always),
  each with its own sequence, sound, effects sends and automation. **ALGORITHM** scrolls T1..T6.
- **Same storage, everything kept**: still 8 songs × 4 sections plus the 4 project slots, and the song sequencer
  (rows of sections with repeats and scenes) works as before, so you can build whole songs with intro, verse,
  chorus and bridge. To fit six tracks in the same space, sections are now saved **compressed**: only the steps
  that have something, and only the parameters you changed, are stored. The music at power-on takes about a
  quarter of a section; even six tracks with 4-note chords on all 64 steps fit.
- **A fill warning**: a moment after you stop editing, if the music is near the size of a section the screen says
  **SECTION 90% FULL**; if it no longer fits, **TOO FULL TO SAVE**. Saving a section in that state says
  **SECTION FULL** and changes nothing: the section already saved stays as it was. All memory is fixed at build
  time (nothing is allocated while you play), so a full section can never crash the system.
- **Projects and sections from 0.5 load as they were**: their four tracks keep their sound and steps, T5 and T6
  start empty on their default sounds.
- **GLO** (held): black keys 1–6 mute T1–T6, 7–10 the DRUM groups (KICK, SNARE, HAT, PERC); white keys F3–D4 solo
  T1–T6 while held, E4 unmutes everything, F4 tap tempo. The four knobs are T1–T4's levels, or T3–T6's when T5 or
  T6 is selected. **FX** (held): black keys 1–6 mute T1–T6.
- **MIXER** shows four tracks at a time and follows the selected one (T5 or T6 selected: T3–T6). (0.6.1: all six in the JIANT style.)
- **HOME**: six beings in the ecosystem, a bit smaller.
- **Song scenes** remember the mutes of T5 and T6 too. **Automation** (knobs and per-step locks) works on all six.
- **MIDI IN**: ROUT **CH1-6** (was CH1-4): channels 1–6 play tracks 1–6, 7–16 stay free for other instruments.
- Web editor: six tracks in the track selector, the mixer and automation.

### v0.5.5
- **FLOAT is a floatbeat generator now**: bytebeat's sibling in floats. Each of the 32 ALGOs is a formula of `t`
  made of sines, as in the floatbeat scene: `sin(t·(t>>11 ^ t>>13 & 7))`, FM whose index walks with `t`'s bits,
  melodies read from nibble tables, kicks, snares and hats inside the formula, Sierpinski partials, drive and folds,
  staircase sines, xor'd and dropped bits, risers, polyrhythms, chords whose root walks… rhythm, melody and timbre at
  once. `t` runs 32 steps per cycle of the note, so every formula plays in the key you hold (and, as in bytebeat, its
  rhythm runs faster up the keyboard). It shares BYTE's controls: **VAR** mutates `t` (8 ways), **MOTN** runs its
  slow bits faster or slower, **BEND** folds it, **LOOP** locks it into a short window; the new **DRV** folds the
  output from clean to wild.

### v0.5.4
- **FLOAT makes patterns now, not just waveforms.** Every ALGO is a step sequencer: 8 patterns (an arpeggio, a
  melody, a Sierpinski rhythm, a 5-in-8 euclid, a random walk, a `t>>9 % 13` line, a drone with 3-against-4 accents,
  octave leaps) × 4 smooth timbres (sine, FM that darkens as each note falls, wavefold, hard sync sweeping down).
  The notes are harmonics of the key you play (just intonation), the steps are the song's 1/16 notes.
- **New knob, MOTN (motion)**, on BYTE and FLOAT, in the slot that was empty: the speed of the formula's time. In
  FLOAT it goes from one step per bar to steps so fast the pattern turns into a timbre; in BYTE it speeds up or slows
  down the formula's slow bits (its rhythm and melody) without changing the pitch. 64 is as written.
- **VAR is drastic now**, on every BYTE and FLOAT formula: it mutates the formula's time in 8 different ways (stairs
  that transpose as they go, xor and or with itself, holes in the rhythm, the phrase backwards, multiplied,
  scrambled), so each formula gives many timbres and sequences.
- **The second LOFI page cleaned up**: BYTE shows MOTN · GRIT · BEND · LOOP (GRIT holds samples longer, a rate
  crusher), FLOAT MOTN · DCY · BEND · LOOP (DCY: from short plucks to legato). The second RES (already on the FILTER
  page) and the empty slot are gone. Projects saved before keep their sound (MOTN set to 64, GRIT/DCY to 0).

### v0.5.3
- **A new power-on screen**, after feedback from the community: the signature used to be drawn as straight segments
  (it looked like MS Paint next to the rest of the interface). Now each stroke is smoothed and drawn as a curve with a
  round pen that tapers at the ends, like ink; the pen's tip is a glowing nucleus; and three small beings (a
  radiolarian, an amoeba, a hydra) swim in from outside the screen and circle the signature while it is written,
  warming up as it grows and pulsing once it is done. It costs 16 bytes of RAM, and the splash now runs on the clock
  (drawing the beings does not make it longer). MENU > ANIM OFF still shows it whole at once.

### v0.5.2
- **BYTE: new formulas, no silent starts.** The 17 most familiar one-liners (`t*(42&t>>10)`, `t&t>>8` and friends)
  were replaced by less obvious ones: rhythms and arpeggios after Tejeez, xpansive's "lost in space", a melody of
  `t>>9` modulo 13, textures built from `t%24` and `t%19`, and more. Each formula now starts where it is already
  moving: 14 of the 32 used to sit silent for up to half a second after the note; now all 32 sound within 30 ms.
- **FLOAT: 32 formulas** (16 new): drifting unison, sub-and-fifth organ, self-FM, glides through partials, gated
  rhythms, CZ-style phase distortion, formants, breath, harmonic chords, breathing bells, stepped sines, PWM, random
  held partials, hard sync, three-against-four. **CHIP now sets FLOAT's resolution**: FULL (smooth, the default),
  4BIT, 4B/2, 1BIT, STEP.
- **LOFI presets in order**: browsing shows the BYTE presets, then FLOAT, then the classic chip sounds.
- **Parameter check**: NOISE's resonance is now **PEAK** (it sat next to the FILTER page's RES with the same name).
- **Lighter audio interrupt**: 0.5.1's X-MOD LFO had been compiled into the audio interrupt's inner path with its
  divisions (its cost estimate almost tripled); it now runs once per block, outside it, and the interrupt is back to
  its 0.5 budget.

### v0.5.1
- **Master no longer smears the mix.** The always-on leveler was riding every hit: measured on the demo song, its
  gain swung about 5 dB inside each beat and the limiter was working 95 % of the time. Now it follows the song's
  loudness over about a second and a half (0.3 dB of movement per beat) and leaves the limiter more headroom (active
  less than half as often, and lighter). Same loudness, transients and space come back.
- **CLOUD is bigger**: 12 overlapping grains instead of 8, each one a few cents off (a chorused shimmer), the grains
  fed back into the cloud so it keeps building, and a louder bloom.
- **RESO rings much longer**: the top of SIZE now sustains for many seconds.
- **Chorus rebuilt as an ensemble**: three voices swept a third of a cycle apart, left, right and centre: stereo
  even with WIDTH at 0, wider with it.
- **New LOFI wave: FLOAT** (floatbeat, bytebeat's smooth sibling): 16 formulas made of sines (FM, harmonic
  arpeggios, folded sines, beating pairs, plucks, pads), tuned to the note; VAR, BEND, RES and LOOP as in BYTE.
  Three new presets: FLOAT FM, FLOAT ARP, FLOAT PAD.
- **New DRUM-X page: X-MOD**, a modulator for the whole kit: a tempo-synced LFO on MORPH (RATE from 4 bars to
  1/32, DPTH, SHPE: sine, triangle, saw, ramp, sample & hold) and RAND (every hit its own MORPH and tune). Saved
  with the kit; the A–B bar shows the morph moving.

### v0.5
First public release.




