# Optional keyboard split (V10.6c-split beta)

Implemented for both the FM-1 physical keyboard and external MIDI (USB/TRS).
SPLIT is OFF by default, including projects written before this feature.

GLO > SYSTEM > KEY SPLIT:

- SPLIT: OFF/ON, enabling both keyboards.
- LOW TRK / HIGH TRK: tracks 1–6, independently assigned (can be equal).
- KEY CUT: first physical key of the high zone, numbered from 1 at the left, 2–27 (default 14).

KEY SPLIT 2:

- LOW OCT / HIGH OCT: -4 to +4. For melodic physical keys these add to the global OCT; MIDI uses the same zone offsets on incoming notes.
- MIDI CH: input channel 1–16 (default 1); other channels retain normal routing.
- MIDI CUT: first MIDI note of the high zone, 1–127 (default 60 / C4).

The physical split uses the normal engine key/scale mapping. Drum keyboard mappings retain their kit lanes and use the zone octave for kit variations. MIDI drums retain incoming GM notes (octave transpose is melodic only).
Normal split notes take priority over SCENE multi-track fan-out. Names, shortcuts, drum-grid step editing and the LIVE performance board retain their current meanings.

Each held physical/MIDI note retains the track and sounding note until release, even when split, track, octave, cut or selected track changes. Sustain, bend, wheel and channel aftertouch reach the split destinations. Notes share the existing synthesis voice/CPU budget.

Configuration is stored per project/section in the optional KS extension. Older projects decode OFF; disabling split does not discard its configuration. Existing firmware parameter IDs and flash map stay unchanged. MIDI sounding-note ownership uses pool RAM, including constant-time held-note reference counts to handle clamped/overlapping notes.

Automated coverage includes both zones, sustain, changing split while held, clamped duplicate notes, CC120/123, pitch bend, SCENE priority, UI knob editing, NOR project round trips, legacy OFF, autosave signatures and invalid destinations. Existing MIDI/clock, persistence, backup, FIT and USB regressions run with the candidate. Hardware validation is still required before promoting this beta.
