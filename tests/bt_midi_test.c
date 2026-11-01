/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.13) Host test of BLE-MIDI to and from JIANT's MIDI (firmware/bt/bt_midi.h, with the stack's codec
 * firmware/bt/ble/ble_midi.c, tests/ble_midi_test.c):
 *   in    packets as phone apps send them (several messages, running status with and without its timestamp, real
 *         time inside a message, SysEx over two packets, system common) decoded and taken as USB's midi_in_event
 *         takes them: channel messages and F8 FA FB FC into the input ring, SysEx, F1 F2 F3 F6 FE FF dropped
 *   out   MIDI OUT's ring: nothing kept while nobody listens, full at 64 (the rest dropped), the order kept; drained
 *         as ble_att.c att_midi_out packs it (a 20-octet notification: the MTU 23): the header, the timestamps, running
 *         status inside a packet, the clock; a second packet when the first is full, decoded back to the same events */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/bt/ble/ble_midi.c"
#include "../firmware/bt/bt_midi.h"

static int fails;
static void check(const char *what, int ok)
{
    printf("%-100s %s\n", what, ok ? "ok" : "FAIL");
    fails += !ok;
}
#define P(cin, a, b, c) ((uint32_t)(cin) | (uint32_t)(a) << 8 | (uint32_t)(b) << 16 | (uint32_t)(c) << 24)

static uint32_t ring[256];
static unsigned nring, nseen;
static void sink(void *ctx, uint32_t pkt, uint16_t ts)
{
    (void)ctx, (void)ts;
    nseen++;
    if ((pkt = bt_midi_accept(pkt)) != 0u)
        ring[nring++ % 256u] = pkt;
}
static struct ble_midi_dec D;
#define IN(...)                                                                                                    \
    do {                                                                                                           \
        static const uint8_t b_[] = {__VA_ARGS__};                                                                 \
        ble_midi_dec(&D, b_, (uint16_t)sizeof b_, sink, 0);                                                        \
    } while (0)

static void test_in(void)
{
    ble_midi_dec_reset(&D);
    nring = nseen = 0;
    IN(0x80, 0x81, 0x90, 0x3C, 0x64, 0x40, 0x50, 0x82, 0x80, 0x3C, 0x00);   /* note on, running status, note off */
    check("in: two notes on (running status without a timestamp) and a note off, in order",
          nring == 3 && ring[0] == P(9, 0x90, 0x3C, 0x64) && ring[1] == P(9, 0x90, 0x40, 0x50) &&
              ring[2] == P(8, 0x80, 0x3C, 0));
    nring = 0;
    IN(0x80, 0x81, 0xB3, 0x01, 0x40, 0x83, 0x07, 0x7F, 0x84, 0xC3, 0x05, 0x85, 0xE3, 0x00, 0x40);   /* CC, CC (RS), PC, bend */
    check("in: CC, CC by running status with its timestamp, PROGRAM CHANGE, PITCH BEND (channel 4)",
          nring == 4 && ring[0] == P(0xB, 0xB3, 1, 0x40) && ring[1] == P(0xB, 0xB3, 7, 0x7F) &&
              ring[2] == P(0xC, 0xC3, 5, 0) && ring[3] == P(0xE, 0xE3, 0, 0x40));
    nring = 0;
    IN(0x80, 0x81, 0xFA, 0x81, 0xF8, 0x82, 0x90, 0x3C, 0x83, 0xF8, 0x64, 0x84, 0xFC);   /* START, clock inside a note, STOP */
    check("in: START, the clock (also inside a note's octets), the note whole, STOP",
          nring == 5 && ring[0] == P(0xF, 0xFA, 0, 0) && ring[1] == P(0xF, 0xF8, 0, 0) && ring[2] == P(0xF, 0xF8, 0, 0) &&
              ring[3] == P(9, 0x90, 0x3C, 0x64) && ring[4] == P(0xF, 0xFC, 0, 0));
    nring = nseen = 0;
    IN(0x80, 0x81, 0xF0, 0x7D, 0x01, 0x02);                                  /* SysEx over two packets */
    IN(0x80, 0x03, 0x04, 0x82, 0xF7, 0x83, 0xFB);
    check("in: SysEx over two packets decoded but dropped (USB only), the CONTINUE after it kept",
          nseen == 4 && nring == 1 && ring[0] == P(0xF, 0xFB, 0, 0));
    nring = nseen = 0;
    IN(0x80, 0x81, 0xF2, 0x10, 0x20, 0x82, 0xF6, 0x83, 0xF3, 0x01, 0x84, 0xFE, 0x85, 0xFF, 0x86, 0xF1, 0x12);
    check("in: SONG POSITION, TUNE REQUEST, SONG SELECT, ACTIVE SENSING, RESET, MTC dropped", nseen == 6 && nring == 0);
    check("in: a channel message with a data octet over 127 dropped (as USB)",
          !bt_midi_accept(P(9, 0x90, 0x80, 1)) && !bt_midi_accept(P(9, 0x90, 1, 0x80)) &&
              !bt_midi_accept(P(9, 0x80, 1, 1)) && bt_midi_accept(P(0xD, 0xD0, 0x10, 0)) == P(0xD, 0xD0, 0x10, 0));
    nring = 0;
    IN(0x40, 0x80, 0x90, 0x3C, 0x64);                                         /* no header bit: not a packet */
    check("in: a packet without a valid header ignored", nring == 0);
}

static struct bt_oq O;
static void test_out(void)
{
    uint8_t buf[20];
    struct ble_midi_enc e;
    uint32_t pkt, due, i, n, ok;
    memset(&O, 0, sizeof O);
    bt_oq_put(&O, P(9, 0x90, 60, 100));
    check("out: nothing kept while no central listens", O.w == 0 && !bt_oq_peek(&O, &pkt, &due));
    O.on = 1;
    for (i = 0; i < 70u; i++)
        bt_oq_put(&O, P(9, 0x90, i, 1));
    for (ok = 1, n = 0; bt_oq_peek(&O, &pkt, &due); n++) {
        ok &= pkt == P(9, 0x90, n, 1);
        bt_oq_pop(&O);
    }
    check("out: the ring holds 64, the rest dropped, the order kept", n == 64u && ok);
    /* att_midi_out: what is waiting packed into notifications of 20 octets (MTU 23 - 3), events at 1000 ms */
    bt_oq_put(&O, P(9, 0x90, 60, 100));
    bt_oq_put(&O, P(9, 0x90, 64, 90));
    bt_oq_put(&O, P(0xF, 0xF8, 0, 0));
    bt_oq_put(&O, P(8, 0x80, 60, 0));
    ble_midi_enc_begin(&e, buf, sizeof buf);
    while (bt_oq_peek(&O, &pkt, &due) && ble_midi_enc_add(&e, pkt, 1000u))
        bt_oq_pop(&O);
    {
        static const uint8_t want[] = {0x87, 0xE8, 0x90, 60, 100, 0xE8, 64, 90, 0xE8, 0xF8, 0xE8, 0x80, 60, 0};
        check("out: one packet: the header (1000 ms: high 7), each event's timestamp (low 0x68), running status, the clock",
              ble_midi_enc_len(&e) == sizeof want && !memcmp(buf, want, sizeof want) && O.r == O.w);
    }
    for (i = 0; i < 12u; i++)
        bt_oq_put(&O, P(0xB, 0xB0, i, i));
    for (n = 0, ok = 1; bt_oq_peek(&O, &pkt, &due); n++) {
        ble_midi_enc_begin(&e, buf, sizeof buf);
        while (bt_oq_peek(&O, &pkt, &due) && ble_midi_enc_add(&e, pkt, 2000u + n))
            bt_oq_pop(&O);
        nring = 0;
        ble_midi_dec_reset(&D);
        ble_midi_dec(&D, buf, ble_midi_enc_len(&e), sink, 0);
        for (i = 0; i < nring; i++)
            ok &= ring[i] == P(0xB, 0xB0, ring[0] >> 16 & 0x7Fu, ring[0] >> 16 & 0x7Fu) + (i << 16) + (i << 24);
        if (n > 4u)
            break;
    }
    check("out: twelve CCs over packets of 20 octets (the first full), decoded back in order", n >= 2u && n <= 4u && ok &&
                                                                                                O.r == O.w);
}

int main(void)
{
    test_in();
    test_out();
    printf("%s\n", fails ? "BT-MIDI: FAILED" : "BT-MIDI: all passed");
    return fails != 0;
}
