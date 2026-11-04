// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems 1.0 (android/README.md): record the FM-1's USB stems on the phone, each lane its own WAV, the phone's
// mic through the FM-1's last track and back (STEMS+IN) moved earlier by the measured round trip, then trim, mix
// and export a ZIP of the stems or a stereo mix. One activity, four pages on a stack: HOME (connect, the takes),
// REC, EDIT, EXPORT. Plain Android views, drawn in the FM-1's palette.
package fm.jiant.stems

import android.Manifest
import android.app.Activity
import android.app.AlertDialog
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.SharedPreferences
import android.content.pm.PackageManager
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.media.AudioManager
import android.os.Build
import android.os.Bundle
import android.os.StatFs
import android.view.Gravity
import android.view.View
import android.view.WindowManager
import android.widget.FrameLayout
import android.widget.ScrollView
import android.widget.TextView
import fm.jiant.stems.core.Lang
import fm.jiant.stems.core.Layout
import fm.jiant.stems.core.Session
import fm.jiant.stems.core.T
import fm.jiant.stems.core.UsbMode
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** a page of the app */
abstract class Page(val app: MainActivity) {
    abstract val view: View
    open fun onShow() {}
    open fun onHide() {}
    /** a page leaving the stack for good */
    open fun onClose() {}
    open fun onUsbDetached() {}
    open fun onActivityResult(request: Int, result: Int, data: Intent?) {}
}

class MainActivity : Activity() {
    lateinit var um: UsbManager
    lateinit var am: AudioManager
    lateinit var prefs: SharedPreferences
    private lateinit var root: FrameLayout
    private val stack = ArrayList<Page>()
    private var onPermission: ((Boolean) -> Unit)? = null
    private var onMicPermission: ((Boolean) -> Unit)? = null

    private val rx = object : BroadcastReceiver() {
        override fun onReceive(c: Context, i: Intent) {
            when (i.action) {
                USB_PERM -> {
                    val cb = onPermission
                    onPermission = null
                    cb?.invoke(i.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false))
                }
                UsbManager.ACTION_USB_DEVICE_DETACHED -> stack.toList().forEach { it.onUsbDetached() }
                UsbManager.ACTION_USB_DEVICE_ATTACHED -> (stack.lastOrNull() as? HomePage)?.refresh()
            }
        }
    }

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        Lang.init(Locale.getDefault().language)
        Fonts.init(assets)
        um = getSystemService(Context.USB_SERVICE) as UsbManager
        am = getSystemService(Context.AUDIO_SERVICE) as AudioManager
        prefs = getSharedPreferences("stems", Context.MODE_PRIVATE)
        window.statusBarColor = Pal.BLACK
        window.navigationBarColor = Pal.BLACK
        root = FrameLayout(this)
        root.setBackgroundColor(Pal.BLACK)
        setContentView(root)
        val f = IntentFilter(USB_PERM)
        f.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        f.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
        if (Build.VERSION.SDK_INT >= 33) registerReceiver(rx, f, Context.RECEIVER_NOT_EXPORTED) else registerReceiver(rx, f)
        push(HomePage(this))
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        if (intent.action == UsbManager.ACTION_USB_DEVICE_ATTACHED) (stack.lastOrNull() as? HomePage)?.refresh()
    }

    override fun onDestroy() {
        while (stack.isNotEmpty()) stack.removeAt(stack.size - 1).let { it.onHide(); it.onClose() }
        try { unregisterReceiver(rx) } catch (_: Throwable) { }
        super.onDestroy()
    }

    // ---- the page stack
    fun push(p: Page) {
        stack.lastOrNull()?.onHide()
        stack.add(p)
        show(p)
    }

    fun pop() {
        if (stack.size <= 1) { finish(); return }
        val p = stack.removeAt(stack.size - 1)
        p.onHide()
        p.onClose()
        show(stack.last())
    }

    /** back to [p] (closing the pages above it) */
    fun popTo(p: Page) {
        while (stack.size > 1 && stack.last() !== p) {
            val q = stack.removeAt(stack.size - 1)
            q.onHide(); q.onClose()
        }
        show(stack.last())
    }

    private fun show(p: Page) {
        root.removeAllViews()
        root.addView(p.view, FrameLayout.LayoutParams(MATCH, MATCH))
        p.onShow()
    }

    @Deprecated("the platform's back (targetSdk 34, no OnBackInvokedCallback needed for a plain back)")
    override fun onBackPressed() = pop()

    override fun onActivityResult(request: Int, result: Int, data: Intent?) {
        super.onActivityResult(request, result, data)
        stack.lastOrNull()?.onActivityResult(request, result, data)
    }

    fun keepScreenOn(on: Boolean) {
        if (on) window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON) else window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    }

    // ---- permissions
    fun askUsb(dev: UsbDevice, cb: (Boolean) -> Unit) {
        if (um.hasPermission(dev)) { cb(true); return }
        onPermission = cb
        val pi = PendingIntent.getBroadcast(this, 0, Intent(USB_PERM).setPackage(packageName), PendingIntent.FLAG_MUTABLE)
        um.requestPermission(dev, pi)
    }

    fun askMic(cb: (Boolean) -> Unit) {
        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) { cb(true); return }
        onMicPermission = cb
        requestPermissions(arrayOf(Manifest.permission.RECORD_AUDIO), REQ_MIC)
    }

    override fun onRequestPermissionsResult(request: Int, perms: Array<out String>, results: IntArray) {
        if (request == REQ_MIC) {
            val cb = onMicPermission
            onMicPermission = null
            cb?.invoke(results.isNotEmpty() && results[0] == PackageManager.PERMISSION_GRANTED)
        }
    }

    // ---- the takes on disk
    val takesDir: File get() = File(getExternalFilesDir(null) ?: filesDir, "takes").apply { mkdirs() }

    fun takes(): List<Session> {
        val l = ArrayList<Session>()
        for (d in takesDir.listFiles() ?: emptyArray()) {
            if (!d.isDirectory) continue
            try {
                Session.forList(d)?.let { l.add(it) }            // (recovered if the app was killed while recording it;
            } catch (_: Throwable) { }                           // 1.1: never the take being recorded)
        }
        l.sortByDescending { it.created }
        return l
    }

    fun newTakeDir(): File = File(takesDir, SimpleDateFormat("yyyyMMdd_HHmmss", Locale.ROOT).format(Date()))

    fun freeBytes(): Long = try { StatFs(takesDir.path).availableBytes } catch (_: Throwable) { -1L }

    fun dialog(title: String?, msg: String?, ok: String, onOk: () -> Unit, cancel: Boolean = true) {
        val b = AlertDialog.Builder(this, android.R.style.Theme_DeviceDefault_Dialog_Alert)
        if (title != null) b.setTitle(title)
        if (msg != null) b.setMessage(msg)
        b.setPositiveButton(ok) { _, _ -> onOk() }
        if (cancel) b.setNegativeButton(T.CANCEL.s(), null)
        b.show()
    }

    companion object {
        const val USB_PERM = "fm.jiant.stems.USB"
        const val REQ_MIC = 7
        const val REQ_SAVE = 8
    }
}

/** the first page: the FM-1 (found or not, its mode, how to set it), CONNECT; the takes below */
class HomePage(app: MainActivity) : Page(app) {
    private val header = Header(app, "J", "STEMS")
    private val status = app.text("", 20f)
    private val detail = app.text("", 13f, Pal.DIM, Fonts.body)
    private val how = app.text(T.HOW_STEPS.s(), 13f, Pal.FG, Fonts.body)
    private val connect = TextBtn(app, T.CONNECT.s(), primary = true) { connect() }
    private val search = TextBtn(app, T.SEARCH.s()) { refresh() }
    private val list = app.vbox()
    private val footer = Footer(app)
    private var found: Fm1? = null
    private var busy = false

    override val view: View = run {
        val v = app.vbox()
        val pad = app.dpi(16)
        v.setPadding(pad, app.dpi(28), pad, app.dpi(12))
        v.setBackgroundColor(Pal.BLACK)
        v.addView(header, lp(MATCH, WRAP))
        val sv = ScrollView(app)
        sv.isVerticalScrollBarEnabled = false
        val c = app.vbox()
        c.setPadding(0, app.dpi(14), 0, app.dpi(14))
        c.addView(app.text("JIANT STEMS", 11f, Pal.DIM, Fonts.mono).apply { letterSpacing = 0.16f })
        c.addView(app.text(T.APP_SUB.s(), 13f, Pal.DIM, Fonts.body), lp(MATCH, WRAP).apply { topMargin = app.dpi(4) })
        c.addView(status, lp(MATCH, WRAP).apply { topMargin = app.dpi(18) })
        c.addView(detail, lp(MATCH, WRAP).apply { topMargin = app.dpi(6) })
        detail.setLineSpacing(0f, 1.25f)
        c.addView(app.text(T.ON_FM1.s(), 11f, Pal.DIM, Fonts.mono).apply { letterSpacing = 0.12f }, lp(MATCH, WRAP).apply { topMargin = app.dpi(16) })
        how.setLineSpacing(app.dp(3f), 1.1f)
        c.addView(how, lp(MATCH, WRAP).apply { topMargin = app.dpi(6) })
        val row = app.hbox()
        row.addView(connect, lp(0, WRAP, 2f))
        row.addView(search, lp(0, WRAP, 1f).apply { leftMargin = app.dpi(8) })
        c.addView(row, lp(MATCH, WRAP).apply { topMargin = app.dpi(16) })
        c.addView(app.text(T.TAKES.s(), 11f, Pal.DIM, Fonts.mono).apply { letterSpacing = 0.12f }, lp(MATCH, WRAP).apply { topMargin = app.dpi(26) })
        c.addView(list, lp(MATCH, WRAP).apply { topMargin = app.dpi(6) })
        sv.addView(c)
        v.addView(sv, lp(MATCH, 0, 1f))
        v.addView(footer, lp(MATCH, WRAP))
        v
    }

    override fun onShow() { refresh() }

    fun refresh() {
        val all = Fm1.find(app.um)
        found = all.firstOrNull()
        val f = found
        if (f == null) {
            header.status.text = ""
            status.text = T.NO_DEVICE.s()
            status.setTextColor(Pal.FG)
            detail.text = T.CONNECT_HINT.s()
            connect.enabledLook = false
        } else {
            val ch = f.guessChannels
            header.status.text = modeName(ch) + " · " + ch + " CH"
            status.text = T.FOUND.s(f.name)
            status.setTextColor(Pal.ORANGE)
            detail.text = modeText(ch)
            connect.enabledLook = true
        }
        takesList()
        val free = app.freeBytes()
        footer.set("JIANT", if (free >= 0) T.FREE.s(bytesText(free)) else "", "HOME")
    }

    private fun takesList() {
        list.removeAllViews()
        val takes = app.takes()
        if (takes.isEmpty()) list.addView(app.text(T.NO_TAKES.s(), 13f, Pal.DIM, Fonts.body))
        val df = SimpleDateFormat("d MMM HH:mm", Lang.locale)
        for (s in takes) {
            val row = app.vbox()
            val top = app.hbox()
            top.setPadding(app.dpi(4), app.dpi(10), app.dpi(4), app.dpi(2))
            top.addView(app.text(s.name, 15f), lp(0, WRAP, 1f))
            top.addView(app.text(mmss(s.length, s.rate, false), 13f), lp(WRAP, WRAP))
            row.addView(top, lp(MATCH, WRAP))
            val meta = s.lanes.joinToString(" ") { it.id } + " · " + df.format(Date(s.created))
            row.addView(app.text(meta, 11f, Pal.DIM, Fonts.medium).apply { setPadding(app.dpi(4), 0, app.dpi(4), app.dpi(10)) })
            val rule = View(app); rule.setBackgroundColor(Pal.RULE2)
            row.addView(rule, lp(MATCH, app.dpi(1)))
            row.isClickable = true
            row.setOnClickListener { app.push(EditPage(app, s)) }
            row.setOnLongClickListener {
                app.dialog(null, T.DELETE_TAKE.s(s.name), T.DELETE.s(), { s.dir.deleteRecursively(); takesList() })
                true
            }
            list.addView(row, lp(MATCH, WRAP))
        }
    }

    private fun connect() {
        val f = found ?: return
        if (busy) return
        busy = true
        app.askUsb(f.dev) { ok ->
            busy = false
            if (!ok) { detail.text = T.USB_DENIED.s(); return@askUsb }
            try {
                val st = Fm1Stream.open(app.um, f)
                app.push(RecordPage(app, st))
            } catch (e: Fm1Stream.Failure) {
                detail.text = if (e.why == "open" || e.why == "claim") T.OPEN_FAILED.s() else T.BAD_FORMAT.s(e.why)
            } catch (e: Throwable) {
                detail.text = T.BAD_FORMAT.s(e.toString())
            }
        }
    }

    companion object {
        fun modeName(ch: Int) = when (Layout.mode(ch)) {
            UsbMode.STEMS -> "STEMS"
            UsbMode.STEMS_IN -> "STEMS+IN"
            UsbMode.STEREO -> "STEREO"
            UsbMode.OTHER -> "USB"
        }

        fun modeText(ch: Int) = when (Layout.mode(ch)) {
            UsbMode.STEMS -> T.MODE_STEMS.s()
            UsbMode.STEMS_IN -> T.MODE_STEMS_IN.s()
            UsbMode.STEREO -> T.MODE_STEREO.s()
            UsbMode.OTHER -> T.MODE_OTHER.s(ch)
        }
    }
}

/** a page's message line (hints, results, errors) */
fun Context.note(): TextView = text("", 12f, Pal.DIM, Fonts.body).apply { gravity = Gravity.START; setLineSpacing(0f, 1.2f) }
