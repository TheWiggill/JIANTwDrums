// SPDX-License-Identifier: GPL-3.0-only
// JIANT Stems: finding the FM-1 among the USB devices and opening its audio the way JIANT Probe's tests 5 and 7 did
// on the phone: the audio-streaming interfaces with an isochronous endpoint (IN: the stems; OUT in STEMS+IN: the
// mic), their format from the raw descriptors, claimed (the kernel's driver let go of), alternate setting 1 set, the
// sample rate confirmed, then usbstream.c streams on the connection's file descriptor.
package fm.jiant.stems

import android.hardware.usb.UsbConstants
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint
import android.hardware.usb.UsbInterface
import android.hardware.usb.UsbManager
import fm.jiant.stems.core.Layout
import fm.jiant.stems.core.Uac
import java.util.Locale

object UsbNative {
    init { System.loadLibrary("jiantstems") }

    /** submits the URBs: the IN endpoint (packets of [maxp] bytes, frames of [frameBytes]), and the OUT one when
     * [epOut] >= 0. 0 or -errno */
    @JvmStatic external fun start(fd: Int, ep: Int, maxp: Int, epOut: Int, frameBytes: Int): Int
    /** the bytes that came in (whole frames) within [timeoutMs]; -errno when the stream broke */
    @JvmStatic external fun pump(out: ByteArray, timeoutMs: Int, stats: IntArray): Int
    @JvmStatic external fun stop()
    /** the phone's mic (mono 16 bit, 44.1 kHz) for the OUT side */
    @JvmStatic external fun pushMic(pcm: ShortArray, n: Int)
}

/** an FM-1 (or another USB audio device) seen on the bus, before it is opened */
class Fm1(val dev: UsbDevice, val inOn: UsbInterface, val inOff: UsbInterface?, val ep: UsbEndpoint,
          val outOn: UsbInterface?, val outOff: UsbInterface?, val epOut: UsbEndpoint?) {
    val name: String get() = dev.productName ?: "USB audio"
    /** the channel count the IN packet size implies (the descriptors, read once opened, have the last word) */
    val guessChannels get() = Uac.channelsFromPacket(ep.maxPacketSize)
    val looksLikeFm1: Boolean get() {
        val n = (dev.productName ?: "").uppercase(Locale.ROOT)
        return n.contains("JIANT") || n.contains("FELUCCA") || n.contains("FM")
    }

    companion object {
        /** the streaming interface with an isochronous endpoint going [dir], its alt 0, the endpoint */
        private fun stream(dv: UsbDevice, dir: Int): Triple<UsbInterface, UsbInterface?, UsbEndpoint>? {
            for (i in 0 until dv.interfaceCount) {
                val it = dv.getInterface(i)
                if (it.interfaceClass != UsbConstants.USB_CLASS_AUDIO || it.interfaceSubclass != 2) continue
                for (e in 0 until it.endpointCount) {
                    val x = it.getEndpoint(e)
                    if (x.type != UsbConstants.USB_ENDPOINT_XFER_ISOC || x.direction != dir) continue
                    var off: UsbInterface? = null
                    for (j in 0 until dv.interfaceCount) {
                        val o = dv.getInterface(j)
                        if (o.id == it.id && o.alternateSetting == 0) off = o
                    }
                    return Triple(it, off, x)
                }
            }
            return null
        }

        /** the USB audio devices that can record, the FM-1 first */
        fun find(um: UsbManager): List<Fm1> {
            val l = ArrayList<Fm1>()
            for (dv in um.deviceList.values) {
                val i = stream(dv, UsbConstants.USB_DIR_IN) ?: continue
                val o = stream(dv, UsbConstants.USB_DIR_OUT)
                l.add(Fm1(dv, i.first, i.second, i.third, o?.first, o?.second, o?.third))
            }
            l.sortBy { if (it.looksLikeFm1) 0 else 1 }
            return l
        }
    }
}

/** the FM-1 opened and streaming (one at a time: usbstream.c keeps one stream) */
class Fm1Stream private constructor(val fm1: Fm1, private val conn: UsbDeviceConnection, val channels: Int, val withOut: Boolean) {
    val mode get() = Layout.mode(channels)
    /** (1.1) an FM-1 whose STEMS packets do not fit its own endpoint (firmware 0.9.7 and older): the stems lose audio */
    val oldStemsFirmware get() = Uac.oldStemsFirmware(channels, fm1.ep.maxPacketSize)

    fun pump(buf: ByteArray, timeoutMs: Int, stats: IntArray) = UsbNative.pump(buf, timeoutMs, stats)

    fun close() {
        UsbNative.stop()
        try {
            if (withOut) fm1.outOff?.let { conn.setInterface(it) }
            fm1.inOff?.let { conn.setInterface(it) }
        } catch (_: Throwable) { }
        try { if (withOut) conn.releaseInterface(fm1.outOn) } catch (_: Throwable) { }
        try { conn.releaseInterface(fm1.inOn) } catch (_: Throwable) { }
        conn.close()
    }

    class Failure(val why: String) : Exception(why)

    companion object {
        /** opens [f] (the USB permission already granted); throws [Failure] with what went wrong */
        fun open(um: UsbManager, f: Fm1): Fm1Stream {
            val conn = um.openDevice(f.dev) ?: throw Failure("open")
            try {
                val raw = conn.rawDescriptors
                val fmt = Uac.format(raw, f.inOn.id, f.inOn.alternateSetting)
                val ch = fmt?.channels ?: f.guessChannels
                if (ch <= 0 || (fmt != null && fmt.subframeBytes != 2)) {
                    throw Failure(if (fmt == null) "? ch" else "${fmt.channels} ch, ${fmt.bits} bit")
                }
                // the OUT side only as the FM-1's STEMS+IN has it: stereo 16 bit (what usbstream.c sends)
                var out = ch >= 10 && f.outOn != null && f.epOut != null      // (IN+OUT's stereo output: not ours to feed)
                if (out) {
                    val of = Uac.format(raw, f.outOn!!.id, f.outOn.alternateSetting)
                    out = of == null || (of.channels == 2 && of.subframeBytes == 2)
                }
                if (!conn.claimInterface(f.inOn, true)) throw Failure("claim")
                if (out && !conn.claimInterface(f.outOn, true)) out = false
                if (!conn.setInterface(f.inOn)) throw Failure("SET_INTERFACE")
                // the sample rate (SET_CUR on the endpoint, as the probe's test 5): the FM-1 runs at 44.1 kHz only
                conn.controlTransfer(0x22, 0x01, 0x0100, f.ep.address, byteArrayOf(0x44, 0xAC.toByte(), 0x00), 3, 500)
                if (out && !conn.setInterface(f.outOn)) out = false
                val r = UsbNative.start(conn.fileDescriptor, f.ep.address, f.ep.maxPacketSize, if (out) f.epOut!!.address else -1, ch * 2)
                if (r != 0) throw Failure("start $r")
                return Fm1Stream(f, conn, ch, out)
            } catch (t: Throwable) {
                try { conn.releaseInterface(f.inOn) } catch (_: Throwable) { }
                try { f.outOn?.let { conn.releaseInterface(it) } } catch (_: Throwable) { }
                conn.close()
                throw if (t is Failure) t else Failure(t.toString())
            }
        }
    }
}
