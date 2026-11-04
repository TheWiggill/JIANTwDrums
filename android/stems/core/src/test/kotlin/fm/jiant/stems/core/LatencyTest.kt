// SPDX-License-Identifier: GPL-3.0-only
package fm.jiant.stems.core

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Random

class LatencyTest {
    /** a drum-like signal: decaying noise bursts on a 16th grid at 124 BPM, a quieter bed of noise */
    private fun drums(n: Int, seed: Long): FloatArray {
        val r = Random(seed)
        val x = FloatArray(n)
        val step = (Layout.RATE * 60.0 / 124 / 4).toInt()
        var env = 0f
        for (i in 0 until n) {
            if (i % step == 0 && r.nextInt(3) != 0) env = 0.5f + r.nextFloat() * 0.5f
            env *= 0.9993f
            x[i] = (r.nextFloat() * 2 - 1) * (env * 8000f + 200f)
        }
        return x
    }

    /** [x] delayed by [lag], scaled, with the room's noise and a little hum: what the phone's mic hears */
    private fun heard(x: FloatArray, lag: Int, seed: Long): FloatArray {
        val r = Random(seed)
        return FloatArray(x.size) { i -> (if (i >= lag) x[i - lag] * 0.3f else 0f) + (r.nextFloat() * 2 - 1) * 300f + 200f * kotlin.math.sin(i * 0.0071f) }
    }

    @Test fun findsTheRoundTrip() {
        for ((lag, seed) in listOf(1 to 1L, 441 to 2L, 1102 to 3L, 1323 to 4L, 3001 to 5L)) {   // 0.02 .. 68 ms
            val a = drums(Layout.RATE * 8, seed)
            val r = Latency.lag(a, heard(a, lag, seed + 100), Layout.RATE / 5)
            assertEquals("lag $lag", lag.toDouble(), r.frames.toDouble(), 1.0)
            assertTrue("clarity ${r.confidence} at $lag", r.clear)
        }
    }

    @Test fun unrelatedSignalsAreNotClear() {
        val a = drums(Layout.RATE * 8, 7)
        val b = drums(Layout.RATE * 8, 8)
        assertFalse(Latency.lag(a, b, Layout.RATE / 5).clear)
    }

    @Test fun tooShortGivesNothing() {
        val r = Latency.lag(FloatArray(2000), FloatArray(2000), Layout.RATE / 5)
        assertEquals(-1, r.frames)
        assertFalse(r.clear)
    }

    @Test fun probeKeepsTheWindowWhereTheMicHearsTheSpeaker() {
        // 30 s: the mic hears the speaker (lag 1200) only from 20 s on (before, the singer is far off)
        val n = Layout.RATE * 30
        val a = drums(n, 11)
        val h = heard(a, 1200, 12)
        val quiet = Random(13)
        val p = LatencyProbe()
        for (i in 0 until n) p.feed(a[i], if (i < Layout.RATE * 20) (quiet.nextFloat() * 2 - 1) * 50f else h[i])
        val r = p.estimate()
        assertEquals(1200.0, r.frames.toDouble(), 1.0)
        assertTrue(r.clear)
        assertEquals(27.2, r.ms(Layout.RATE), 0.1)
    }
}
