// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the round trip's latency, measured as JIANT Probe 0.4 (test 7) does. In STEMS+IN the phone's mic
// goes out to the FM-1 and comes back in channel 10 (DRY, the mic as it came); with the phone near the FM-1's
// speaker the mic hears the stems, so DRY is the stems' sum delayed by the whole loop (the FM-1's output, the air,
// Android's input, the USB ring, the FM-1's input). The lag where DRY best follows the stems' sum is that delay;
// the take's VOX and DRY are then played that much earlier and land where the singer heard the music.
package fm.jiant.stems.core

import kotlin.math.abs

class LatencyResult(val frames: Int, val confidence: Double) {
    /** clear enough to trust (the probe's threshold) */
    val clear get() = frames > 0 && confidence > 3.0
    fun ms(rate: Int) = frames * 1000.0 / rate
}

object Latency {
    /** the lag (frames, 0..max) at which [b] best follows [a], on the slopes (the onsets), first at a quarter rate
     * then refined at the full rate; and the peak's share of the correlation (> 3: clear) */
    fun lag(a: FloatArray, b: FloatArray, max: Int, na: Int = a.size, nb: Int = b.size): LatencyResult {
        val d = 4
        val n = minOf(na, nb) / d - max / d - 1
        if (n < 1000) return LatencyResult(-1, 0.0)
        val m = n + max / d + 1
        val x = FloatArray(m)
        val y = FloatArray(m)
        for (i in 0 until m) {
            var sa = 0f; var sb = 0f
            for (k in 0 until d) { sa += a[i * d + k]; sb += b[i * d + k] }
            x[i] = sa; y[i] = sb
        }
        for (i in m - 1 downTo 1) { x[i] -= x[i - 1]; y[i] -= y[i - 1] }
        var best = 0
        var bv = -1e30
        var sum = 0.0
        for (l in 0..max / d) {
            var v = 0.0
            for (i in 1 until n) v += x[i].toDouble() * y[i + l]
            sum += abs(v)
            if (v > bv) { bv = v; best = l }
        }
        var fb = best * d
        var fv = -1e30
        for (l in maxOf(0, best * d - d)..best * d + d) {
            var v = 0.0
            var i = d
            while (i + l < nb && i < na - 1 && i < n * d) {
                v += (a[i] - a[i - 1]).toDouble() * (b[i + l] - b[i + l - 1])
                i++
            }
            if (v > fv) { fv = v; fb = l }
        }
        return LatencyResult(fb, if (sum > 0) bv / sum * (max / d + 1) / 8.0 else 0.0)
    }
}

/** keeps the take's few loudest windows of the stems' sum and the dry mic (fed a frame at a time from the
 * recorder's thread, cheap) for [estimate] to measure after the take, off that thread */
class LatencyProbe(val window: Int = Layout.RATE * 10, private val keep: Int = 3) {
    private class Win(val mix: FloatArray, val raw: FloatArray, var n: Int, var score: Double)

    private var cur = Win(FloatArray(window), FloatArray(window), 0, 0.0)
    private val best = ArrayList<Win>()
    private var em = 0.0
    private var er = 0.0
    private var pm = 0f
    private var pr = 0f

    fun feed(mix: Float, raw: Float) {
        val w = cur
        w.mix[w.n] = mix
        w.raw[w.n] = raw
        val dm = mix - pm; val dr = raw - pr
        pm = mix; pr = raw
        em += dm.toDouble() * dm
        er += dr.toDouble() * dr
        if (++w.n == window) close()
    }

    private fun close() {
        val w = cur
        w.score = if (em > 0 && er > 0) Math.log(em) + Math.log(er) else Double.NEGATIVE_INFINITY
        em = 0.0; er = 0.0
        if (best.size < keep) {
            best.add(w)
            cur = Win(FloatArray(window), FloatArray(window), 0, 0.0)
        } else {
            val worst = best.minByOrNull { it.score }!!
            if (w.score > worst.score) {
                best.remove(worst)
                best.add(w)
                cur = worst                              // (its arrays reused)
            }
            cur.n = 0
        }
    }

    /** the clearest of the kept windows (and the one being filled when it holds a few seconds) */
    fun estimate(maxLag: Int = Layout.RATE / 5): LatencyResult {
        val all = ArrayList(best)
        if (cur.n >= Layout.RATE * 2) all.add(cur)
        var r = LatencyResult(-1, 0.0)
        for (w in all) {
            val e = Latency.lag(w.mix, w.raw, maxLag, w.n, w.n)
            if (e.confidence > r.confidence) r = e
        }
        return r
    }
}
