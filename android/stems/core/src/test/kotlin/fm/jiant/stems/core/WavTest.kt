// SPDX-License-Identifier: GPL-3.0-only
package fm.jiant.stems.core

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.RandomAccessFile

class WavTest {
    @get:Rule val tmp = TemporaryFolder()

    private fun le16(v: Int) = byteArrayOf(v.toByte(), (v shr 8).toByte())

    @Test fun headerIsCanonicalPcm() {
        val h = Wav.header(2, 44100, 16, 1000)
        assertEquals(44, h.size)
        assertEquals("RIFF", String(h, 0, 4))
        assertEquals(36 + 4000L, Wav.u32(h, 4))
        assertEquals("WAVEfmt ", String(h, 8, 8))
        assertEquals(16L, Wav.u32(h, 16))
        assertEquals(1, Wav.u16(h, 20))                 // PCM
        assertEquals(2, Wav.u16(h, 22))
        assertEquals(44100L, Wav.u32(h, 24))
        assertEquals(44100L * 4, Wav.u32(h, 28))
        assertEquals(4, Wav.u16(h, 32))
        assertEquals(16, Wav.u16(h, 34))
        assertEquals("data", String(h, 36, 4))
        assertEquals(4000L, Wav.u32(h, 40))
        val h24 = Wav.header(1, 44100, 24, 10)
        assertEquals(3, Wav.u16(h24, 32))
        assertEquals(30L, Wav.u32(h24, 40))
    }

    @Test fun fileWriterRoundTripsBitExact() {
        val f = File(tmp.root, "t.wav")
        val samples = intArrayOf(0, 1, -1, 32767, -32768, 1234, -4321, 77)
        WavFileWriter(f, 2, 44100).use { w ->
            for (s in samples) w.write(le16(s), 0, 2)              // a sample at a time: the buffer's edges
        }
        assertEquals(44L + samples.size * 2, f.length())
        WavSource(f).use { src ->
            assertEquals(2, src.channels)
            assertEquals(4L, src.frames)
            val x = FloatArray(8)
            src.read(0, 4, x)
            for (i in samples.indices) assertEquals(samples[i], Wav.to16(x[i]))
        }
    }

    @Test fun readOutsideTheFileIsSilence() {
        val f = File(tmp.root, "m.wav")
        WavFileWriter(f, 1, 44100).use { w -> for (s in intArrayOf(100, 200, 300)) w.write(le16(s), 0, 2) }
        WavSource(f).use { src ->
            val x = FloatArray(7)
            src.read(-2, 7, x)
            assertArrayEquals(intArrayOf(0, 0, 100, 200, 300, 0, 0), x.map { Wav.to16(it) }.toIntArray())
        }
    }

    @Test fun syncPatchesTheHeaderWhileRecording() {
        val f = File(tmp.root, "s.wav")
        val w = WavFileWriter(f, 1, 44100)
        val big = ByteArray(200_000) { (it % 251).toByte() }
        w.write(big, 0, big.size)
        w.sync()
        RandomAccessFile(f, "r").use { raf -> assertEquals(100_000L, Wav.info(raf).frames) }
        w.close()
    }

    @Test fun repairFixesATakeCutShort() {
        val f = File(tmp.root, "cut.wav")
        RandomAccessFile(f, "rw").use { raf ->
            raf.write(Wav.header(2, 44100, 16, 0))               // as recording starts: sizes 0
            raf.write(ByteArray(4 * 500 + 3))                     // 500 frames and a partial one
        }
        val i = Wav.repair(f)
        assertEquals(500L, i.frames)
        assertEquals(44L + 2000, f.length())
        RandomAccessFile(f, "r").use { raf ->
            val h = ByteArray(44); raf.readFully(h)
            assertEquals(2000L, Wav.u32(h, 40))
            assertEquals(2036L, Wav.u32(h, 4))
        }
    }

    @Test fun streamWriter24BitRoundTrips() {
        val bo = ByteArrayOutputStream()
        val x = floatArrayOf(0f, 0.5f, -0.5f, 1f, -1f, 0.25f)
        val w = WavStreamWriter(bo, 2, 44100, 24, 3)
        w.write(x, 3)
        w.finish()
        val f = File(tmp.root, "24.wav")
        f.writeBytes(bo.toByteArray())
        assertEquals(44L + 18, f.length())
        WavSource(f).use { src ->
            assertEquals(3L, src.frames)
            val y = FloatArray(6)
            src.read(0, 3, y)
            for (i in x.indices) assertEquals(x[i].coerceAtMost(8388607f / 8388608f), y[i], 1e-6f)
        }
    }

    @Test fun sixteenBitSamplesSurviveA24BitExport() {
        for (s in intArrayOf(-32768, -1, 0, 1, 32767, 12345)) {
            val f = s / 32768f
            assertEquals(s, Wav.to16(f))
            assertEquals(s * 256, Wav.to24(f))
        }
    }

    @Test(expected = IllegalStateException::class)
    fun streamWriterRefusesAShortExport() {
        val w = WavStreamWriter(ByteArrayOutputStream(), 1, 44100, 16, 10)
        w.write(FloatArray(5), 5)
        w.finish()
    }

    @Test fun infoSkipsOtherChunks() {
        val f = File(tmp.root, "list.wav")
        val bo = ByteArrayOutputStream()
        val h = Wav.header(1, 48000, 16, 2)
        bo.write(h, 0, 36)                                        // RIFF, fmt
        bo.write("LIST".toByteArray()); bo.write(byteArrayOf(3, 0, 0, 0)); bo.write(byteArrayOf(1, 2, 3, 0))   // odd: padded
        bo.write(h, 36, 8)
        bo.write(le16(-5)); bo.write(le16(9))
        f.writeBytes(bo.toByteArray())
        WavSource(f).use { src ->
            assertEquals(48000, src.info.rate)
            assertEquals(2L, src.frames)
            val y = FloatArray(2); src.read(0, 2, y)
            assertEquals(-5, Wav.to16(y[0])); assertEquals(9, Wav.to16(y[1]))
        }
    }
}
