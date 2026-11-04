// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the record page's meters (the FM-1's segmented heat gauges, one a channel, the number badge orange
// when its lane is armed) and the editor's lanes (each lane's waveform in heat, the trim's orange edges, the
// playhead).
package fm.jiant.stems

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.view.MotionEvent
import android.view.View
import fm.jiant.stems.core.Heat
import fm.jiant.stems.core.Lane
import fm.jiant.stems.core.Layout
import fm.jiant.stems.core.Session
import fm.jiant.stems.core.Trim
import java.util.Locale
import kotlin.math.abs
import kotlin.math.log10

@SuppressLint("ViewConstructor")
class MetersView(ctx: Context, private val channels: Int, private val armed: (Int) -> Boolean, private val onTap: (Int) -> Unit) : View(ctx) {
    private val segs = 16
    private val level = FloatArray(channels)
    private val hold = FloatArray(channels)
    private val holdUntil = LongArray(channels)
    private val clipUntil = LongArray(channels)
    private val p = Paint(Paint.ANTI_ALIAS_FLAG)
    private val pt = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Fonts.bold; textSize = ctx.sp(11f); textAlign = Paint.Align.CENTER }
    private val pdb = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Fonts.medium; textSize = ctx.sp(9f); textAlign = Paint.Align.CENTER; color = Pal.FAINT }
    private val r = RectF()

    /** a peak (0..32768) a channel and whether it hit full scale, from the splitter */
    fun push(peaks: IntArray, clips: BooleanArray) {
        val now = System.currentTimeMillis()
        for (c in 0 until channels) {
            val db = if (peaks[c] > 0) 20 * log10(peaks[c] / 32768.0) else -120.0
            val f = ((db + 48) / 48).toFloat().coerceIn(0f, 1f)          // -48..0 dBFS
            level[c] = maxOf(f, level[c] - 0.04f)
            if (f >= hold[c] || now > holdUntil[c]) { hold[c] = f; holdUntil[c] = now + 1500 }
            if (clips[c]) clipUntil[c] = now + 2000
        }
        invalidate()
    }

    override fun onDraw(c: Canvas) {
        val gap = context.dp(5f)
        val w = (width - gap * (channels - 1)) / channels
        val badge = context.dp(20f)
        val top = context.dp(14f)
        val bottom = height - badge - context.dp(8f)
        val sh = (bottom - top) / segs
        val now = System.currentTimeMillis()
        for (ch in 0 until channels) {
            val x = ch * (w + gap)
            val lit = Math.round(level[ch] * segs)
            val hs = Math.round(hold[ch] * segs) - 1
            for (k in 0 until segs) {
                val y = bottom - (k + 1) * sh
                p.color = if (k < lit || k == hs) Heat.argb(30 + k * 220 / segs) else Pal.SEG
                r.set(x, y + 1.5f, x + w, y + sh - 1.5f)
                c.drawRoundRect(r, 1.5f, 1.5f, p)
            }
            if (now < clipUntil[ch]) {                                // the clip lamp
                p.color = Pal.RED
                r.set(x, 0f, x + w, context.dp(6f))
                c.drawRoundRect(r, 2f, 2f, p)
            }
            val a = armed(ch)
            p.color = if (a) Pal.ORANGE else Pal.CHIP
            val bw = minOf(w, context.dp(26f))
            r.set(x + (w - bw) / 2, height - badge, x + (w + bw) / 2, height.toFloat())
            c.drawRoundRect(r, context.dp(2f), context.dp(2f), p)
            pt.color = if (a) Pal.BLACK else Pal.FG
            c.drawText(Layout.channelTag(channels, ch), x + w / 2, height - badge / 2 + pt.textSize * 0.36f, pt)
            val db = if (hold[ch] > 0f) String.format(Locale.ROOT, "%.0f", hold[ch] * 48 - 48) else ""
            c.drawText(db, x + w / 2, bottom + context.dp(11f) - context.dp(2f), pdb)
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(e: MotionEvent): Boolean {
        if (e.actionMasked == MotionEvent.ACTION_UP) {
            val ch = (e.x / width * channels).toInt().coerceIn(0, channels - 1)
            onTap(ch)
            performClick()
        }
        return true
    }

    override fun performClick(): Boolean { super.performClick(); return true }
}

/** the editor's timeline: a row a lane (its name, M and S; its waveform), the ruler above, the trim and playhead */
@SuppressLint("ViewConstructor")
class LanesView(ctx: Context, private val s: Session, private val listener: Listener) : View(ctx) {
    interface Listener {
        fun onSelect(i: Int)
        fun onSeek(frame: Long)
        fun onTrim()
        fun onMuteSolo()
    }

    /** each lane's peaks (Peaks.compute, [bucket] frames a value); null while they are read */
    val peaks = HashMap<String, FloatArray>()
    val bucket = 512
    var selected = 0
        set(v) { field = v; invalidate() }
    var playhead = 0L
        set(v) { field = v; invalidate() }

    private val p = Paint()
    private val pa = Paint(Paint.ANTI_ALIAS_FLAG)
    private val pt = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Fonts.display; textSize = ctx.sp(12f) }
    private val ps = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Fonts.bold; textSize = ctx.sp(9f); textAlign = Paint.Align.CENTER }
    private val pr = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Fonts.medium; textSize = ctx.sp(9f); color = Pal.DIM }
    private val r = RectF()
    private val labelW get() = context.dp(52f)
    private val rulerH get() = context.dp(18f)
    private val gap get() = context.dp(4f)

    private fun rowH(): Float = if (s.lanes.isEmpty()) 0f else minOf(context.dp(56f), (height - rulerH - gap * s.lanes.size) / s.lanes.size)
    private fun rowTop(i: Int) = rulerH + gap + i * (rowH() + gap)
    private fun x0() = labelW + context.dp(4f)
    private fun frameToX(f: Long): Float = Trim.xOf(f, s.frames, x0(), width.toFloat())
    private fun xToFrame(x: Float): Long = Trim.frameAt(x, s.frames, x0(), width.toFloat(), 0f)

    override fun onDraw(c: Canvas) {
        val x0 = x0()
        val w = width - x0
        // the ruler: seconds (a label every so many, as fits)
        val secs = s.frames / s.rate.toDouble()
        val step = listOf(1, 2, 5, 10, 15, 30, 60, 120, 300).firstOrNull { secs / it <= 6 } ?: 600
        p.color = Pal.RULE
        var t = 0
        while (t <= secs) {
            val x = frameToX(t.toLong() * s.rate)
            c.drawRect(x, rulerH - context.dp(5f), x + 1, rulerH, p)
            c.drawText(if (t >= 60) String.format(Locale.ROOT, "%d:%02d", t / 60, t % 60) else "$t", x + context.dp(2f), rulerH - context.dp(7f), pr)
            t += step
        }
        val h = rowH()
        val anySolo = s.lanes.any { it.solo }
        s.lanes.forEachIndexed { i, l ->
            val y = rowTop(i)
            val sel = i == selected
            val audible = !l.mute && (!anySolo || l.solo)
            // the label: the lane's name, M and S
            pt.color = if (sel) Pal.ORANGE else if (audible) Pal.FG else Pal.FAINT
            c.drawText(l.id, 0f, y + pt.textSize, pt)
            val bs = context.dp(18f)
            val by = y + h - bs
            box(c, 0f, by, bs, "M", l.mute, Pal.RED)
            box(c, bs + context.dp(4f), by, bs, "S", l.solo, Pal.CYAN)
            // the wave
            p.color = Pal.WAVE_BG
            c.drawRect(x0, y, x0 + w, y + h, p)
            p.color = Pal.RULE2
            var g = 0
            while (g <= secs) { val x = frameToX(g.toLong() * s.rate); c.drawRect(x, y, x + 1, y + h, p); g += step }
            val pk = peaks[l.id]
            if (pk != null) {
                val mid = y + h / 2
                val fpp = s.frames.toDouble() / w                  // frames a pixel
                var px = 0
                while (px < w) {
                    val a = (px * fpp).toLong() - l.offset
                    val b = ((px + 1) * fpp).toLong() - l.offset
                    var m = 0f
                    var k = maxOf(0L, a / bucket).toInt()
                    val ke = minOf(pk.size.toLong(), b / bucket + 1).toInt()
                    while (k < ke) { if (pk[k] > m) m = pk[k]; k++ }
                    if (m > 0f) {
                        val v = (m.coerceAtMost(1f)).let { Math.sqrt(it.toDouble()).toFloat() }   // (a gentle curve: quiet stems still show)
                        val hh = v * (h / 2 - 1)
                        p.color = if (!audible) Pal.FAINT else if (sel) Pal.ORANGE else Heat.argb((40 + v * 200).toInt())
                        c.drawRect(x0 + px, mid - hh, x0 + px + 1, mid + hh, p)
                    }
                    px++
                }
            } else {
                pr.color = Pal.FAINT
                c.drawText("…", x0 + context.dp(6f), y + h / 2 + context.dp(3f), pr)
                pr.color = Pal.DIM
            }
            if (sel) { p.color = Pal.ORANGE; c.drawRect(x0, y, x0 + context.dp(2f), y + h, p) }
        }
        // the trim: the outside shaded, the edges orange with a grip in the ruler
        val a = frameToX(s.trimStart)
        val b = frameToX(s.trimEnd)
        val bottom = if (s.lanes.isEmpty()) height.toFloat() else rowTop(s.lanes.size - 1) + h
        p.color = 0xB0000000.toInt()
        c.drawRect(x0, rulerH, a, bottom, p)
        c.drawRect(b, rulerH, width.toFloat(), bottom, p)
        p.color = Pal.ORANGE
        c.drawRect(a - context.dp(1f), 0f, a + context.dp(1f), bottom, p)
        c.drawRect(b - context.dp(1f), 0f, b + context.dp(1f), bottom, p)
        r.set(a, 0f, a + context.dp(8f), context.dp(10f)); c.drawRect(r, p)
        r.set(b - context.dp(8f), 0f, b, context.dp(10f)); c.drawRect(r, p)
        // the playhead
        val x = frameToX(playhead)
        p.color = Pal.FG
        c.drawRect(x, rulerH, x + context.dp(1f), bottom, p)
    }

    private fun box(c: Canvas, x: Float, y: Float, s: Float, t: String, on: Boolean, col: Int) {
        r.set(x, y, x + s, y + s)
        pa.style = Paint.Style.FILL
        pa.color = if (on) col else Pal.SEG
        c.drawRoundRect(r, context.dp(2f), context.dp(2f), pa)
        ps.color = if (on) Pal.BLACK else Pal.MID
        c.drawText(t, x + s / 2, y + s / 2 + ps.textSize * 0.36f, ps)
    }

    private var drag = 0                                          // 1 the trim's start, 2 its end, 3 the playhead
    private var downX = 0f
    private var dragFrom = 0L                                     // (1.1) the edge's frame when it was taken

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(e: MotionEvent): Boolean {
        val x = e.x
        val y = e.y
        when (e.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                downX = x
                parent?.requestDisallowInterceptTouchEvent(true)
                val a = frameToX(s.trimStart); val b = frameToX(s.trimEnd)
                val near = context.dp(22f)
                drag = 0
                if (x > x0() - near / 2) drag = Trim.grab(x, y, a, b, rulerH + context.dp(6f), near)   // (1.1: by the grip)
                dragFrom = if (drag == Trim.START) s.trimStart else s.trimEnd
                val row = rowAt(y)
                if (drag == 0 && x < x0()) {                           // the label: select, M, S
                    if (row >= 0) {
                        val l = s.lanes[row]
                        val bs = context.dp(18f)
                        val by = rowTop(row) + rowH() - bs
                        if (y >= by && x < bs + context.dp(2f)) { l.mute = !l.mute; listener.onMuteSolo() }
                        else if (y >= by && x < 2 * bs + context.dp(6f)) { l.solo = !l.solo; listener.onMuteSolo() }
                        selected = row
                        listener.onSelect(row)
                    }
                    return true
                }
                if (drag == 0) {
                    drag = 3
                    if (row >= 0 && row != selected) { selected = row; listener.onSelect(row) }
                    seek(x)
                }
            }
            MotionEvent.ACTION_MOVE -> when (drag) {
                1, 2 -> {                                              // (1.1) from where it was, past a slop, ends snapped
                    Trim.drag(s, drag, dragFrom, downX, x, x0(), width.toFloat(), context.dp(10f), context.dp(4f), (s.rate / 10).toLong())
                    listener.onTrim(); invalidate()
                }
                3 -> seek(x)
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                parent?.requestDisallowInterceptTouchEvent(false)
                if (drag == 1 || drag == 2) { s.clampTrim(); listener.onTrim() }
                drag = 0
                performClick()
            }
        }
        return true
    }

    override fun performClick(): Boolean { super.performClick(); return true }

    private fun seek(x: Float) {
        playhead = xToFrame(x).coerceIn(s.trimStart, s.trimEnd)
        listener.onSeek(playhead)
    }

    private fun rowAt(y: Float): Int {
        for (i in s.lanes.indices) { val t = rowTop(i); if (y >= t && y < t + rowH() + gap) return i }
        return -1
    }

    fun lane(): Lane? = s.lanes.getOrNull(selected)
}
