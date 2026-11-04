// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: what the FM-1 sends in each USB mode (docs/MANUAL.md, "Stems" and "Stems with a voice") and how the
// channels group into the lanes the app records, one WAV a lane:
//   STEMS     (0.7.1, 8 ch):  T1..T6 (each track mono, before the master), FX L R (the effects' return)
//   STEMS+IN  (0.8.0, 10 ch): the same, then VOX (the phone's mic as the last live track mixed it, in its own channel)
//                             and DRY (the mic as it came: the reference the latency is measured against)
//   STEREO / CONSOLE / IN+OUT (2 ch): the master L R
// The sum of T1..T6 and FX (and VOX) is the mix as the FM-1's buses hear it.
package fm.jiant.stems.core

enum class LaneKind { TRACK, FX, VOICE, DRY, MIX, OTHER }

/** a lane: its id (the WAV's name), the device channels it takes (0-based) */
class LaneSpec(val id: String, val kind: LaneKind, val channels: IntArray)

enum class UsbMode { STEMS, STEMS_IN, STEREO, OTHER }

object Layout {
    const val RATE = 44100

    fun mode(channels: Int): UsbMode = when (channels) {
        8 -> UsbMode.STEMS
        10 -> UsbMode.STEMS_IN
        2 -> UsbMode.STEREO
        else -> UsbMode.OTHER
    }

    fun lanes(channels: Int): List<LaneSpec> {
        val l = ArrayList<LaneSpec>()
        when (mode(channels)) {
            UsbMode.STEMS, UsbMode.STEMS_IN -> {
                for (t in 0 until 6) l.add(LaneSpec("T${t + 1}", LaneKind.TRACK, intArrayOf(t)))
                l.add(LaneSpec("FX", LaneKind.FX, intArrayOf(6, 7)))
                if (channels == 10) {
                    l.add(LaneSpec("VOX", LaneKind.VOICE, intArrayOf(8)))
                    l.add(LaneSpec("DRY", LaneKind.DRY, intArrayOf(9)))
                }
            }
            UsbMode.STEREO -> l.add(LaneSpec("MIX", LaneKind.MIX, intArrayOf(0, 1)))
            UsbMode.OTHER -> for (c in 0 until channels) l.add(LaneSpec("CH${c + 1}", LaneKind.OTHER, intArrayOf(c)))
        }
        return l
    }

    /** the short name under a channel's meter: 1..6, L R, V, D */
    fun channelTag(channels: Int, c: Int): String = when (mode(channels)) {
        UsbMode.STEMS, UsbMode.STEMS_IN -> when {
            c < 6 -> "${c + 1}"
            c == 6 -> "L"
            c == 7 -> "R"
            c == 8 -> "V"
            else -> "D"
        }
        UsbMode.STEREO -> if (c == 0) "L" else "R"
        UsbMode.OTHER -> "${c + 1}"
    }

    /** the lane a device channel belongs to */
    fun laneOf(lanes: List<LaneSpec>, c: Int): Int = lanes.indexOfFirst { l -> l.channels.contains(c) }
}

/** the USB Audio Class 1 descriptors the FM-1 sends: its streaming formats */
object Uac {
    class Format(val channels: Int, val subframeBytes: Int, val bits: Int)

    /** the type I format of interface [ifNum], alternate setting [alt], from the device's raw descriptors (as
     * UsbDeviceConnection.getRawDescriptors gives them); null when there is none */
    fun format(raw: ByteArray?, ifNum: Int, alt: Int): Format? {
        if (raw == null) return null
        var curIf = -1
        var curAlt = -1
        var streaming = false
        var i = 0
        while (i + 1 < raw.size) {
            val len = raw[i].toInt() and 0xFF
            if (len < 2) break
            val type = raw[i + 1].toInt() and 0xFF
            if (type == 4 && len >= 9 && i + 3 < raw.size) {               // INTERFACE
                curIf = raw[i + 2].toInt() and 0xFF
                curAlt = raw[i + 3].toInt() and 0xFF
                streaming = i + 6 < raw.size && raw[i + 5].toInt() == 1 && raw[i + 6].toInt() == 2   // AUDIO, STREAMING
            } else if (type == 0x24 && len >= 8 && i + 6 < raw.size && streaming && curIf == ifNum && curAlt == alt
                       && raw[i + 2].toInt() == 2 && raw[i + 3].toInt() == 1) {   // CS_INTERFACE FORMAT_TYPE, type I
                return Format(raw[i + 4].toInt() and 0xFF, raw[i + 5].toInt() and 0xFF, raw[i + 6].toInt() and 0xFF)
            }
            i += len
        }
        return null
    }

    /** the channel count from the endpoint's largest packet when the descriptors say nothing: 16-bit, 46 frames a
     * packet (STEMS: 736 bytes, 720 = 45 before the FM-1's 0.9.8; STEMS+IN: 920; stereo: 184 or 196 with 49) */
    fun channelsFromPacket(maxPacket: Int): Int {
        for (f in intArrayOf(45, 46, 49, 44)) if (maxPacket % (f * 2) == 0 && maxPacket / (f * 2) in 1..16) return maxPacket / (f * 2)
        return 0
    }

    /** (1.1) the FM-1 before 0.9.8 in STEMS: an endpoint of 720 bytes (45 frames) while it sends packets of 46 frames
     * (736 bytes) whenever its clock gets ahead; the phone drops each one and stalls, and the FM-1 drops whole blocks
     * (a click in every stem, ~100 a second). Nothing the app can mend: the FM-1 wants its update */
    fun oldStemsFirmware(channels: Int, maxPacket: Int): Boolean = channels == 8 && maxPacket < 46 * 16
}
