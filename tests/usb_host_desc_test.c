/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.41) USB > HOST: what the host takes from an attached device (firmware/src/usb_host_desc.h, Melodee's): a
 * keyboard's MIDI IN endpoint found, a mass-storage stick and an output-only synth refused (JIANT's host is input
 * only: usb_host.c uh_probe needs in_ep), and the event filter (channel messages, clock and transport; no SysEx).
 * Packets as USB-MIDI sends them, lowest byte first: cable / CIN, status, data 1, data 2.
 *   cc -O1 -o /tmp/uhd tests/usb_host_desc_test.c && /tmp/uhd */
#include <stdio.h>
#include <string.h>
#include "../firmware/src/usb_host_desc.h"
static int bad;
static void check(const char *what, int ok) { printf("usbhost: %-70s %s\n", what, ok ? "ok" : "FAIL"); bad += !ok; }
int main(void)
{
    /* a class compliant keyboard: config 1, audio control interface 0, MIDI streaming interface 1 (class 1, subclass 3),
     * a bulk IN endpoint 0x81 (64) and a bulk OUT 0x02 (64) */
    static const uint8_t KBD[] = {9, 2, 0, 0, 2, 1, 0, 0x80, 50,
                                  9, 4, 0, 0, 0, 1, 1, 0, 0,
                                  9, 4, 1, 0, 2, 1, 3, 0, 0,
                                  7, 5, 0x81, 2, 64, 0, 0,
                                  7, 5, 0x02, 2, 64, 0, 0};
    static const uint8_t STICK[] = {9, 2, 0, 0, 1, 1, 0, 0x80, 50,
                                    9, 4, 0, 0, 2, 8, 6, 80, 0,
                                    7, 5, 0x81, 2, 64, 0, 0,
                                    7, 5, 0x02, 2, 64, 0, 0};
    static const uint8_t SYNTH_OUT[] = {9, 2, 0, 0, 1, 1, 0, 0x80, 50,
                                        9, 4, 0, 0, 1, 1, 3, 0, 0,
                                        7, 5, 0x01, 2, 64, 0, 0};
    static const uint8_t INTR[] = {9, 2, 0, 0, 1, 2, 0, 0x80, 50,
                                   9, 4, 3, 1, 1, 1, 3, 0, 0,
                                   7, 5, 0x83, 3, 8, 0, 4};
    uh_midi_t m;
    check("a keyboard: its MIDI streaming IN endpoint (1, bulk, 64) and OUT", uh_parse_midi(KBD, sizeof KBD, &m) &&
          m.cfg == 1 && m.iface == 1 && m.alt == 0 && m.in_ep == 1 && m.in_type == 2 && m.in_mps == 64 && m.out_ep == 2);
    check("a mass-storage stick: no MIDI", !uh_parse_midi(STICK, sizeof STICK, &m));
    check("an output-only synth: found, but no IN (JIANT's host takes input only: refused)",
          uh_parse_midi(SYNTH_OUT, sizeof SYNTH_OUT, &m) && !m.in_ep);
    check("an interrupt IN endpoint on alternate setting 1, config 2", uh_parse_midi(INTR, sizeof INTR, &m) &&
          m.cfg == 2 && m.iface == 3 && m.alt == 1 && m.in_ep == 3 && m.in_type == 3 && m.in_interval == 4);
    check("a descriptor cut short: no crash, nothing found", !uh_parse_midi(KBD, 20, &m));
    check("note on, cable 1: passed, the cable dropped", uh_midi_ok(0x643C9019u) == 0x643C9009u);
    check("note off, CC, program change, bend: passed",
          uh_midi_ok(0x003C8008u) && uh_midi_ok(0x4007B00Bu) && uh_midi_ok(0x0005C00Cu) && uh_midi_ok(0x4000E00Eu));
    check("clock, start, continue, stop: passed; active sensing: not",
          uh_midi_ok(0x0000F80Fu) && uh_midi_ok(0x0000FA0Fu) && uh_midi_ok(0x0000FB0Fu) && uh_midi_ok(0x0000FC0Fu) &&
          !uh_midi_ok(0x0000FE0Fu));
    check("SysEx (start, continue, end): never", !uh_midi_ok(0x7E00F004u) && !uh_midi_ok(0x00F7F7u | 7u));
    check("a status that does not match its CIN, a data byte with bit 7: refused",
          !uh_midi_ok(0x643C8009u) && !uh_midi_ok(0xE43C9009u));
    {   /* (V10.53) audio interfaces' capture (USB audio 1.0): a UM2-like one (an output and an input interface, each
         * alt 0 empty and alt 1 streaming; input: 2 ch, 16 bit, 44.1 / 48 kHz, iso IN 0x82, 192), a 24-bit only one
         * (refused), a mono 16-bit with one rate 44.1 (taken, no SET_CUR), a 48 kHz only one (refused) */
        static const uint8_t UM2[] = {9, 2, 0, 0, 3, 1, 0, 0x80, 50,
                                      9, 4, 0, 0, 0, 1, 1, 0, 0,                       /* audio control */
                                      9, 4, 1, 0, 0, 1, 2, 0, 0,                       /* playback: alt 0 */
                                      9, 4, 1, 1, 1, 1, 2, 0, 0,                       /* playback: alt 1 */
                                      7, 0x24, 1, 1, 1, 1, 0,
                                      14, 0x24, 2, 1, 2, 2, 16, 2, 0x44, 0xAC, 0, 0x80, 0xBB, 0,
                                      9, 5, 0x01, 9, 192, 0, 1, 0, 0,                  /* iso OUT: not ours */
                                      9, 4, 2, 0, 0, 1, 2, 0, 0,                       /* capture: alt 0 */
                                      9, 4, 2, 1, 1, 1, 2, 0, 0,                       /* capture: alt 1 */
                                      7, 0x24, 1, 4, 1, 1, 0,
                                      14, 0x24, 2, 1, 2, 2, 16, 2, 0x44, 0xAC, 0, 0x80, 0xBB, 0,
                                      9, 5, 0x82, 5, 192, 0, 1, 0, 0};
        static const uint8_t D24[] = {9, 2, 0, 0, 2, 1, 0, 0x80, 50,
                                      9, 4, 1, 1, 1, 1, 2, 0, 0,
                                      11, 0x24, 2, 1, 2, 3, 24, 1, 0x44, 0xAC, 0,
                                      9, 5, 0x81, 5, 0x10, 1, 1, 0, 0};
        static const uint8_t MONO[] = {9, 2, 0, 0, 2, 1, 0, 0x80, 50,
                                       9, 4, 1, 1, 1, 1, 2, 0, 0,
                                       11, 0x24, 2, 1, 1, 2, 16, 1, 0x44, 0xAC, 0,
                                       9, 5, 0x83, 5, 96, 0, 1, 0, 0};
        static const uint8_t F48[] = {9, 2, 0, 0, 2, 1, 0, 0x80, 50,
                                      9, 4, 1, 1, 1, 1, 2, 0, 0,
                                      11, 0x24, 2, 1, 2, 2, 16, 1, 0x80, 0xBB, 0,
                                      9, 5, 0x81, 5, 192, 0, 1, 0, 0};
        uh_aud_t a;
        check("UM2-like: the capture alt (iface 2 alt 1, ep 2, stereo, SET_CUR, 192)",
              uh_parse_uac(UM2, sizeof UM2, &a) && a.cfg == 1 && a.iface == 2 && a.alt == 1 && a.ep == 2 && a.ch == 2 &&
              a.setf && a.mps == 192);
        check("24-bit only: refused", !uh_parse_uac(D24, sizeof D24, &a));
        check("mono 16-bit at 44.1 only: taken, no SET_CUR", uh_parse_uac(MONO, sizeof MONO, &a) && a.ch == 1 && !a.setf && a.ep == 3);
        check("48 kHz only: refused", !uh_parse_uac(F48, sizeof F48, &a));
        check("a keyboard is no audio input", !uh_parse_uac(KBD, sizeof KBD, &a));
    }
    printf("usbhost: %s\n", bad ? "FAILED" : "all ok");
    return bad != 0;
}
