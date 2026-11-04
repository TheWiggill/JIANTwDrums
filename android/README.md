# JIANT on Android

Two apps, both built by GitHub Actions (`.github/workflows/android.yml`) on every push that touches `android/`:

| | What it is | APK |
|---|---|---|
| **JIANT Stems** (`android/stems`, 1.1) | Records the FM-1's USB stems on the phone, lines the voice up by the measured latency, trims and mixes, exports a ZIP of the stems or a stereo mix. | `jiant-stems.apk` |
| **JIANT Probe** (`android/probe`, 0.4) | Phase 0: the one-screen test of what a phone does with the FM-1 (channel counts, the own USB driver, the mic through the FM-1 and back). Kept as it was, for diagnosing a phone. | `jiant-probe.apk` |

## Install

1. Download the APK:
   - from the site, once the change is on `main`: <https://juanjiant-bit.github.io/JIANT/app/jiant-stems.apk>;
   - from any branch: the Actions tab → the **android** run → **Artifacts** → `jiant-stems` (a ZIP with `app-debug.apk`).
2. Open it on the phone and allow installing from this source when Android asks (it is not from the Play Store).
3. Every build is signed with the same public debug key (`app/stems-debug.keystore`), so a new APK installs over the
   last one and keeps the takes.

Android 10 or later, a USB-C data cable (or an OTG adapter), the FM-1 on firmware **0.8.0** or later for the voice,
**0.9.8** or later for clean STEMS (before it, the 8-channel stream lost blocks: clicks in every stem; the app says so).

**1.1:** only whole frames are kept from a USB packet in error (a frame cut short no longer moves every channel after
it); the mic sent to the FM-1 follows the clocks' drift by reading a little faster or slower instead of skipping or
repeating a sample (a click in VOX); a take being recorded is never "recovered" by the takes list, and a take cut short
keeps all it wrote (1.0 cut it to its header's last patch); the trim's edges are taken by their grips (a touch on a lane
near one moves the playhead), move from where they were taken and land exactly on the take's ends; EXPORT says when the
take is trimmed; in Spanish the trim buttons read INICIO AQUÍ / FINAL AQUÍ / SIN RECORTE.

## On the FM-1

**MENU › SYSTEM › USB**, then unplug and plug the cable again (the FM-1 re-enumerates):

| Mode | Channels to the phone | In the app |
|---|---|---|
| **STEMS+IN** (0.8.0) | 10: T1–T6, FX L R, VOX (your voice as the last live track mixed it), DRY (your voice as it came) | the stems **and** the phone's mic sent through the FM-1 |
| **STEMS** (0.7.1) | 8: T1–T6, FX L R | the stems |
| STEREO / CONSOLE / IN+OUT | 2: the master | the stereo mix only (the app says how to switch) |

The tracks are each mono, before the master (no leveler, limiter or MASTER knob): their sum with the FX return is the
mix as the FM-1's buses hear it. USB LEVEL at −6 dB or lower gives headroom.

## Using it

**HOME.** Plug the FM-1 in (Android may offer to open JIANT Stems for it: say yes and tick "always", and the USB
permission comes with it). The page shows the device and its mode; **CONNECT** asks for the USB access and opens the
record page. Below: the takes on the phone (tap to open, long-press to delete).

**REC.** A meter for every channel (heat colours, peak hold, a red lamp on a clip). Tap a meter to arm or disarm its
lane (FX L and R are one stereo lane); each armed lane is recorded to its own WAV. In STEMS+IN:

- **MIC → FM-1**: the phone's microphone (a wired headset's if one is plugged in) goes out to the FM-1 by the app's
  own USB driver and comes into the **last live track**: its LEVEL, PAN, MUTE and its CHORUS, DELAY and REVERB sends,
  so your voice goes through that track's effects, and comes back as VOX (and DRY). Headphones on the FM-1 avoid
  feedback through its speaker. **MIC GAIN** adds up to +24 dB before it is sent.
- **LATENCY** (tap: MEASURE): six seconds with no headphones, the phone about 20 cm from the FM-1's speaker, a song
  with drums playing and the last live track's LEVEL down or muted. The app finds where the dry mic best follows the
  stems' sum (the same method as JIANT Probe's test 7) and keeps the result for the next takes.

**REC** starts the take, **REC** or **STOP** ends it. The screen stays on while the page is open; the files are written
as it records (the headers updated every two seconds: a take cut short by a crash is repaired the next time the app
starts). An hour of the 10 channels is about 3.2 GB.

**When a take stops** the app works out the round trip, in this order: measured in the take itself (when the mic heard
the speaker clearly enough, clarity above 3), else the last MEASURE, else none. The VOX and DRY lanes are then played
that much **earlier**, so the voice lands where the singer heard the music; the WAVs are not touched (it is an offset in
the session). DRY is muted in the mix by default (VOX is the voice as the FM-1 mixed it). The voice's share of the FX
return cannot be moved (it is mixed with every track's), so its reverb and delays stay the latency late (~20–30 ms:
like a pre-delay).

**EDIT** (opens by itself after a take). All the lanes on one timeline, with their waveforms.

- Tap a lane's name to select it; **M** mutes it, **S** solos it.
- The cards, for the selected lane: **GAIN** (−24..+12 dB), **PAN** (L100..R100: centre is unity on both sides, as
  the stems come), **SHIFT** (±200 ms, any lane, e.g. the FX return by hand), and the take's **LATENCY** (moves VOX and
  DRY together). Drag a card sideways to change it, double-tap it to reset it.
- The trim: drag the orange edges, or **SET IN** / **SET OUT** at the playhead, **FULL** to undo it.
- Tap or drag on the lanes to move the playhead; **PLAY** / **STOP** the mix (to the phone's speaker, headphones or
  Bluetooth, never back into the FM-1). Tap the title (or **RENAME**) to rename the take.

**EXPORT.**

- **STEMS ZIP**: one WAV a lane over the trim, with the latency already applied. *As recorded*: each lane's own channels
  at 16 bit, the very samples the FM-1 sent, only moved and cut (for a DAW). *With the mix*: stereo, each lane's gain
  and pan applied, muted lanes left out. **ADD MIX.wav** puts the stereo mix beside them. A `README.txt` in the ZIP
  says what each file is and how much the voice was moved.
- **MERGE**: the stereo mix as EDIT plays it, 16 or 24 bit, **NORMALIZE** to −1 dBFS (the stems come before the
  FM-1's master, so their sum can pass full scale).
- **SAVE TO FILES** opens Android's document picker (Downloads, Drive, a USB stick); **SHARE** opens the share sheet
  (WhatsApp, mail, Drive…). 44.1 kHz, the FM-1's own rate.

Takes live in the app's own folder, one folder a take: `Android/data/fm.jiant.stems/files/takes/<date_time>/` with
`T1.wav` … `T6.wav`, `FX.wav` (stereo), `VOX.wav`, `DRY.wav` and `session.properties` (the name, trim, latency and
each lane's gain, pan, mute, solo and shift). Uninstalling the app deletes them: export what you want to keep.

The interface is in Spanish on a phone set to Spanish, in English otherwise.

## How it works

- **The FM-1's audio is read by the app itself**, not through Android's audio system: on some phones (the S23 Ultra
  among them) Android passes only two of a USB device's channels to apps. As the probe did, the app takes the USB
  permission, claims the audio-streaming interfaces and runs the isochronous transfers through usbfs
  (`app/src/main/cpp/usbstream.c`): 8 URBs of 8 one-millisecond packets in, and in STEMS+IN 4 URBs out with the mic
  (44.1 stereo frames a millisecond, the mono mic on both sides). The stream stays open while the record page is.
- **The mic ring** holds about 10 ms. The phone's mic and the USB schedule run on two clocks; so that the round trip
  stays the same through a long take, the driver skips or repeats one sample when the ring's average strays by more
  than 3 ms (at most one every 10 ms), with the probe's hard limits behind it.
- **`android/stems/core`** is plain Kotlin with no Android in it: the WAV reader and writers, the FM-1's channel layout
  and its USB descriptors, the stem splitter, the latency measure, the session, the renderer (gain, pan, mute, solo,
  offsets), the exports and the strings. Its JVM tests are the ones the workflow runs first.

## Building and testing

No Android Studio needed; JDK 17 and Gradle 8.9 (the workflow's versions).

```sh
gradle -p android/stems -PcoreOnly :core:test         # the JVM tests: no Android SDK needed
gradle -p android/stems :app:assembleDebug            # the APK (Android SDK, NDK 27.2)
gradle -p android/stems :app:testDebugUnitTest        # the pages on Robolectric
cc -std=gnu11 -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" \
   android/stems/app/src/test/cpp/usbstream_test.c -o /tmp/t && /tmp/t   # the USB driver against a pretend FM-1
gradle -p android/probe assembleDebug                 # the probe, as before
```

- `core/src/test`: WAV headers and round trips (16 and 24 bit, a take cut short repaired, other chunks skipped), the
  splitter (lanes from the interleaved stream even when frames split across reads, meters, clips), the latency (found
  within a frame on drum-like material from 0 to 68 ms, unrelated signals not taken, the probe keeping the window where
  the mic hears the speaker), the shift (the voice moved earlier lands on the beat; the ZIP's stems too), the mix
  (unity centre, balance pan, mute and solo), the mixdown (normalized to −1 dBFS), the ZIP (names, trim, alignment,
  bit-exact stems, README), the session's save and load, the strings (both languages, the same format arguments).
- `app/src/test/cpp`: `usbstream.c` against a pretend usbfs: every frame once and in order across reads, the mic out
  in order on both channels at 44.1 frames a millisecond, ten minutes of ±200 ppm drift with no drop and no silence,
  an unplug, every URB taken back.
- `app/src/test/kotlin`: HOME → EDIT → EXPORT on Robolectric, the Spanish strings, the meters.

## What only the FM-1 and a phone can tell

Tested here without either; still to confirm on the S23 Ultra and others:

- The stream: STEMS and STEMS+IN read for minutes with no errors (the footer shows **USB OK** or the error count), the
  sample rate SET_CUR accepted, the interfaces released cleanly (if normal USB audio misbehaves after, replug).
- The mic out in STEMS+IN: the voice heard through the last live track with its sends; no clicks from the drift
  correction; the gain range.
- The latency: MEASURE clear (clarity above 3) at a normal distance; the value against the probe's test 7; in a take
  sung with headphones (the mic not hearing the speaker) the take falling back to the last MEASURE; a long take (10+
  min) staying in time to the end.
- Long takes and storage: an hour on the phone, the screen kept on, a call coming in, the app killed mid-take and the
  take recovered.
- Playback routing: EDIT plays to the phone's speaker or headphones, not to the FM-1, with the FM-1 still plugged in.
- Export to Downloads, Drive and WhatsApp; the ZIP opening in a DAW with the voice in time.

## Next (phase 2 and on)

Pinch-zoom and a bar grid; cut, move and fade regions; undo; FLAC / AAC and 48 kHz (a resampler); SYNC MIDI (PLAY
and STOP on the FM-1 start and end a take); a USB microphone through a hub; the FM-1's beings on the record page;
recording in a foreground service (the screen off); a DAWproject export.

## An iOS version

Not in this phase. What it would take:

- **Audio in:** iOS opens class-compliant multichannel USB inputs itself, so no own driver: `AVAudioSession` with the
  FM-1 as the preferred input and `AVAudioEngine`'s input node at its 8 or 10 channels (to confirm that iOS passes all
  ten of STEMS+IN). iPhone 15 and later (USB-C) directly; Lightning phones with Apple's camera adapter.
- **The mic to the FM-1:** the hard part. An `AVAudioSession` has one input route: with the FM-1 as the input, the
  iPhone's own microphone is not available at the same time. Options to try: a USB mic and the FM-1 on a hub (iOS can
  aggregate some), or the FM-1's audio input on a wired mic; to measure on a real iPhone before committing.
- **The rest ports straight:** `core` is plain Kotlin (WAV, latency, mix, ZIP) and can be shared through Kotlin
  Multiplatform (swapping `java.io` / `java.util.zip` for Okio and a small ZIP writer) or rewritten in Swift; the UI is
  four pages (SwiftUI); the share sheet is `UIActivityViewController`.
- **Distribution:** an Apple developer account (USD 99 a year) for TestFlight and the App Store, and a macOS runner in
  the workflow to build and sign.
