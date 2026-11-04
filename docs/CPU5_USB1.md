# 0.9.9 cpu5usb1: USB multichannel trial

Based on CPU5 (009faf97764cf872fd5f392155d944a684127132). The USB copy loops snapshot the volatile mode once and walk ring slots instead of recalculating them for every sample/channel. Rates, timing, interpolation and DSP remain unchanged; uint32 rollover is preserved.

Validation passed for the target layout/instruction budgets, 157 golden audio renders, audio/concurrency, USB/stems, drift/starvation and Android core checks. In a 10-second simulation, mode reads fall from 6,270,730 to 74,930 (8 channels) and 7,155,100 to 76,772 (10 channels), with identical delivered frame counts and zero lost/repeated/moved samples. These are operation counts, not measured hardware CPU savings. Hardware confirmation of CPU reduction and resolution of recorded glitches is still pending.

The complete host suite retains a preexisting DJ centre golden mismatch against its historical 0.9.0 reference. Original CPU5 and this USB trial produce identical complete DJ output and the same sole legacy failure (894b0fcf vs 8b898fe6); the golden was preserved. An offline Gradle cache miss was resolved by rerunning with dependencies available. Optional valgrind, Emscripten and official V15 restore checks were unavailable.

Default package identity is FM-1_909 and version is v0.9.9-cpu5usb1, matching the validated --release build. Validated package SHA256: daac8c63762e483be36167db560fdcd117816392376b35854b57ab2d49a57037. Test with the same phone APK/song first to isolate the firmware change.
