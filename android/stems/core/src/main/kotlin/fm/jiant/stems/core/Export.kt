// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the two exports, both over the trimmed take with the latency compensation applied.
//   stemsZip: one WAV a lane in a ZIP. As recorded (each lane's own channels, 16 bit: the same samples the FM-1 sent,
//             only moved and cut), or with the mix (stereo, gain and pan applied, the muted lanes left out); MIX.wav
//             beside them if asked; a README.txt with what the files are.
//   mixdown:  the stereo mix as playback hears it, 16 or 24 bit, normalized to -1 dBFS if asked (the FM-1's stems
//             come before its master: their sum can pass full scale).
package fm.jiant.stems.core

import java.io.OutputStream
import java.util.Locale
import java.util.zip.Deflater
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream
import kotlin.math.abs
import kotlin.math.log10

class ExportOptions(
    val bits: Int = 24,
    /** stems: gain, pan and mute applied (stereo files) instead of as recorded */
    val stemsWithMix: Boolean = false,
    /** stems: MIX.wav in the ZIP too */
    val includeMix: Boolean = true,
    /** mix: the peak brought to -1 dBFS */
    val normalize: Boolean = true,
)

fun interface Progress {
    /** 0..1; false cancels */
    fun at(f: Float): Boolean
}

class CancelledException : RuntimeException("cancelled")

object Export {
    const val BLOCK = 4096
    private const val TARGET = 0.891251f                      // -1 dBFS

    /** the mix's peak over the trimmed take (0..) */
    fun mixPeak(r: Renderer, progress: Progress? = null): Float {
        val s = r.session
        val buf = FloatArray(BLOCK * 2)
        var pk = 0f
        var t = s.trimStart
        while (t < s.trimEnd) {
            val n = minOf(BLOCK.toLong(), s.trimEnd - t).toInt()
            r.mix(t, n, buf)
            for (i in 0 until n * 2) { val a = abs(buf[i]); if (a > pk) pk = a }
            t += n
            if (progress != null && !progress.at((t - s.trimStart).toFloat() / maxOf(1L, s.length))) throw CancelledException()
        }
        return pk
    }

    /** the stereo mix as a WAV into [out]; returns the gain applied (1 when not normalized) */
    fun mixdown(r: Renderer, out: OutputStream, o: ExportOptions, progress: Progress? = null): Float {
        val s = r.session
        var g = 1f
        if (o.normalize) {
            val pk = mixPeak(r, progress?.let { p -> Progress { p.at(it * 0.5f) } })
            if (pk > 0f) g = TARGET / pk
        }
        val base = if (o.normalize) 0.5f else 0f
        val w = WavStreamWriter(out, 2, s.rate, o.bits, s.length)
        val buf = FloatArray(BLOCK * 2)
        var t = s.trimStart
        while (t < s.trimEnd) {
            val n = minOf(BLOCK.toLong(), s.trimEnd - t).toInt()
            r.mix(t, n, buf)
            if (g != 1f) for (i in 0 until n * 2) buf[i] *= g
            w.write(buf, n)
            t += n
            if (progress != null && !progress.at(base + (1 - base) * (t - s.trimStart).toFloat() / maxOf(1L, s.length)))
                throw CancelledException()
        }
        w.finish()
        return g
    }

    /** the file names the ZIP will hold, in order */
    fun zipNames(s: Session, o: ExportOptions): List<String> {
        val names = ArrayList<String>()
        for (l in s.lanes) if (!o.stemsWithMix || s.audible(l)) names.add(l.id + ".wav")
        if (o.includeMix) names.add("MIX.wav")
        names.add("README.txt")
        return names
    }

    /** the stems (and the mix) as a ZIP into [out] (not closed) */
    fun stemsZip(r: Renderer, out: OutputStream, o: ExportOptions, progress: Progress? = null) {
        val s = r.session
        val zip = ZipOutputStream(out)
        zip.setLevel(Deflater.BEST_SPEED)
        val lanes = s.lanes.filter { !o.stemsWithMix || s.audible(it) }
        val steps = lanes.size + (if (o.includeMix) 1 else 0)
        var done = 0
        fun sub(): Progress? = progress?.let { p -> Progress { p.at((done + it) / maxOf(1, steps)) } }
        val folder = Session.safeName(s.name) + "/"
        for (l in lanes) {
            zip.putNextEntry(ZipEntry(folder + l.id + ".wav"))
            val ch = if (o.stemsWithMix) 2 else l.channels
            val bits = if (o.stemsWithMix) o.bits else 16
            val w = WavStreamWriter(zip, ch, s.rate, bits, s.length)
            val buf = FloatArray(BLOCK * ch)
            var t = s.trimStart
            val p = sub()
            while (t < s.trimEnd) {
                val n = minOf(BLOCK.toLong(), s.trimEnd - t).toInt()
                if (o.stemsWithMix) r.laneStereo(l, t, n, buf) else r.lane(l, t, n, buf, false)
                w.write(buf, n)
                t += n
                if (p != null && !p.at((t - s.trimStart).toFloat() / maxOf(1L, s.length))) throw CancelledException()
            }
            w.finish()
            zip.closeEntry()
            done++
        }
        var g = 1f
        if (o.includeMix) {
            zip.putNextEntry(ZipEntry(folder + "MIX.wav"))
            g = mixdown(r, zip, o, sub())
            zip.closeEntry()
        }
        zip.putNextEntry(ZipEntry(folder + "README.txt"))
        zip.write(readme(s, o, lanes, g).toByteArray(Charsets.UTF_8))
        zip.closeEntry()
        zip.finish()
        zip.flush()
    }

    fun readme(s: Session, o: ExportOptions, lanes: List<Lane>, mixGain: Float): String {
        val b = StringBuilder()
        b.append("JIANT Stems: ").append(s.name).append('\n')
        b.append(String.format(Locale.ROOT, "%d Hz, %.2f s (trimmed from %.2f s at %.2f s)\n", s.rate,
                               s.length / s.rate.toDouble(), s.frames / s.rate.toDouble(), s.trimStart / s.rate.toDouble()))
        b.append(if (o.stemsWithMix) "Stems with the mix: stereo, ${o.bits} bit, each lane's gain and pan applied, muted lanes left out.\n"
                 else "Stems as recorded: each lane's own channels, 16 bit, the FM-1's samples (only moved and trimmed).\n")
        b.append(String.format(Locale.ROOT, "Latency: %d frames (%.1f ms, %s)", s.latency, s.latency * 1000.0 / s.rate, s.latencySource))
        b.append(": the voice lanes are already moved earlier by it.\n\n")
        for (l in lanes) {
            b.append(String.format(Locale.ROOT, "%-4s %-28s gain %+.1f dB  pan %+.2f%s%s%s\n", l.id, describe(l.kind),
                                   l.gainDb, l.pan, if (l.offset != 0L) String.format(Locale.ROOT, "  moved %+.1f ms", l.offset * 1000.0 / s.rate) else "",
                                   if (l.mute) "  (muted)" else "", if (l.solo) "  (solo)" else ""))
        }
        if (o.includeMix)
            b.append(String.format(Locale.ROOT, "MIX  the stereo mix, %d bit%s\n", o.bits,
                                   if (o.normalize && mixGain > 0f) String.format(Locale.ROOT, ", normalized (%+.1f dB)", 20 * log10(mixGain.toDouble())) else ""))
        b.append("\nThe sum of T1..T6, FX and VOX at their recorded level and centre pan is the mix as the FM-1's buses hear it\n")
        b.append("(before its master: no leveler, limiter or MASTER knob).\n")
        return b.toString()
    }

    private fun describe(k: LaneKind) = when (k) {
        LaneKind.TRACK -> "track (mono, before the master)"
        LaneKind.FX -> "effects return (stereo)"
        LaneKind.VOICE -> "voice as the track mixed it"
        LaneKind.DRY -> "voice as it came (dry)"
        LaneKind.MIX -> "the FM-1's stereo master"
        LaneKind.OTHER -> "channel"
    }
}
