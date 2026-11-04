/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio): the sequence, from Melodee's firmware/hal/fm1_power.h
 * (https://github.com/keremimo/melodee, GPL-3.0-only); JIANT's port and comments, (C) 2026 Juan Ignacio Antonucci */
/* (V10.11) The FM-1's supplies set up as the stock V15 firmware does before its radio can start (docs/BLE_FEASIBILITY.md
 * stage 2, step 2): the DC-DC for the 1.3 V rail on (LDO13 handing over to DCDC13), the analog configuration bytes,
 * VDDIO 3.2 V, VDC14 1.60 V and SYSVDD 1.38 V last. Register order and values: stock app 0x02004F90..0x0200509A, as
 * the AC79 SDK's __hw_power_init (Melodee's identification). JIANT had only the two core rails (fm1_rails_boot, which
 * still runs after it and finds them set). Called once at boot with IRQs off and the second core held. A P33 transfer
 * that times out stops the sequence where it is (no write of a dummy byte); 1 = done and read back as expected. */
#pragma once
#include <stdint.h>
#include "fm1_sys.h"

/* the stock delay(1000): its nop / increment / branch loop, not a time (the memory clobber keeps it between the P33
 * operations) */
static inline __attribute__((always_inline)) void fm1_power_settle(void)
{
    int32_t n = -1000;
    __asm__ volatile("goto 2f\n1:\n\tnop\n\t%0 += 1\n2:\n\tif (%0 != 0) goto 1b" : "+r"(n) :: "memory");
}

static uint8_t fm1_power_ready;                         /* 1: the sequence ran and read back (MENU > ABOUT, console) */

static int fm1_power_field(uint32_t reg, uint32_t start, uint32_t width, uint32_t value)
{
    uint32_t t = fm1_p33_timeouts, mask = ((1u << width) - 1u) << start;
    uint8_t old = fm1_p33_read(reg);
    if (fm1_p33_timeouts != t)
        return 0;
    fm1_p33_write(reg, (uint8_t)((old & ~mask) | ((value << start) & mask)));
    return fm1_p33_timeouts == t;
}

static int fm1_power_init(void)
{
    uint32_t t = fm1_p33_timeouts;
    uint8_t mode, io, vdc14, sysvdd;
#define PW_STEP(op) do { op; if (fm1_p33_timeouts != t) return 0; } while (0)
#define PW_FIELD(reg, start, width, value) do { if (!fm1_power_field(reg, start, width, value)) return 0; } while (0)
    PW_FIELD(0x00u, 2, 1, 1);                           /* LDO13 on during the handover */
    fm1_power_settle();
    PW_FIELD(0x00u, 3, 1, 1);                           /* DCDC13 on */
    fm1_power_settle();
    PW_FIELD(0x00u, 2, 1, 0);                           /* LDO13 off */
    fm1_power_settle();
    PW_STEP(fm1_p33_write(0x01u, 0x20));
    PW_STEP(fm1_p33_write(0x05u, 0x40));
    PW_STEP(fm1_p33_write(0x06u, 0x0C));
    PW_STEP(fm1_p33_write(0x07u, 0xA5));
    PW_STEP(fm1_p33_write(0x08u, 0x96));
    PW_FIELD(0x09u, 4, 1, 1);                           /* SYSVDD drive */
    PW_FIELD(0x05u, 0, 3, 4);                           /* VDDIOM 3.2 V */
    PW_FIELD(0x06u, 0, 3, 7);                           /* VDC14 1.60 V */
    PW_FIELD(0x05u, 4, 2, 3);                           /* VDDIOW 3.2 V */
    PW_STEP((void)fm1_p33_read(0x06u));                 /* (the stock's unused reads, in their place) */
    PW_STEP((void)fm1_p33_read(0x05u));
    PW_STEP((void)fm1_p33_read(0x05u));
    PW_STEP(fm1_p33_or(0x00u, 0x80));                   /* RC_250k_EN */
    PW_FIELD(0x16u, 0, 6, 0);                           /* analog keep */
    PW_STEP(fm1_p33_or(0x02u, 0x40));                   /* VDDOK_DIS */
    PW_STEP(fm1_p33_and(0x17u, 0xDF));
    PW_FIELD(0x09u, 0, 4, 15);                          /* SYSVDD 1.38 V, last */
#undef PW_FIELD
#undef PW_STEP
    mode = fm1_p33_read(0x00u);                         /* read back: no retry, no rollback */
    io = fm1_p33_read(0x05u);
    vdc14 = fm1_p33_read(0x06u);
    sysvdd = fm1_p33_read(0x09u);
    return fm1_p33_timeouts == t && (mode & 0x0Cu) == 0x08u && (io & 0x37u) == 0x34u && (vdc14 & 7u) == 7u &&
           (sysvdd & 15u) == 15u;
}
