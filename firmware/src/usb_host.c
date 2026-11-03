/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio), Melodee's usb_host.c (1.6.x, c14912b), which retains USB host enumeration
 * and input contributed by recursivefieldinstruments (2026). JIANT FM V10.41: input only (no host OUT), JIANT's names.
 * (V10.41, a user's ask) MENU > SYSTEM > USB > HOST: the port as the host of a class compliant USB-MIDI keyboard or
 * controller, alone or behind a hub; what it plays comes in as USB-MIDI does (source 1: CLK USB+BT, LEARN, MAP alike).
 * Never saved: every power-on is a device, so the installer and the editor always work. While HOST is on there is no
 * USB device (no editor, stems, MIDI to a computer). The FM-1 does not power the port (VBUS is not driven): a keyboard
 * powered on its own, or behind a powered hub.
 *   main loop (uh_task)   waits for a device, resets the bus, enumerates it with blocking EP0 transfers (a hub: powers
 *                         its ports, resets those with something on them, tries each device until one has a USB-MIDI
 *                         streaming IN endpoint). A failed probe waits for unplug; an idle hub is polled for changes.
 *   TIMER5 (uh_poll)      once uh.run, the SIE is the ISR's alone: EP1 IN asked for a packet (ReqPkt), read when
 *                         RxPktRdy, its channel / clock events into the MIDI ring. A stall or a run of errors sets
 *                         uh.fault; the main loop then resets the bus and looks again.
 * Register sequences after the AC79 SDK's USB0 host code (usb_h_sie_init, usb_h_chirp_and_reset, usb0_h_ep_read,
 * usb_ctlXfer), as Melodee: EP0 data through ep0buf, the target in FADDR. Not driven: VBUS, low-speed devices. */
#if FELUCCA_USB_HOST
#include "usb_host_desc.h"

enum { S_DEVCTL = 15, S_RXTYPE = 26, S_RXINTERVAL = 27 };
enum { UH_MSG_NONE, UH_MSG_ON, UH_MSG_OFF, UH_MSG_MIDI, UH_MSG_NOMIDI, UH_MSG_LOW, UH_MSG_LOST, UH_MSG_AUDIO };
#define UH_FIRST_ADDR 1u
/* (JIANT: the app slot is full) everything of the main loop here lives in the Bluetooth segment (app.ld .btapp.text,
 * as bt.c's BTAPP_FN): run only once bt_hw_seg() said the segment is this build's (main.c); uh_poll (TIMER5) stays */
#define UH_SEG __attribute__((section(".btapp.text"), used, noinline))
#define UH_MAX_ERRORS 16u

static uint8_t uh_rx[64 + 4] __attribute__((aligned(4)));
static uint8_t uh_cfg[512];                                  /* a configuration, cut short if longer (V10.53: 512, an
                                                              * audio interface's is long) */
static struct {
    uint8_t want;                                       /* the menu (main loop) */
    uint8_t on;                                         /* the port is a host */
    volatile uint8_t run;                               /* TIMER5 owns the MIDI IN endpoint */
    volatile uint8_t fault;                             /* TIMER5 -> main loop: reset the bus and look again */
    uint8_t seen, next_addr, dev, tried;
    uint8_t hub_addr, hub_mps, hub_ports, hub_poll_port;
    uint8_t line_idle;
    uint8_t msg;                                        /* one UH_MSG_* for the main loop to show */
    uint8_t aud;                                        /* (V10.53) the device is an audio interface: its channels (1, 2) */
    volatile uint8_t errors;
    uint32_t t_ms, retry_ms, hub_poll_ms;
    volatile uint32_t pkts, events;
} uh;

UH_SEG static void uh_wait(uint32_t ms)                        /* main loop only */
{
    uint32_t t = fm1_ms;
    while (fm1_ms - t < ms)
        fm1_wdt_feed();
}

/* CSR0 (host) until want_set bits are all set and want_clr are all clear: >= 0 the CSR0, < 0 a stall / error / timeout */
UH_SEG static int uh_csr0(uint32_t want_set, uint32_t want_clr)
{
    uint32_t t = fm1_ms, v;
    for (;;) {
        v = sie_rd(S_CSR0);
        if (v & 0x04u) {                                /* RxStall */
            sie_wr(S_CSR0, 0);
            return -2;
        }
        if (v & 0x10u) {                                /* Error: no answer */
            sie_wr(S_CSR0, 0);
            return -1;
        }
        if ((v & want_set) == want_set && !(v & want_clr))
            return (int)v;
        if (fm1_ms - t > 500u)
            return -3;
        fm1_wdt_feed();
    }
}

/* One control transfer to addr: SETUP, the IN data stage if any, the status stage. Bytes read, or < 0 (no OUT data
 * stage: none used here) */
UH_SEG static int uh_ctl(uint8_t addr, uint8_t rt, uint8_t rq, uint16_t val, uint16_t idx, uint16_t len, uint8_t *out,
                  uint8_t mps)
{
    uint32_t got = 0, n, i;
    int r;
    sie_wr(S_FADDR, addr);
    sie_wr(S_INDEX, 0);
    ep0buf[0] = rt;
    ep0buf[1] = rq;
    ep0buf[2] = (uint8_t)val;
    ep0buf[3] = (uint8_t)(val >> 8);
    ep0buf[4] = (uint8_t)idx;
    ep0buf[5] = (uint8_t)(idx >> 8);
    ep0buf[6] = (uint8_t)len;
    ep0buf[7] = (uint8_t)(len >> 8);
    fm1_usb_ep0_send(ep0buf, 8);
    sie_wr(S_CSR0, 0x0Au);                              /* SetupPkt | TxPktRdy */
    r = uh_csr0(0, 0x02u);
    if (r < 0)
        return r;
    if (len && (rt & 0x80u)) {
        while (got < len) {
            sie_wr(S_CSR0, 0x20u);                      /* ReqPkt */
            r = uh_csr0(0x01u, 0);
            if (r < 0)
                return r;
            n = sie_rd(S_COUNT0);
            if (n > 64u)
                n = 64u;
            fm1_usb_rx_sync();
            for (i = 0; i < n && got + i < len; i++)
                out[got + i] = ep0buf[i];
            sie_wr(S_CSR0, 0);                          /* RxPktRdy done */
            got += n;
            if (n < mps)
                break;
        }
        if (got > len)
            got = len;
        sie_wr(S_CSR0, 0x42u);                          /* status: StatusPkt | TxPktRdy (zero-length OUT) */
        r = uh_csr0(0, 0x02u);
    } else {
        sie_wr(S_CSR0, 0x60u);                          /* status: StatusPkt | ReqPkt (zero-length IN) */
        r = uh_csr0(0x01u, 0);
        if (r >= 0)
            sie_wr(S_CSR0, 0);
    }
    return r < 0 ? r : (int)got;
}

#if FELUCCA_UAC
/* (V10.53) a control transfer with a short OUT data stage (SET_CUR: an audio interface's sample rate): SETUP, the data,
 * the status (a zero-length IN). 0 done, < 0 a stall / error / timeout */
UH_SEG static int uh_ctl_out(uint8_t addr, uint8_t rt, uint8_t rq, uint16_t val, uint16_t idx, const uint8_t *data,
                             uint32_t len)
{
    uint32_t i;
    int r;
    sie_wr(S_FADDR, addr);
    sie_wr(S_INDEX, 0);
    ep0buf[0] = rt; ep0buf[1] = rq;
    ep0buf[2] = (uint8_t)val; ep0buf[3] = (uint8_t)(val >> 8);
    ep0buf[4] = (uint8_t)idx; ep0buf[5] = (uint8_t)(idx >> 8);
    ep0buf[6] = (uint8_t)len; ep0buf[7] = 0;
    fm1_usb_ep0_send(ep0buf, 8);
    sie_wr(S_CSR0, 0x0Au);                              /* SetupPkt | TxPktRdy */
    if ((r = uh_csr0(0, 0x02u)) < 0)
        return r;
    for (i = 0; i < len; i++)
        ep0buf[i] = data[i];
    fm1_usb_ep0_send(ep0buf, len);
    sie_wr(S_CSR0, 0x02u);                              /* TxPktRdy: the data */
    if ((r = uh_csr0(0, 0x02u)) < 0)
        return r;
    sie_wr(S_CSR0, 0x60u);                              /* status: StatusPkt | ReqPkt (zero-length IN) */
    r = uh_csr0(0x01u, 0);
    if (r >= 0)
        sie_wr(S_CSR0, 0);
    return r < 0 ? r : 0;
}
#endif

static int uh_hub_probe(uint8_t addr, uint8_t mps, uint8_t cfgval, uh_midi_t *m);

/* The device at address 0 (just reset): an address, its configuration, and if it has a USB-MIDI streaming IN endpoint
 * that interface selected. 1 found, 0 not (a hub below depth 0, a device that fails, an output-only one) */
UH_SEG static int uh_probe(uint8_t depth, uh_midi_t *m)
{
    uint8_t d8[8], mps, cls, addr = uh.next_addr++;
    uint32_t total;
    int r;
    r = uh_ctl(0, 0x80, 6, 0x0100, 0, 8, d8, 8);        /* GET_DESCRIPTOR(device), first 8 bytes */
    if (r < 8)
        return 0;
    mps = d8[7] == 16u || d8[7] == 32u || d8[7] == 64u ? d8[7] : 8u;
    cls = d8[4];
    if (uh_ctl(0, 0x00, 5, addr, 0, 0, 0, mps) < 0)     /* SET_ADDRESS */
        return 0;
    uh_wait(10);
    r = uh_ctl(addr, 0x80, 6, 0x0200, 0, 9, uh_cfg, mps);
    if (r < 9)
        return 0;
    total = (uint32_t)(uh_cfg[2] | uh_cfg[3] << 8);
    if (total > sizeof uh_cfg)
        total = sizeof uh_cfg;
    if (total > 9u) {
        r = uh_ctl(addr, 0x80, 6, 0x0200, 0, (uint16_t)total, uh_cfg, mps);
        if (r < 9)
            return 0;
        total = (uint32_t)r;
    }
    if (cls == 9u)                                      /* a hub */
        return depth == 0u ? uh_hub_probe(addr, mps, uh_cfg[5], m) : 0;
    uh.aud = 0;
    if (!uh_parse_midi(uh_cfg, total, m) || !m->in_ep) {   /* (JIANT: input only) */
#if FELUCCA_UAC
        uh_aud_t a;                                     /* (V10.53) not MIDI: an audio interface's input? */
        static const uint8_t F441[3] = {0x44, 0xAC, 0x00}, F48[3] = {0x80, 0xBB, 0x00};
        if (!uh_parse_uac(uh_cfg, total, &a))
            return 0;
        if (uh_ctl(addr, 0x00, 9, a.cfg, 0, 0, 0, mps) < 0 ||   /* SET_CONFIGURATION, SET_INTERFACE (the capture alt) */
            uh_ctl(addr, 0x01, 11, a.alt, a.iface, 0, 0, mps) < 0)
            return 0;
        if (a.setf)                                     /* SET_CUR SAMPLING_FREQ_CONTROL: 44,100, or (V10.57) 48,000 when the
                                                         * interface has no 44,100 (a refusal: tried as is) */
            uh_ctl_out(addr, 0x22, 1, 0x0100, (uint16_t)(0x80u | a.ep), a.r48 ? F48 : F441, 3);
        uin_r48 = a.r48;
        *m = (uh_midi_t){0};
        m->in_ep = a.ep;
        m->in_type = 1;                                 /* isochronous */
        m->in_mps = a.mps;
        uh.aud = a.ch;
        uh.dev = addr;
        return 1;
#else
        return 0;
#endif
    }
    if (uh_ctl(addr, 0x00, 9, m->cfg, 0, 0, 0, mps) < 0)   /* SET_CONFIGURATION */
        return 0;
    if (m->alt && uh_ctl(addr, 0x01, 11, m->alt, m->iface, 0, 0, mps) < 0)   /* SET_INTERFACE */
        return 0;
    uh.dev = addr;
    return 1;
}

/* this downstream port only; the hub and the other devices kept. Each port reuses its address on reconnect */
UH_SEG static int uh_hub_port_probe(uint8_t p, uint32_t st, uh_midi_t *m)
{
    uint8_t b[4], addr = uh.hub_addr, mps = uh.hub_mps;
    uint32_t t;
    if (!(st & 0x0001u) || (st & 0x0200u))              /* nothing there, or low speed */
        return 0;
    if (uh_ctl(addr, 0x23, 3, 4, p, 0, 0, mps) < 0)    /* SET_FEATURE(PORT_RESET) */
        return 0;
    t = fm1_ms;
    for (;;) {
        if (uh_ctl(addr, 0xA3, 0, 0, p, 4, b, mps) >= 4 && (b[2] & 0x10u))   /* C_PORT_RESET */
            break;
        if (fm1_ms - t > 500u)
            return 0;
        uh_wait(10);
    }
    uh_ctl(addr, 0x23, 1, 20, p, 0, 0, mps);            /* CLEAR_FEATURE(C_PORT_RESET) */
    if (uh_ctl(addr, 0xA3, 0, 0, p, 4, b, mps) < 4 || !(b[0] & 0x02u))   /* enabled? */
        return 0;
    uh_wait(20);
    uh.next_addr = (uint8_t)(UH_FIRST_ADDR + p);
    return uh_probe(1, m);
}

/* read and acknowledge connection changes; a hub that stops answering is left alone until the root is replugged */
UH_SEG static int uh_hub_status(uint8_t p, uint8_t *b)
{
    if (uh_ctl(uh.hub_addr, 0xA3, 0, 0, p, 4, b, uh.hub_mps) < 4 ||
        ((b[2] & 1u) && uh_ctl(uh.hub_addr, 0x23, 1, 16, p, 0, 0, uh.hub_mps) < 0)) {
        uh.hub_addr = 0;
        return 0;
    }
    return 1;
}

UH_SEG static int uh_hub_probe(uint8_t addr, uint8_t mps, uint8_t cfgval, uh_midi_t *m)
{
    uint8_t b[16], ports, p;
    uint32_t good;
    if (uh_ctl(addr, 0x00, 9, cfgval, 0, 0, 0, mps) < 0)
        return 0;
    if (uh_ctl(addr, 0xA0, 6, 0x2900, 0, 9, b, mps) < 7)   /* hub descriptor */
        return 0;
    ports = b[2] > 7u ? 7u : b[2];
    good = (uint32_t)b[5] * 2u;                         /* bPwrOn2PwrGood, in 2 ms */
    for (p = 1; p <= ports; p++)
        uh_ctl(addr, 0x23, 3, 8, p, 0, 0, mps);         /* SET_FEATURE(PORT_POWER) */
    uh_wait((good < 100u ? 100u : good) + 100u);
    uh.hub_addr = addr;
    uh.hub_mps = mps;
    uh.hub_ports = ports;
    uh.hub_poll_port = 1;
    uh.hub_poll_ms = fm1_ms;
    for (p = 1; p <= ports; p++) {
        if (!uh_hub_status(p, b))
            return 0;
        if (uh_hub_port_probe(p, (uint32_t)(b[0] | b[1] << 8), m))
            return 1;
    }
    return 0;
}

UH_SEG static void uh_start_ep(const uh_midi_t *m)
{
    sie_wr(S_FADDR, uh.dev);
    sie_wr(S_INDEX, 1);
#if FELUCCA_UAC
    if (uh.aud) {                                       /* (V10.53) an audio interface: its packets into IN+OUT's buffer */
        fm1_usb_ep_rxbuf(1, ep4rx);
        uin.gen++;                                      /* (the reader drops what the ring held: a new stream) */
    } else
#endif
    fm1_usb_ep_rxbuf(1, uh_rx);
    sie_wr(S_RXMAXP, 0xFF);
    sie_wr(S_RXCSR1, 0x90);                             /* clear data toggle, flush */
    sie_wr(S_RXCSR2, 0);
    sie_wr(S_RXTYPE, (uint32_t)(m->in_type << 4 | m->in_ep));
    sie_wr(S_RXINTERVAL, m->in_type == 3u ? m->in_interval : m->in_type == 1u ? 1u : 0u);   /* (iso: every frame) */
    sie_wr(S_RXCSR1, 0x20u);                            /* ReqPkt */
    fm1_usb_ep_enable(1u << 1);
    uh.errors = 0;
    uh.fault = 0;
    RING_PUBLISH();
    uh.run = 1;
    uh_aud_on = uh.aud != 0u;                           /* (V10.53: audio.c reads the ring) */
    uin_feed = (uint8_t)(usb_duplex | uh_aud_on);
}

UH_SEG static void uh_session_end(void)                        /* the input stops: every note it held let go */
{
    uh.run = 0;
    uh_aud_on = 0;
    uin_feed = usb_duplex;
    RING_PUBLISH();
    midi_in_overflow = 1;                               /* (seq.c: every note off, the ring emptied) */
}

UH_SEG static void uh_host_up(void)                            /* the port as a host with nothing attached yet */
{
    usb.timeouts = 0;
    fm1_usb_reset();
    uh_wait(25);
    fm1_usb_host_attach(ep0buf);
    uh_wait(25);
    sie_wr(S_INTRUSBE, 0);
    sie_wr(S_INTRTX1E, 0);
    sie_wr(S_INTRTX2E, 0);
    sie_wr(S_INTRRX1E, 0);
    sie_wr(S_INTRRX2E, 0);
    fm1_usb_ep_enable(0x1Fu);
    sie_wr(S_FADDR, 0);
    sie_wr(S_DEVCTL, 0x03u);                            /* session + host request */
    uh.seen = 0;
    uh.line_idle = 0;
    uh.hub_addr = 0;
}

UH_SEG static void uh_connect(void)                            /* a device on the line: a bus reset, its MIDI endpoint found */
{
    uh_midi_t m;
    if (sie_rd(S_DEVCTL) & 0x20u) {                     /* low speed */
        if (!uh.tried)
            uh.msg = UH_MSG_LOW;
        uh.tried = 1;
        return;
    }
    sie_wr(S_POWER, 0x08u);                             /* bus reset */
    uh_wait(30);
    sie_wr(S_POWER, 0);
    uh_wait(30);
    uh.next_addr = UH_FIRST_ADDR;
    if (uh_probe(0, &m)) {
        uh_start_ep(&m);
        uh.msg = uh.aud ? UH_MSG_AUDIO : UH_MSG_MIDI;
        uh.tried = 0;
    } else {
        if (!uh.tried)
            uh.msg = UH_MSG_NOMIDI;
        uh.tried = 1;
    }
}

/* main loop: the menu's HOST (uh.want) switches the port; as a host this finds and keeps a controller */
UH_SEG static void uh_task(uint32_t now)
{
    if (uh.want != uh.on) {
        usb.up = 0;                                     /* TIMER5 leaves the SIE to this context */
        usb.config = 0;
        usb.suspended = 0;
        uh_session_end();
        uh.fault = 0;
        uh.tried = 0;
        fm1_usb_off();
        if (uh.want) {
            usb.sx_on = 0;
            uh.on = 1;
            uh_host_up();
            uh.retry_ms = now;
            uh.msg = UH_MSG_ON;
        } else {
            uh.on = 0;
            usb.replug = 1;                             /* (usb_retry: a device again, USB_REPLUG_MS later) */
            uh.msg = UH_MSG_OFF;
        }
        return;
    }
    if (!uh.on)
        return;
    if (uh.fault) {                                     /* a stall, or the controller stopped answering */
        uh_session_end();
        uh.fault = 0;
        uh_host_up();
        uh.retry_ms = now + 500u;
        uh.msg = UH_MSG_LOST;
        return;
    }
    if (uh.run)
        return;
    if ((int32_t)(now - uh.t_ms) < 20 || (int32_t)(now - uh.retry_ms) < 0)
        return;
    uh.t_ms = now;
    if (!fm1_usb_line()) {
        uh.seen = 0;
        if (uh.line_idle < 5u) uh.line_idle++;
        if (uh.line_idle >= 5u) {                       /* (a root disconnect, debounced) */
            uh.tried = 0;
            uh.hub_addr = 0;
        }
        return;
    }
    uh.line_idle = 0;
    if (uh.tried) {
        if (uh.hub_addr && uh.hub_ports && (uint32_t)(now - uh.hub_poll_ms) >= 100u) {
            uint8_t b[4], p = uh.hub_poll_port;
            uh_midi_t m;
            uh.hub_poll_ms = now;
            uh.hub_poll_port = p == uh.hub_ports ? 1u : (uint8_t)(p + 1u);
            if (uh_hub_status(p, b) && (b[2] & 1u) && (b[0] & 1u) && !(b[1] & 2u)) {
                uh_wait(100);
                if (uh_hub_status(p, b) && uh_hub_port_probe(p, (uint32_t)(b[0] | b[1] << 8), &m)) {
                    uh_start_ep(&m);
                    uh.msg = uh.aud ? UH_MSG_AUDIO : UH_MSG_MIDI;
                    uh.tried = 0;
                }
            }
        }
        return;
    }
    if (uh.seen < 5u) {                                 /* 100 ms of a steady line before the bus reset */
        uh.seen++;
        return;
    }
    uh.seen = 0;
    uh_connect();
}

static void uh_fail(void)
{
    uh.run = 0;
    uh.fault = 1;
}

static void uh_poll(void)                               /* TIMER5 ISR, 2 kHz, while uh.run */
{
    uint32_t csr, n, i;
    if (!fm1_usb_line()) {
        if (++uh.line_idle >= 16u) uh_fail();           /* (8 ms of idle samples, not one during a packet's EOP) */
        return;
    }
    uh.line_idle = 0;
    sie_wr(S_INDEX, 1);
    csr = sie_rd(S_RXCSR1);
    if (csr & 0x40u) {                                  /* RxStall: the endpoint is gone */
        uh_fail();
        return;
    }
#if FELUCCA_UAC
    if (uh.aud) {                                       /* (V10.53) an audio interface: a packet each frame into the ring
                                                         * (usb.c uin_take, as IN+OUT's), a mono one doubled; an
                                                         * isochronous error is a lost packet, never a fault */
        if (csr & 0x01u) {
            n = sie_rd(S_RXCOUNT1) | (sie_rd(S_RXCOUNT2) << 8);
            if (n > sizeof ep4rx)
                n = sizeof ep4rx;
            fm1_usb_rx_sync();
            if (uh.aud == 1u) {
                const uint16_t *m16 = (const uint16_t *)ep4rx;
                uint32_t f = n / 2u > UI_MAXF_RX ? UI_MAXF_RX : n / 2u;
                for (i = f; i--;)
                    ep4rx[i] = (uint32_t)m16[i] | (uint32_t)m16[i] << 16;
                n = f * 4u;
            }
            uin_take(ep4rx, n);
            uh.pkts++;
            sie_wr(S_RXCSR1, 0x10u);
            sie_wr(S_RXCSR1, 0x20u);
            return;
        }
        if ((csr & 0x0Cu) || !(csr & 0x20u))
            sie_wr(S_RXCSR1, 0x20u);
        return;
    }
#endif
    if (csr & 0x0Cu) {                                  /* Error / DataError */
        if (++uh.errors >= UH_MAX_ERRORS) {
            uh_fail();
            return;
        }
        sie_wr(S_RXCSR1, 0x20u);                        /* again (the data toggle stays) */
        return;
    }
    if (csr & 0x01u) {                                  /* a packet in uh_rx */
        n = sie_rd(S_RXCOUNT1) | (sie_rd(S_RXCOUNT2) << 8);
        if (n > 64u)
            n = 64u;
        fm1_usb_rx_sync();
        if (MQ - (mi_w - mi_r) < EP1_ROOM)
            return;                                     /* the ring too full: the packet stays, taken next poll */
        for (i = 0; i + 3u < n; i += 4u) {
            uint32_t p = uh_midi_ok((uint32_t)uh_rx[i] | (uint32_t)uh_rx[i + 1] << 8 |
                                    (uint32_t)uh_rx[i + 2] << 16 | (uint32_t)uh_rx[i + 3] << 24);
            if (p) {
                midi_enqueue(p, 1u);
                uh.events++;
            }
        }
        uh.pkts++;
        uh.errors = 0;
        sie_wr(S_RXCSR1, 0x10u);                        /* the packet released */
        sie_wr(S_RXCSR1, 0x20u);                        /* and the next asked for */
        return;
    }
    if (!(csr & 0x20u))
        sie_wr(S_RXCSR1, 0x20u);
}
#endif /* FELUCCA_USB_HOST */
