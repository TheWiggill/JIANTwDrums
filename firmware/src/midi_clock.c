/* SPDX-License-Identifier: GPL-3.0-only
 * Adapted from MIDI clock contributions by ChanceTheMaker and keremimo (2026).
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Audio ISR state. Input timestamps keep USB/TRS jitter outside the render loop. */
static struct {
    uint32_t pos, rendered, last_ms, start_ms, rem, interval_ms;
    uint32_t pulse_samples, interp_q8, tempo_ms;
    uint8_t mode, have_pulse, tempo_valid, tempo_n;
} midi_clock;

/* (V10.33) the tempo of what leads: the pulses' times over up to two beats (48 pulses: a millisecond in a second,
 * USB's frames and Bluetooth's bursts averaged out), smoothed, then a whole BPM held until the measure leaves it by
 * more than 0.6 (V10.32 and before: six pulses, raw: 104 showed 103 105 104.. and the steps' length moved with it).
 * The phase stays the pulses' own (midi_clock.pos steps a 24th of a beat each) */
#define MCK_N 48u
static uint32_t mck_t[MCK_N + 1u], mck_n, mck_sm;      /* the pulses' ms, how many, a beat's ms (Q8, smoothed) */
static void mck_reset(void) { mck_n = 0; mck_sm = 0; }
static __attribute__((noinline)) void midi_clock_transport(uint32_t status, uint32_t ms)
{
    mck_reset();
    if (status == 0xFAu) {                         /* Start: step zero */
        midi_clock.pos = midi_clock.rendered = midi_clock.rem = 0;
        midi_clock.have_pulse = midi_clock.tempo_valid = 0;
        midi_clock.interval_ms = 0;
        midi_clock.start_ms = ms;
        seq_start();
    } else if (status == 0xFBu) {                  /* Continue: preserve the step */
        midi_clock.pos = midi_clock.rendered;
        midi_clock.rem = 0;
        midi_clock.have_pulse = midi_clock.tempo_valid = 0;
        midi_clock.interval_ms = 0;
        midi_clock.start_ms = ms;
        if (!song.playing)
            motion_begin();
        song.playing = 1;
    } else if (status == 0xFCu) {
        seq_stop();
    }
}

static __attribute__((noinline)) void midi_clock_pulse(uint32_t ms)
{
    uint32_t q, span, bpm = 0;
    if (!midi_clock.tempo_valid || (mck_n && ms - mck_t[(mck_n - 1u) % (MCK_N + 1u)] > 250u)) {
        midi_clock.tempo_valid = 1;                /* (the first pulse, or after a gap: measured afresh) */
        mck_reset();
    }
    mck_t[mck_n % (MCK_N + 1u)] = ms;
    mck_n++;
    span = mck_n - 1u < MCK_N ? mck_n - 1u : MCK_N;
    if (span >= 6u) {
        uint32_t dt = ms - mck_t[(mck_n - 1u - span) % (MCK_N + 1u)];
        if (dt >= span * 2u && dt <= span * 63u) { /* (40 .. 240 BPM's pulses, room either side) */
            uint32_t meas = dt * 24u * 256u / span, b;
            mck_sm = !mck_sm ? meas : (uint32_t)((int32_t)mck_sm + ((int32_t)(meas - mck_sm) >> (span < 24u ? 1 : 2)));
            b = 60000u * 256u / mck_sm * 256u + (60000u * 256u % mck_sm) * 256u / mck_sm;   /* BPM, Q8 */
            bpm = (uint32_t)song.g[G_BPM];
            if (!midi_beat_samples || (int32_t)(b - bpm * 256u) > 154 || (int32_t)(bpm * 256u - b) > 154)
                bpm = (b + 128u) >> 8;             /* (held within 0.6 BPM: a whole BPM, steady) */
            bpm = (uint32_t)clamp((int32_t)bpm, 40, 240);
        }
    }
    if (bpm) {
        {
            uint32_t old_beat = beat_samples(), new_beat = (uint32_t)FS * 60u / bpm;
            if (song.playing && new_beat != old_beat) {
                uint32_t i, ratio = (new_beat << 12) / old_beat;
                /* Preserve each track's fractional step phase when the master
                 * changes tempo. The next pulse then lands on its new boundary. */
                for (i = 0; i < NTRK; i++) {
                    if (trk[i].seq_pos < (uint32_t)FS * 2u)
                        trk[i].seq_pos = (uint32_t)(((uint64_t)trk[i].seq_pos * ratio + 2048u) >> 12);
                    if (trk[i].seq_off)
                        trk[i].seq_off = (uint32_t)(((uint64_t)trk[i].seq_off * ratio + 2048u) >> 12);
                }
                if (clk_pos < (uint32_t)FS * 2u)   /* (the metronome's beat as the steps) */
                    clk_pos = (uint32_t)(((uint64_t)clk_pos * ratio + 2048u) >> 12);
            }
            midi_beat_samples = new_beat;
            song.g[G_BPM] = (int16_t)bpm;
        }
    }
    if (song.playing) {
        if (midi_clock.have_pulse) {
            q = beat_samples() + midi_clock.rem;
            midi_clock.pos += q / 24u;
            midi_clock.rem = q % 24u;
        }
        midi_clock.have_pulse = 1;
    }
    midi_clock.pulse_samples = beat_samples() / 24u;
    midi_clock.interval_ms = (beat_samples() * 1000u / ((uint32_t)FS * 24u)) + 2u;   /* (V10.33: the steady tempo's, a little
                                                         * room: interpolate up to it, never past the next pulse) */
    midi_clock.interp_q8 = (uint32_t)((uint64_t)FS * 256u / 1000u);   /* samples a ms, Q8: as the tempo, not each pulse's jitter */
    midi_clock.last_ms = ms;
}

static uint32_t midi_clock_advance(uint32_t now)
{
    uint32_t target, elapsed, offset, n;
    if (!midi_clock.have_pulse)
        return 0;
    elapsed = now - midi_clock.last_ms;
    /* Interpolate to the next pulse, never across it before it arrives. */
    if (elapsed > midi_clock.interval_ms)
        elapsed = midi_clock.interval_ms;
    offset = elapsed * midi_clock.interp_q8 >> 8;
    if (offset >= midi_clock.pulse_samples)
        offset = midi_clock.pulse_samples - 1u;
    target = midi_clock.pos + offset;
    n = (int32_t)(target - midi_clock.rendered) > 0 ? target - midi_clock.rendered : 0u;
    if (n > (uint32_t)FS / 8u)
        n = (uint32_t)FS / 8u;
    midi_clock.rendered += n;
    return n;
}

