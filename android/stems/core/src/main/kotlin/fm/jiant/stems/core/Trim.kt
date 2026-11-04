// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems (1.1): the editor's trim edges as numbers, apart from the view (LanesView draws and touches them), so the
// JVM tests can drag them. 1.0 took a touch anywhere within 22 dp of an edge, on any lane, as a grab, and moved the edge
// to the finger at the first jitter: a tap near the end to move the playhead cut the take short, and an edge dragged to
// the screen's end stopped short of the take's end (the last pixel is not the last frame). Now: an edge is taken by its
// grip in the ruler, or on a lane only right at its line; it moves with the finger from where it was taken (no jump),
// only once the finger has really moved; within a few pixels of either end of the timeline it is the end exactly.
package fm.jiant.stems.core

object Trim {
    const val NONE = 0
    const val START = 1
    const val END = 2

    /** the timeline from x0 to width: frame [f] of [frames] */
    fun xOf(f: Long, frames: Long, x0: Float, width: Float): Float = x0 + (width - x0) * (f.toDouble() / maxOf(1L, frames)).toFloat()

    /** the frame at [x]; within [snap] pixels of either end, that end exactly */
    fun frameAt(x: Float, frames: Long, x0: Float, width: Float, snap: Float): Long {
        if (x >= width - snap) return frames
        if (x <= x0 + snap) return 0L
        return ((x - x0).toDouble() / (width - x0) * frames).toLong().coerceIn(0L, frames)
    }

    /** which edge a touch at [x], [y] takes: in the ruler (y < [rulerH]) the nearer within [near]; on the lanes only
     * within [near] / 3 of its line (else the touch moves the playhead) */
    fun grab(x: Float, y: Float, a: Float, b: Float, rulerH: Float, near: Float): Int {
        val r = if (y < rulerH) near else near / 3f
        val da = kotlin.math.abs(x - a)
        val db = kotlin.math.abs(x - b)
        if (da < r && da <= db) return START
        if (db < r) return END
        return NONE
    }

    /** a drag of edge [edge] that was at frame [from] when taken at x [downX], the finger now at [x]: the session's
     * new trim (start, end), [minLen] frames apart at least; nothing moves until the finger has gone [slop] pixels */
    fun drag(s: Session, edge: Int, from: Long, downX: Float, x: Float, x0: Float, width: Float, snap: Float, slop: Float, minLen: Long) {
        if (kotlin.math.abs(x - downX) < slop) return
        val at = xOf(from, s.frames, x0, width) + (x - downX)
        val f = frameAt(at, s.frames, x0, width, snap)
        when (edge) {
            START -> s.trimStart = minOf(f, s.trimEnd - minLen).coerceAtLeast(0L)
            END -> s.trimEnd = maxOf(f, s.trimStart + minLen).coerceAtMost(s.frames)
        }
    }

    /** the trim leaves part of the take out */
    fun trimmed(s: Session): Boolean = s.trimStart > 0L || s.trimEnd < s.frames
}
