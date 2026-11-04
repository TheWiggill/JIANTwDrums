# REC controls and timing

- Hold **REC + A3**: UNDO; repeat to REDO. **GLO + G4** remains available. There is one global undo/redo level, not a history stack.
- Arming REC captures the music before the take. Disarming does not overwrite that snapshot.
- Undo from REC disarms recording, so the restored take is not immediately overwritten. Normal playback can resume; song-chain editing guards remain.
- Hold **REC + B3**: toggle REC Q between 0% (off) and 100% (hard quantization).
- Hold **REC + KNOB 2**: adjust REC Q continuously between 0 and 100% for the selected track.
- Existing **REC + F3 CLEAR**, **REC + G3 CLICK**, and **REC + KNOB 1 CLICK** remain.
- The layer's key map, LEDs, knobs and state view show the new controls. No project-format change or additional undo buffers.

## Actual timing resolution

The stored microtiming is **1/64 of the track's current step**, from -32/64 to +31/64.
At DIV 1/16 this is 256 timing subdivisions per quarter note:

| BPM | One 1/16 step | One 1/64 of that step |
|---:|---:|---:|
| 60 | 250 ms | 3.906 ms |
| 90 | 166.667 ms | 2.604 ms |
| 120 | 125 ms | 1.953 ms |
| 180 | 83.333 ms | 1.302 ms |

Faster DIV gives finer stored spacing; swing changes the lengths of adjacent steps. Playback schedules the stored offsets within audio blocks. These numbers describe the storage grid, not measured input accuracy on physical FM-1 hardware. Input scanning, audio buffering and existing latency compensation also matter.

**REC Q 0% preserves that microtiming, not an unlimited list of timestamped MIDI events.**
A step has one shared microtiming offset; notes grouped into the same step share it, with up to four melodic notes per step. Repeated hits within a step remain constrained by the sequencer's step/lane representation. This change makes existing recording quantization accessible; it does not redesign recording storage.

## Validation

The focused test exercises real button/key gestures, undo/redo of a take, arming/disarming, guards, click settings, quantization and layer labels. Timing tests are compared against each edition's exact source baseline. The baseline contains an old audio-hash assertion against 0.9.0 that may already fail; equality of complete timing output and exit status ensures this patch does not introduce another timing difference. Target image capacity, RAM and DSP budgets must pass before publication.
