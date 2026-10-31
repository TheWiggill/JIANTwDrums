/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) */
/* (V10.12) MENU > SYSTEM > BLUETOOTH OFF / ON: Bluetooth LE MIDI, as a peripheral that advertises "JIANT FM-1" with the
 * BLE-MIDI service (phones and computers list it as a MIDI device); docs/BLE_FEASIBILITY.md stage 2. MIDI is not routed
 * yet: a central can connect, nothing is played or sent.
 *   OFF (the default)  the radio is never started
 *   ON                 started at once if the music is stopped (else "STOP FIRST", the row stays OFF), or at power-on
 *                      when ON was saved; the sound is quiet for the start (~0.3 s: the interrupts off while the radio
 *                      calibrates itself). Its value then reads ADV (advertising) or CONN (a central connected), with
 *                      the connections since power-on after it; NO SEG: the Bluetooth segment at 0xEA000 is not this
 *                      build's (the update loader writes it with the app: install the package again); the radio's
 *                      start-up returned unfinished: "BLUETOOTH FAILED TO START", OFF (no retry until the next power-on)
 *   OFF again          advertising stops and a link is closed; the radio stays set up (no second calibration)
 * The stack and the radio drivers live in the segment (firmware/bt/bt_seg.c, app.ld BTSEG), called through bt_hw.c only
 * after the segment's CRC-32 matched this build's.
 * Boot guard: a word in .noinit (bt_crumb) says the radio's start is running; a reset in it (a hang: the watchdog, or a
 * crash) leaves it set, and the next power-on turns BLUETOOTH OFF, saves that, and says so. A crash or a hang while it
 * ran (bt_crumb BT_RUN) does the same.
 * The settings bytes: favorites.factory[6][24..31] (TRIO's retired favourites row: never selectable, nothing reads
 * its stars; its presets were < 64, so bytes 8.. were always 0): [24] BT_MARK (else none: OFF, no address), [25] bit 0
 * ON, [26..31] the static random address (LSB first, generated once at the first start, top bits 11). */
#define BT_ROW (&favorites.factory[6][24])
#define BT_MARK 0xB7u
enum { BTS_NONE, BT_RUN, BT_FAIL, BT_NOSEG };
enum { BT_CR_START = 0xB7570001u, BT_CR_RUN = 0xB7570002u };
static uint8_t bt_up;                                   /* BT_*: this power-on's radio */
static uint32_t bt_crumb __attribute__((section(".noinit.zbt")));   /* (after every other .noinit: none moves) */
#if FELUCCA_BT
static int bt_hw_start(uint8_t addr[6]);                /* bt_hw.c: 0 started, 1 the radio's start-up failed, 2 no segment */
static void bt_hw_enable(int on);
static uint32_t bt_hw_status(void);
#else
static int bt_hw_start(uint8_t addr[6]) { (void)addr; return 2; }   /* (the host, the emulator: no radio) */
static void bt_hw_enable(int on) { (void)on; }
static uint32_t bt_hw_status(void) { return 0; }
#endif
static uint32_t bt_get(void) { return BT_ROW[0] == BT_MARK && (BT_ROW[1] & 1u); }
static void bt_set(uint32_t on)
{
    uint8_t *r = BT_ROW;
    if (r[0] != BT_MARK) {
        memset(r, 0, 8);
        r[0] = BT_MARK;
    }
    r[1] = (uint8_t)((r[1] & ~1u) | (on & 1u));
}
static void bt_start(void)
{
    int rc;
    if (bt_up == BT_RUN) {
        bt_hw_enable(1);
        bt_crumb = BT_CR_RUN;
        return;
    }
    if (bt_up)                                          /* (failed or missing this power-on: not again) */
        return;
    bt_crumb = BT_CR_START;
    rc = bt_hw_start(BT_ROW + 2);
    bt_crumb = rc ? 0u : BT_CR_RUN;
    bt_up = (uint8_t)(rc == 2 ? BT_NOSEG : rc ? BT_FAIL : BT_RUN);
    if (rc == 1) {                                      /* (the start-up returned unfinished: OFF, not again at power-on) */
        bt_set(0);
        ui_message("BLUETOOTH FAILED TO START");
    }
    settings_save();                                    /* (the address, made at the first start) */
}
static void bt_put(uint32_t v)
{
    if (v && song.playing) {
        ui_message("STOP FIRST");
        return;
    }
    bt_set(v);
    if (v)
        bt_start();
    else if (bt_up == BT_RUN) {
        bt_hw_enable(0);
        bt_crumb = 0;
    }
}
static char bt_val[12];
static const char *bt_vname(uint32_t v)
{
    uint32_t st = bt_hw_status(), n = st >> 8 & 255u;
    if (!v)
        return "OFF";
    if (bt_up != BT_RUN)
        return bt_up == BT_NOSEG ? "NO SEG" : bt_up == BT_FAIL ? "FAILED" : "ON";
    str_cpy(bt_val, st & 1u ? "CONN" : "ADV", sizeof bt_val);
    if (n) {
        str_cpy(bt_val + str_len(bt_val), " ", 2);
        fmt_int(bt_val + str_len(bt_val), (int32_t)n);
    }
    return bt_val;
}
/* power-on (fm1_main, after the splash: the last run's crash or hang known, boot_clean): the guard, then the saved ON
 * starts from bt_tick */
static void bt_boot(uint32_t clean)
{
    if (bt_crumb == BT_CR_START || (bt_crumb == BT_CR_RUN && !clean)) {
        if (bt_get()) {
            bt_set(0);
            settings_save();
        }
        ui_message(bt_crumb == BT_CR_START ? "BLUETOOTH OFF: START FAILED" : "BLUETOOTH OFF AFTER A CRASH");
    }
    bt_crumb = 0;
}
static void bt_tick(void)                               /* main loop: the saved ON, once the music is stopped */
{
    if (!bt_up && bt_get() && !song.playing && fm1_ms > 3000u)
        bt_start();
}
