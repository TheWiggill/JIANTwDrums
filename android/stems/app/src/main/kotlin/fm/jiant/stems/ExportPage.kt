// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the export page. STEMS ZIP (a WAV a lane, as recorded or with the mix, MIX.wav beside them if asked)
// or MERGE (the stereo mix, 16 or 24 bit, normalized to -1 dBFS if asked), both over the trim with the latency
// applied. SAVE TO FILES opens Android's document picker (Downloads by default); SHARE writes the file to the app's
// cache and hands it to the share sheet.
package fm.jiant.stems

import android.app.Activity
import android.content.ClipData
import android.content.Intent
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.view.View
import fm.jiant.stems.core.CancelledException
import fm.jiant.stems.core.Export
import fm.jiant.stems.core.ExportOptions
import fm.jiant.stems.core.FrameSource
import fm.jiant.stems.core.Progress
import fm.jiant.stems.core.Renderer
import fm.jiant.stems.core.Session
import fm.jiant.stems.core.T
import fm.jiant.stems.core.Trim
import fm.jiant.stems.core.WavSource
import java.io.File
import java.io.OutputStream

class ExportPage(app: MainActivity, private val s: Session) : Page(app) {
    private val ui = Handler(Looper.getMainLooper())
    private var zip = true
    private var bits = app.prefs.getInt("exportBits", 24)
    private var withMix = app.prefs.getBoolean("stemsWithMix", false)
    private var addMix = app.prefs.getBoolean("addMix", true)
    private var normalize = app.prefs.getBoolean("normalize", true)
    @Volatile private var running = false
    @Volatile private var cancel = false

    private val header = Header(app, "J", T.EXPORT.s())
    private val seg = Seg(app, listOf(T.STEMS_ZIP.s(), T.MERGE.s())) { zip = it == 0; refresh() }
    private val what = app.note()
    private val files = app.text("", 12f, Pal.MID, Fonts.medium)
    private val format = OptRow(app, T.FORMAT.s()) { bits = if (bits == 24) 16 else 24; refresh() }
    private val rate = OptRow(app, T.RATE.s()) { }
    private val stems = OptRow(app, T.STEMS.s()) { withMix = !withMix; refresh() }
    private val mixRow = OptRow(app, T.ADD_MIX.s()) { addMix = !addMix; refresh() }
    private val norm = OptRow(app, T.NORMALIZE.s()) { normalize = !normalize; refresh() }
    private val progress = app.note()
    private val save = TextBtn(app, T.SAVE.s(), primary = true) { saveToFiles() }
    private val share = TextBtn(app, T.SHARE.s()) { shareIt() }
    private val footer = Footer(app)

    override val view: View = run {
        val v = app.vbox()
        val pad = app.dpi(16)
        v.setPadding(pad, app.dpi(28), pad, app.dpi(12))
        v.setBackgroundColor(Pal.BLACK)
        header.status.text = mmss(s.length, s.rate, false)
        v.addView(header, lp(MATCH, WRAP))
        v.addView(seg, lp(MATCH, WRAP).apply { topMargin = app.dpi(14) })
        v.addView(what, lp(MATCH, WRAP).apply { topMargin = app.dpi(10) })
        v.addView(files, lp(MATCH, WRAP).apply { topMargin = app.dpi(8) })
        val opts = app.vbox()
        for (o in listOf(format, rate, stems, mixRow, norm)) opts.addView(o, lp(MATCH, WRAP))
        v.addView(opts, lp(MATCH, WRAP).apply { topMargin = app.dpi(10) })
        v.addView(View(app), lp(MATCH, 0, 1f))
        v.addView(progress, lp(MATCH, WRAP).apply { bottomMargin = app.dpi(8) })
        v.addView(save, lp(MATCH, WRAP))
        v.addView(share, lp(MATCH, WRAP).apply { topMargin = app.dpi(8); bottomMargin = app.dpi(10) })
        footer.set(s.name, "", T.EXPORT.s())
        v.addView(footer, lp(MATCH, WRAP))
        v
    }

    init { refresh() }

    private fun options() = ExportOptions(bits = bits, stemsWithMix = withMix, includeMix = addMix, normalize = normalize)

    private fun fileName() = Session.safeName(s.name) + if (zip) "_stems.zip" else "_mix.wav"

    private fun refresh() {
        val o = options()
        what.text = (if (zip) T.EXPORT_HINT_ZIP.s() else T.EXPORT_HINT_MIX.s()) +
            (if (Trim.trimmed(s)) "\n" + T.EXPORT_TRIMMED.s(mmss(s.length, s.rate), mmss(s.frames, s.rate)) else "")   // (1.1)
        // the files and about how big (WAV is barely compressed by the ZIP)
        var bytes = 0L
        val names: List<String>
        if (zip) {
            names = Export.zipNames(s, o).filter { it != "README.txt" }
            for (l in s.lanes) if (!o.stemsWithMix || s.audible(l)) bytes += s.length * (if (o.stemsWithMix) 2 * bits / 8 else l.channels * 2)
            if (o.includeMix) bytes += s.length * 2 * bits / 8
        } else {
            names = listOf(fileName())
            bytes = s.length * 2 * bits / 8
        }
        files.text = names.joinToString("  ") + "\n" + T.EXPORT_FILES.s(names.size, bytesText(bytes + 44L * names.size))
        format.set(T.WAV_BITS.s(if (zip && !withMix) 16 else bits).let { if (zip && !withMix && addMix) it + " · MIX " + bits else it })
        rate.set("44.1 kHz", Pal.DIM)
        stems.visibility = if (zip) View.VISIBLE else View.GONE
        mixRow.visibility = if (zip) View.VISIBLE else View.GONE
        stems.set(if (withMix) T.WITH_MIX.s() else T.AS_RECORDED.s(), Pal.VIOLET)
        mixRow.set(if (addMix) T.YES.s() else T.NO.s())
        norm.visibility = if (!zip || addMix) View.VISIBLE else View.GONE
        norm.set(if (normalize) "-1 dBFS" else T.OFF.s())
        app.prefs.edit().putInt("exportBits", bits).putBoolean("stemsWithMix", withMix).putBoolean("addMix", addMix).putBoolean("normalize", normalize).apply()
    }

    /** the export into [out] on a thread of its own; [done] on the main thread with null or the error */
    private fun exportTo(out: () -> OutputStream, done: (String?) -> Unit) {
        if (running) return
        running = true
        cancel = false
        save.enabledLook = false
        share.enabledLook = false
        progress.text = T.EXPORTING.s(0)
        val o = options()
        val asZip = zip
        Thread {
            var err: String? = null
            val srcs = ArrayList<WavSource>()
            try {
                val map = HashMap<String, FrameSource>()
                for (l in s.lanes) { val f = s.file(l); if (f.isFile) { val w = WavSource(f); srcs.add(w); map[l.id] = w } }
                val r = Renderer(s, map)
                var last = -1
                val p = Progress { f ->
                    val pc = (f * 100).toInt()
                    if (pc != last) { last = pc; ui.post { if (running) progress.text = T.EXPORTING.s(pc) } }
                    !cancel
                }
                out().use { os ->
                    val bo = java.io.BufferedOutputStream(os, 1 shl 16)
                    if (asZip) Export.stemsZip(r, bo, o, p) else Export.mixdown(r, bo, o, p)
                    bo.flush()
                }
            } catch (_: CancelledException) {
                err = "cancelled"
            } catch (t: Throwable) {
                err = t.message ?: t.toString()
            } finally {
                srcs.forEach { try { it.close() } catch (_: Throwable) { } }
            }
            ui.post {
                running = false
                save.enabledLook = true
                share.enabledLook = true
                done(err)
            }
        }.start()
    }

    private fun saveToFiles() {
        if (running) return
        val i = Intent(Intent.ACTION_CREATE_DOCUMENT)
        i.addCategory(Intent.CATEGORY_OPENABLE)
        i.type = ExportProvider.mime(fileName())
        i.putExtra(Intent.EXTRA_TITLE, fileName())
        try {
            @Suppress("DEPRECATION")
            app.startActivityForResult(i, MainActivity.REQ_SAVE)
        } catch (t: Throwable) {
            progress.text = T.EXPORT_FAILED.s(t.toString())
        }
    }

    override fun onActivityResult(request: Int, result: Int, data: Intent?) {
        if (request != MainActivity.REQ_SAVE) return
        val uri: Uri = data?.data ?: return
        if (result != Activity.RESULT_OK) return
        exportTo({ app.contentResolver.openOutputStream(uri, "wt") ?: throw java.io.IOException("no stream") }) { err ->
            progress.text = if (err == null) T.SAVED.s(fileName()) else T.EXPORT_FAILED.s(err)
        }
    }

    private fun shareIt() {
        if (running) return
        val dir = ExportProvider.dir(app)
        dir.listFiles()?.forEach { it.delete() }                  // (one export at a time in the cache)
        val f = File(dir, fileName())
        exportTo({ f.outputStream() }) { err ->
            if (err != null) { progress.text = T.EXPORT_FAILED.s(err); f.delete(); return@exportTo }
            progress.text = ""
            val u = ExportProvider.uri(f)
            val i = Intent(Intent.ACTION_SEND)
            i.type = ExportProvider.mime(f.name)
            i.putExtra(Intent.EXTRA_STREAM, u)
            i.putExtra(Intent.EXTRA_SUBJECT, s.name)
            i.clipData = ClipData.newRawUri(f.name, u)
            i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            app.startActivity(Intent.createChooser(i, T.SHARE_TITLE.s()).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION))
        }
    }

    override fun onClose() { cancel = true }
}
