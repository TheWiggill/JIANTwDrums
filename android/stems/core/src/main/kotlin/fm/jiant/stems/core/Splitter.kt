// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the USB stream (16-bit little-endian frames of every channel, as the FM-1 sends them) split into one
// stream a lane, plus the meters and the latency window. One splitter a take; the recorder's thread alone calls it.
package fm.jiant.stems.core

class StemSplitter(
    val channels: Int,
    val lanes: List<LaneSpec>,
    /** where each lane goes; null: not armed (only metered) */
    private val sinks: List<PcmSink?>,
    /** fed the stems' sum and the dry mic when the stream has them (STEMS+IN) */
    private val probe: LatencyProbe? = null,
) {
    private val frameBytes = channels * 2
    private val carry = ByteArray(frameBytes)
    private var carried = 0
    private val laneBuf = Array(lanes.size) { ByteArray(0) }
    private val peaks = IntArray(channels)
    private val clips = BooleanArray(channels)

    /** frames taken so far */
    @Volatile var frames = 0L
        private set

    init { require(sinks.size == lanes.size) }

    /** [len] bytes of the stream; a frame split across two calls is put back together */
    fun write(b: ByteArray, off: Int, len: Int) {
        var o = off
        var left = len
        if (carried > 0) {
            val k = minOf(frameBytes - carried, left)
            System.arraycopy(b, o, carry, carried, k)
            carried += k; o += k; left -= k
            if (carried < frameBytes) return
            frames(carry, 0, 1)
            carried = 0
        }
        val n = left / frameBytes
        if (n > 0) frames(b, o, n)
        val rest = left - n * frameBytes
        if (rest > 0) {
            System.arraycopy(b, o + n * frameBytes, carry, 0, rest)
            carried = rest
        }
    }

    private fun frames(b: ByteArray, off: Int, n: Int) {
        synchronized(this) {                                           // meters (read by the UI's thread)
            for (c in 0 until channels) {
                var pk = peaks[c]
                var p = off + 2 * c
                for (i in 0 until n) {
                    val v = (b[p].toInt() and 0xFF) or (b[p + 1].toInt() shl 8)
                    val a = if (v < 0) -v else v
                    if (a > pk) pk = a
                    if (a >= 32767) clips[c] = true
                    p += frameBytes
                }
                peaks[c] = pk
            }
        }
        for (li in lanes.indices) {                                    // the lanes' own streams
            val s = sinks[li] ?: continue
            val chs = lanes[li].channels
            val need = n * chs.size * 2
            if (laneBuf[li].size < need) laneBuf[li] = ByteArray(need)
            val d = laneBuf[li]
            var q = 0
            var p = off
            for (i in 0 until n) {
                for (c in chs) { d[q] = b[p + 2 * c]; d[q + 1] = b[p + 2 * c + 1]; q += 2 }
                p += frameBytes
            }
            s.write(d, 0, need)
        }
        if (probe != null && channels >= 10) {
            var p = off
            for (i in 0 until n) {
                var m = 0
                for (c in 0 until 8) m += (b[p + 2 * c].toInt() and 0xFF) or (b[p + 2 * c + 1].toInt() shl 8)
                val raw = (b[p + 18].toInt() and 0xFF) or (b[p + 19].toInt() shl 8)
                probe.feed(m.toFloat(), raw.toFloat())
                p += frameBytes
            }
        }
        frames += n
    }

    /** each channel's peak (0..32768) since the last call, and whether it hit full scale; then both cleared */
    fun takePeaks(out: IntArray, clipped: BooleanArray) {
        synchronized(this) {
            for (c in 0 until channels) {
                out[c] = peaks[c]; peaks[c] = 0
                clipped[c] = clips[c]; clips[c] = false
            }
        }
    }
}
