// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the record page. The meters of every channel the FM-1 sends (tap one to arm or disarm its lane), the
// mic sent to the FM-1 (STEMS+IN: MIC on or off, its gain, the round trip's latency and MEASURE), REC and STOP. A
// take stopped goes through the latency: measured in the take when the mic heard the speaker clearly, else the last
// MEASURE, else none; the voice lanes are moved earlier by it and the take opens in EDIT.
package fm.jiant.stems

import android.os.Handler
import android.os.Looper
import android.view.View
import fm.jiant.stems.core.LaneKind
import fm.jiant.stems.core.Layout
import fm.jiant.stems.core.LatencyProbe
import fm.jiant.stems.core.LatencyResult
import fm.jiant.stems.core.Session
import fm.jiant.stems.core.T
import fm.jiant.stems.core.UsbMode
import java.io.IOException
import java.util.Locale

class RecordPage(app: MainActivity, stream: Fm1Stream) : Page(app), Engine.Events {
    private val engine = Engine(stream, this)
    private val ch = engine.channels
    private val stemsIn = stream.mode == UsbMode.STEMS_IN && stream.withOut
    private val mic = MicSender(app.am)
    private var micWanted = stemsIn && app.prefs.getBoolean("micOn", true)
    private val armed: MutableSet<String> = Engine.defaultArmed(engine.lanes, micWanted)
    private val ui = Handler(Looper.getMainLooper())
    private var closed = false
    private var lastTake: Session? = null

    private val header = Header(app, "J", T.REC.s())
    private val clock = app.text("00:00.0", 34f)
    private val state = app.text(T.READY.s(), 13f, Pal.ORANGE)
    private val meters = MetersView(app, ch, { c -> armed.contains(engine.lanes[Layout.laneOf(engine.lanes, c)].id) }) { c -> toggleArm(c) }
    private val hint = app.note()
    private val msg = app.note()
    private val micCard = Card(app, T.MIC.s(), 0f, 1f, 0f) { "" }
    private val gainCard = Card(app, T.MIC_GAIN.s(), 0f, 24f, 0f) { String.format(Locale.ROOT, "+%.0f dB", it) }
    private val latCard = Card(app, T.LATENCY.s(), 0f, 1f, 0f) { "" }
    private val rec = RoundBtn(app, RoundBtn.Kind.REC) { recPressed() }
    private val stop = RoundBtn(app, RoundBtn.Kind.STOP) { engine.stopTake() }
    private val play = RoundBtn(app, RoundBtn.Kind.PLAY) { lastTake?.let { app.push(EditPage(app, it)) } }
    private val footer = Footer(app)
    private val peaks = IntArray(ch)
    private val clips = BooleanArray(ch)

    override val view: View = run {
        val v = app.vbox()
        val pad = app.dpi(16)
        v.setPadding(pad, app.dpi(28), pad, app.dpi(12))
        v.setBackgroundColor(Pal.BLACK)
        header.status.text = HomePage.modeName(ch) + " · " + ch + " CH"
        v.addView(header, lp(MATCH, WRAP))
        val cl = app.hbox()
        cl.setPadding(app.dpi(2), app.dpi(14), app.dpi(2), app.dpi(10))
        cl.addView(clock, lp(0, WRAP, 1f))
        cl.addView(state, lp(WRAP, WRAP))
        v.addView(cl, lp(MATCH, WRAP))
        v.addView(meters, lp(MATCH, 0, 1f))
        hint.text = T.ARM_HINT.s()
        v.addView(hint, lp(MATCH, WRAP).apply { topMargin = app.dpi(10) })
        if (stemsIn) {
            val cards = app.hbox()
            micCard.first = true
            micCard.tap = { toggleMic() }
            gainCard.value = app.prefs.getFloat("micGain", 0f)
            gainCard.onChange = { mic.gainDb = it }
            gainCard.onDone = { app.prefs.edit().putFloat("micGain", it).apply() }
            latCard.tap = { measure() }
            cards.addView(micCard, lp(0, WRAP, 1f))
            cards.addView(gainCard, lp(0, WRAP, 1f))
            cards.addView(latCard, lp(0, WRAP, 1f))
            v.addView(cards, lp(MATCH, WRAP).apply { topMargin = app.dpi(12) })
        } else if (stream.mode == UsbMode.STEMS) {
            msg.text = if (stream.oldStemsFirmware) T.OLD_FIRMWARE.s() else T.MIC_NEEDS_STEMS_IN.s()   // (1.1)
        } else if (stream.mode == UsbMode.STEREO) {
            msg.text = T.MODE_STEREO.s()
        }
        v.addView(msg, lp(MATCH, WRAP).apply { topMargin = app.dpi(8) })
        val tr = app.hbox()
        tr.gravity = android.view.Gravity.CENTER
        tr.setPadding(0, app.dpi(12), 0, app.dpi(10))
        tr.addView(stop, lp(WRAP, WRAP))
        tr.addView(rec, lp(WRAP, WRAP).apply { leftMargin = app.dpi(36); rightMargin = app.dpi(36) })
        tr.addView(play, lp(WRAP, WRAP))
        v.addView(tr, lp(MATCH, WRAP))
        v.addView(footer, lp(MATCH, WRAP))
        v
    }

    init {
        mic.gainDb = app.prefs.getFloat("micGain", 0f)
        if (micWanted) app.askMic { ok -> if (closed) return@askMic; if (ok) startMic() else { micOff(); msg.text = T.MIC_PERMISSION.s() } }
        refreshCards()
        play.alpha = 0.35f
    }

    private var ticks = 0

    private val tick = object : Runnable {
        override fun run() {
            if (closed) return
            if (++ticks % 30 == 0) footerText()
            engine.current.takePeaks(peaks, clips)
            meters.push(peaks, clips)
            val sp = engine.current
            clock.text = mmss(if (engine.recording) sp.frames else 0L, Layout.RATE)
            if (mic.on) micCard.level = mic.level
            ui.postDelayed(this, 33)
        }
    }

    override fun onShow() {
        app.keepScreenOn(true)
        ui.removeCallbacks(tick)
        ui.post(tick)
        footerText()
        val free = app.freeBytes()
        if (free in 0 until 200L * 1024 * 1024) msg.text = T.LOW_SPACE.s(bytesText(free))
    }

    override fun onHide() {
        ui.removeCallbacks(tick)
        if (!engine.recording) app.keepScreenOn(false)
    }

    override fun onClose() {
        closed = true
        ui.removeCallbacks(tick)
        engine.stopTake()
        mic.stop()
        engine.close()
        app.keepScreenOn(false)
    }

    override fun onUsbDetached() {
        if (!Fm1.find(app.um).any { it.dev.deviceName == engine.stream.fm1.dev.deviceName }) {
            msg.text = T.DETACHED.s()
        }
    }

    private fun footerText() {
        val free = app.freeBytes()
        // the stream's health: packets with an error, and (STEMS+IN) mic samples sent as silence or dropped
        val st = engine.stats
        val bad = st[2] + (if (st.size > 9 && st[9] > 0) 1 else 0) + (if (stemsIn && mic.on) st[7] / 441 else 0)
        val usb = if (bad == 0) "USB OK" else "USB $bad ERR"
        footer.set("44.1 kHz · 16 bit", (if (free >= 0) T.FREE.s(bytesText(free)) + " · " else "") + usb, "REC")
    }

    private fun refreshCards() {
        if (!stemsIn) return
        micCard.override = if (mic.on) T.ON.s() else T.OFF.s()
        micCard.hot = mic.on
        if (!mic.on) micCard.level = 0f
        val lat = app.prefs.getInt("latency", 0)
        latCard.override = if (engine.isMeasuring) "…" else if (lat > 0) String.format(Locale.ROOT, "%.1f ms", lat * 1000.0 / Layout.RATE) else T.MEASURE.s()
        latCard.hot = engine.isMeasuring
        meters.invalidate()
    }

    private fun startMic() {
        if (!mic.start()) { micOff(); msg.text = T.MIC_FAILED.s() } else { msg.text = T.MIC_HINT.s() }
        refreshCards()
    }

    /** the voice lanes armed with the mic, disarmed without it (they would be silence) */
    private fun armVoice(on: Boolean) {
        for (l in engine.lanes) if (l.kind == LaneKind.VOICE || l.kind == LaneKind.DRY) { if (on) armed.add(l.id) else armed.remove(l.id) }
        meters.invalidate()
    }

    private fun micOff() {
        mic.stop()
        micWanted = false
        armVoice(false)
        refreshCards()
    }

    private fun toggleMic() {
        if (engine.recording || engine.isMeasuring) return
        if (mic.on) {
            micOff()
            msg.text = ""
        } else {
            micWanted = true
            armVoice(true)
            app.askMic { ok -> if (ok) startMic() else { micOff(); msg.text = T.MIC_PERMISSION.s() } }
        }
        app.prefs.edit().putBoolean("micOn", micWanted).apply()
        refreshCards()
    }

    private fun toggleArm(c: Int) {
        if (engine.recording) return
        val id = engine.lanes[Layout.laneOf(engine.lanes, c)].id
        if (!armed.remove(id)) armed.add(id)
        meters.invalidate()
    }

    private fun recPressed() {
        if (engine.recording) { engine.stopTake(); return }
        if (engine.isMeasuring) return
        if (armed.isEmpty()) { msg.text = T.NOTHING_ARMED.s(); return }
        val n = app.takes().size + 1
        try {
            engine.startTake(app.newTakeDir(), T.TAKE_NAME.s(n), armed.toSet())
        } catch (e: IOException) {
            msg.text = T.DISK_ERROR.s(e.message ?: e.toString())
            return
        }
        app.keepScreenOn(true)
        rec.active = true
        header.dot.visibility = View.VISIBLE
        state.text = T.RECORDING.s()
        state.setTextColor(Pal.RED)
        msg.text = ""
    }

    private fun measure() {
        if (engine.recording || engine.isMeasuring) return
        app.askMic { ok ->
            if (!ok) { msg.text = T.MIC_PERMISSION.s(); return@askMic }
            if (!mic.on && !mic.start()) { msg.text = T.MIC_FAILED.s(); return@askMic }
            if (engine.startMeasure()) {
                msg.text = T.MEASURING.s()
                refreshCards()
            }
        }
    }

    // ---- the engine's events (on the main thread)
    override fun onStreamError(err: Int) {
        msg.text = T.STREAM_ERROR.s(-err)
        state.text = ""
        rec.active = false
        header.dot.visibility = View.GONE
    }

    override fun onTakeError(why: String) {
        msg.text = T.DISK_ERROR.s(why)
        rec.active = false
        header.dot.visibility = View.GONE
        state.text = T.READY.s(); state.setTextColor(Pal.ORANGE)
    }

    override fun onTakeDone(s: Session, probe: LatencyProbe?) {
        rec.active = false
        header.dot.visibility = View.GONE
        state.text = T.SAVING.s()
        footerText()
        val hasVoice = s.lanes.any { it.kind == LaneKind.VOICE || it.kind == LaneKind.DRY }
        Thread {
            // the take's own measure when the mic heard the speaker clearly, else the last MEASURE
            val r: LatencyResult? = if (hasVoice && probe != null) probe.estimate() else null
            val cal = app.prefs.getInt("latency", 0)
            val calConf = app.prefs.getFloat("latencyConf", 0f).toDouble()
            when {
                r != null && r.clear -> s.applyLatency(r.frames, Session.LATENCY_TAKE, r.confidence)
                hasVoice && cal > 0 -> s.applyLatency(cal, Session.LATENCY_CALIBRATED, calConf)
                else -> s.applyLatency(0, Session.LATENCY_NONE, 0.0)
            }
            try { s.save() } catch (_: IOException) { }
            ui.post {
                if (closed) return@post
                lastTake = s
                play.alpha = 1f
                state.text = T.READY.s(); state.setTextColor(Pal.ORANGE)
                if (!engine.recording) app.keepScreenOn(true)
                app.push(EditPage(app, s))
            }
        }.start()
    }

    override fun onMeasureDone(probe: LatencyProbe) {
        Thread {
            val r = probe.estimate()
            ui.post {
                if (closed) return@post
                if (r.clear) {
                    app.prefs.edit().putInt("latency", r.frames).putFloat("latencyConf", r.confidence.toFloat()).apply()
                    msg.text = T.MEASURED.s(r.ms(Layout.RATE), r.confidence)
                } else {
                    msg.text = T.UNCLEAR.s(r.confidence)
                }
                if (!micWanted) mic.stop()
                refreshCards()
            }
        }.start()
    }
}
