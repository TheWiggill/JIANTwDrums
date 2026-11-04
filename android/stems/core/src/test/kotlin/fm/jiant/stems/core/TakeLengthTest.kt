// SPDX-License-Identifier: GPL-3.0-only
// (1.1) A take's length from the recorder to the export: a ZIP exported with the trim at the ends holds the whole take
// (a field take of 37.27 s came out 23.49 s); the trim's edges dragged to the ends are the ends exactly, a tap near one
// is not a trim, a touch on a lane near one moves the playhead; a take being recorded is never "recovered" (its WAVs cut
// to their headers' last patch while the recorder still writes), and a take cut short keeps all it wrote.
package fm.jiant.stems.core

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.io.File
import java.util.zip.ZipInputStream

class TakeLengthTest {
    @get:Rule val tmp = TemporaryFolder()

    /** a take as the recorder makes it: an 8-channel stream (STEMS) through the splitter into the lanes' WAVs, the
     * session saved at the start and finished as Engine.finish does; [secs] long */
    private fun record(secs: Double): Session {
        val dir = tmp.newFolder("take")
        val lanes = Layout.lanes(8)
        val s = Session(dir)
        s.name = "TOMA 03"; s.deviceChannels = 8; s.complete = false
        val writers = lanes.map { WavFileWriter(File(dir, it.id + ".wav"), it.channels.size, Layout.RATE) }
        for (l in lanes) s.lanes.add(Session.defaultLane(l))
        s.save()
        Session.recording = dir
        val sp = StemSplitter(8, lanes, writers)
        val total = (secs * Layout.RATE).toLong()
        val pkt = ByteArray(46 * 16)
        var f = 0L
        var acc = 0
        var ms = 0
        while (f < total) {                                        // packets of 44 / 45 / 46 frames, a millisecond each
            acc += 100; var n = 44; if (acc >= 1000) { acc -= 1000; n = 45 }
            if (ms % 57 == 0) n++                                  // (the FM-1's clock ahead: one frame more now and then)
            n = minOf(n.toLong(), total - f).toInt()
            for (i in 0 until n) for (c in 0 until 8) {
                val v = ((f + i) * 8 + c).toInt()
                pkt[(i * 8 + c) * 2] = v.toByte(); pkt[(i * 8 + c) * 2 + 1] = (v shr 8).toByte()
            }
            sp.write(pkt, 0, n * 16)
            f += n; ms++
            if (ms % 2000 == 0) writers.forEach { it.sync() }       // (the recorder's header patch every 2 s)
        }
        writers.forEach { it.close() }
        s.frames = sp.frames; s.trimStart = 0; s.trimEnd = s.frames; s.complete = true
        s.save()
        Session.recording = null
        return s
    }

    private fun zipWavFrames(b: ByteArray): Map<String, Long> {
        val m = LinkedHashMap<String, Long>()
        ZipInputStream(ByteArrayInputStream(b)).use { z ->
            while (true) {
                val e = z.nextEntry ?: break
                val d = z.readBytes()
                if (e.name.endsWith(".wav")) {
                    val f = File.createTempFile("zip", ".wav", tmp.root); f.writeBytes(d)
                    WavSource(f).use { m[e.name.substringAfterLast('/')] = it.frames }
                }
            }
        }
        return m
    }

    @Test fun anUntrimmedTakeExportsWhole() {
        val s0 = record(37.27)
        val s = Session.forList(s0.dir)!!                          // (as the list opens it)
        assertEquals(s0.frames, s.frames)
        assertEquals(0L, s.trimStart); assertEquals(s.frames, s.trimEnd)
        val bo = ByteArrayOutputStream()
        Export.stemsZip(Renderer(s, s.lanes.associate { it.id to WavSource(s.file(it)) as FrameSource }), bo, ExportOptions(includeMix = true))
        val frames = zipWavFrames(bo.toByteArray())
        assertEquals(listOf("T1.wav", "T2.wav", "T3.wav", "T4.wav", "T5.wav", "T6.wav", "FX.wav", "MIX.wav"), frames.keys.toList())
        for ((name, n) in frames) assertEquals(name, s.frames, n)
        assertTrue(Export.readme(s, ExportOptions(), s.lanes, 1f).contains("37.27 s (trimmed from 37.27 s at 0.00 s)"))
    }

    @Test fun theTrimEdgesDraggedToTheEndsAreTheEnds() {
        val s = Session(tmp.newFolder("t")); s.frames = 1_643_607; s.trimStart = 0; s.trimEnd = s.frames
        val x0 = 140f; val w = 1080f; val snap = 25f; val slop = 10f; val min = 4410L
        // the end edge dragged away and back to the screen's last pixel: the take's last frame, not a pixel short
        Trim.drag(s, Trim.END, s.trimEnd, w - 2f, 600f, x0, w, snap, slop, min)
        assertTrue(s.trimEnd < s.frames * 6 / 10)
        Trim.drag(s, Trim.END, s.trimEnd, 600f, w - 1f, x0, w, snap, slop, min)
        assertEquals(s.frames, s.trimEnd)
        // the start edge to the timeline's first pixel: frame 0
        Trim.drag(s, Trim.START, 0, x0 + 1f, 400f, x0, w, snap, slop, min)
        assertTrue(s.trimStart > 0)
        Trim.drag(s, Trim.START, s.trimStart, 400f, x0 + 2f, x0, w, snap, slop, min)
        assertEquals(0L, s.trimStart)
        // taken 20 px left of its line and jittered 6 px: nothing moves (1.0: the edge jumped to the finger)
        Trim.drag(s, Trim.END, s.trimEnd, w - 20f, w - 26f, x0, w, snap, slop, min)
        assertEquals(s.frames, s.trimEnd)
        // a touch on a lane 30 px (10 dp) from the end's line moves the playhead; in the ruler it takes the edge
        val b = Trim.xOf(s.trimEnd, s.frames, x0, w)
        assertEquals(Trim.NONE, Trim.grab(b - 30f, 300f, Trim.xOf(0, s.frames, x0, w), b, 60f, 66f))
        assertEquals(Trim.END, Trim.grab(b - 30f, 20f, Trim.xOf(0, s.frames, x0, w), b, 60f, 66f))
        assertTrue(!Trim.trimmed(s))
    }

    @Test fun theTakeBeingRecordedIsLeftAloneAndACutTakeKeepsItsEnd() {
        val dir = tmp.newFolder("rec")
        val s = Session(dir); s.name = "R"; s.deviceChannels = 8; s.complete = false
        s.lanes.add(Session.defaultLane(Layout.lanes(8)[0]))
        s.save()
        val w = WavFileWriter(File(dir, "T1.wav"), 1, Layout.RATE)
        val sec = ByteArray(Layout.RATE * 2) { (it * 7).toByte() }
        w.write(sec, 0, sec.size); w.sync()                        // 1 s, the header patched
        w.write(sec, 0, sec.size); w.write(sec, 0, sec.size)       // 2 s more, written through (64 KB buffers)
        Session.recording = dir
        assertNull(Session.forList(dir))                           // (listed while it records: 1.0 cut its WAV to 1 s here)
        assertTrue(File(dir, "T1.wav").length() > 44L + 2L * Layout.RATE * 2)
        Session.recording = null
        // the app killed now: the next list recovers it from what the file holds, not from the 1 s header
        val r = Session.forList(dir)!!
        val onDisk = (File(dir, "T1.wav").length() - 44) / 2
        assertTrue("recovered ${r.frames} of $onDisk", r.frames == onDisk && r.frames > 2L * Layout.RATE)
        assertEquals(r.frames, r.trimEnd)
        w.close()
    }
}
