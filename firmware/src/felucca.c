/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FELUCCA: one compilation unit (the HAL is header-only). Each file uses what the files above it
 * define, so the order matters. */

/* ---------------------------------------------------- build options --- */
/* build.py: FELUCCA_<NAME>=0 / 1 in the environment overrides these. Elsewhere: FELUCCA_FM4
 * (core.h: the DIGITAL engine), FELUCCA_ICONS (icons.c, the parameter icons; on by default),
 * FELUCCA_OTA_RAMONLY (ota.c). */
#ifndef FELUCCA_FLASH
#define FELUCCA_FLASH 1          /* flash driver + storage.c: settings, projects, user presets */
#endif
#ifndef FELUCCA_OTA
#define FELUCCA_OTA 1            /* M-UPGRADE update entry and the editor SysEx; needs FELUCCA_FLASH */
#endif
#ifndef FELUCCA_OTA_DRYRUN
#define FELUCCA_OTA_DRYRUN 0     /* 1 = stage, ask "success", then undo: no record, no reset */
#endif
#ifndef FELUCCA_CDC
#define FELUCCA_CDC 1            /* USB CDC-ACM serial console */
#endif
#ifndef FELUCCA_UAC
#define FELUCCA_UAC 1            /* USB audio input: the master output, 16-bit stereo 44.1 kHz (usb.c) */
#endif
#ifndef FELUCCA_UAC_TONE
#define FELUCCA_UAC_TONE 0       /* bench: the USB input sends test triangles instead of the music */
#endif
#ifndef FELUCCA_UART
#define FELUCCA_UART 1           /* TRS MIDI IN on UART1 / PH8 */
#endif
#ifndef FELUCCA_DUAL_CORE
#define FELUCCA_DUAL_CORE 1      /* (JIANT 0.6.71) the second core renders some synth tracks (hal/fm1_multicore.h) */
#endif
#ifndef FELUCCA_HOTRAM
#define FELUCCA_HOTRAM 1         /* (JIANT 0.6.78, experimental) core 0's hottest audio code in RAM (core.h) */
#endif
#ifndef FELUCCA_BT
#define FELUCCA_BT 1             /* (V10.12) MENU > SYSTEM > BLUETOOTH: bt.c, bt_hw.c, the segment firmware/bt/bt_seg.c */
#endif
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_900"    /* package identity (build.py: the .fwsc marker string) */
#endif
#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "V10.24" /* (2026-11-02) V10.24: the user's picks from the redesign mock: SEQ > EUCLID the lanes as concentric rings (heat colours, VARY's turns under the ring); SEQ > MUTATE the anchor above, the pattern playing below (changed steps lit), an arc to the next mutation; SAVE > PRESETS two columns (the engines, their presets), OCT+ keeps the sound browsed to, OCT- brings back the one before; SAVE > TOOLS by scope (KNOB 1 TRACK SOUND SONG SYSTEM, KNOB 2 the action), OCT+ held a second does it (no dialog); SCL > SCENE, the multitimbral mode: one key plays every track given a role (KEEP CHORD ARP BASS LEAD; the kit keeps its sequence), REC records each into its own steps. (2026-11-01) V10.23: STRING FDBK heard (its BLOOM level was applied twice, -15 dB); EXCITE into / out of FDBK keeps each side's EDIT 2 knobs (GAIN no longer starts at STRETCH's 0), FDBK's defaults audible. (2026-11-01) V10.22: testers' asks: ENV > ENV 2 (a second ADSR, MOD SRC ENV2, any DEST), FILTER TYPE LP24 (4 poles), ANALOG WAVE PLS3 (three pulses, SHAPE the width). (2026-11-01) V10.21: EUCLID's lane rows named right (they were one off: ALL's name on BD's row); the ring shows one cycle (the lane's, ALL's, the melody's) and the turn playing, its playhead going round; PROX's page says the phone, not the hand. (2026-11-01) V10.20: PROX smooth: the RSSI reading fixed on word 0 as the level (V10.18's widest range picked a noisy byte: a random sample and hold), the median of nine and ~0.3 s; FAR and NEAR too close (under 4 levels) refused. (2026-10-31) V10.19: the PROX page redraws when a reading arrives (all-zero words left it on NOT CONNECTED) and shows the link as seen: L (link), the RSSI hook's packets / the good packets. (2026-10-31) V10.18: PROX finds the RSSI reading that moves (the four RSSI words of each packet, twelve readings, the widest range on the link; kept once FAR / NEAR is stored), the page shows the four words. (2026-10-31) V10.17: SAVE > SECTIONS KNOB 2 ERASE (the section empty again, RAM and flash; the music stays). (2026-10-31) V10.16: SEQ > EUCLID's VARY in four turns (A A' A'' B, each moved anew, the motif answered); SEQ > MUTATE (EVERY 1..16 bars, AMOUNT, MODE ORBIT / DRIFT / PULSE, WHAT GROOVE / RHYTHM / FEEL / ALL; OCT+ a mutation now, OCT- back to the anchor). (2026-10-31) V10.15: VOICE's English dictionary in the Bluetooth segment (without it: the letter rules). (2026-10-31) V10.14: PROX, a modulation source from the Bluetooth signal (a hand near the antenna), calibrated on MENU > SYSTEM > BLUETOOTH ON + OCT+ (OCT- FAR, OCT+ NEAR). (2026-10-31) V10.13: BLE-MIDI in and out (what a central sends plays as USB-MIDI, MIDI OUT goes to it too); ABOUT's credits in the Bluetooth segment (the app slot had 128 B free). (2026-10-31) V10.12: MENU > SYSTEM > BLUETOOTH OFF / ON: a BLE-MIDI peripheral "JIANT FM-1" (advertising, connectable; MIDI not routed yet), Optimist's stack in the Bluetooth segment at 0xEA000 (written by the update loader from the package's bt.bin), live RF calibration, a boot guard. (2026-10-31) V10.11: the supplies set up as stock V15 does (the DC-DC on, hal/fm1_power.h from Melodee), the radio's prerequisite; 0xEA000..0xFBFFF reserved for the Bluetooth segment. (2026-10-31) V10.10: the radio's calibration kept: the update loader keeps the stock VM sector (0xE8000: RF trims, BT address) but its record's 256 B; VM_READ (editor 80) reads that area for web/btcheck. (2026-10-31) V10.9: MENU > LARGE shows the animations (HOME: the six tracks' creatures in the strip; EDIT: the engine's). (2026-10-31) V10.8: testers' fixes: GEN parts set OFF stay off; GEN keeps its GENRE between visits; SEQ > STEP KNOB 4 VEL (1..127, then ACC SLD A+S); delay 1/4D 1/8D 1/16D; less pitch wobble from the generators (no macro on PITCH, pitched delay seldom, octaves); STOP TO EDIT says SONG PLAYS. (2026-10-31) V10.7: SEQ > EUCLID on a kit: LANE ALL first (DENSITY: a whole usable kit), a lane picked writes that lane alone; EDIT + PRESETS jumps between engines; the kit's SEQ dice plays in every genre; EUCLID's HITS exact (VARY moved them only); REC held alone and let go: UNDO (again: REDO). (2026-10-31) V10.6: SEQ > EUCLID and MODES (a euclidean pattern generator, melodies in the key) in MUTATE's place; MUTATE and SEQ TOOLS' RANDOM COOK BEAT gone. (2026-10-31) V10.5: automation thinned (a recording at the stop, the 64 when full) into holds and RAMPs. (2026-10-31) V10.4: ARRANGE / TIMELINE: OCT- deletes the row picked (DELETE ROW?). (2026-10-31) V10.3: TIMELINE: the lines between lanes soft (the block's colour, darker), the rows' edges stronger. (2026-10-31) V10.2: TIMELINE keeps the black line between lanes (only the rows touch); SAVE composes first: TIMELINE SECTIONS PROJECTS ARRANGE TOOLS USER PRESETS. (2026-10-30) V10.1: TIMELINE blocks fill the lanes (no black gaps between rows or tracks). (2026-10-30) JIANT FM V10: add15-beta6 as the new base (TIMELINE, VAR, ADD, official V16 both ways); the version history before it: the git log of the add15 line (codex/add15-private) */
#endif
#if FELUCCA_OTA && !FELUCCA_FLASH
#error "FELUCCA_OTA needs FELUCCA_FLASH"
#endif
#if FELUCCA_BT && !FELUCCA_FLASH
#error "FELUCCA_BT needs FELUCCA_FLASH"
#endif

/* ------------------------------------------------------------- HAL --- */
#include <stdint.h>
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_power.h"                  /* (V10.11) the stock supply setup (main.c, before the rails) */
#include "fm1_irq.h"
#include "fm1_guard.h"
#if FELUCCA_DUAL_CORE
#include "fm1_multicore.h"
#endif
#include "fm1_input.h"
#include "fm1_timer.h"
#include "fm1_audio.h"
#include "fm1_adc.h"
#include "fm1_lcd_hw.h"
#include "felucca_tables.h"      /* generated by build.py (tools/gen_tables.py) */

/* ----------------------------------------------------- base, display --- */
#include "libc.c"
#include "lcd.c"
#include "gfx.c"

/* ----------------------------------------------------------- sound --- */
#include "core.h"
#include "engines.c"             /* dsp.c, the eng_*.c files, the factory patterns */
#include "params.c"
#include "mod.c"               /* the modulation matrix, used by voice.c and fx.c */
#include "voice.c"
#include "slicer.c"              /* the SLICER, one on a bus (JIANT 0.6.3), used by fx.c */
#include "fx.c"

/* ------------------------------------------------- MIDI, sequencer --- */
#include "usb.c"
#if FELUCCA_UART
#include "midi_uart.c"
#endif
#include "song_chain.c"
#include "seq.c"
#include "audio.c"

/* -------------------------------------------------------------- UI --- */
#include "panel.c"
#include "ui.c"
#include "menu_items.c"          /* MENU's rows and settings (ui_menu.c, editor_menu.c) */
#include "icons.c"               /* parameter icons (FELUCCA_ICONS), used by ui_draw.c */
#include "ui_graph.c"
#include "ui_draw.c"
#include "ui_menu.c"
#include "ui_input.c"
#include "ui_layer.c"              /* the quick layers (FX GLO SCL EDIT REC held; SEQ TOOLS: ui_tools.c) */

/* --------------------------------------------- storage, update, editor --- */
#if FELUCCA_FLASH
#if FELUCCA_DUAL_CORE
#define FL_PARK() fm1_mc_park()  /* (0.9.9-stab) core 1 out of any job before the flash goes off (hal/fm1_multicore.h) */
#endif
#include "fm1_flash.h"
#include "fm1_clk320.h"          /* (0.9.9-stab, as 0.9.12) MENU > SYSTEM > CPU CLOCK 320 MHZ's switch (curist's, MIT; main.c) */
#include "storage_hw.c"
#include "storage.c"
#endif
#include "upreset.c"             /* user presets (RAM mirror; flash with FELUCCA_FLASH) */
#include "project.c"
#if FELUCCA_OTA
#include "ota.c"
#include "ota_hw.c"
#include "editor.c"              /* web editor SysEx (needs the OTA SysEx plumbing) */
#endif
#if FELUCCA_CDC
#include "console.c"
#endif
#include "master.c"              /* the MASTER pot (#137: read before the audio starts) */
#if FELUCCA_BT
#include "bt_hw.c"               /* (V10.12) bt.c's calls into the Bluetooth segment */
#endif
#include "main.c"


