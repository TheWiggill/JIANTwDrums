// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the session page. The lanes' waveforms on one timeline, PLAY / STOP of the mix, a lane's M and S, the
// selected lane's GAIN, PAN and SHIFT on the FM-1's cards (drag, double-tap resets), the take's LATENCY (the voice
// lanes' shift, by hand), the trim (the orange edges, or SET IN / SET OUT at the playhead), RENAME, EXPORT. Nothing
// here touches the WAVs: it is all in session.properties, applied by playback and the exports.
package fm.jiant.stems

import android.os.Handler
import android.os.Looper
import android.text.InputType
import android.view.View
import android.widget.EditText
import fm.jiant.stems.core.LaneKind
import fm.jiant.stems.core.Peaks
import fm.jiant.stems.core.Session
import fm.jiant.stems.core.T
import fm.jiant.stems.core.WavSource
import java.util.Locale
import kotlin.math.abs
import kotlin.math.roundToLong

class EditPage(app: MainActivity, private val s: Session) : Page(app), LanesView.Listener {
    private val ui = Handler(Looper.getMainLooper())
    private val player = Player(app.am, s)
    private var closed = false
    private val hasVoice = s.lanes.any { it.kind == LaneKind.VOICE || it.kind == LaneKind.DRY }

    private val header = Header(app, "J", s.name)
    private val gain = Card(app, T.GAIN.s(), -24f, 12f, 0f) { String.format(Locale.ROOT, "%+.1f dB", it) }
    private val pan = Card(app, T.PAN.s(), -1f, 1f, 0f) { panText(it) }
    private val shift = Card(app, T.SHIFT.s(), -200f, 200f, 0f) { String.format(Locale.ROOT, "%+.1f ms", it) }
    private val lat = Card(app, T.LATENCY.s(), 0f, 300f, 0f) { String.format(Locale.ROOT, "%.1f ms", it) }
    private val lanes = LanesView(app, s, this)
    private val time = app.text("00:00.0", 18f)
    private val playBtn = RoundBtn(app, RoundBtn.Kind.PLAY) { togglePlay() }
    private val stopBtn = RoundBtn(app, RoundBtn.Kind.STOP) { stopPlay(true) }
    private val info = app.note()
    private val footer = Footer(app)

    override val view: View = run {
        val v = app.vbox()
        val pad = app.dpi(16)
        v.setPadding(pad, app.dpi(28), pad, app.dpi(12))
        v.setBackgroundColor(Pal.BLACK)
        header.title.setOnClickListener { rename() }
        v.addView(header, lp(MATCH, WRAP))
        val cards = app.hbox()
        gain.first = true
        for (c in listOf(gain, pan, shift, lat)) cards.addView(c, lp(0, WRAP, 1f))
        v.addView(cards, lp(MATCH, WRAP).apply { topMargin = app.dpi(10) })
        v.addView(lanes, lp(MATCH, 0, 1f).apply { topMargin = app.dpi(6) })
        val tools = app.hbox()
        val tin = TextBtn(app, T.TRIM_IN.s()) { s.trimStart = minOf(lanes.playhead, s.trimEnd - s.rate / 10).coerceAtLeast(0); s.clampTrim(); onTrim(); save() }
        val tout = TextBtn(app, T.TRIM_OUT.s()) { s.trimEnd = maxOf(lanes.playhead, s.trimStart + s.rate / 10).coerceAtMost(s.frames); s.clampTrim(); onTrim(); save() }
        val tall = TextBtn(app, T.TRIM_ALL.s()) { s.trimStart = 0; s.trimEnd = s.frames; onTrim(); save() }
        val ren = TextBtn(app, T.RENAME.s()) { rename() }
        for ((i, b) in listOf(tin, tout, tall, ren).withIndex()) tools.addView(b, lp(0, WRAP, 1f).apply { if (i > 0) leftMargin = app.dpi(4) })
        v.addView(tools, lp(MATCH, WRAP).apply { topMargin = app.dpi(8) })
        info.setPadding(0, app.dpi(6), 0, 0)
        v.addView(info, lp(MATCH, WRAP))
        val tr = app.hbox()
        tr.setPadding(0, app.dpi(8), 0, app.dpi(8))
        tr.addView(time, lp(0, WRAP, 1f))
        tr.addView(stopBtn, lp(WRAP, WRAP))
        tr.addView(playBtn, lp(WRAP, WRAP).apply { leftMargin = app.dpi(14) })
        val exp = TextBtn(app, T.EXPORT.s(), primary = true) { stopPlay(false); save(); app.push(ExportPage(app, s)) }
        tr.addView(exp, lp(0, WRAP, 1f).apply { leftMargin = app.dpi(14) })
        v.addView(tr, lp(MATCH, WRAP))
        v.addView(footer, lp(MATCH, WRAP))
        v
    }

    init {
        gain.onChange = { lanes.lane()?.gainDb = it }
        pan.onChange = { lanes.lane()?.pan = if (abs(it) < 0.02f) 0f else it }
        shift.onChange = { v -> lanes.lane()?.let { it.offset = (v / 1000.0 * s.rate).roundToLong(); lanes.invalidate() } }
        lat.onChange = { v ->
            s.applyLatency((v / 1000.0 * s.rate).toInt(), Session.LATENCY_MANUAL, 0.0)
            lanes.invalidate(); select(lanes.selected); infoText()
        }
        for (c in listOf(gain, pan, shift, lat)) c.onDone = { save() }
        if (!hasVoice) { lat.override = "—"; lat.isEnabled = false }
        lanes.playhead = s.trimStart
        select(0)
        loadPeaks()
        infoText()
        onTrim()
    }

    private fun panText(p: Float): String {
        val k = Math.round(abs(p) * 100)
        return if (k == 0) "C" else (if (p < 0) "L" else "R") + k
    }

    private fun loadPeaks() {
        Thread {
            for (l in s.lanes) {
                if (closed) return@Thread
                val f = s.file(l)
                if (!f.isFile) continue
                val p = try { WavSource(f).use { Peaks.compute(it, lanes.bucket) { closed } } } catch (_: Throwable) { null } ?: continue
                ui.post { lanes.peaks[l.id] = p; lanes.invalidate() }
            }
        }.start()
    }

    private fun select(i: Int) {
        val l = s.lanes.getOrNull(i) ?: return
        gain.value = l.gainDb
        pan.value = l.pan
        shift.value = (l.offset * 1000.0 / s.rate).toFloat()
        if (hasVoice) lat.value = (s.latency * 1000.0 / s.rate).toFloat()
        gain.hot = true
        footer.set(kindName(l.kind), l.id + (if (!s.file(l).isFile) "  " + T.LANE_MISSING.s(l.id) else ""), T.EDIT.s())
    }

    private fun kindName(k: LaneKind) = when (k) {
        LaneKind.TRACK -> "TRACK"
        LaneKind.FX -> "FX RETURN"
        LaneKind.VOICE -> "VOICE"
        LaneKind.DRY -> "VOICE DRY"
        LaneKind.MIX -> "MIX"
        LaneKind.OTHER -> "CH"
    }

    private fun infoText() {
        val src = when (s.latencySource) {
            Session.LATENCY_TAKE -> T.LAT_TAKE.s()
            Session.LATENCY_CALIBRATED -> T.LAT_CAL.s()
            Session.LATENCY_MANUAL -> T.LAT_MANUAL.s()
            else -> T.LAT_NONE.s()
        }
        info.text = (if (hasVoice) T.LAT_INFO.s(s.latency * 1000.0 / s.rate, src) + "\n" else "") + T.EDIT_HINT.s()
    }

    private fun save() { try { s.save() } catch (_: Throwable) { } }

    private fun rename() {
        val e = EditText(app)
        e.setText(s.name)
        e.inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS
        e.setSelectAllOnFocus(true)
        val b = android.app.AlertDialog.Builder(app, android.R.style.Theme_DeviceDefault_Dialog_Alert)
            .setTitle(T.TAKE_NAME_TITLE.s()).setView(e)
            .setPositiveButton(T.OK.s()) { _, _ ->
                val n = e.text.toString().trim()
                if (n.isNotEmpty()) { s.name = n.take(40); header.title.text = s.name; save() }
            }
            .setNegativeButton(T.CANCEL.s(), null)
        b.show()
    }

    // ---- playback
    private val tick = object : Runnable {
        override fun run() {
            if (closed) return
            if (player.playing) { lanes.playhead = player.position; time.text = mmss(lanes.playhead - s.trimStart, s.rate) }
            if (player.playing) ui.postDelayed(this, 33)
        }
    }

    private fun togglePlay() {
        if (player.playing) { stopPlay(false); return }
        val from = if (lanes.playhead >= s.trimEnd - s.rate / 20) s.trimStart else lanes.playhead
        player.play(from) { playBtn.active = false; lanes.playhead = s.trimStart; time.text = mmss(0, s.rate) }
        playBtn.active = true
        ui.removeCallbacks(tick)
        ui.post(tick)
    }

    /** stops; [rewind] back to the trim's start */
    private fun stopPlay(rewind: Boolean) {
        val at = player.position
        player.stop()
        playBtn.active = false
        lanes.playhead = if (rewind) s.trimStart else at.coerceIn(s.trimStart, s.trimEnd)
        time.text = mmss(lanes.playhead - s.trimStart, s.rate)
    }

    override fun onSelect(i: Int) { select(i) }

    override fun onSeek(frame: Long) {
        time.text = mmss(frame - s.trimStart, s.rate)
        if (player.playing) { player.play(frame) { playBtn.active = false }; ui.removeCallbacks(tick); ui.post(tick) }
    }

    override fun onTrim() {
        header.status.text = mmss(s.length, s.rate, false) + " / " + mmss(s.frames, s.rate, false)
        if (lanes.playhead !in s.trimStart..s.trimEnd) lanes.playhead = s.trimStart
        lanes.invalidate()
    }

    override fun onMuteSolo() { lanes.invalidate(); save() }

    override fun onHide() {
        stopPlay(false)
        save()
    }

    override fun onClose() {
        closed = true
        player.stop()
        save()
    }
}
