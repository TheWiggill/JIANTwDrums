# 0.9.9 cpu5: narrower dependencies and hardware diagnostics

CPU4 hardware feedback still reported drum dropouts with four tracks and historical overload (peak 104%, late 10, shed 61), with 24% load while stopped. CPU4's empty-buffer shortcuts therefore do not establish a solution to that scene. The exact engines, sends, reverb and release durations are still needed for reproduction.

## Repartition independent work

The existing safety groups keep shared random streams on one core, preserving serial output. Their predicates were broader than the actual dependencies:

* Every AUTO PAN track was grouped, although only S&H uses its shared RNG. SIN, TRI, SAW and SQR now remain independent; S&H and preset fades keep their conservative grouping.
* Every VOICE track was grouped, although LANG OFF / BREATH 0 does not consume noise. Avoid the unnecessary shared noise load/store in that path, and allow independent scheduling. WORDS, nonzero BREATH, fades and any active matrix slot targeting BREATH keep the safety group.

No random sequence, engine quality, sample rate, shared voice cap or overload threshold is changed. No extra persistent state is needed. This only helps scenes with previously unnecessary dependencies; it does not make a single expensive VOICE/WORDS track run across two cores.

The threaded mixer test adds six clean VOICE tracks with triangular pan, six FM6/ANALOG tracks with triangular pan, and a BREATH matrix case. CPU4 cannot split either independent scene (zero mixed-core masks); cpu5 splits all 1024 blocks. Serial/deferred/threaded/adversarial output and RNG hashes match, and clean scene hashes also match the CPU4 serial reference. Existing random pan, WORDS/noise, fades, shared RNG, polyphony and timeout cases remain covered. Core-mask counts demonstrate schedulability, not measured CPU savings on the FM1.

## ABOUT diagnostics on Android-compatible firmware

ABOUT now shows admitted voices and cap, allocation cuts and quiet trims, the existing FX elapsed time in microseconds per control block, and engine/active-voice count/time for each enabled track. It reads the scheduler's existing smoothed measurements from main while audio IRQ workers are joined. There are no additional timers, audio callbacks or per-voice counters. The screen refreshes roughly five times a second while open.

Each control block was 32 samples at 44100 Hz (about 726 microseconds) when this was written; since 0.9.9 ctl64 it is 64 (1.45 ms; `docs/CPU3_PERFORMANCE.md`). Times overlap between cores and must not be summed as total audio load. Active counts include fading voices, while VOICES excludes already-surrendered fading slots. ALLOC CUT uses the existing console definition (`voice_kills - trims - shed`), which can also include mode/cap/track transitions; it is not a drum-specific counter. On the browser emulator there is no target scheduler clock, so times are zero.

Hardware evaluation: capture ABOUT once with the same four-track pattern playing and once stopped after its tails, including the detail rows. Compare against fresh-boot load. This distinguishes active voice work, long/expensive FX processing and allocation pressure before deciding on quality tradeoffs such as FM4. CPU5 is still based on stable 0.9.9; FM6 remains six operators.
