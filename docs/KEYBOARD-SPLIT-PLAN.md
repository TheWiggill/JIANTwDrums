# Two-zone physical keyboard: implementation proposal

This is a proposal, not an implemented feature. The intended default is the physical
FM-1 keyboard; an external MIDI keyboard continues to use the existing channel routing.

Proposed pages:

- **GLO > KEYBOARD**: SPLIT OFF/ON, CUT (first physical key of the right zone), LOW TRACK,
  HIGH TRACK. Tracks 1–6, including the same track on both zones if useful.
- **GLO > KEYBOARD 2**: LOW OCT, HIGH OCT. Zone offsets add to the global OCT setting;
  melodic keys retain their physical pitch layout. Show both zones on the keyboard map.

Routing belongs in `keyboard_block`: after FX/SEQ/REC/SAVE shortcuts, names and drum-grid
handling, choose the track on the note-on and apply its zone octave. The existing
`kb_trk[27]`, `kb_note[27]` and `kb_oct[27]` already retain the note's owner. Note-off must
use those retained values, even after changing selection, split point, zone track or
octave. ARP/chords and live recording then use the destination track's normal input path.

Do not repurpose MIDI input channels, change the selected UI track on every note, or
allocate more synthesis voices. Two keyboard zones share the existing CPU/voice budget.
Names, step-entry and the drum grid keep their current key meanings. Define and test
priority relative to SCENE mode before implementation: fixed split routing and SCENE's
multi-track keyboard behavior must not both consume the same physical key.

Store the six configuration values per project in an optional extension, default OFF
for older projects. Keep parameter IDs/count and the flash map unchanged. Keep the
small runtime configuration in the UI pool to avoid changing DSP global addressing.

Required tests: independent tracks, hold/release across configuration changes, 0/127 MIDI
note bounds, zone octave changes, chords, ARP, REC on one/both tracks, FX and editing
shortcuts, drum key mapping, SCENE priority, project load and crash-free panic/stop.
Measure firmware size and hardware CPU load before publishing a split candidate.
