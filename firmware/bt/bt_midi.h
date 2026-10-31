/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.13) BLE-MIDI to and from JIANT's MIDI, the parts without hardware: what a received message becomes in the MIDI
 * input ring (src/usb.c midi_in_q, as USB's midi_in_event takes it), and the ring of what goes out (MIDI OUT, src/usb.c
 * midi_out_event, the audio interrupt its producer; the BLE interrupts its consumer: bt_seg.c ble_app_midi_peek / pop,
 * packed per connection event by the stack's ble_att.c att_midi_out). Used by the app, the segment and
 * tests/ble_midi_test.c. */
#ifndef BT_MIDI_H
#define BT_MIDI_H
#include <stdint.h>
#ifndef RING_PUBLISH
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#endif

/* a received USB-MIDI event (the stack's decoder: cable 0, ble_midi.h) -> what goes into the input ring, 0 dropped:
 * channel messages, the clock, START CONTINUE STOP (USB's midi_in_event; no SysEx over Bluetooth: the editor and the
 * updater use USB) */
static inline uint32_t bt_midi_accept(uint32_t pkt)
{
    uint32_t cin = pkt & 15u, st = pkt >> 8 & 0xFFu;
    if (cin == 0xFu)
        return st == 0xF8u || st == 0xFAu || st == 0xFBu || st == 0xFCu ? 0xFu | st << 8 : 0u;
    return cin >= 8u && cin <= 0xEu && st >> 4 == cin && !(pkt & 0x80800000u) ? pkt : 0u;
}

/* MIDI OUT's copy for Bluetooth: one producer (the audio interrupt, which preempts the BLE ones), one consumer */
#define BT_OQ 64u
struct bt_oq {
    uint32_t q[BT_OQ];
    volatile uint32_t w, r;
    volatile uint8_t on;                 /* a central listens (its MIDI notifications on): the segment sets it */
};
static inline void bt_oq_put(struct bt_oq *o, uint32_t pkt)   /* (full or nobody listening: dropped) */
{
    if (o->on && o->w - o->r < BT_OQ) {
        o->q[o->w % BT_OQ] = pkt;
        RING_PUBLISH();
        o->w++;
    }
}
static inline int bt_oq_peek(const struct bt_oq *o, uint32_t *pkt)
{
    if (o->r == o->w)
        return 0;
    RING_PUBLISH();
    *pkt = o->q[o->r % BT_OQ];
    return 1;
}
static inline void bt_oq_pop(struct bt_oq *o)
{
    RING_PUBLISH();
    o->r++;
}
#endif
