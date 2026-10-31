/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.12) The Bluetooth segment: Optimist's BLE stack (Henri David, w0ts/optimist; the hardware-tested copy in
 * charlesvestal/fm1-base bt/ble, GPL-3.0-only OR MPL-2.0, used here under GPL-3.0-only) with the WL82 radio drivers
 * (hal/fm1_ble.h, hal/fm1_ble_rf.h: Melodee's versions, Kerem Kilic, GPL-3.0-only; hal/fm1_rf_cal.h and
 * hal/ble_rf_tables_v15.h from felucca-bsp, Apache-2.0), as a translation unit of its own that the linker puts in
 * flash at 0xEA000 (firmware/app.ld BTSEG, XIP 0x020E6000), outside the app slot, run in place through the plain XIP
 * window (hal/fm1_flash.h fl_plain_window_init, set by src/bt_hw.c before the first call). docs/BLE_FEASIBILITY.md
 * route C2.
 *
 * Peripheral only: advertising as "JIANT FM-1" with the BLE-MIDI service (connectable), no scanner, no central, no
 * pairing (BLE_SMP_LEGACY 0: a Pairing Request gets Pairing Not Supported), no encryption. The radio is calibrated by
 * its own measurement at the first start (fm1_rf_cal.h: records 106 / 107 / 108 the SDK board's constants, 187
 * measured, ~160 ms), as Melodee and fm1-base do on units whose stock calibration is gone.
 *
 * What the app (firmware/src/bt.c) calls, only after it checked the segment installed is this build's (bt_head below:
 * its length and CRC-32, which tools/build.py also writes into the app's bt_want):
 *   bt_seg_start(addr)   the first ON: the vectors (IRQ 45 / 29 at priority 2, below the audio and TIMER5), the radio's
 *                        start-up and calibration, the stack, advertising. The caller has the interrupts off, the
 *                        audio buffer silent, the second core parked
 *   bt_seg_enable(on)    advertising on again / off (and any link closed); the radio stays set up until power-off
 *   bt_seg_status()      bit 0 a central connected, bit 1 its MIDI notifications on, [15:8] connections since start
 * MIDI is not routed yet (the next step): what a central sends is dropped, nothing is sent.
 *
 * Everything here is in the segment (code, constants) or in the pool (its zeroed RAM, app.ld); .ram_text (the radio
 * ramp's burst, fm1_ble_rf.h) goes with the app's RAM code. No initialised data (tools/build.py refuses it). */
#include <stdint.h>
#define FM1_IRQ_EXTERN 1                       /* (hal/fm1_irq.h: the crash record is the app's) */
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define BLE_BARRIER() RING_PUBLISH()
#define BLE_DEVICE_NAME "JIANT FM-1"
#define BLE_NAME_HEX 0                         /* the name as it is (no " XXXX") */
#define BLE_LL_MAX_OCTETS 27u                  /* stock's PDU size, as Melodee (radio-tested) */
#define BLE_DIAG 0                             /* (no counters, no console: as Melodee) */
#include "fm1_cc.h"
#include "fm1_time.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#include "ble/ble_stack.c"

/* the segment's header, first in it (app.ld .bt_head): magic, payload length, CRC-32 of the payload (zlib), filled by
 * tools/build.py; the payload follows at 0xEA020 */
#define BT_SEG_MAGIC 0x3154424Au               /* "JBT1" */
__attribute__((section(".bt_head"), used)) const uint32_t bt_head[8] = {BT_SEG_MAGIC, 0, 0, 0, 0, 0, 0, 0};

static uint8_t bs_conns, bs_linked;

BLE_API int ble_app_midi_peek(uint32_t *pkt, uint32_t *t_ms)
{
    (void)pkt, (void)t_ms;
    return 0;                                  /* (nothing to send yet) */
}
BLE_API void ble_app_midi_pop(void) {}
BLE_API void ble_app_midi_in(uint32_t pkt, uint16_t ts, uint16_t last_ts)
{
    (void)pkt, (void)ts, (void)last_ts;        /* (not routed yet) */
}
BLE_API void ble_app_state(void)
{
    uint8_t c = (uint8_t)ble_connected();
    if (c && !bs_linked && bs_conns < 255u)
        bs_conns++;
    bs_linked = c;
}

/* the SDK board's values for the three constant records (crystal load, PA bias, Wi-Fi per-rate gains: zero keeps the
 * defaults); 187 is measured (fm1_rf_cal_on), as fm1-base's bt_own.c and Melodee's midi_ble.c */
static const struct ble_rf_trims BS_TRIMS = {{0x0B, 0x0B}, {1, 7, 4, 7, 11, 1, 7}, {0}, {0}};

int bt_seg_start(const uint8_t addr[6])
{
    fm1_ble_crumb_boot();                      /* (Melodee's .noinit breadcrumb: the last start's, then from zero) */
    fm1_ble_step(FM1_BLE_STEP_STACK_INIT);
    ble_init(addr, 1);
    fm1_guard_unlock_top();                    /* (the vectors are in the protected top of RAM) */
    fm1_ble_irq_attach(BLE_HW_IRQ_PRIO);       /* the two vectors, masked until the radio runs */
    fm1_ble_irqs_hold(1);
    fm1_guard_lock_top();
    fm1_rf_cal_on = 1;                         /* (our own measurement, no stored 187) */
    ble_hw_wl82_start(&BS_TRIMS);              /* the radio's start-up, its calibration (the crystal ramp from RAM) */
    if (fm1_ble_rf_stat.section != FM1_RF_SECT_DONE)
        return 1;                              /* (the start-up did not run to its end: no advertising, held) */
    fm1_ble_step(FM1_BLE_STEP_ENABLE);
    ble_enable(1);
    fm1_ble_step(FM1_BLE_STEP_RUNNING);
    fm1_ble_irqs_hold(0);                      /* (taken once the caller turns the interrupts back on) */
    return 0;
}

void bt_seg_enable(int on)
{
    fm1_ble_irqs_hold(1);
    fm1_ble_step(on ? FM1_BLE_STEP_ENABLE : FM1_BLE_STEP_SET_OFF);
    ble_enable(on);
    if (!on)
        bs_linked = 0;
    fm1_ble_irqs_hold(0);
}

uint32_t bt_seg_status(void)
{
    return (uint32_t)ble_connected() | (uint32_t)ble_midi_ready() << 1 | (uint32_t)bs_conns << 8;
}
