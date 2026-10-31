/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.12) bt.c's calls into the Bluetooth segment (firmware/bt/bt_seg.c, linked at flash 0xEA000: app.ld BTSEG), made
 * only once the segment installed there is this build's: its header (magic, length, CRC-32) and the CRC-32 of its
 * payload as read in place equal bt_want, which tools/build.py writes into this image after the link. A segment from
 * another build, a torn or missing one: NO SEG, nothing of it is called. */
#define BT_SEG_MAGIC 0x3154424Au                       /* "JBT1" (bt_seg.c bt_head) */
extern const volatile uint32_t _bt_start[];            /* (app.ld: the segment's header, read through the plain XIP window) */
int bt_seg_start(const uint8_t addr[6]);
void bt_seg_enable(int on);
uint32_t bt_seg_status(void);
uint32_t bt_seg_rssi(uint16_t *w, uint32_t *rd);
uint32_t bt_seg_rxdiag(void);
/* build.py finds this by name and writes the segment's payload length and CRC-32 into the image (a section of its own,
 * after the code: in .rodata it would move the sound's tables, tools/audio_layout.txt 'r' rows) */
__attribute__((used, section(".rodata.btwant"))) static const volatile uint32_t bt_want[3] = {0x544E5742u, 0u, 0u};   /* BWNT */

static int bt_seg_ok(void)
{
    uint32_t len = bt_want[1];
    return bt_want[0] == 0x544E5742u && len && len < FL_EXT_HI - FL_EXT_LO - 32u && _bt_start[0] == BT_SEG_MAGIC &&
           _bt_start[1] == len && _bt_start[2] == bt_want[2] &&
           st_crc32((const uint8_t *)(uintptr_t)_bt_start + 32u, len) == bt_want[2];
}

/* (V10.13) the segment installed is this build's: checked once (the flash there changes only with an update, then a
 * reset), the plain window set first (flash from 0x93000 up read as it is through XIP, as the SDK's VM set-up; JIANT
 * reads nothing there through XIP otherwise: the segment is written plain). At power-on (V10.15: main.c fm1_main,
 * before felucca_init, under the splash: the window's set-up has the interrupts off a moment; V10.13 bt.c bt_boot, after
 * it, finds it done), before anything reads it (VOICE's English dictionary, MENU > ABOUT's texts: bt.c BTAPP_RO) or calls
 * it */
static uint8_t bt_seg_st;                              /* 0 not checked, 1 this build's, 2 not */
static int bt_hw_seg(void)
{
    if (!bt_seg_st) {
        fl_plain_window_init();
        bt_seg_st = bt_seg_ok() ? 1u : 2u;
    }
    return bt_seg_st == 1u;
}

/* the first ON: the interrupts off (the second core parked out of any job: FL_PARK), the audio buffer silent (the DMA
 * keeps playing it), the radio's start-up and calibration in the segment, then the interrupts back (the audio takes
 * up where it was; the late halves are its overload path's) */
static int bt_hw_start(uint8_t addr[6])
{
    uint32_t f, i, z = 0;
    int rc;
    if (!bt_hw_seg())
        return 2;
    for (i = 0; i < 6u; i++)
        z |= addr[i];
    if (!z) {                                          /* the static random address, once (Core Vol 6 Part B 1.3.2.1) */
        uint32_t h = fm1_ticks() * 2654435761u ^ (uint32_t)fm1_adc_read(FM1_ADC_BATT) << 7 ^ fm1_ms * 40503u;
        for (i = 0; i < 6u; i++) {
            h ^= h << 13;
            h ^= h >> 17;
            h ^= h << 5 ^ fm1_ticks();
            addr[i] = (uint8_t)(h >> 9);
        }
        addr[5] |= 0xC0u;
    }
    fm1_wdt_feed();
    f = irq_save();
    memset(abuf, 0, sizeof abuf);
    rc = bt_seg_start(addr);
    irq_restore(f);
    fm1_wdt_feed();
    return rc ? 1 : 0;
}
static void bt_hw_enable(int on) { bt_seg_enable(on); }
static uint32_t bt_hw_status(void) { return bt_up == BT_RUN ? bt_seg_status() : 0u; }
static uint32_t bt_hw_rssi(uint16_t *w, uint32_t *rd) { return bt_up == BT_RUN ? bt_seg_rssi(w, rd) : 0u; }
static uint32_t bt_hw_rxdiag(void) { return bt_up == BT_RUN ? bt_seg_rxdiag() : 0u; }
