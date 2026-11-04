// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: playback of a session's mix (the editor's PLAY), float stereo through an AudioTrack, rendered a block
// at a time by the same Renderer the exports use, so mute, solo, gain, pan and the latency shift are heard as they
// change. It plays to the phone (headphones, Bluetooth, the speaker), not to the FM-1: in STEMS+IN the FM-1 is a
// USB output too, and what reached it would come back into its last track.
package fm.jiant.stems

import android.media.AudioAttributes
import android.media.AudioDeviceInfo
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import fm.jiant.stems.core.FrameSource
import fm.jiant.stems.core.Renderer
import fm.jiant.stems.core.Session
import fm.jiant.stems.core.WavSource

class Player(private val am: AudioManager, private val session: Session) {
    @Volatile private var running = false
    private var thread: Thread? = null
    @Volatile private var startFrame = 0L
    @Volatile private var track: AudioTrack? = null
    @Volatile private var endedAt = -1L
    var onEnd: (() -> Unit)? = null
    val playing get() = running

    /** where playback is on the take's timeline (frames) */
    val position: Long get() {
        val t = track ?: return if (endedAt >= 0) endedAt else startFrame
        val head = try { t.playbackHeadPosition.toLong() and 0xFFFFFFFFL } catch (_: Throwable) { 0L }   // (released meanwhile)
        return (startFrame + head).coerceAtMost(session.trimEnd)
    }

    private fun output(): AudioDeviceInfo? {
        val outs = am.getDevices(AudioManager.GET_DEVICES_OUTPUTS)
        val order = intArrayOf(AudioDeviceInfo.TYPE_WIRED_HEADPHONES, AudioDeviceInfo.TYPE_WIRED_HEADSET, AudioDeviceInfo.TYPE_USB_HEADSET,
                               AudioDeviceInfo.TYPE_BLUETOOTH_A2DP, AudioDeviceInfo.TYPE_BUILTIN_SPEAKER)
        for (t in order) outs.firstOrNull { it.type == t }?.let { return it }
        return null
    }

    fun play(from: Long, onEnd: () -> Unit) {
        stop()
        this.onEnd = onEnd
        startFrame = from.coerceIn(session.trimStart, session.trimEnd)
        endedAt = -1
        val rate = session.rate
        val min = AudioTrack.getMinBufferSize(rate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_FLOAT)
        val t = AudioTrack.Builder()
            .setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
            .setAudioFormat(AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_FLOAT).setSampleRate(rate)
                                .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build())
            .setBufferSizeInBytes(maxOf(min * 2, BLOCK * 8 * 2))
            .setTransferMode(AudioTrack.MODE_STREAM)
            .build()
        output()?.let { t.setPreferredDevice(it) }
        track = t
        running = true
        thread = Thread({ run(t) }, "player").apply { priority = Thread.MAX_PRIORITY; start() }
    }

    private fun run(t: AudioTrack) {
        val sources = HashMap<String, FrameSource>()
        val closers = ArrayList<WavSource>()
        try {
            for (l in session.lanes) {
                val f = session.file(l)
                if (f.isFile) { val w = WavSource(f); closers.add(w); sources[l.id] = w }
            }
            val r = Renderer(session, sources)
            val buf = FloatArray(BLOCK * 2)
            var pos = startFrame
            t.play()
            while (running && pos < session.trimEnd) {
                val n = minOf(BLOCK.toLong(), session.trimEnd - pos).toInt()
                r.mix(pos, n, buf)
                for (i in 0 until n * 2) buf[i] = buf[i].coerceIn(-1f, 1f)
                var o = 0
                while (o < n * 2 && running) {
                    val k = t.write(buf, o, n * 2 - o, AudioTrack.WRITE_BLOCKING)
                    if (k < 0) { running = false; break }
                    o += k
                }
                pos += n
            }
            if (running) {                                     // the end: let the buffer play out
                val end = pos - startFrame
                val until = System.currentTimeMillis() + 2000
                while (running && (t.playbackHeadPosition.toLong() and 0xFFFFFFFFL) < end && System.currentTimeMillis() < until) Thread.sleep(10)
            }
        } catch (_: Throwable) {
        } finally {
            closers.forEach { try { it.close() } catch (_: Throwable) { } }
            val natural = running
            endedAt = position
            track = null
            try { t.pause(); t.flush(); t.stop() } catch (_: Throwable) { }
            t.release()
            running = false
            if (natural) onEnd?.let { cb -> android.os.Handler(android.os.Looper.getMainLooper()).post { cb() } }
        }
    }

    fun stop() {
        running = false
        try { thread?.join(1500) } catch (_: InterruptedException) { }
        thread = null
    }

    companion object { const val BLOCK = 2048 }
}
