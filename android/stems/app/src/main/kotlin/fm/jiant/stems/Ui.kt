// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the FM-1's screen language on the phone (ui_draw.c, the "JIANT Stems App" design): black, cream text,
// violet for what is selected, orange for the one active thing, the heat ramp for levels; Chakra Petch; the header
// with its badge, the four cards with a label, a value and a hairline gauge, the footer breadcrumb.
package fm.jiant.stems

import android.annotation.SuppressLint
import android.content.Context
import android.content.res.AssetManager
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.util.TypedValue
import android.view.GestureDetector
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.widget.LinearLayout
import android.widget.TextView
import fm.jiant.stems.core.Heat

object Pal {
    const val BG = 0xFF07070A.toInt()
    const val BLACK = 0xFF000000.toInt()
    const val PANEL = 0xFF111016.toInt()
    const val RULE = 0xFF24212C.toInt()
    const val RULE2 = 0xFF1C1A22.toInt()
    const val FG = 0xFFF4F1EA.toInt()
    const val DIM = 0xFF8B8697.toInt()
    const val MID = 0xFFA39FAE.toInt()
    const val FAINT = 0xFF6E6A78.toInt()
    const val VIOLET = 0xFFB870FF.toInt()
    const val ORANGE = 0xFFFF7A00.toInt()
    const val CYAN = 0xFF3FD6DC.toInt()
    const val RED = 0xFFFF3B4A.toInt()
    const val SEG = 0xFF1D1A24.toInt()
    const val CHIP = 0xFF55505F.toInt()
    const val BORDER = 0xFF34303D.toInt()
    const val WAVE_BG = 0xFF0B0A0E.toInt()
}

object Fonts {
    var display: Typeface = Typeface.DEFAULT_BOLD          // Chakra Petch SemiBold
    var bold: Typeface = Typeface.DEFAULT_BOLD
    var medium: Typeface = Typeface.DEFAULT
    var body: Typeface = Typeface.DEFAULT                   // Inter Tight
    var mono: Typeface = Typeface.MONOSPACE                 // IBM Plex Mono

    fun init(a: AssetManager) {
        fun load(n: String, fallback: Typeface) = try { Typeface.createFromAsset(a, "fonts/$n") } catch (_: Throwable) { fallback }
        display = load("ChakraPetch-SemiBold.ttf", display)
        bold = load("ChakraPetch-Bold.ttf", bold)
        medium = load("ChakraPetch-Medium.ttf", medium)
        body = load("InterTight.ttf", body)
        mono = load("IBMPlexMono-SemiBold.ttf", mono)
    }
}

fun Context.dp(v: Float): Float = v * resources.displayMetrics.density
fun Context.dpi(v: Int): Int = Math.round(v * resources.displayMetrics.density)
fun Context.sp(v: Float): Float = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, v, resources.displayMetrics)

fun Context.text(s: String, size: Float, color: Int = Pal.FG, font: Typeface = Fonts.display): TextView {
    val t = TextView(this)
    t.text = s
    t.setTextColor(color)
    t.setTextSize(TypedValue.COMPLEX_UNIT_SP, size)
    t.typeface = font
    t.includeFontPadding = false
    return t
}

fun Context.vbox(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
fun Context.hbox(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; gravity = Gravity.CENTER_VERTICAL }

fun lp(w: Int, h: Int, weight: Float = 0f) = LinearLayout.LayoutParams(w, h, weight)
const val MATCH = LinearLayout.LayoutParams.MATCH_PARENT
const val WRAP = LinearLayout.LayoutParams.WRAP_CONTENT

fun border(ctx: Context, stroke: Int, fill: Int = 0, radius: Float = 3f): GradientDrawable = GradientDrawable().apply {
    setColor(fill)
    setStroke(ctx.dpi(1), stroke)
    cornerRadius = ctx.dp(radius)
}

/** the header: the badge, a dot (red: recording), the title, the status at the right; a hairline under */
class Header(ctx: Context, badge: String, title: String) : LinearLayout(ctx) {
    val badge: TextView = ctx.text(badge, 12f, Pal.BLACK, Fonts.bold)
    val dot = View(ctx)
    val title: TextView = ctx.text(title, 16f)
    val status: TextView = ctx.text("", 11f, Pal.DIM, Fonts.medium)

    init {
        orientation = VERTICAL
        val row = ctx.hbox()
        this.badge.gravity = Gravity.CENTER
        this.badge.background = GradientDrawable().apply { setColor(Pal.ORANGE); cornerRadius = ctx.dp(2f) }
        row.addView(this.badge, lp(ctx.dpi(20), ctx.dpi(20)))
        dot.background = GradientDrawable().apply { shape = GradientDrawable.OVAL; setColor(Pal.RED) }
        dot.visibility = GONE
        row.addView(dot, lp(ctx.dpi(10), ctx.dpi(10)).apply { leftMargin = ctx.dpi(8) })
        row.addView(this.title, lp(0, WRAP, 1f).apply { leftMargin = ctx.dpi(8) })
        this.status.letterSpacing = 0.04f
        row.addView(this.status, lp(WRAP, WRAP))
        addView(row, lp(MATCH, WRAP))
        val rule = View(ctx)
        rule.setBackgroundColor(Pal.RULE)
        addView(rule, lp(MATCH, ctx.dpi(1)).apply { topMargin = ctx.dpi(8) })
    }
}

/** the footer breadcrumb: violet at the left, the middle, the page at the right */
class Footer(ctx: Context) : LinearLayout(ctx) {
    val left: TextView = ctx.text("", 11f, Pal.VIOLET, Fonts.medium)
    val mid: TextView = ctx.text("", 11f, Pal.FG, Fonts.medium)
    val right: TextView = ctx.text("", 11f, Pal.FG, Fonts.medium)

    init {
        orientation = VERTICAL
        val rule = View(ctx)
        rule.setBackgroundColor(Pal.RULE2)
        addView(rule, lp(MATCH, ctx.dpi(1)))
        val row = ctx.hbox()
        row.setPadding(0, ctx.dpi(6), 0, 0)
        row.addView(left, lp(WRAP, WRAP))
        row.addView(mid, lp(0, WRAP, 1f).apply { leftMargin = ctx.dpi(10) })
        row.addView(right, lp(WRAP, WRAP))
        addView(row, lp(MATCH, WRAP))
    }

    fun set(l: String, m: String, r: String) { left.text = l; mid.text = m; right.text = r }
}

/** a text button: the hairline border of the FM-1's tools, orange when [on]; [primary] the cream one of the design */
class TextBtn(ctx: Context, label: String, private val primary: Boolean = false, onClick: (TextBtn) -> Unit) : TextView(ctx) {
    var on = false
        set(v) { field = v; style() }
    var enabledLook = true
        set(v) { field = v; style() }

    init {
        text = label
        typeface = if (primary) Fonts.bold else Fonts.display
        setTextSize(TypedValue.COMPLEX_UNIT_SP, if (primary) 14f else 12f)
        gravity = Gravity.CENTER
        letterSpacing = 0.04f
        val v = ctx.dpi(if (primary) 12 else 8)
        setPadding(ctx.dpi(8), v, ctx.dpi(8), v)
        isClickable = true
        setOnClickListener { if (enabledLook) onClick(this) }
        style()
    }

    private fun style() {
        when {
            primary -> {
                background = GradientDrawable().apply { setColor(if (enabledLook) Pal.FG else Pal.RULE); cornerRadius = context.dp(4f) }
                setTextColor(if (enabledLook) Pal.BLACK else Pal.FAINT)
            }
            on -> { background = border(context, Pal.ORANGE); setTextColor(Pal.ORANGE) }
            else -> { background = border(context, Pal.BORDER); setTextColor(if (enabledLook) Pal.MID else Pal.FAINT) }
        }
    }
}

/** one of the FM-1's cards: a label, a value, a hairline gauge under them (orange when it is the active one). Drag
 * sideways to change the value, double-tap to reset it; [tap] makes it a button instead */
@SuppressLint("ViewConstructor")
class Card(ctx: Context, var label: String, val min: Float, val max: Float, val reset: Float, private val fmt: (Float) -> String) : View(ctx) {
    var value = reset
        set(v) { field = v.coerceIn(min, max); invalidate() }
    var hot = false
        set(v) { field = v; invalidate() }
    var first = false
    var onChange: ((Float) -> Unit)? = null
    var onDone: ((Float) -> Unit)? = null
    var tap: (() -> Unit)? = null
    /** the value as text (when [fmt] is not enough: a word) */
    var override: String? = null
        set(v) { field = v; invalidate() }
    /** a level shown in the gauge instead of the value (0..1), -1 none */
    var level = -1f
        set(v) { field = v; invalidate() }

    private val pl = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Fonts.medium; textSize = ctx.sp(10f); color = Pal.MID }
    private val pv = Paint(Paint.ANTI_ALIAS_FLAG).apply { typeface = Fonts.display; textSize = ctx.sp(17f) }
    private val pg = Paint()
    private var downX = 0f
    private var downV = 0f
    private var dragging = false
    private val gd = GestureDetector(ctx, object : GestureDetector.SimpleOnGestureListener() {
        override fun onDoubleTap(e: MotionEvent): Boolean {
            if (tap != null) return false
            value = reset; onChange?.invoke(value); onDone?.invoke(value); return true
        }
        override fun onSingleTapConfirmed(e: MotionEvent): Boolean { tap?.invoke(); return tap != null }
    })

    override fun onMeasure(w: Int, h: Int) {
        setMeasuredDimension(MeasureSpec.getSize(w), context.dpi(50))
    }

    override fun onDraw(c: Canvas) {
        val pad = if (first) context.dp(2f) else context.dp(7f)
        if (!first) { pg.color = Pal.RULE; c.drawRect(0f, 0f, context.dp(1f), height.toFloat(), pg) }
        c.drawText(label, pad, context.dp(12f), pl)
        pv.color = if (hot) Pal.ORANGE else Pal.FG
        c.drawText(override ?: fmt(value), pad, context.dp(34f), pv)
        val y = height - context.dp(3f)
        pg.color = Pal.RULE
        c.drawRect(pad, y, width - context.dp(5f), y + context.dp(1f), pg)
        val f = if (level >= 0f) level.coerceIn(0f, 1f) else if (max > min) (value - min) / (max - min) else 0f
        pg.color = if (level >= 0f) Heat.argb((f * 256).toInt()) else if (hot) Pal.ORANGE else Pal.MID
        c.drawRect(pad, y - context.dp(1f), pad + (width - context.dp(5f) - pad) * f, y + context.dp(1f), pg)
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(e: MotionEvent): Boolean {
        if (!isEnabled) return true
        gd.onTouchEvent(e)
        if (tap != null) return true
        when (e.actionMasked) {
            MotionEvent.ACTION_DOWN -> { downX = e.x; downV = value; dragging = false; parent?.requestDisallowInterceptTouchEvent(true) }
            MotionEvent.ACTION_MOVE -> {
                val dx = e.x - downX
                if (!dragging && Math.abs(dx) > context.dp(6f)) dragging = true
                if (dragging) {
                    val span = context.dp(220f)                      // a full sweep of the range over ~220 dp
                    val v = (downV + dx / span * (max - min)).coerceIn(min, max)
                    if (v != value) { value = v; onChange?.invoke(value) }
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                parent?.requestDisallowInterceptTouchEvent(false)
                if (dragging) onDone?.invoke(value)
                dragging = false
            }
        }
        return true
    }
}

/** an option row of the export page: a label, its value; a tap steps it */
class OptRow(ctx: Context, label: String, onTap: (OptRow) -> Unit) : LinearLayout(ctx) {
    val value: TextView = ctx.text("", 13f)

    init {
        orientation = VERTICAL
        val row = ctx.hbox()
        row.setPadding(ctx.dpi(4), ctx.dpi(11), ctx.dpi(4), ctx.dpi(11))
        row.addView(ctx.text(label, 11f, Pal.MID, Fonts.medium), lp(0, WRAP, 1f))
        row.addView(value, lp(WRAP, WRAP))
        addView(row, lp(MATCH, WRAP))
        val rule = View(ctx); rule.setBackgroundColor(Pal.RULE2)
        addView(rule, lp(MATCH, ctx.dpi(1)))
        isClickable = true
        setOnClickListener { onTap(this) }
    }

    fun set(v: String, color: Int = Pal.FG) { value.text = v; value.setTextColor(color) }
}

/** the two-way switch of the export page */
class Seg(ctx: Context, labels: List<String>, private val onSelect: (Int) -> Unit) : LinearLayout(ctx) {
    private val items = ArrayList<TextView>()
    var selected = 0
        private set

    init {
        orientation = HORIZONTAL
        background = border(ctx, Pal.BORDER, 0, 4f)
        setPadding(ctx.dpi(1), ctx.dpi(1), ctx.dpi(1), ctx.dpi(1))
        labels.forEachIndexed { i, s ->
            val t = ctx.text(s, 13f, Pal.DIM)
            t.gravity = Gravity.CENTER
            t.setPadding(0, ctx.dpi(10), 0, ctx.dpi(10))
            t.setOnClickListener { select(i); onSelect(i) }
            items.add(t)
            addView(t, lp(0, WRAP, 1f))
        }
        select(0)
    }

    fun select(i: Int) {
        selected = i
        items.forEachIndexed { k, t ->
            t.setTextColor(if (k == i) Pal.BLACK else Pal.DIM)
            t.background = if (k == i) GradientDrawable().apply { setColor(Pal.ORANGE); cornerRadius = context.dp(3f) } else null
        }
    }
}

/** the transport's round buttons: REC (the red ring and dot; a square while recording), STOP, PLAY */
@SuppressLint("ViewConstructor")
class RoundBtn(ctx: Context, val kind: Kind, onClick: () -> Unit) : View(ctx) {
    enum class Kind { REC, STOP, PLAY }

    var active = false
        set(v) { field = v; invalidate() }
    private val p = Paint(Paint.ANTI_ALIAS_FLAG)

    init {
        isClickable = true
        setOnClickListener { onClick() }
    }

    override fun onMeasure(w: Int, h: Int) {
        val s = context.dpi(if (kind == Kind.REC) 68 else 52)
        setMeasuredDimension(s, s)
    }

    override fun onDraw(c: Canvas) {
        val cx = width / 2f; val cy = height / 2f
        val r = minOf(cx, cy) - context.dp(2f)
        p.style = Paint.Style.STROKE
        p.strokeWidth = context.dp(if (kind == Kind.REC) 2.5f else 1.2f)
        p.color = if (kind == Kind.REC) Pal.RED else if (active) Pal.VIOLET else Pal.BORDER
        c.drawCircle(cx, cy, r, p)
        p.style = Paint.Style.FILL
        when (kind) {
            Kind.REC -> {
                p.color = Pal.RED
                if (active) { val s = r * 0.42f; c.drawRoundRect(RectF(cx - s, cy - s, cx + s, cy + s), context.dp(3f), context.dp(3f), p) }
                else c.drawCircle(cx, cy, r * 0.46f, p)
            }
            Kind.STOP -> { p.color = Pal.FG; val s = r * 0.3f; c.drawRect(cx - s, cy - s, cx + s, cy + s, p) }
            Kind.PLAY -> {
                p.color = if (active) Pal.ORANGE else Pal.VIOLET
                val s = r * 0.36f
                val path = android.graphics.Path()
                path.moveTo(cx - s * 0.7f, cy - s); path.lineTo(cx + s, cy); path.lineTo(cx - s * 0.7f, cy + s); path.close()
                c.drawPath(path, p)
            }
        }
    }
}

fun mmss(frames: Long, rate: Int, tenths: Boolean = true): String {
    val t = frames.toDouble() / rate
    val m = (t / 60).toInt()
    val s = t - m * 60
    return if (tenths) String.format(java.util.Locale.ROOT, "%02d:%04.1f", m, Math.floor(s * 10) / 10)
           else String.format(java.util.Locale.ROOT, "%d:%02d", m, s.toInt())
}

fun bytesText(b: Long): String = when {
    b >= 1L shl 30 -> String.format(java.util.Locale.ROOT, "%.1f GB", b / (1L shl 30).toDouble())
    b >= 1L shl 20 -> String.format(java.util.Locale.ROOT, "%.0f MB", b / (1L shl 20).toDouble())
    else -> String.format(java.util.Locale.ROOT, "%d KB", b / 1024)
}
