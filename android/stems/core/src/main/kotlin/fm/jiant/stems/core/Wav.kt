// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: WAV files. PCM, 16 or 24 bit, any channel count, little-endian; a 44-byte header (RIFF, fmt, data).
// Three ways in and out: WavFileWriter records a take (the sizes patched as it grows, so a crash leaves a file that
// plays to the last patch, and repair() fixes it fully); WavStreamWriter writes an export of known length straight
// into a stream (a ZIP entry, a document); WavSource reads any frame range back as floats.
package fm.jiant.stems.core

import java.io.Closeable
import java.io.File
import java.io.IOException
import java.io.OutputStream
import java.io.RandomAccessFile
import kotlin.math.roundToInt

object Wav {
    const val HEADER = 44

    /** the 44-byte header of a PCM WAV of [frames] frames */
    fun header(channels: Int, rate: Int, bits: Int, frames: Long): ByteArray {
        require(bits == 16 || bits == 24) { "bits $bits" }
        val bpf = channels * bits / 8
        val data = frames * bpf
        require(data + 36 <= 0xFFFFFFFFL) { "too long for a WAV" }
        val h = ByteArray(HEADER)
        ascii(h, 0, "RIFF"); le32(h, 4, (36 + data).toInt()); ascii(h, 8, "WAVE")
        ascii(h, 12, "fmt "); le32(h, 16, 16); le16(h, 20, 1); le16(h, 22, channels)
        le32(h, 24, rate); le32(h, 28, rate * bpf); le16(h, 32, bpf); le16(h, 34, bits)
        ascii(h, 36, "data"); le32(h, 40, data.toInt())
        return h
    }

    internal fun ascii(b: ByteArray, o: Int, s: String) { for (i in s.indices) b[o + i] = s[i].code.toByte() }
    internal fun le16(b: ByteArray, o: Int, v: Int) { b[o] = v.toByte(); b[o + 1] = (v shr 8).toByte() }
    internal fun le32(b: ByteArray, o: Int, v: Int) { le16(b, o, v); le16(b, o + 2, v shr 16) }
    internal fun u16(b: ByteArray, o: Int) = (b[o].toInt() and 0xFF) or ((b[o + 1].toInt() and 0xFF) shl 8)
    internal fun u32(b: ByteArray, o: Int) = u16(b, o).toLong() or (u16(b, o + 2).toLong() shl 16)

    /** a float sample (-1..1) as a 16-bit integer: x 32768, so a 16-bit sample read back comes out bit-exact */
    fun to16(x: Float): Int = (x * 32768f).roundToInt().coerceIn(-32768, 32767)
    fun to24(x: Float): Int = (x * 8388608f).roundToInt().coerceIn(-8388608, 8388607)

    /** reads a WAV's format and where its samples are; a data size of 0 or past the end (a take cut short) is taken
     * from the file's length */
    fun info(raf: RandomAccessFile): WavInfo {
        val len = raf.length()
        val h = ByteArray(12)
        raf.seek(0)
        raf.readFully(h)
        if (String(h, 0, 4, Charsets.US_ASCII) != "RIFF" || String(h, 8, 4, Charsets.US_ASCII) != "WAVE")
            throw IOException("not a WAV")
        var pos = 12L
        var ch = 0; var rate = 0; var bits = 0
        val c = ByteArray(8)
        while (pos + 8 <= len) {
            raf.seek(pos)
            raf.readFully(c)
            val id = String(c, 0, 4, Charsets.US_ASCII)
            val size = u32(c, 4)
            if (id == "fmt ") {
                val f = ByteArray(16)
                raf.readFully(f)
                val tag = u16(f, 0)
                if (tag != 1 && tag != 0xFFFE) throw IOException("not PCM ($tag)")
                ch = u16(f, 2); rate = u32(f, 4).toInt(); bits = u16(f, 14)
            } else if (id == "data") {
                if (ch == 0) throw IOException("data before fmt")
                if (bits != 16 && bits != 24) throw IOException("$bits bit")
                val bpf = ch * bits / 8
                val avail = len - (pos + 8)
                val bytes = if (size == 0L || size > avail) avail else size
                return WavInfo(ch, rate, bits, pos + 8, bytes / bpf)
            }
            pos += 8 + size + (size and 1)
        }
        throw IOException("no data")
    }

    /** a take left unfinished (the app killed while recording): its header's sizes set from the file's length. (1.1)
     * From the length, not the header: a header patched before the last writes said less than the file holds, and
     * 1.0 cut the file to it (the take's end lost); only a frame cut short at the end goes */
    fun repair(file: File): WavInfo = RandomAccessFile(file, "rw").use { raf ->
        val i0 = info(raf)
        if (i0.dataOffset != HEADER.toLong()) return@use i0
        val i = WavInfo(i0.channels, i0.rate, i0.bits, i0.dataOffset, (raf.length() - HEADER) / i0.bytesPerFrame)
        raf.seek(0)
        raf.write(header(i.channels, i.rate, i.bits, i.frames), 0, HEADER)
        raf.setLength(HEADER + i.frames * i.bytesPerFrame)
        i
    }
}

class WavInfo(val channels: Int, val rate: Int, val bits: Int, val dataOffset: Long, val frames: Long) {
    val bytesPerFrame get() = channels * bits / 8
}

/** where the splitter puts one lane's samples: 16-bit little-endian, the lane's channels interleaved */
interface PcmSink {
    fun write(b: ByteArray, off: Int, len: Int)
}

/** a take being recorded: 16-bit, buffered, the header's sizes patched by [sync] (the recorder calls it every couple
 * of seconds) and by [close] */
class WavFileWriter(val file: File, val channels: Int, val rate: Int) : PcmSink, Closeable {
    private val raf = RandomAccessFile(file, "rw")
    private val buf = ByteArray(1 shl 16)
    private var n = 0
    var bytes = 0L
        private set
    val frames get() = bytes / (channels * 2)

    init {
        raf.setLength(0)
        raf.write(Wav.header(channels, rate, 16, 0))
    }

    override fun write(b: ByteArray, off: Int, len: Int) {
        var o = off
        var left = len
        while (left > 0) {
            val k = minOf(left, buf.size - n)
            System.arraycopy(b, o, buf, n, k)
            n += k; o += k; left -= k
            if (n == buf.size) flush()
        }
        bytes += len
    }

    private fun flush() {
        if (n > 0) raf.write(buf, 0, n)
        n = 0
    }

    /** the buffer to the file and the sizes into the header */
    fun sync() {
        flush()
        val pos = raf.filePointer
        val h = Wav.header(channels, rate, 16, frames)
        raf.seek(4); raf.write(h, 4, 4)
        raf.seek(40); raf.write(h, 40, 4)
        raf.seek(pos)
    }

    override fun close() {
        try { sync() } finally { raf.close() }
    }
}

/** an export of [frames] frames written front to back into [out]: the header first (the length is known), then
 * the samples as floats, converted to 16 or 24 bit. Writing more or fewer frames than promised is an error */
class WavStreamWriter(private val out: OutputStream, val channels: Int, val rate: Int, val bits: Int, val frames: Long) {
    private var written = 0L
    private val bps = bits / 8
    private var buf = ByteArray(0)

    init { out.write(Wav.header(channels, rate, bits, frames)) }

    /** [n] frames of interleaved floats ([channels] a frame) */
    fun write(x: FloatArray, n: Int) {
        require(written + n <= frames) { "more frames than the header says" }
        val need = n * channels * bps
        if (buf.size < need) buf = ByteArray(need)
        var o = 0
        for (i in 0 until n * channels) {
            if (bits == 16) {
                val v = Wav.to16(x[i])
                buf[o] = v.toByte(); buf[o + 1] = (v shr 8).toByte()
            } else {
                val v = Wav.to24(x[i])
                buf[o] = v.toByte(); buf[o + 1] = (v shr 8).toByte(); buf[o + 2] = (v shr 16).toByte()
            }
            o += bps
        }
        out.write(buf, 0, need)
        written += n
    }

    fun finish() {
        check(written == frames) { "wrote $written of $frames frames" }
        out.flush()
    }
}

/** frames from somewhere (a WAV file; a test's array), as floats -1..1 */
interface FrameSource {
    val channels: Int
    val frames: Long
    /** [n] frames from [frame] into [dst] interleaved; frames before 0 or past the end are silence */
    fun read(frame: Long, n: Int, dst: FloatArray)
}

/** a WAV file read back for playback, the editor's waveforms and the exports (one reader a thread) */
class WavSource(file: File) : FrameSource, Closeable {
    private val raf = RandomAccessFile(file, "r")
    val info = Wav.info(raf)
    override val channels get() = info.channels
    override val frames get() = info.frames
    private var buf = ByteArray(0)

    override fun read(frame: Long, n: Int, dst: FloatArray) {
        val ch = info.channels
        java.util.Arrays.fill(dst, 0, n * ch, 0f)
        val a = maxOf(frame, 0L)
        val b = minOf(frame + n, info.frames)
        if (b <= a) return
        val k = (b - a).toInt()
        val bpf = info.bytesPerFrame
        if (buf.size < k * bpf) buf = ByteArray(k * bpf)
        raf.seek(info.dataOffset + a * bpf)
        raf.readFully(buf, 0, k * bpf)
        var o = ((a - frame) * ch).toInt()
        if (info.bits == 16) {
            for (i in 0 until k * ch) {
                val v = (buf[2 * i].toInt() and 0xFF) or (buf[2 * i + 1].toInt() shl 8)
                dst[o++] = v / 32768f
            }
        } else {
            for (i in 0 until k * ch) {
                val v = (buf[3 * i].toInt() and 0xFF) or ((buf[3 * i + 1].toInt() and 0xFF) shl 8) or (buf[3 * i + 2].toInt() shl 16)
                dst[o++] = v / 8388608f
            }
        }
    }

    override fun close() = raf.close()
}

/** a test's (or a measurement's) samples held in memory, interleaved */
class ArraySource(override val channels: Int, private val x: FloatArray) : FrameSource {
    override val frames get() = (x.size / channels).toLong()
    override fun read(frame: Long, n: Int, dst: FloatArray) {
        for (i in 0 until n) {
            val f = frame + i
            for (c in 0 until channels)
                dst[i * channels + c] = if (f in 0 until frames) x[(f * channels + c).toInt()] else 0f
        }
    }
}
