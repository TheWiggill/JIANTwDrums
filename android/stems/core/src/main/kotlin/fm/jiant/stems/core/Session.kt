// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: a take on disk. A folder with one WAV a lane, exactly as recorded, and session.properties: the name,
// the length, the trim, the measured latency and each lane's gain, pan, mute, solo and offset. Editing never
// touches the WAVs: playback and the exports apply the session to them.
package fm.jiant.stems.core

import java.io.File
import java.io.IOException
import java.util.Locale
import java.util.Properties

class Lane(
    val id: String,
    val kind: LaneKind,
    /** the WAV's name in the session's folder */
    val file: String,
    val channels: Int,
) {
    @Volatile var gainDb = 0f
    /** -1 left .. 1 right */
    @Volatile var pan = 0f
    @Volatile var mute = false
    @Volatile var solo = false
    /** where the file's first frame sits on the take's timeline: negative plays it earlier (the latency) */
    @Volatile var offset = 0L
}

class Session(val dir: File) {
    var name = dir.name
    var rate = Layout.RATE
    var created = System.currentTimeMillis()
    /** the device's channel count (8, 10, 2) */
    var deviceChannels = 0
    /** the take's length (frames), the longest lane */
    var frames = 0L
    @Volatile var trimStart = 0L
    @Volatile var trimEnd = 0L
    /** the round trip (frames) the voice lanes were moved earlier by, how it was found and how clearly */
    var latency = 0
    var latencySource = LATENCY_NONE
    var latencyConfidence = 0.0
    /** false while the take is being recorded (a session left so was cut short: [recover] it) */
    var complete = true
    val lanes = ArrayList<Lane>()

    val length get() = trimEnd - trimStart

    fun lane(id: String) = lanes.firstOrNull { it.id == id }
    fun file(l: Lane) = File(dir, l.file)

    /** the lanes the mix plays: not muted, and the soloed ones when any is */
    fun audible(l: Lane): Boolean {
        if (l.mute) return false
        val anySolo = lanes.any { it.solo }
        return !anySolo || l.solo
    }

    /** moves the voice lanes (and the FX return too if [fx]) earlier by [frames]: the latency compensation */
    fun applyLatency(frames: Int, source: String, confidence: Double, fx: Boolean = false) {
        latency = frames
        latencySource = source
        latencyConfidence = confidence
        for (l in lanes)
            if (l.kind == LaneKind.VOICE || l.kind == LaneKind.DRY || (fx && l.kind == LaneKind.FX)) l.offset = -frames.toLong()
    }

    /** a take cut short (the app killed while recording): each WAV's header fixed from its length, the take's
     * length from the longest, the trim reset. True when anything was recovered */
    fun recover(): Boolean {
        var n = 0L
        for (l in lanes) {
            val f = file(l)
            if (!f.isFile) continue
            try { n = maxOf(n, Wav.repair(f).frames) } catch (_: IOException) { }
        }
        frames = n
        trimStart = 0
        trimEnd = n
        complete = true
        save()
        return n > 0
    }

    fun clampTrim() {
        trimStart = trimStart.coerceIn(0L, frames)
        trimEnd = trimEnd.coerceIn(trimStart, frames)
    }

    fun save() {
        val p = Properties()
        p["name"] = name
        p["rate"] = rate.toString()
        p["created"] = created.toString()
        p["deviceChannels"] = deviceChannels.toString()
        p["frames"] = frames.toString()
        p["trimStart"] = trimStart.toString()
        p["trimEnd"] = trimEnd.toString()
        p["latency"] = latency.toString()
        p["latencySource"] = latencySource
        p["latencyConfidence"] = String.format(Locale.ROOT, "%.3f", latencyConfidence)
        p["complete"] = complete.toString()
        p["lanes"] = lanes.size.toString()
        lanes.forEachIndexed { i, l ->
            val k = "lane.$i."
            p[k + "id"] = l.id
            p[k + "kind"] = l.kind.name
            p[k + "file"] = l.file
            p[k + "channels"] = l.channels.toString()
            p[k + "gainDb"] = String.format(Locale.ROOT, "%.2f", l.gainDb)
            p[k + "pan"] = String.format(Locale.ROOT, "%.3f", l.pan)
            p[k + "mute"] = l.mute.toString()
            p[k + "solo"] = l.solo.toString()
            p[k + "offset"] = l.offset.toString()
        }
        val tmp = File(dir, "$FILE.tmp")
        tmp.outputStream().use { p.store(it, "JIANT Stems session") }
        if (!tmp.renameTo(File(dir, FILE))) {
            File(dir, FILE).delete()
            if (!tmp.renameTo(File(dir, FILE))) throw IOException("could not save the session")
        }
    }

    companion object {
        const val FILE = "session.properties"
        const val LATENCY_NONE = "none"
        const val LATENCY_TAKE = "take"          // measured in the take itself
        const val LATENCY_CALIBRATED = "calibrated"
        const val LATENCY_MANUAL = "manual"

        fun exists(dir: File) = File(dir, FILE).isFile

        /** (1.1) the take being recorded now (the recorder sets it): never recovered nor listed while it grows. 1.0
         * listed every take, recovering the ones not complete, and a take being recorded is not complete: its WAVs were
         * cut to their headers' last patch, under the recorder still writing */
        @Volatile var recording: File? = null

        /** a take on disk for the list: loaded, and recovered if the app was killed while recording it; null for the
         * take being recorded (or none) */
        fun forList(dir: File): Session? {
            if (!exists(dir) || dir.absoluteFile == recording?.absoluteFile) return null
            val s = load(dir)
            if (!s.complete) s.recover()
            return s
        }

        fun load(dir: File): Session {
            val p = Properties()
            File(dir, FILE).inputStream().use { p.load(it) }
            val s = Session(dir)
            s.name = p.getProperty("name", dir.name)
            s.rate = p.getProperty("rate", "44100").toInt()
            s.created = p.getProperty("created", "0").toLong()
            s.deviceChannels = p.getProperty("deviceChannels", "0").toInt()
            s.frames = p.getProperty("frames", "0").toLong()
            s.trimStart = p.getProperty("trimStart", "0").toLong()
            s.trimEnd = p.getProperty("trimEnd", s.frames.toString()).toLong()
            s.latency = p.getProperty("latency", "0").toInt()
            s.latencySource = p.getProperty("latencySource", LATENCY_NONE)
            s.latencyConfidence = p.getProperty("latencyConfidence", "0").toDouble()
            s.complete = p.getProperty("complete", "true").toBoolean()
            val n = p.getProperty("lanes", "0").toInt()
            for (i in 0 until n) {
                val k = "lane.$i."
                val l = Lane(p.getProperty(k + "id"), LaneKind.valueOf(p.getProperty(k + "kind", "OTHER")),
                             p.getProperty(k + "file"), p.getProperty(k + "channels", "1").toInt())
                l.gainDb = p.getProperty(k + "gainDb", "0").toFloat()
                l.pan = p.getProperty(k + "pan", "0").toFloat()
                l.mute = p.getProperty(k + "mute", "false").toBoolean()
                l.solo = p.getProperty(k + "solo", "false").toBoolean()
                l.offset = p.getProperty(k + "offset", "0").toLong()
                s.lanes.add(l)
            }
            s.clampTrim()
            return s
        }

        /** a lane's defaults: the dry mic is the latency's reference, muted in the mix (VOX is the voice as the
         * FM-1 mixed it) */
        fun defaultLane(spec: LaneSpec): Lane {
            val l = Lane(spec.id, spec.kind, spec.id + ".wav", spec.channels.size)
            if (spec.kind == LaneKind.DRY) l.mute = true
            return l
        }

        /** a file-safe version of a session's name, for the exports */
        fun safeName(name: String): String {
            val s = name.trim().replace(Regex("[^A-Za-z0-9._ -]"), "_").replace(' ', '_').replace(Regex("_+"), "_").trim('_', '.')
            return if (s.isEmpty()) "take" else s.take(60)
        }
    }
}
