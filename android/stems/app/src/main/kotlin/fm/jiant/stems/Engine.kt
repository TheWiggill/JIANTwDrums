// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the recorder. One thread pumps the FM-1's stream for as long as the record page is open: always into
// a splitter that only meters, and during a take into one that also writes each armed lane's WAV (the headers
// patched every two seconds, so a take cut short keeps what it had). The latency measure is a third kind of
// splitter: six seconds into a LatencyProbe. Swaps happen on the thread itself, between two pumps, so nothing is
// written while it changes. The mic sender is a second thread: the phone's mic into usbstream.c's ring.
package fm.jiant.stems

import android.annotation.SuppressLint
import android.media.AudioDeviceInfo
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioRecord
import android.media.MediaRecorder
import android.os.Handler
import android.os.Looper
import fm.jiant.stems.core.Layout
import fm.jiant.stems.core.LaneKind
import fm.jiant.stems.core.LaneSpec
import fm.jiant.stems.core.LatencyProbe
import fm.jiant.stems.core.Mix
import fm.jiant.stems.core.Session
import fm.jiant.stems.core.StemSplitter
import fm.jiant.stems.core.WavFileWriter
import java.io.File
import java.io.IOException
import kotlin.math.abs
import kotlin.math.roundToInt

class Engine(val stream: Fm1Stream, private val events: Events) {
    interface Events {
        /** the stream broke (the FM-1 unplugged): -errno */
        fun onStreamError(err: Int)
        fun onTakeDone(s: Session, probe: LatencyProbe?)
        fun onTakeError(why: String)
        fun onMeasureDone(probe: LatencyProbe)
    }

    val channels = stream.channels
    val lanes: List<LaneSpec> = Layout.lanes(channels)
    private val ui = Handler(Looper.getMainLooper())

    private class Take(val session: Session, val writers: List<WavFileWriter?>, val splitter: StemSplitter, val probe: LatencyProbe?)

    private val meter = StemSplitter(channels, lanes, lanes.map { null })
    @Volatile var current: StemSplitter = meter
        private set
    @Volatile private var pendingTake: Take? = null
    @Volatile private var stopTake = false
    @Volatile private var pendingMeasure: StemSplitter? = null
    @Volatile private var measuring: Pair<StemSplitter, LatencyProbe>? = null
    @Volatile private var running = true
    @Volatile var recording = false
        private set
    @Volatile var stats = IntArray(10)
        private set

    private val thread = Thread({ loop() }, "fm1-usb").apply { priority = Thread.MAX_PRIORITY; start() }

    private fun loop() {
        val buf = ByteArray(1 shl 16)
        val st = IntArray(10)
        var take: Take? = null
        var lastSync = System.nanoTime()
        try {
            while (running) {
                pendingTake?.let { t -> pendingTake = null; take = t; current = t.splitter; recording = true; lastSync = System.nanoTime() }
                pendingMeasure?.let { m -> pendingMeasure = null; current = m }
                if (stopTake) {
                    stopTake = false
                    take?.let { finish(it) }
                    take = null
                    recording = false
                    current = measuringSplitter() ?: meter
                }
                val n = stream.pump(buf, 20, st)
                stats = st.clone()
                if (n < 0) {
                    take?.let { finish(it) }
                    take = null
                    recording = false
                    val e = n
                    ui.post { events.onStreamError(e) }
                    break
                }
                if (n > 0) {
                    try {
                        current.write(buf, 0, n)
                    } catch (e: IOException) {
                        take?.let { finish(it) }
                        take = null
                        recording = false
                        current = meter
                        val why = e.message ?: e.toString()
                        ui.post { events.onTakeError(why) }
                    }
                }
                val m = measuring
                if (m != null && current === m.first && m.first.frames >= MEASURE_FRAMES) {
                    measuring = null
                    current = meter
                    ui.post { events.onMeasureDone(m.second) }
                }
                val t = take
                if (t != null && System.nanoTime() - lastSync > 2_000_000_000L) {
                    lastSync = System.nanoTime()
                    try { for (w in t.writers) w?.sync() } catch (_: IOException) { }
                }
            }
        } finally {
            take?.let { finish(it) }
        }
    }

    private fun measuringSplitter(): StemSplitter? = measuring?.first

    /** closes a take's files and saves its session; the result to the page */
    private fun finish(t: Take) {
        var err: String? = null
        for (w in t.writers) try { w?.close() } catch (e: IOException) { err = e.message ?: e.toString() }
        val s = t.session
        s.frames = t.splitter.frames
        s.trimStart = 0
        s.trimEnd = s.frames
        s.complete = true
        try { s.save() } catch (e: IOException) { err = e.message ?: e.toString() }
        if (Session.recording?.absoluteFile == s.dir.absoluteFile) Session.recording = null   // (1.1: listed from now on)
        val why = err
        ui.post { if (why != null) events.onTakeError(why) else events.onTakeDone(s, t.probe) }
    }

    /** starts a take of the [armed] lanes into [dir]; the files exist when this returns (or it throws) */
    fun startTake(dir: File, name: String, armed: Set<String>) {
        check(!recording && pendingTake == null && measuring == null)
        dir.mkdirs()
        val s = Session(dir)
        s.name = name
        s.deviceChannels = channels
        s.complete = false
        Session.recording = dir                                 // (1.1: not recovered or listed while it grows)
        val writers = ArrayList<WavFileWriter?>()
        try {
            for (spec in lanes) {
                if (spec.id !in armed) { writers.add(null); continue }
                s.lanes.add(Session.defaultLane(spec))
                writers.add(WavFileWriter(File(dir, spec.id + ".wav"), spec.channels.size, Layout.RATE))
            }
            s.save()
        } catch (e: IOException) {
            for (w in writers) try { w?.close() } catch (_: IOException) { }
            Session.recording = null
            throw e
        }
        val probe = if (channels >= 10) LatencyProbe() else null
        pendingTake = Take(s, writers, StemSplitter(channels, lanes, writers, probe), probe)
        recording = true
    }

    fun stopTake() { if (recording) stopTake = true }

    /** six seconds of the stems and the dry mic into a probe (STEMS+IN); [Events.onMeasureDone] after */
    fun startMeasure(): Boolean {
        if (recording || measuring != null || channels < 10) return false
        val p = LatencyProbe(window = MEASURE_FRAMES.toInt() + 1)
        val sp = StemSplitter(channels, lanes, lanes.map { null }, p)
        measuring = sp to p
        pendingMeasure = sp
        return true
    }

    val isMeasuring get() = measuring != null

    fun close() {
        running = false
        try { thread.join(3000) } catch (_: InterruptedException) { }
        stream.close()
    }

    companion object {
        const val MEASURE_FRAMES = Layout.RATE * 6L

        /** the lanes the record page arms by default: everything, the voice lanes only with the mic on */
        fun defaultArmed(lanes: List<LaneSpec>, mic: Boolean): MutableSet<String> =
            lanes.filter { mic || (it.kind != LaneKind.VOICE && it.kind != LaneKind.DRY) }.map { it.id }.toMutableSet()
    }
}

/** the phone's mic into the FM-1 (STEMS+IN): 44.1 kHz mono, the gain applied, pushed to usbstream.c's ring */
class MicSender(private val am: AudioManager) {
    @Volatile var gainDb = 0f
    /** the last block's peak (0..1), for the page's meter */
    @Volatile var level = 0f
    @Volatile private var running = false
    private var thread: Thread? = null
    private var rec: AudioRecord? = null
    val on get() = running

    @SuppressLint("MissingPermission")
    private fun open(src: Int): AudioRecord? = try {
        val r = AudioRecord.Builder().setAudioSource(src)
            .setAudioFormat(AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(Layout.RATE)
                                .setChannelMask(AudioFormat.CHANNEL_IN_MONO).build())
            .setBufferSizeInBytes(Layout.RATE / 10 * 2).build()
        if (r.state != AudioRecord.STATE_INITIALIZED) { r.release(); null } else r
    } catch (_: Throwable) { null }

    /** starts it (the RECORD_AUDIO permission granted); false when the mic would not open */
    fun start(): Boolean {
        if (running) return true
        // UNPROCESSED when the phone has it (no noise suppression, no AGC), else the plain mic: as the probe's test 7
        val unprocessed = "true" == am.getProperty(AudioManager.PROPERTY_SUPPORT_AUDIO_SOURCE_UNPROCESSED)
        val r = (if (unprocessed) open(MediaRecorder.AudioSource.UNPROCESSED) else null) ?: open(MediaRecorder.AudioSource.MIC) ?: return false
        var mic: AudioDeviceInfo? = null
        for (d in am.getDevices(AudioManager.GET_DEVICES_INPUTS)) {
            if (d.type == AudioDeviceInfo.TYPE_WIRED_HEADSET) { mic = d; break }     // a headset's mic first
            if (d.type == AudioDeviceInfo.TYPE_BUILTIN_MIC && mic == null) mic = d
        }
        if (mic != null) r.setPreferredDevice(mic)
        rec = r
        running = true
        thread = Thread({
            val b = ShortArray(128)
            try {
                r.startRecording()
                while (running) {
                    val k = r.read(b, 0, b.size)
                    if (k <= 0) break
                    val g = Mix.gain(gainDb)
                    var pk = 0
                    for (i in 0 until k) {
                        val v = if (g == 1f) b[i].toInt() else (b[i] * g).roundToInt().coerceIn(-32768, 32767)
                        b[i] = v.toShort()
                        val a = abs(v)
                        if (a > pk) pk = a
                    }
                    level = maxOf(pk / 32768f, level * 0.9f)
                    UsbNative.pushMic(b, k)
                }
            } catch (_: Throwable) {
            } finally {
                try { r.stop() } catch (_: Throwable) { }
            }
        }, "mic").apply { priority = Thread.MAX_PRIORITY; start() }
        return true
    }

    fun stop() {
        running = false
        try { thread?.join(1000) } catch (_: InterruptedException) { }
        thread = null
        rec?.release()
        rec = null
        level = 0f
    }
}
