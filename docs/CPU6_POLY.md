# 0.9.9 cpu6 (poly-099): averaged guard, predicted admission, FX at rest

Based on cpu5usb1 (6cb8561). Three commits in `firmware/src/audio.c`, `voice.c` and `fx.c`; the hardware deadline, the
engines, the shared voice cap (8, 12 with two cores, ECO 6) and every render loop are unchanged. 157 golden renders
unchanged.

## Why four mono tracks cut each other

With four single-note tracks `voices_busy()` sits around 4 against a cap of 8, so the count never steals: the cuts came
from the load mechanisms. `load_tight` was set by one 2.9 ms half over 65 % and cleared after 16 halves under 60 %, and
a voice was shed after two halves in a row over 80 %. A half's measured share is noisy (a chord's note-ons, a cache
miss, a UI frame nested in the render): one spike closed the budget for 50 ms, two faded a release tail with nothing
late yet. Four mechanisms were stacked at a 2.9 ms resolution (budget, shed, headroom, LOAD).

## 1. The averaged guard (`voice.c load_half`, after LOOM-8's `engine.c`)

`audio.c shed_check` feeds every half's render share (TIMER5 nested in it excluded, as before) to `load_half`, which
keeps an EMA over ~32 halves (93 ms) and a run count. The guard trips on an average past 85 %, or on three halves in a
row past 92 % (a load that keeps the audio late); one spike never trips it. Tripped, it caps the shared voices at what
sounds (at least 4: a lead a track) and lets go a voice for each half second of calm under 70 % with no such run, until
the cap is the budget's again. The extra voices (12) open and close on the same average with the nested interrupts
included (50 / 65 %). LOAD ECO keeps its earlier budget (65 / 75 / 50). While tripped `load_tight` keeps its meaning
for the rest of the code (CLOUD's grains halved, dying tails trimmed, the cap's own-released-slot-first reuse).

The hard shed stays only as the last resort: a half that really came late (the DMA moved on while it rendered, already
a click) fades one voice. Console: `guard_trips`, `load_avg_pct`.

## 2. Predicted admission (`voice.c load_admit`, after LOOM-8's `synth.c find_voice`)

A voice added for a new note costs its part's measured block time (`fx.c mc_tk`, smoothed, shared by the part's
voices; a part not measured yet, or the host: 4 % FM6, 2 % LOFI / NOISE, 3 % the rest) as a share of a half, 60 % of it
with both cores rendering. Added to the guard's average plus what the voices admitted since add (decaying over the
average's 93 ms, bounded by what sounds), a prediction past 90 % (ECO 70 %) does not add the voice: a released voice is
given up for it with the usual ~4 ms fade, the quietest of a POLY part's first (the part's own restarted in place),
else the oldest held of the part's own but its bass. A MONO / LEGATO / UNISON part's released tail is never taken
while a voice is free, nor any held lead (cpu4's live drum protection kept); with nothing to give up the voice is added
after all. An extra UNISON voice past the prediction is not started; a lead always is. Console: `load_steals`.

## 3. FX at rest and the empty skips extended (`fx.c fx_rest`, `rev_idle`, `delay_zero`)

After 0.9.14's `fx_rest`, on cpu4's certificates. The gate shut (the buses give exactly 0 until something is sent),
nothing sent, the delay's level under the floor and its loop under 100 %, no rate change, the model settled, CLOUD not
frozen: the buses are not run. Lines and states are silenced over the first six blocks and the certificates hold from
there, so the next send starts from silence as a never-excited bus does. The clocks that move whatever the sound keep
moving (the chorus's LFO and glides, the delay's WOW). What the gate hid is silence now: a residue under -55 dB (the
feedback's limit cycle), the delay's flicker that reopened the gate now and then, and the reverb's WIDE side, which the
gate never covered. This is the one audible change: silence where a sub--55 dB residue was.

While awake, bit for bit the same (`empty_fx_test`: 12000 blocks, 11496 skipped, every model's grain and feedback
state hashed): the reverb certificate (ROOM only in cpu4) covers SPRING, SHIMMER, RESO and CLOUD with each model's
block clock advanced exactly (SIZE's glide, LFO phases, RESO's chord glide, CLOUD's grain scheduler with its random
numbers and the grains' ages); the delay's covers WIDE, PING-PONG, a read point's crossfade and the half / quarter-rate
line (not the GRAIN read, which draws the shared random numbers). The DJ FILTER with nothing in, zero states and
settled coefficients is not run. The SLICER was left alone: STUT repeats a recording, so a zero input does not give
a zero output, and GATE's ramp depends on the step boundary inside the block.

`fx_rest_test` runs the same scene against a build with `FX_REST_OFF`: the same output until the gate shuts, exact
zeros through the rest (the reference leaks a residue up to 395 there), the rest the block after the gate, the wake
at level.

## Measurements (host, callgrind instruction counts: `tests/cpu_vg.py`, exact; not FM1 percentages)

Against the base at the same scenes (`cpu_vg.c` gained `mix/idle`, the power-on state 2 s on, and `mix/idle.tail`,
4 ANALOG parts into CLOUD, the delay and the chorus, 6 s after the keys went up):

| Section | base | cpu6 | delta |
|---|---:|---:|---:|
| mix/idle | 519 | 344 | -33.7 % |
| mix/idle.tail | 815 | 329 | -59.6 % |
| mix/full | 2671 | 2701 | +1.1 % |
| rev/CLOUD | 1597 | 1628 | +1.9 % |
| rev/SPRING | 1493 | 1511 | +1.2 % |
| rev/ROOM, SHIMR, RESO, dly/*, seq/* | | | within ±0.3 % |
| eng/* (every engine) | | | within ±0.1 % |
| song/cputest, .djf | 3016 / 3085 | 3044 / 3113 | +0.9 % |

The +1 % on the busy scenes is the admission's bookkeeping at every note and the per-block helper calls the reverb
models gained (`spring_block`, `cl_spawn` / `cl_age`); the idle floor is what the FM-1 showed at 20-26 % before the
first voice. Target budget (`tests/target_budget.py`): every loop within budget, `fm1_alnk0_irq` 270 -> 76 (the
simpler `shed_check`), `rev_spring` 127 -> 138 (+8.7 %, the budget's tolerance is 10 %).

Image 566720 B (base 564296: +2424), 14844 B under the XIP region's 0x8DFBC; 35392 B under 0x93000.

## Known

* `djf_test`'s "centred bit for bit 0.9.0's (894b0fcf)" fails on the base too (CPU5_USB1.md); `regress` reports 50
  renders not in `golden.txt` on the base too.
* The cost model for an unmeasured part is an estimate; the FM-1 measures a part once it has rendered. The share with
  two cores (60 %) is LOOM-8's constant; JIANT's split is by measured cost, so the true share varies.
* Hardware evaluation: the same four-track pattern on the FM-1, fresh boot, ABOUT's NOW / PEAK / LATE / SHED and the
  console's `guard_trips`, `load_steals`, `load_avg_pct`, stopped and playing.
