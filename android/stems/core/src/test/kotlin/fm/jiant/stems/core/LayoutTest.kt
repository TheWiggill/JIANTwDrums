// SPDX-License-Identifier: GPL-3.0-only
package fm.jiant.stems.core

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class LayoutTest {
    @Test fun theFm1Modes() {
        assertEquals(listOf("T1", "T2", "T3", "T4", "T5", "T6", "FX"), Layout.lanes(8).map { it.id })
        assertEquals(listOf("T1", "T2", "T3", "T4", "T5", "T6", "FX", "VOX", "DRY"), Layout.lanes(10).map { it.id })
        assertEquals(listOf("MIX"), Layout.lanes(2).map { it.id })
        assertEquals(4, Layout.lanes(4).size)
        assertArrayEquals(intArrayOf(6, 7), Layout.lanes(10)[6].channels)
        assertArrayEquals(intArrayOf(9), Layout.lanes(10)[8].channels)
        assertEquals(UsbMode.STEMS_IN, Layout.mode(10))
        assertEquals("L", Layout.channelTag(10, 6)); assertEquals("D", Layout.channelTag(10, 9)); assertEquals("3", Layout.channelTag(8, 2))
        assertEquals(6, Layout.laneOf(Layout.lanes(8), 7))
        // every channel in exactly one lane
        for (n in intArrayOf(2, 8, 10, 3)) assertEquals((0 until n).toList(), Layout.lanes(n).flatMap { it.channels.toList() }.sorted())
    }

    /** the configuration descriptor's pieces the parser reads: an AC interface (whose input terminal has
     * subtype 2 too, and must not be taken for a format), an AS interface's alt 0 and alt 1 with its format */
    private fun descriptors(ch: Int, sub: Int): ByteArray {
        val b = ArrayList<Int>()
        fun add(vararg v: Int) { v.forEach { b.add(it) } }
        add(9, 2, 0, 0, 3, 1, 0, 0x80, 50)                                      // configuration (lengths not checked)
        add(9, 4, 0, 0, 0, 1, 1, 0, 0)                                          // interface 0: audio control
        add(12, 0x24, 2, 1, 0x01, 0x02, 0, 2, 3, 0, 0, 0)                     // input terminal: "2 channels" at [4]
        add(9, 4, 1, 0, 0, 1, 2, 0, 0)                                          // interface 1 alt 0: no endpoint
        add(9, 4, 1, 1, 1, 1, 2, 0, 0)                                          // interface 1 alt 1
        add(7, 0x24, 1, 1, 1, 1, 0)                                             // AS_GENERAL
        add(11, 0x24, 2, 1, ch, sub, sub * 8, 1, 0x44, 0xAC, 0x00)             // FORMAT_TYPE I, 44100
        add(9, 5, 0x81, 5, 0xD0, 2, 1, 0, 0)                                     // endpoint
        return ByteArray(b.size) { b[it].toByte() }
    }

    @Test fun formatFromTheDescriptors() {
        val f = Uac.format(descriptors(10, 2), 1, 1)!!
        assertEquals(10, f.channels); assertEquals(2, f.subframeBytes); assertEquals(16, f.bits)
        assertNull(Uac.format(descriptors(10, 2), 1, 0))
        assertNull(Uac.format(descriptors(10, 2), 0, 0))
        assertNull(Uac.format(null, 1, 1))
        assertNull(Uac.format(byteArrayOf(9, 4, 1, 1), 1, 1))                    // truncated
        assertNotEquals(null, Uac.format(descriptors(8, 2), 1, 1))
    }

    @Test fun channelsFromThePacketSize() {
        assertEquals(8, Uac.channelsFromPacket(720))
        assertEquals(8, Uac.channelsFromPacket(736))                           // (the FM-1's 0.9.8: 46 frames)
        assertEquals(10, Uac.channelsFromPacket(920))
        assertEquals(2, Uac.channelsFromPacket(196))
        assertEquals(2, Uac.channelsFromPacket(184))
        assertEquals(0, Uac.channelsFromPacket(7))
    }

    /** (1.1) STEMS from an FM-1 before 0.9.8: its 720-byte endpoint cannot carry the 46-frame packets it sends */
    @Test fun oldStemsFirmwareTold() {
        assertTrue(Uac.oldStemsFirmware(8, 720))
        assertFalse(Uac.oldStemsFirmware(8, 736))
        assertFalse(Uac.oldStemsFirmware(10, 920))
        assertFalse(Uac.oldStemsFirmware(2, 184))
    }

    @Test fun heatRamp() {
        assertEquals(0xFF00D0FF.toInt(), Heat.argb(0))
        assertEquals(0xFFFFFFFF.toInt(), Heat.argb(256))
        assertEquals(0xFFF72831.toInt(), Heat.argb(128))                      // the red (ui_draw.c)
    }

    @Test fun everyStringInBothLanguagesWithTheSameArguments() {
        val spec = Regex("%(\\d+\\$)?[-+ 0#,(]*\\d*(\\.\\d+)?[a-zA-Z%]")
        for (t in T.values()) {
            assertTrue(t.name, t.en.isNotBlank() && t.es.isNotBlank())
            assertEquals(t.name, spec.findAll(t.en).map { it.value.last() }.toList(), spec.findAll(t.es).map { it.value.last() }.toList())
        }
        Lang.spanish = true
        assertEquals("TOMA 03", T.TAKE_NAME.s(3))
        assertEquals("Exportando… 50%", T.EXPORTING.s(50))
        Lang.spanish = false
        assertEquals("TAKE 03", T.TAKE_NAME.s(3))
        Lang.init("es"); assertTrue(Lang.spanish); Lang.init("en")
    }
}
