// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the editor's waveforms (each lane's peak a bucket of frames) and the FM-1's heat colours.
package fm.jiant.stems.core

import kotlin.math.abs

object Peaks {
    /** the peak (0..1, every channel) of each [bucket] frames of [src] */
    fun compute(src: FrameSource, bucket: Int = 512, cancelled: () -> Boolean = { false }): FloatArray {
        val nb = ((src.frames + bucket - 1) / bucket).toInt()
        val out = FloatArray(nb)
        val ch = src.channels
        val per = 64                                          // buckets a read
        val buf = FloatArray(bucket * per * ch)
        var b = 0
        while (b < nb && !cancelled()) {
            val k = minOf(per, nb - b)
            src.read(b.toLong() * bucket, k * bucket, buf)
            for (j in 0 until k) {
                var pk = 0f
                val o = j * bucket * ch
                for (i in o until o + bucket * ch) { val a = abs(buf[i]); if (a > pk) pk = a }
                out[b + j] = pk
            }
            b += k
        }
        return out
    }
}

/** the firmware's heat ramp (ui_draw.c heat_col: cyan, blue-violet, violet, red, orange, yellow, white), 0..256 */
object Heat {
    private val H = arrayOf(intArrayOf(0, 208, 255), intArrayOf(90, 60, 255), intArrayOf(181, 48, 230), intArrayOf(247, 40, 49),
                            intArrayOf(255, 121, 0), intArrayOf(255, 212, 0), intArrayOf(255, 255, 255))

    fun argb(v0: Int): Int {
        val v = v0.coerceIn(0, 256)
        val k = minOf(5, v * 6 / 257)
        val m = (v * 6 - k * 256) / 256f
        val p = H[k]; val q = H[k + 1]
        val r = (p[0] + (q[0] - p[0]) * m).toInt().coerceIn(0, 255)
        val g = (p[1] + (q[1] - p[1]) * m).toInt().coerceIn(0, 255)
        val b = (p[2] + (q[2] - p[2]) * m).toInt().coerceIn(0, 255)
        return (0xFF shl 24) or (r shl 16) or (g shl 8) or b
    }
}
