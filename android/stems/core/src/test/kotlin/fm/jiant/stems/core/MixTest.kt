// SPDX-License-Identifier: GPL-3.0-only
package fm.jiant.stems.core

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class MixTest {
    @get:Rule val tmp = TemporaryFolder()

    /** a session of a T1 (mono), FX (stereo) and VOX (mono) of [n] frames each, from arrays */
    private fun session(n: Int, t1: FloatArray, fx: FloatArray, vox: FloatArray): Pair<Session, Renderer> {
        val s = Session(tmp.newFolder())
        s.frames = n.toLong(); s.trimEnd = s.frames
        s.lanes.add(Lane("T1", LaneKind.TRACK, "T1.wav", 1))
        s.lanes.add(Lane("FX", LaneKind.FX, "FX.wav", 2))
        s.lanes.add(Lane("VOX", LaneKind.VOICE, "VOX.wav", 1))
        return s to Renderer(s, mapOf("T1" to ArraySource(1, t1), "FX" to ArraySource(2, fx), "VOX" to ArraySource(1, vox)))
    }

    @Test fun centreSumsTheStemsAtUnity() {
        val (_, r) = session(4, floatArrayOf(0.1f, 0.2f, 0.3f, 0.4f), floatArrayOf(0.01f, 0.02f, 0f, 0f, 0f, 0f, 0f, 0f), FloatArray(4))
        val o = FloatArray(8)
        r.mix(0, 4, o)
        assertEquals(0.11f, o[0], 1e-6f); assertEquals(0.12f, o[1], 1e-6f)
        assertEquals(0.2f, o[2], 1e-6f); assertEquals(0.2f, o[3], 1e-6f)
    }

    @Test fun gainAndBalancePan() {
        val (s, r) = session(1, floatArrayOf(0.5f), FloatArray(2), FloatArray(1))
        val t1 = s.lane("T1")!!
        t1.gainDb = -6.0206f
        t1.pan = 0.5f                                     // right: the left side at half
        val o = FloatArray(2)
        r.mix(0, 1, o)
        assertEquals(0.125f, o[0], 1e-4f)
        assertEquals(0.25f, o[1], 1e-4f)
        t1.pan = -1f
        r.mix(0, 1, o)
        assertEquals(0.25f, o[0], 1e-4f); assertEquals(0f, o[1], 0f)
        assertEquals(0f, Mix.gain(-60f), 0f)
    }

    @Test fun muteAndSolo() {
        val (s, r) = session(1, floatArrayOf(0.5f), floatArrayOf(0.25f, 0.25f), floatArrayOf(0.125f))
        val o = FloatArray(2)
        s.lane("FX")!!.mute = true
        r.mix(0, 1, o)
        assertEquals(0.625f, o[0], 1e-6f)
        s.lane("VOX")!!.solo = true
        r.mix(0, 1, o)
        assertEquals(0.125f, o[0], 1e-6f)
        assertTrue(s.audible(s.lane("VOX")!!)); assertFalse(s.audible(s.lane("T1")!!))
        s.lane("FX")!!.solo = true                        // soloed but muted: mute wins
        r.mix(0, 1, o)
        assertEquals(0.125f, o[0], 1e-6f)
    }

    @Test fun latencyShiftMovesTheVoiceEarlier() {
        val n = 5000
        val lat = 1234
        val t1 = FloatArray(n); t1[1000] = 0.5f                           // the beat the singer heard at 1000
        val vox = FloatArray(n); vox[1000 + lat] = 0.25f                   // their voice came back the loop later
        val (s, r) = session(n, t1, FloatArray(2 * n), vox)
        s.lanes.add(Lane("DRY", LaneKind.DRY, "DRY.wav", 1))
        s.applyLatency(lat, Session.LATENCY_TAKE, 9.0)
        assertEquals(-lat.toLong(), s.lane("VOX")!!.offset)
        assertEquals(-lat.toLong(), s.lane("DRY")!!.offset)
        assertEquals(0L, s.lane("FX")!!.offset)                           // FX stays unless asked
        assertEquals(0L, s.lane("T1")!!.offset)
        val o = FloatArray(2 * n)
        r.mix(0, n, o)
        assertEquals(0.75f, o[2 * 1000], 1e-6f)                           // together
        for (i in 0 until n) if (i != 1000) assertEquals("frame $i", 0f, o[2 * i], 0f)
        // the lane alone, as the ZIP exports it: the voice at 1000, the tail silent
        val v = FloatArray(n)
        r.lane(s.lane("VOX")!!, 0, n, v, false)
        assertEquals(0.25f, v[1000], 0f)
        assertEquals(0f, v[n - 1], 0f)
        s.applyLatency(lat, Session.LATENCY_MANUAL, 0.0, fx = true)
        assertEquals(-lat.toLong(), s.lane("FX")!!.offset)
    }
}
