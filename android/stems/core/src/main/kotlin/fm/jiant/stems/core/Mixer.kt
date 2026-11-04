// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the session applied to its lanes, a block at a time, for playback and the exports. Each lane is read
// at its offset (a voice lane moved earlier by the latency), scaled by its gain, and for the mix panned with a
// balance law: centre is unity on both sides (the FM-1's stems are what each track adds to the mix with its pan
// left out, so the eight summed at centre give the mix as the FM-1's buses hear it), towards one side the other
// side fades out. Mute and solo decide what the mix hears.
package fm.jiant.stems.core

import kotlin.math.pow

object Mix {
    fun gain(db: Float): Float = if (db <= -60f) 0f else 10f.pow(db / 20f)
    fun panL(p: Float) = (1f - p).coerceIn(0f, 1f)
    fun panR(p: Float) = (1f + p).coerceIn(0f, 1f)
}

class Renderer(val session: Session, private val sources: Map<String, FrameSource>) {
    private var tmp = FloatArray(0)

    /** lane [l] on the timeline from [start] for [n] frames, in its own channels, with its gain if [withGain] */
    fun lane(l: Lane, start: Long, n: Int, out: FloatArray, withGain: Boolean) {
        val src = sources[l.id]
        val ch = l.channels
        if (src == null) { java.util.Arrays.fill(out, 0, n * ch, 0f); return }
        src.read(start - l.offset, n, out)
        if (withGain) {
            val g = Mix.gain(l.gainDb)
            if (g != 1f) for (i in 0 until n * ch) out[i] *= g
        }
    }

    /** lane [l] as a stereo pair with its gain and pan (an export's "with the mix" stem) */
    fun laneStereo(l: Lane, start: Long, n: Int, out: FloatArray) {
        java.util.Arrays.fill(out, 0, n * 2, 0f)
        addLane(l, start, n, out)
    }

    private fun addLane(l: Lane, start: Long, n: Int, out: FloatArray) {
        val ch = l.channels
        if (tmp.size < n * ch) tmp = FloatArray(n * ch)
        lane(l, start, n, tmp, false)
        val g = Mix.gain(l.gainDb)
        val gl = g * Mix.panL(l.pan)
        val gr = g * Mix.panR(l.pan)
        if (gl == 0f && gr == 0f) return
        if (ch == 1) {
            for (i in 0 until n) { val v = tmp[i]; out[2 * i] += v * gl; out[2 * i + 1] += v * gr }
        } else {
            for (i in 0 until n) { out[2 * i] += tmp[i * ch] * gl; out[2 * i + 1] += tmp[i * ch + 1] * gr }
        }
    }

    /** the stereo mix of the audible lanes from [start], [n] frames, interleaved L R */
    fun mix(start: Long, n: Int, out: FloatArray) {
        java.util.Arrays.fill(out, 0, n * 2, 0f)
        for (l in session.lanes) if (session.audible(l)) addLane(l, start, n, out)
    }
}
