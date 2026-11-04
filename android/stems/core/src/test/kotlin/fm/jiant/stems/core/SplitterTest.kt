// SPDX-License-Identifier: GPL-3.0-only
package fm.jiant.stems.core

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.ByteArrayOutputStream

class SplitterTest {
    private class Collect : PcmSink {
        val b = ByteArrayOutputStream()
        override fun write(b: ByteArray, off: Int, len: Int) = this.b.write(b, off, len)
        fun shorts(): IntArray { val a = b.toByteArray(); return IntArray(a.size / 2) { (a[2 * it].toInt() and 0xFF) or (a[2 * it + 1].toInt() shl 8) } }
    }

    /** n frames of the STEMS+IN stream: channel c of frame f = f * 16 + c (and a few full-scale ones) */
    private fun stream(ch: Int, n: Int, value: (Int, Int) -> Int): ByteArray {
        val b = ByteArray(n * ch * 2)
        for (f in 0 until n) for (c in 0 until ch) {
            val v = value(f, c)
            b[(f * ch + c) * 2] = v.toByte(); b[(f * ch + c) * 2 + 1] = (v shr 8).toByte()
        }
        return b
    }

    @Test fun lanesGetTheirChannelsEvenWhenFramesAreSplit() {
        val lanes = Layout.lanes(10)
        val sinks = lanes.map { if (it.id == "T2" || it.id == "FX" || it.id == "DRY") Collect() else null }
        val sp = StemSplitter(10, lanes, sinks)
        val b = stream(10, 100) { f, c -> f * 16 + c }
        var o = 0
        val cuts = intArrayOf(7, 1, 333, 20, 999, 1)              // odd sizes: frames and samples split
        var k = 0
        while (o < b.size) { val n = minOf(cuts[k++ % cuts.size], b.size - o); sp.write(b, o, n); o += n }
        assertEquals(100L, sp.frames)
        val t2 = (sinks[1] as Collect).shorts()
        assertArrayEquals(IntArray(100) { it * 16 + 1 }, t2)
        val fx = (sinks[6] as Collect).shorts()
        assertArrayEquals(IntArray(200) { (it / 2) * 16 + 6 + it % 2 }, fx)
        val dry = (sinks[8] as Collect).shorts()
        assertArrayEquals(IntArray(100) { it * 16 + 9 }, dry)
    }

    @Test fun metersHoldThePeakAndTheClip() {
        val lanes = Layout.lanes(8)
        val sp = StemSplitter(8, lanes, lanes.map { null })
        sp.write(stream(8, 50) { f, c -> if (c == 3 && f == 10) -32768 else if (c == 0) -f * 100 else c }, 0, 50 * 16)
        val pk = IntArray(8); val cl = BooleanArray(8)
        sp.takePeaks(pk, cl)
        assertEquals(4900, pk[0])
        assertEquals(32768, pk[3])
        assertEquals(7, pk[7])
        assertTrue(cl[3]); assertFalse(cl[0])
        sp.takePeaks(pk, cl)
        assertEquals(0, pk[0]); assertFalse(cl[3])
    }

    @Test fun theProbeGetsTheStemsSumAndTheDryMic() {
        val lanes = Layout.lanes(10)
        val probe = LatencyProbe(window = Layout.RATE, keep = 2)
        val sp = StemSplitter(10, lanes, lanes.map { null }, probe)
        // the stems' sum a click train; the dry mic the same 37 frames later: the probe must find 37
        val n = Layout.RATE * 3
        val mix = IntArray(n) { if ((it / 997) % 3 == 0 && it % 997 < 3) 3000 else ((it * 7919) % 61) - 30 }
        val b = stream(10, n) { f, c -> when { c < 8 -> mix[f] / 8; c == 9 -> if (f >= 37) mix[f - 37] / 8 else 0; else -> 0 } }
        sp.write(b, 0, b.size)
        val r = probe.estimate(maxLag = 2000)
        assertEquals(37, r.frames)
    }
}
