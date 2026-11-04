/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FM1_INLINE: register helpers that must inline into their caller, so the
 * SFR access sequence (and its timing, in ISRs and IRQ-off sections) is the
 * one written at the call site; -Os would otherwise out-line a helper with
 * several callers. RAM-resident code (.ram_text) may only use these, never
 * a called helper (build.py checks .ram_text for calls). */
#pragma once

#define FM1_INLINE static inline __attribute__((always_inline))
/* FM1_OPAQUE(v): v as the compiler cannot know it (an empty asm): a function pointer through it is called through a
 * register, never as a direct call (JIANT 0.9.9-snd8: RAM code reaching a function in the flash past the tables, out of
 * a direct call's reach) */
#define FM1_OPAQUE(v) __asm__("" : "+r"(v))
