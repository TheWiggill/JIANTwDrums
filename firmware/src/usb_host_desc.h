/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Retains USB host descriptor parsing contributed by recursivefieldinstruments (2026). */
/* USB host: what Melodee reads from an attached device, with no register access (usb_host.c uses it; the host
 * tests include it alone).
 *   uh_parse_midi   a configuration descriptor -> the first MIDI 1.0 streaming alternate setting with usable
 *                   IN and/or OUT endpoints (an output-only synth is usable too)
 *   uh_midi_ok      one USB-MIDI event packet -> 1 when it is a channel message or a clock / transport byte */
#pragma once
#include <stdint.h>

typedef struct {
    uint8_t found;               /* a usable endpoint of a MIDI streaming interface was found */
    uint8_t cfg;                 /* bConfigurationValue */
    uint8_t iface, alt;          /* its interface and alternate setting */
    uint8_t in_ep;               /* endpoint number (without the direction bit) */
    uint8_t in_type;             /* 2 bulk, 3 interrupt */
    uint8_t in_interval;         /* bInterval (interrupt only) */
    uint16_t in_mps;
    uint8_t out_ep;              /* the OUT endpoint of the same interface, 0 = none */
    uint8_t out_type, out_interval;
    uint16_t out_mps;
} uh_midi_t;

/* d / len: the configuration descriptor as read (possibly cut short: a descriptor that does not fit is ignored) */
static int uh_parse_midi(const uint8_t *d, uint32_t len, uh_midi_t *m)
{
    uint32_t i = 0;
    uint8_t cfg = 0, in_midi = 0;
    *m = (uh_midi_t){0};
    while (i + 2u <= len) {
        uint32_t l = d[i];
        if (l < 2u || i + l > len)
            break;
        if (d[i + 1] == 2u && l >= 9u) {
            cfg = d[i + 5];
        } else if (d[i + 1] == 4u && l >= 9u) {
            if (m->found) break;                        /* never combine interfaces or alternate settings */
            m->cfg = cfg;
            m->iface = d[i + 2];
            m->alt = d[i + 3];
            in_midi = d[i + 5] == 1u && d[i + 6] == 3u && d[i + 7] == 0u;
        } else if (d[i + 1] == 5u && l >= 7u && in_midi) {
            uint8_t ea = d[i + 2], type = d[i + 3] & 3u;
            uint16_t mps = (uint16_t)(d[i + 4] | d[i + 5] << 8);
            /* Full-speed USB-MIDI 1.0 events are four bytes. The buffers hold at most 64 bytes. */
            if ((ea & 15u) && !(ea & 0x70u) && mps >= 4u && mps <= 64u &&
                ((type == 2u && (mps == 8u || mps == 16u || mps == 32u || mps == 64u)) ||
                 (type == 3u && d[i + 6]))) {
                if ((ea & 0x80u) && !m->in_ep) {
                    m->found = 1;
                    m->in_ep = ea & 0x0Fu;
                    m->in_type = type;
                    m->in_interval = d[i + 6];
                    m->in_mps = mps;
                } else if (!(ea & 0x80u) && !m->out_ep) {
                    m->found = 1;
                    m->out_ep = ea & 0x0Fu;
                    m->out_type = type;
                    m->out_interval = d[i + 6];
                    m->out_mps = mps & ~3u;          /* send whole event packets only */
                }
            }
        }
        i += l;
    }
    return m->found;
}

/* A USB-MIDI event packet: cable / CIN, then the MIDI bytes. Channel messages (CIN 8..E whose status matches)
 * and the one-byte clock, start, continue and stop are passed on; SysEx and everything else is not: a
 * controller has no business with the SysEx commands (update, editor) that usb.c's own parser answers. The
 * cable number is dropped. Returns the packet to enqueue, or 0. */
static uint32_t uh_midi_ok(uint32_t pkt)
{
    uint32_t cin = pkt & 15u, st = (pkt >> 8) & 0xFFu;
    pkt &= ~0xF0u;
    if (cin == 0xFu)
        return st == 0xF8u || st == 0xFAu || st == 0xFBu || st == 0xFCu ? pkt : 0u;
    if (cin >= 8u && cin <= 0xEu && (st >> 4) == cin && !((pkt >> 16) & 0x80u) &&
        ((cin == 0xCu || cin == 0xDu) || !(pkt & 0x80000000u)))
        return pkt;
    return 0u;
}

/* (JIANT V10.53) a USB audio class 1.0 capture interface (an audio interface's input: Behringer UM2 / UCA, ...): the
 * first AudioStreaming alternate setting with an isochronous IN endpoint whose format is PCM type I, 16-bit, mono or
 * stereo, and whose sample rates take 44,100 Hz (the FM-1's own: no conversion) or, since V10.57, 48,000 (converted as it is read). setf: more than one rate (or a range):
 * the rate must be set (SET_CUR to the endpoint) */
typedef struct {
    uint8_t found, cfg, iface, alt;
    uint8_t ep;                  /* endpoint number (without the direction bit) */
    uint8_t ch, setf;
    uint8_t r48;                 /* (V10.57) no 44,100 among its rates, 48,000 taken: the FM-1 converts (usb.c uac_in_fill) */
    uint16_t mps;
} uh_aud_t;

static int uh_parse_uac(const uint8_t *d, uint32_t len, uh_aud_t *a)
{
    uint32_t i = 0;
    uint8_t cfg = 0, as = 0, fmt_ok = 0, ch = 0, setf = 0, iface = 0, alt = 0, r48 = 0;
    *a = (uh_aud_t){0};
    while (i + 2u <= len) {
        uint32_t l = d[i];
        if (l < 2u || i + l > len)
            break;
        if (d[i + 1] == 2u && l >= 9u) {
            cfg = d[i + 5];
        } else if (d[i + 1] == 4u && l >= 9u) {         /* an interface: AudioStreaming, an alternate setting with data */
            as = d[i + 5] == 1u && d[i + 6] == 2u && d[i + 3] != 0u;
            iface = d[i + 2];
            alt = d[i + 3];
            fmt_ok = 0;
        } else if (d[i + 1] == 0x24u && l >= 8u && as && d[i + 2] == 2u) {   /* FORMAT_TYPE */
            uint32_t ft = d[i + 7], k, has = 0, h48 = 0;
            if (d[i + 3] == 1u && (d[i + 4] == 1u || d[i + 4] == 2u) && d[i + 5] == 2u && d[i + 6] == 16u) {
                if (!ft && l >= 14u) {                  /* a continuous range */
                    uint32_t lo = d[i + 8] | d[i + 9] << 8 | (uint32_t)d[i + 10] << 16;
                    uint32_t hi = d[i + 11] | d[i + 12] << 8 | (uint32_t)d[i + 13] << 16;
                    has = lo <= 44100u && hi >= 44100u;
                    h48 = lo <= 48000u && hi >= 48000u;
                }
                for (k = 0; ft && k < ft && 8u + 3u * k + 3u <= l; k++) {
                    uint32_t f = d[i + 8 + 3 * k] | d[i + 9 + 3 * k] << 8 | (uint32_t)d[i + 10 + 3 * k] << 16;
                    has |= f == 44100u;
                    h48 |= f == 48000u;
                }
            }
            fmt_ok = (uint8_t)(has || h48);              /* (V10.57: 48,000 too, when there is no 44,100) */
            r48 = (uint8_t)(!has && h48);
            ch = d[i + 4];
            setf = ft != 1u;
        } else if (d[i + 1] == 5u && l >= 7u && as && fmt_ok && !a->found) {
            uint8_t ea = d[i + 2];
            uint16_t mps = (uint16_t)(d[i + 4] | d[i + 5] << 8);
            if ((ea & 0x80u) && (ea & 15u) && (d[i + 3] & 3u) == 1u && mps >= 88u && mps <= 200u) {
                a->found = 1;
                a->cfg = cfg;
                a->iface = iface;
                a->alt = alt;
                a->ep = ea & 15u;
                a->ch = ch;
                a->setf = setf;
                a->r48 = r48;
                a->mps = mps;
                if (ch == 2u)                           /* (stereo: taken; a mono one is kept looking past) */
                    break;
            }
        }
        i += l;
    }
    return a->found;
}
