// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: the pages on Robolectric (no phone, no FM-1): HOME lists a take written to disk, EDIT draws its lanes,
// a drag trims it, M mutes a lane, a card's drag sets a gain, PLAY and STOP run, EXPORT shares a ZIP (through the
// app's provider) and a mix, and the edits are saved; the Spanish strings and the record page's meters.
package fm.jiant.stems

import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Canvas
import android.os.Looper
import android.os.SystemClock
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import fm.jiant.stems.core.Lang
import fm.jiant.stems.core.Layout
import fm.jiant.stems.core.Session
import fm.jiant.stems.core.WavFileWriter
import fm.jiant.stems.core.WavSource
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config
import java.io.File
import java.util.zip.ZipFile

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [34])
class PagesTest {
    private fun stack(a: MainActivity): List<Page> {
        val f = MainActivity::class.java.getDeclaredField("stack"); f.isAccessible = true
        @Suppress("UNCHECKED_CAST") return f.get(a) as List<Page>
    }

    private fun draw(v: View, w: Int = 1080, h: Int = 2200) {
        v.measure(View.MeasureSpec.makeMeasureSpec(w, View.MeasureSpec.EXACTLY), View.MeasureSpec.makeMeasureSpec(h, View.MeasureSpec.EXACTLY))
        v.layout(0, 0, w, h)
        val b = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        v.draw(Canvas(b))
    }

    private fun texts(v: View, out: MutableList<String> = ArrayList()): List<String> {
        if (v is TextView) out.add(v.text.toString())
        if (v is ViewGroup) for (i in 0 until v.childCount) texts(v.getChildAt(i), out)
        return out
    }

    private fun find(v: View, pred: (View) -> Boolean): View? {
        if (pred(v)) return v
        if (v is ViewGroup) for (i in 0 until v.childCount) find(v.getChildAt(i), pred)?.let { return it }
        return null
    }

    private fun idle(ms: Long = 300) {
        for (i in 0 until (ms / 50)) { Thread.sleep(50); shadowOf(Looper.getMainLooper()).idle() }
    }

    private fun take(dir: File): Session {
        dir.mkdirs()
        val n = Layout.RATE * 4
        val lanes = Layout.lanes(10)
        val s = Session(dir)
        s.name = "TAKE 01"; s.deviceChannels = 10
        for (spec in lanes) {
            s.lanes.add(Session.defaultLane(spec))
            WavFileWriter(File(dir, spec.id + ".wav"), spec.channels.size, Layout.RATE).use { w ->
                val b = ByteArray(n * spec.channels.size * 2)
                for (i in 0 until n * spec.channels.size) { val v = ((i * 37) % 2000) - 1000; b[2 * i] = v.toByte(); b[2 * i + 1] = (v shr 8).toByte() }
                w.write(b, 0, b.size)
            }
        }
        s.frames = n.toLong(); s.trimEnd = s.frames
        s.applyLatency(1000, Session.LATENCY_TAKE, 9.0)
        s.save()
        return s
    }

    private fun touch(v: View, action: Int, x: Float, y: Float) {
        val t = SystemClock.uptimeMillis()
        val e = MotionEvent.obtain(t, t, action, x, y, 0)
        v.dispatchTouchEvent(e)
        e.recycle()
    }

    @Test fun homeEditExport() {
        java.util.Locale.setDefault(java.util.Locale.US)
        val c = Robolectric.buildActivity(MainActivity::class.java).setup()
        val a = c.get()
        val home = stack(a).last() as HomePage
        draw(home.view)
        assertTrue(texts(home.view).toString(), texts(home.view).contains("No FM-1 found"))

        // a take on disk appears; a tap opens it
        val s = take(File(a.takesDir, "20261017_120000"))
        home.refresh()
        val row = find(home.view) { it is ViewGroup && it.isClickable && texts(it).contains("TAKE 01") }
        assertNotNull(row)
        row!!.performClick()
        val edit = stack(a).last() as EditPage
        idle(1500)                                            // the peaks
        draw(edit.view)
        val lanes = find(edit.view) { it is LanesView } as LanesView
        assertEquals(9, lanes.peaks.size)
        // drag the trim's start from its edge to a quarter
        val x0 = a.dp(56f)
        touch(lanes, MotionEvent.ACTION_DOWN, x0 + 1, a.dp(60f))
        touch(lanes, MotionEvent.ACTION_MOVE, x0 + (lanes.width - x0) / 4, a.dp(60f))
        touch(lanes, MotionEvent.ACTION_UP, x0 + (lanes.width - x0) / 4, a.dp(60f))
        val es = (stack(a).last() as EditPage)
        val sess = EditPage::class.java.getDeclaredField("s").let { it.isAccessible = true; it.get(es) as Session }
        assertTrue("trimStart ${sess.trimStart}", sess.trimStart in (sess.frames / 4 - Layout.RATE / 5)..(sess.frames / 4 + Layout.RATE / 5))
        // mute T2 by its M box, select T3
        val rowH = minOf(a.dp(56f), (lanes.height - a.dp(18f) - a.dp(4f) * 9) / 9)
        val top2 = a.dp(18f) + a.dp(4f) + 1 * (rowH + a.dp(4f))
        touch(lanes, MotionEvent.ACTION_DOWN, a.dp(5f), top2 + rowH - a.dp(5f))
        touch(lanes, MotionEvent.ACTION_UP, a.dp(5f), top2 + rowH - a.dp(5f))
        assertTrue(sess.lane("T2")!!.mute)
        assertEquals(1, lanes.selected)
        // a card dragged: the gain of T2
        val gain = find(edit.view) { it is Card && (it as Card).label == "GAIN" } as Card
        touch(gain, MotionEvent.ACTION_DOWN, 10f, 10f)
        touch(gain, MotionEvent.ACTION_MOVE, 10f + a.dp(110f), 10f)
        touch(gain, MotionEvent.ACTION_UP, 10f + a.dp(110f), 10f)
        assertTrue("gain ${sess.lane("T2")!!.gainDb}", sess.lane("T2")!!.gainDb > 10f)
        draw(edit.view)
        // the latency card moves the voice lanes
        val lat = find(edit.view) { it is Card && (it as Card).label == "LATENCY" } as Card
        assertEquals(1000 * 1000.0 / 44100, lat.value.toDouble(), 0.1)
        // play a moment (Robolectric's AudioTrack), then stop
        val play = find(edit.view) { it is RoundBtn && (it as RoundBtn).kind == RoundBtn.Kind.PLAY }!!
        play.performClick(); idle(300)
        val stop = find(edit.view) { it is RoundBtn && (it as RoundBtn).kind == RoundBtn.Kind.STOP }!!
        stop.performClick()

        // EXPORT: the ZIP, shared
        val exp = find(edit.view) { it is TextBtn && (it as TextBtn).text == "EXPORT" }!!
        exp.performClick()
        val ex = stack(a).last() as ExportPage
        draw(ex.view)
        val share = find(ex.view) { it is TextBtn && (it as TextBtn).text == "SHARE" }!!
        share.performClick()
        idle(4000)
        val started = shadowOf(a).nextStartedActivity
        assertNotNull("share sheet", started)
        assertEquals(Intent.ACTION_CHOOSER, started.action)
        val zipFile = File(ExportProvider.dir(a), "TAKE_01_stems.zip")
        assertTrue(zipFile.isFile)
        ZipFile(zipFile).use { z ->
            val names = z.entries().toList().map { it.name }
            assertEquals(listOf("T1", "T2", "T3", "T4", "T5", "T6", "FX", "VOX", "DRY", "MIX").map { "TAKE_01/$it.wav" } + "TAKE_01/README.txt", names)
        }
        // the provider serves it
        val u = ExportProvider.uri(zipFile)
        Robolectric.setupContentProvider(ExportProvider::class.java, ExportProvider.AUTHORITY)
        a.contentResolver.openFileDescriptor(u, "r")!!.use { assertEquals(zipFile.length(), it.statSize) }
        a.contentResolver.query(u, null, null, null, null)!!.use { cur -> cur.moveToFirst(); assertEquals("TAKE_01_stems.zip", cur.getString(0)) }
        assertEquals("application/zip", a.contentResolver.getType(u))
        // MERGE
        val seg = find(ex.view) { it is Seg } as Seg
        (seg.getChildAt(1)).performClick()
        draw(ex.view)
        share.performClick()
        idle(3000)
        val mix = File(ExportProvider.dir(a), "TAKE_01_mix.wav")
        assertTrue(mix.isFile)
        WavSource(mix).use { assertEquals(sess.length, it.frames); assertEquals(2, it.channels); assertEquals(24, it.info.bits) }

        // back to HOME; the take is listed with its edits saved
        a.onBackPressed(); a.onBackPressed()
        assertTrue(stack(a).last() is HomePage)
        val saved = Session.load(s.dir)
        assertTrue(saved.lane("T2")!!.mute)
        assertTrue(saved.trimStart > 0)
        c.pause().stop().destroy()
    }

    @Test fun spanishAndMeters() {
        java.util.Locale.setDefault(java.util.Locale.forLanguageTag("es-AR"))
        val c = Robolectric.buildActivity(MainActivity::class.java).setup()
        val a = c.get()
        val home = stack(a).last() as HomePage
        draw(home.view)
        assertTrue(texts(home.view).toString(), texts(home.view).contains("No encuentro el FM-1"))
        val armed = HashSet<Int>()
        val m = MetersView(a, 10, { it in armed }) { if (!armed.remove(it)) armed.add(it) }
        val pk = IntArray(10) { it * 3000 }; val cl = BooleanArray(10) { it == 9 }
        m.push(pk, cl)
        draw(m, 1000, 600)
        touch(m, MotionEvent.ACTION_UP, 150f, 300f)
        assertTrue(1 in armed)
        c.pause().stop().destroy()
        java.util.Locale.setDefault(java.util.Locale.US)
        Lang.init("en")
    }
}
