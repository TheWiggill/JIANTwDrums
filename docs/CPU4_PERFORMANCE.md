# 0.9.9 cpu4: live drum protection and empty FX work

## Confirmed allocator defect

A one-shot DRUM-X hit keeps sounding after MIDI key-up. The allocator and overload shedder previously classified it by `gate == 0`, like a released melodic tail. This allowed the last live kick to be discarded while three held melodic tracks remained protected; cpu3's requesting-track release preference could also reuse that kick for a new hat.

`voice_held()` now classifies live one-shot voices as held for allocation and overload protection only. The drum envelope, MIDI gate, choke groups and rendering are unchanged. The existing lowest-note protection retains one live drum hit, while additional hits can still be stolen under load. This does not remove the physical CPU deadline or guarantee every hit survives any scene.

`audio_test` reproduces both failures against cpu3 and passes with this fix: four tracks under shedding, and a new hat at a full eight-voice budget. It also checks pending drum hits under tight admission. Two existing mixed-song golden hashes intentionally change because allocation preserves the drum differently; all other reference hashes stay unchanged.

## Empty bus work without losing tails

Two conservative flags prove delay and plain ROOM buffers have never received nonzero input since boot or an actual state clear. In that case, avoid delay interpolated reads/feedback filters and ROOM's comb/allpass sample loops. Preserve write positions, parameter glides, LFO clocks and shared FX outputs. First nonzero input resumes the original kernels in the same block. Stereo/grain/low-rate delay and non-plain ROOM models conservatively disable their shortcut.

These certificates do **not** assume zero sends imply zero tails. After real input, normal processing continues until an explicit clear. This improves unused buses, including on fresh boot and dry playing, but does not implement sleeping of previously excited tails. They require two bytes of persistent flags (alignment may affect target totals), not additional audio buffers. Chorus retains cpu3's exact zero-ring proof.

`empty_fx_test` forces the original kernels as its reference and compares complete output, FX state, RNG state and final buffers over 12000 blocks, covering fresh silence, wake, tails, resets, delay modes, reverb model transitions, parameter movement and LOAD changes. It matches bit for bit. Host timings are diagnostic only and cannot predict the FM1's reported idle percentage.

## Scope of external reference

The official m-vave/fm1_open README and SDK build script confirm V14 and an `-O2` compiler configuration. Its current application license is GPLv3 with an SDK linking exception; its binary SDK has separate terms. No official source or binary SDK is incorporated here. Assertions about 320/360 MHz, cache failures or unsupported clocks are not established by the public application files examined and are not used to change JIANT's clock. JIANT's current audio compilation uses `-Os`; changing that or adding hardware loops needs separate target resource and timing validation.

FM6 remains six operators. FULL has a per-track cap of four FM6 notes, LITE two, in addition to shared dynamic admission. A measured host benchmark or synthetic deadline test is not a hardware utilization measurement; final evaluation needs fresh-boot LOAD and a repeat of the same four-track pattern on the FM1, checking NOW, PEAK, LATE and SHED.

## Host diagnostic measurement

Five interleaved runs against cpu3's FX implementation, using the same harness and allocation code, produced median idle mixer times of 43.38 versus 29.88 ns/sample (31% lower), and empty FX chain times of 26.36 versus 8.23 ns/sample (69% lower). All scenario checksums match. Raw scenes are in `docs/performance/cpu4-host.jsonl`; timing variation affects unchanged engine scenes too, so those are not evidence of engine speedups. This is GCC `-O2` on the host, not JieLi hardware or a prediction that the reported 19% boot load will fall by 31%.
