// SPDX-License-Identifier: GPL-3.0-only
package fm.jiant.stems.core

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.io.File
import java.util.zip.ZipInputStream
import kotlin.math.abs
import kotlin.math.log10

class ExportTest {
    @get:Rule val tmp = TemporaryFolder()

    /** a recorded take on disk, as the recorder leaves it: T1, FX, VOX, DRY of 3 s, the voice 1000 frames late */
    private fun take(): Session {
        val dir = tmp.newFolder("take")
        val n = Layout.RATE * 3
        val lat = 1000
        fun wav(name: String, ch: Int, v: (Int, Int) -> Int) = WavFileWriter(File(dir, name), ch, Layout.RATE).use { w ->
            val b = ByteArray(n * ch * 2)
            for (f in 0 until n) for (c in 0 until ch) { val x = v(f, c); b[(f * ch + c) * 2] = x.toByte(); b[(f * ch + c) * 2 + 1] = (x shr 8).toByte() }
            w.write(b, 0, b.size)
        }
        wav("T1.wav", 1) { f, _ -> if (f % 4410 == 0) 20000 else 0 }               // a click every 100 ms
        wav("FX.wav", 2) { f, c -> if (c == 0) (f % 200) - 100 else 100 - (f % 200) }
        wav("VOX.wav", 1) { f, _ -> if (f >= lat && (f - lat) % 4410 == 0) 10000 else 0 }
        wav("DRY.wav", 1) { f, _ -> if (f >= lat && (f - lat) % 4410 == 0) 12000 else 0 }
        val s = Session(dir)
        s.name = "Night Drive #3"
        s.deviceChannels = 10
        s.frames = n.toLong(); s.trimEnd = s.frames
        for (spec in Layout.lanes(10).filter { it.id in setOf("T1", "FX", "VOX", "DRY") }) s.lanes.add(Session.defaultLane(spec))
        s.applyLatency(lat, Session.LATENCY_TAKE, 12.0)
        s.save()
        return s
    }

    private fun renderer(s: Session) = Renderer(s, s.lanes.associate { it.id to WavSource(s.file(it)) as FrameSource })

    private fun unzip(b: ByteArray): Map<String, ByteArray> {
        val m = LinkedHashMap<String, ByteArray>()
        ZipInputStream(ByteArrayInputStream(b)).use { z ->
            while (true) { val e = z.nextEntry ?: break; m[e.name] = z.readBytes() }
        }
        return m
    }

    private fun source(b: ByteArray): WavSource { val f = File.createTempFile("exp", ".wav", tmp.root); f.writeBytes(b); return WavSource(f) }

    @Test fun zipHoldsTheStemsTrimmedAndAligned() {
        val s = Session.load(take().dir)                                          // (through the saved session)
        s.trimStart = 2205; s.trimEnd = Layout.RATE * 2L                          // 50 ms .. 2 s
        val bo = ByteArrayOutputStream()
        Export.stemsZip(renderer(s), bo, ExportOptions(includeMix = true))
        val z = unzip(bo.toByteArray())
        val dir = "Night_Drive_3/"
        assertEquals(listOf("T1.wav", "FX.wav", "VOX.wav", "DRY.wav", "MIX.wav", "README.txt").map { dir + it }, z.keys.toList())
        assertEquals(Export.zipNames(s, ExportOptions(includeMix = true)).map { dir + it }, z.keys.toList())
        val len = (Layout.RATE * 2 - 2205).toLong()
        for (k in listOf("T1", "FX", "VOX", "DRY")) assertEquals(k, len, source(z[dir + "$k.wav"]!!).frames)
        val t1 = source(z[dir + "T1.wav"]!!); val vox = source(z[dir + "VOX.wav"]!!); val fx = source(z[dir + "FX.wav"]!!)
        assertEquals(1, t1.channels); assertEquals(16, t1.info.bits); assertEquals(2, fx.channels)
        val a = FloatArray(len.toInt()); val v = FloatArray(len.toInt())
        t1.read(0, len.toInt(), a); vox.read(0, len.toInt(), v)
        // the first click in the trimmed take: 4410 - 2205; the voice's lands on it (moved by the latency)
        assertEquals(20000, Wav.to16(a[2205])); assertEquals(10000, Wav.to16(v[2205]))
        for (i in 0 until len.toInt()) assertEquals("frame $i", a[i] != 0f, v[i] != 0f)
        val f2 = FloatArray(4); fx.read(0, 2, f2)                                   // bit-exact, moved by the trim only
        assertEquals((2205 % 200) - 100, Wav.to16(f2[0])); assertEquals(100 - (2205 % 200), Wav.to16(f2[1]))
        val readme = String(z[dir + "README.txt"]!!)
        assertTrue(readme.contains("1000 frames")); assertTrue(readme.contains("Night Drive #3"))
        val mix = source(z[dir + "MIX.wav"]!!)
        assertEquals(2, mix.channels); assertEquals(24, mix.info.bits); assertEquals(len, mix.frames)
    }

    @Test fun zipWithTheMixLeavesOutMutedLanes() {
        val s = take()
        s.lane("T1")!!.pan = 1f
        val bo = ByteArrayOutputStream()
        val o = ExportOptions(bits = 16, stemsWithMix = true, includeMix = false)
        Export.stemsZip(renderer(s), bo, o)
        val z = unzip(bo.toByteArray())
        assertEquals(listOf("T1.wav", "FX.wav", "VOX.wav", "README.txt").map { "Night_Drive_3/$it" }, z.keys.toList())   // DRY muted
        val t1 = source(z["Night_Drive_3/T1.wav"]!!)
        assertEquals(2, t1.channels)
        val x = FloatArray(2); t1.read(0, 1, x)
        assertEquals(0, Wav.to16(x[0])); assertEquals(20000, Wav.to16(x[1]))       // panned right
    }

    @Test fun mixdownIsTheMixNormalized() {
        val s = take()
        val bo = ByteArrayOutputStream()
        val g = Export.mixdown(renderer(s), bo, ExportOptions(bits = 24, normalize = true))
        val m = source(bo.toByteArray())
        assertEquals(s.frames, m.frames)
        val x = FloatArray((m.frames * 2).toInt()); m.read(0, m.frames.toInt(), x)
        val pk = x.maxOf { abs(it) }
        assertEquals(-1.0, 20 * log10(pk.toDouble()), 0.01)
        assertEquals(((30000 - 90) / 32768f) * g, x[2 * 4410], 1e-4f)     // the click, the voice and the FX there
        val raw = ByteArrayOutputStream()
        assertEquals(1f, Export.mixdown(renderer(s), raw, ExportOptions(bits = 16, normalize = false)), 0f)
        val r = source(raw.toByteArray()); val y = FloatArray(2); r.read(4410, 1, y)
        assertEquals(30000 - 90, Wav.to16(y[0]))                                   // + the FX's -90 there
    }

    @Test fun progressCanCancel() {
        val s = take()
        var calls = 0
        try {
            Export.mixdown(renderer(s), ByteArrayOutputStream(), ExportOptions(), Progress { calls++; calls < 3 })
            throw AssertionError("not cancelled")
        } catch (_: CancelledException) { }
        assertEquals(3, calls)
    }

    @Test fun sessionRoundTrips() {
        val s = take()
        s.lane("T1")!!.gainDb = -3.5f; s.lane("FX")!!.solo = true; s.trimStart = 10; s.trimEnd = 2000; s.name = "Toma ñ"
        s.save()
        val l = Session.load(s.dir)
        assertEquals("Toma ñ", l.name)
        assertEquals(10L, l.trimStart); assertEquals(2000L, l.trimEnd); assertEquals(1000, l.latency)
        assertEquals(Session.LATENCY_TAKE, l.latencySource)
        assertEquals(-3.5f, l.lane("T1")!!.gainDb, 1e-4f); assertTrue(l.lane("FX")!!.solo); assertTrue(l.lane("DRY")!!.mute)
        assertEquals(-1000L, l.lane("VOX")!!.offset)
        assertEquals(listOf("T1", "FX", "VOX", "DRY"), l.lanes.map { it.id })
        assertEquals("Toma", Session.safeName("Toma ñ").trimEnd('_'))
        assertEquals("take", Session.safeName("  "))
    }

    @Test fun aTakeCutShortIsRecovered() {
        val s = take()
        s.complete = false; s.frames = 0; s.trimEnd = 0
        s.save()
        for (l in s.lanes) java.io.RandomAccessFile(s.file(l), "rw").use { raf -> raf.seek(40); raf.write(ByteArray(4)) }   // sizes 0
        val l = Session.load(s.dir)
        assertTrue(!l.complete)
        assertTrue(l.recover())
        val r = Session.load(s.dir)
        assertTrue(r.complete)
        assertEquals(Layout.RATE * 3L, r.frames); assertEquals(r.frames, r.trimEnd)
        WavSource(r.file(r.lane("FX")!!)).use { assertEquals(r.frames, it.frames) }
    }

    @Test fun peaksFollowTheClicks() {
        val s = take()
        WavSource(s.file(s.lane("T1")!!)).use { src ->
            val p = Peaks.compute(src, 441)
            assertEquals(300, p.size)
            assertEquals(20000 / 32768f, p[0], 1e-6f); assertEquals(0f, p[1], 0f); assertEquals(20000 / 32768f, p[10], 1e-6f)
        }
    }
}
