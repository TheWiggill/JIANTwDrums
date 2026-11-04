/* SPDX-License-Identifier: GPL-3.0-only
 * JIANT (0.6.91) the song generator's knowledge, as constant tables (gen.c reads them; included by it). Everything a
 * genre knows is written here, nothing is computed: a genre is a row, its drums a map of priorities (after Mutable
 * Instruments' Grids: a level a step a lane, a hit where the level clears the density), its bass one of ~50 archetypes
 * written against the kick (funk, post punk, jazz, reggae, house, techno, hip hop ..), its chords a progression with a
 * quality a chord, its arp and lead a way of speaking, its sounds a recipe a role, its kit a kit, its space a reverb and
 * a delay. The research behind the tables: reports/ and research_notes/ (practitioner conventions, Grids, Marbles, the
 * FiloBass corpus, Witek on syncopation, Attack's beat dissections). */

/* ---------------------------------------------------------------------------------------------------- roles --- */
enum { GP_DRUMS, GP_BASS, GP_CHORDS, GP_ARP, GP_LEAD, GP_N };   /* the parts the GEN screen weighs */
#define GA_AUTO 255u                                    /* an amount left to the genre */

/* ------------------------------------------------------------------------------------------------ drum maps --- */
/* a lane's 16 steps as levels: '.' 0, '1'..'9' 26 a step (9 = 234), 'X' 255. A step hits when its level > 255 - density;
 * '9' and 'X' are anchors (accented, never dropped by a variation); '1'..'3' ghosts. Lanes: BD SD CP CH OH TM RS CB */
enum { DM_HOUSE, DM_DEEP, DM_MICRO, DM_TECHHOUSE, DM_TECHNO, DM_DETROIT, DM_DUBTECH, DM_HYPNO, DM_HARD, DM_ELECTRO,
       DM_EBM, DM_ITALO, DM_SYNTHPOP, DM_DARKWAVE, DM_POSTPUNK, DM_TRIBAL, DM_RBOX, DM_KLING, DM_MOTORIK, DM_INDIE,
       DM_SHOEGAZE, DM_FUNK, DM_DISCO, DM_BOOMBAP, DM_TRAP, DM_UKG, DM_DUBSTEP, DM_BREAKS, DM_DNB, DM_ONEDROP,
       DM_STEPPERS, DM_ROCKERS, DM_JAZZ, DM_BOSSA, DM_AFRO, DM_SPARSE, DM_IDM, DM_JOYDIV, DM_NEWORDER, DM_NONE };
/* (0.9.7, flash) the maps and the pools below are the text tools/gen_drum_maps.py reads (not compiled): build/gen/
 * gen_drum_maps.h holds them packed, each lane once at 16 levels a nibble (GM_LANE), the tables as lane numbers
 * (DRUM_MAP_I, HAT_POOL_I ..: gen.c gm_level) */
#ifdef GEN_DRUM_MAPS_TEXT
static const char *const DRUM_MAP[][8] = {
    [DM_HOUSE]     = {"X...8...X...8...", "", "....9.......9...", "7.3.7.3.7.3.7.3.", "..8...8...8...8.", "", ".......2......3.", ""},
    [DM_DEEP]      = {"X...8...X...8..2", "", "....7.......7...", ".37..37..37..37.", "..8...8...8...8.", "", "...2......4.....", ""},
    [DM_MICRO]     = {"X...8...X...8...", "", "", "..6...5.6.5...6.", "", "", "....6.......6...", ".2..2....3...2.."},
    [DM_TECHHOUSE] = {"X...X...X...X...", "", "....X.......X...", "7.57.57.7.57.57.", "..8...8...8...8.", "", "...5..5....5..5.", ""},
    [DM_TECHNO]    = {"X...9...X...9...", "", "....6.......6...", "..8...8...8...8.", "", "", "......5.....5..2", ""},
    [DM_DETROIT]   = {"X...9...X...9...", "", "....7.......7...", "6686668666866686", "..8...8...8...8.", "", "", ""},
    [DM_DUBTECH]   = {"X...8...X...8...", "", "", "..5...5...5...5.", "", "", "...3......6.....", ""},
    [DM_HYPNO]     = {"X...9...X...9...", "", "", "5555555555555555", "", "..6..6....6..6..", ".4....4.4....4..", ""},
    [DM_HARD]      = {"X...X...X...X...", "", "....7.......7...", "8484848484848484", "..7...7...7...7.", "", "..3..3....3..3..", "......3.......3."},
    [DM_ELECTRO]   = {"X.....8...8..7..", "....X.......X...", "....5.......5...", "7474747474747474", "", "", "", "6..6..6...6.6..."},
    [DM_EBM]       = {"X...X...X...X...", "....X.......X...", "", "9373937393739373", "", "", "", "..........6....."},
    [DM_ITALO]     = {"X...8...X...8...", "....9.......9...", "", "7.7.7.7.7.7.7.7.", "..7...7...7...7.", "..............44", "", ""},
    [DM_SYNTHPOP]  = {"X.......X.6.....", "....X.......X...", "....4.......4...", "7.6.7.6.7.6.7.6.", "", "", "", ""},
    [DM_DARKWAVE]  = {"X...X...X...X...", "....X.......X..3", "....6.......6...", "8.6.8.6.8.6.8.6.", "", "", "", ""},
    [DM_POSTPUNK]  = {"X...8...X...8...", "....X.......X...", "", "7.8.7.8.7.8.7.8.", "", "", ".....3.......3..", ""},
    [DM_TRIBAL]    = {"X.......X.......", "....X.......X...", "", "...3...3...3...3", "", "7..7..7.7..7..7.", "", ""},
    [DM_RBOX]      = {"X...X...X...X...", "....8.......8...", "", "7.6.7.6.7.6.7.6.", "", "", "", ""},
    [DM_KLING]     = {"X...X...X...X...", "", "....X.......X...", "7.7.7.7.7.7.7.7.", "", "", "", "..........5....."},
    [DM_MOTORIK]   = {"X.6.6...8.6.6...", "....8.......8...", "", "7.5.7.5.7.5.7.5.", "..............3.", "", "", ""},
    [DM_INDIE]     = {"X.....7.8.......", "....X.......X..3", "", "7.5.7.5.7.5.7.5.", "..............4.", "", "", ""},
    [DM_SHOEGAZE]  = {"X.....6.8.......", "....8.......8...", "", "5.4.5.4.5.4.5.4.", "6...............", "", "", ""},
    [DM_FUNK]      = {"X..6..6.8.6.....", "...3X.3..3.3X.3.", "", "7474757474757474", "..............4.", "", "", ""},
    [DM_DISCO]     = {"X...9...X...9...", "....9.......9...", "....6.......6...", "8585858585858585", "..8...8...8...8.", "", "", "2.2.2.2.2.2.2.2."},
    [DM_BOOMBAP]   = {"X......6..8.8...", "....X....3..X...", "....3.......3...", "7.5.7.5.7.5.7.5.", "..............3.", "", "", ""},
    [DM_TRAP]      = {"X......8..8.....", "........X.......", "........6.......", "7.7.7.7.7.7.7.7.", "......4.........", "", "", ""},
    [DM_UKG]       = {"X......6..9.....", "....9.......9..3", "", "..7...7...7...7.", "......4.......5.", "", ".3..3.3...3..3..", ""},
    [DM_DUBSTEP]   = {"X.........8.....", "........X.......", "........4.......", "6.6.6.6.6.6.6.6.", "", "", "", ""},
    [DM_BREAKS]    = {"X.6.......8.....", "....X..3.3..X..3", "", "7.6.7.6.7.6.7.6.", "......5.......5.", "", "", ""},
    [DM_DNB]       = {"X.........8.....", "....X.......X..3", "", "6.5.6.5.6.5.6.5.", "", "", ".......3........", ""},
    [DM_ONEDROP]   = {"........X.......", "", "", "6.4.6.4.6.4.6.4.", "..............4.", "", "........X.......", ""},
    [DM_STEPPERS]  = {"X...X...X...X...", "", "", "..6...6...6...6.", "", "", "........8.......", ""},
    [DM_ROCKERS]   = {"X.......X.......", "....X.......X...", "", "6.4.6.4.6.4.6.4.", "", "", "", ""},
    [DM_JAZZ]      = {"3...3...3...3...", "...2...2.2...2..", "", "....6.......6...", "7...7..77...7..7", "", "", ""},
    [DM_BOSSA]     = {"X..6X..6X..6X..6", "", "", "5.5.5.5.5.5.5.5.", "", "", "6..6..6...6..6..", ""},
    [DM_AFRO]      = {"X6......X6..4...", "....X...3...X.3.", "", "6.567.567.567.56", "......4.........", "......5.......5.", "", ".5..5.5..5..5.5."},
    [DM_SPARSE]    = {"7...............", "", "", "", "", "", "......3.....4...", "..........3....."},
    [DM_IDM]       = {"", "", "", "", "", "", "", ""},   /* (procedural: euclids against each other) */
    /* (0.6.97) post punk after Joy Division (She's Lost Control: the kick syncopated against a flat sixteenth hat, the
     * snare on 2 and 4, a tom at the bar's end) and New Order (Blue Monday, Ceremony: four on the floor with the
     * snare and a clap, driven sixteenths, open hats off the beat, toms into the one) */
    [DM_JOYDIV]    = {"X..X..X.X..X..X.", "....X.......X...", "", "7777777777777777", "", "..........4...5.", "", ""},
    [DM_NEWORDER]  = {"X...X...X...X...", "....X.......X...", "....5.......5...", "7676767676767676", "..6...6...6...6.", "..............44", "", ""},
    [DM_NONE]      = {"", "", "", "", "", "", "", ""},
};
/* (0.6.97) a throw's own hats and percussion: a hat pattern, an open hat pattern and a percussion line from these pools
 * now and then instead of the map's, so a genre's throws differ in their top end */
static const char *const HAT_POOL[8] = {"7.7.7.7.7.7.7.7.", "7373737373737373", "..8...8...8...8.", "7.7.7.7.7.7.77.7",
                                        "7.37.37.37.37.3.", "8.5.8.5.8.5.8.55", "7..7..7.7..7..7.", "6666666666666666"};
static const char *const OH_POOL[6] = {"", "..7...7...7...7.", "......7.......7.", "..............7.", "..7.......7.....", "...7...7...7...7"};
static const char *const RS_POOL[6] = {"", "...5......5.....", ".4....4.4....4..", "......5.....5...", "..5...5...5...5.", "...3..3....3..3."};
static const char *const CB_POOL[4] = {"", "5..5..5...5.5...", "......4.......4.", "2.2.2.2.2.2.2.2."};
static const char *const TM_POOL[4] = {"", "..............44", "......5.......5.", "..6..6....6..6.."};
#endif
#include "gen_drum_maps.h"
/* the snare's ghost positions, the best first (the sixteenth before 2 and 4, then the "a"s, the "e"s, the "&"s) */
static const uint8_t GHOST_RANK[12] = {3, 7, 11, 15, 13, 5, 9, 1, 14, 6, 10, 2};
/* the hats' accent cells (percent of the hit's velocity on the four sixteenths of a beat) */
enum { HC_FLAT, HC_HOUSE, HC_FUNK, HC_GARAGE, HC_MACHINE };
static const uint8_t HAT_CELL[5][4] = {{100, 100, 100, 100}, {100, 55, 80, 55}, {90, 50, 70, 65}, {110, 40, 60, 90}, {100, 70, 100, 70}};
/* fills on the phrase's last bar */
enum { FL_NONE, FL_ROLL, FL_TOM, FL_DROP, FL_HAT, FL_LINEAR, FL_KICK16, FL_DUB, FL_BREAK };
/* (0.9.9-gen2: the 808's and the DUB kit's kick shorter, 22 -> 12, 18 -> 10) the kits: offsets on the factory kit's {PITCH PMOD DECAY NOISE COLOR} a lane (both patches), then the DRUM track's
 * TONE DECY NOIS DRV */
enum { KIT_DEF, KIT_909, KIT_808, KIT_CLICK, KIT_INDUS, KIT_SOFT, KIT_DUB, KIT_BREAKS, KIT_N };
typedef struct { int8_t d[8][5]; int8_t tone, decy, nois, drv; } gkit_t;
static const gkit_t KIT[KIT_N] = {
    [KIT_DEF]    = {{{0}}, 0, 0, 0, 0},
    [KIT_909]    = {{{-2, 25, -6, 0, 25}, {0, 0, 6, 10, 10}, {0, 0, 4, 0, 0}, {4, 0, -4, 0, 6}, {4, 0, 0, 0, 6}, {0, 10, 0, 0, 10}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 6, 0, 0, 6},
    [KIT_808]    = {{{-4, -20, 12, -4, -10}, {-4, -10, -8, 10, -20}, {-2, 0, 0, 0, -10}, {-6, 0, -6, 0, -20}, {-6, 0, -4, 0, -20}, {-4, -10, 10, 0, -10}, {0, 0, 0, 0, -10}, {0, 0, 4, 0, 0}}, -10, 6, 0, 0},
    [KIT_CLICK]  = {{{2, 10, -16, 0, 0}, {6, 0, -20, 20, 10}, {4, 0, -10, 0, 10}, {6, 0, -10, 0, 10}, {6, 0, -16, 0, 10}, {4, 0, -16, 0, 0}, {6, 0, -6, 0, 10}, {4, 0, -10, 0, 10}}, 10, -16, -6, 0},
    [KIT_INDUS]  = {{{-2, 30, 6, 12, 40}, {0, 20, 10, 20, 30}, {0, 0, 6, 10, 20}, {0, 0, 8, 10, 20}, {0, 0, 10, 10, 20}, {0, 20, 8, 10, 30}, {0, 10, 4, 10, 20}, {0, 0, 0, 10, 20}}, 12, 6, 10, 40},
    [KIT_SOFT]   = {{{0, -10, 6, 6, -25}, {-2, 0, 8, 14, -18}, {-2, 0, 6, 0, -20}, {-4, 0, 2, 0, -30}, {-4, 0, 6, 0, -30}, {-2, -10, 6, 6, -20}, {-2, 0, 4, 0, -20}, {-2, 0, 4, 0, -20}}, -20, 4, 4, 0},
    [KIT_DUB]    = {{{-6, -10, 10, 0, -20}, {-6, 0, 12, 8, -10}, {-2, 0, 6, 0, -10}, {-2, 0, 0, 0, -20}, {-2, 0, 6, 0, -20}, {-4, 0, 10, 0, -10}, {-4, 0, 10, 0, 0}, {0, 0, 6, 0, -10}}, -16, 8, 0, 0},
    [KIT_BREAKS] = {{{2, 6, -4, 8, 10}, {2, 6, 2, 16, 20}, {0, 0, 2, 4, 10}, {0, 0, 4, 6, 0}, {0, 0, 8, 6, 0}, {2, 6, 0, 8, 10}, {0, 0, 0, 4, 10}, {0, 0, 0, 0, 0}}, 4, 0, 6, 10},
};

/* ----------------------------------------------------------------------------------------------- bass archetypes --- */
/* 16 steps a bar, a character a step: '.' rest, '=' the note before held on, 'R' root, 'r' root soft, 'x' a dead
 * note (the root, very soft and short), '2' '3' '5' '6' '7' '9' the chord's degrees (in the scale), 'L' the fifth
 * below, '8' the octave, 'A' an approach a semitone under the next chord's root (the fifth when the chord stays),
 * 'B' from a semitone above, 'N' the next chord's root early, 'S' a scale step towards the next root. The flags, a
 * character a step: 'a' accent, 's' slide into the next note, 'g' ghost (soft), '.' nothing. bars: 1, 2 or 4 */
enum { BA_OFFBEAT, BA_OFFOCT, BA_BOUNCE, BA_DEEPWALK, BA_ROLL16, BA_OCTBNC, BA_DETROIT, BA_EBM, BA_ITALO, BA_ITALO16,
       BA_SYNTHW, BA_PSY, BA_PUMP8, BA_UKG, BA_SPEEDG, BA_JAMERSON, BA_AROUND, BA_ONTHEONE, BA_SLAP, BA_GOODTIME,
       BA_WALK, BA_WALK2, BA_TWOFEEL, BA_BOOGIE, BA_COUNTRY, BA_SKA, BA_TUMBAO, BA_BOSSA, BA_ROCK8, BA_ROOT5,
       BA_PUNK16, BA_DUST, BA_MCCART, BA_HOOK, BA_MOTORIK, BA_ANGULAR, BA_INDIE, BA_ONEDROP, BA_ONEDROP2, BA_STEPPERS,
       BA_ROCKERS, BA_DUB, BA_BOOMBAP, BA_TRAP, BA_DNB, BA_JUNGLE, BA_WOBBLE, BA_PEDAL, BA_FIFTHS, BA_NEOARP,
       BA_SUICIDE, BA_DARK8, BA_SEQ16, BA_ACID, BA_NITZER1, BA_NITZER2, BA_NITZER3, BA_RADIO, BA_HOOK2, BA_BLUEMON,
       BA_CEREMONY, BA_DAF, BA_CCARP, BA_ROTHER, BA_VILLA, BA_N };
/* (0.9.7, flash) the archetypes below are the text tools/gen_drum_maps.py reads (not compiled): build/gen/gen_bass.h holds
 * them a byte a step (BA_STEP: the note's code | the flag's << 4; gen.c ba_note, ba_flag), each its first step and bars */
typedef struct { uint16_t at; uint8_t bars; } barch_t;
#ifdef GEN_DRUM_MAPS_TEXT
static const barch_t BASS_ARCH[BA_N] = {
    [BA_OFFBEAT]  = {"..R...R...R...R.", 0, 1},
    [BA_OFFOCT]   = {"..8...R...8...R.", 0, 1},
    [BA_BOUNCE]   = {"R.8.R.8.R.8.R.8A", "a...............", 1},
    [BA_DEEPWALK] = {"R..R..5..7.8..5R", "a.............s.", 1},
    [BA_ROLL16]   = {"RRRRRRRRRRRRRRRR", "..a...a...a...a.", 1},
    [BA_OCTBNC]   = {"R.8.R.8.R.8.R.85", 0, 1},
    [BA_DETROIT]  = {"R..R..5.7..8....R..R..5.6..5..3.", 0, 2},
    [BA_EBM]      = {"R.8RR.8RR.8RR8R8", "a...a...a...a...", 1},
    [BA_ITALO]    = {"R.8.R.8.R.8.R.8.", 0, 1},
    [BA_ITALO16]  = {"RR88RR88RR88RR88", 0, 1},
    [BA_SYNTHW]   = {"R585R585R585R585", 0, 1},
    [BA_PSY]      = {".RRR.RRR.RRR.RRR", 0, 1},
    [BA_PUMP8]    = {"R=R=R=R=R=R=R=R=", 0, 1},
    [BA_UKG]      = {"R..R..5...R..7.N", "..............s.", 1},
    [BA_SPEEDG]   = {"R.8...8.R.8...58", "..............s.", 1},
    [BA_JAMERSON] = {"R.xR.5.67.x8.5A.", "..g.......g.....", 1},
    [BA_AROUND]   = {"R..x3.R.x5.5..NR", "...g....g.....s.", 1},
    [BA_ONTHEONE] = {"R..R..R......R.x", "a..............g", 1},
    [BA_SLAP]     = {"Rx8xR.x8Rx7x8.xR", "ag.ga.g.ag.g..g.", 1},
    [BA_GOODTIME] = {"R...R.5.6..7.8..N...R.5.6.7.8..8", 0, 2},
    [BA_WALK]     = {"R===3===5===A===", 0, 1},
    [BA_WALK2]    = {"R===2===5===6=A=", 0, 1},
    [BA_TWOFEEL]  = {"R=======5=====A.", 0, 1},
    [BA_BOOGIE]   = {"R.3.5.6.7.6.5.3.R.3.5.6.7.6.5.A.", 0, 2},
    [BA_COUNTRY]  = {"R=======L=======R===L===S===A===", 0, 2},
    [BA_SKA]      = {"R.3.5.6.8.6.5.3.", 0, 1},
    [BA_TUMBAO]   = {"R......5====N====......5====N===", 0, 2},
    [BA_BOSSA]    = {"R=====L.R=====L.", 0, 1},
    [BA_ROCK8]    = {"R.R.R.R.R.R.R.Sx", "a.......a......g", 1},
    [BA_ROOT5]    = {"R.R.5.5.R.R.5.A.", 0, 1},
    [BA_PUNK16]   = {"RRRRRRRRRRRRRRRR", "a...a...a...a...", 1},
    [BA_DUST]     = {"R..R.R........78", 0, 1},
    [BA_MCCART]   = {"3=5=6=5=3=2=R=A=", 0, 1},
    [BA_HOOK]     = {"8.8.9.8.7.8.5.5.", 0, 1},
    [BA_MOTORIK]  = {"R.R.R.R.R.R.5.5.", 0, 1},
    [BA_ANGULAR]  = {"R..R..7..R..5..8", 0, 1},
    [BA_INDIE]    = {"R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.R.3.5.6.8.A.", 0, 4},
    [BA_ONEDROP]  = {"..R.5..R..R.7.....R.5..R..8.5.3.", 0, 2},
    [BA_ONEDROP2] = {"R.....8.R....5..", 0, 1},
    [BA_STEPPERS] = {"R.xR.5x.R.xR.5.x", "..g...g...g....g", 1},
    [BA_ROCKERS]  = {"R.R...5.R.R.7...", 0, 1},
    [BA_DUB]      = {"R=====....5R===.", "..........s.....", 1},
    [BA_BOOMBAP]  = {"R==...R=5=.....A", 0, 1},
    [BA_TRAP]     = {"R=========R=====R=====5=7=====R=", "......................s.......s.", 2},
    [BA_DNB]      = {"R=====..R==...7=", "..............s.", 1},
    [BA_JUNGLE]   = {"R..R..R...R.R...", 0, 1},
    [BA_WOBBLE]   = {"R=========R===5=", "..............s.", 1},
    [BA_PEDAL]    = {"R===============================================================", 0, 4},
    [BA_FIFTHS]   = {"R=======L=======", 0, 1},
    [BA_NEOARP]   = {"R==5==8===5===R=", 0, 1},
    [BA_SUICIDE]  = {"R.R.5.R.R.R.7.5.", "a.......a.......", 1},
    [BA_DARK8]    = {"R.R.R.R.R.R.R.R.", "a...a...a...a...", 1},
    [BA_SEQ16]    = {"R858R858R858R858", 0, 1},
    [BA_ACID]     = {0, 0, 1},                          /* (procedural: the 303's rules) */
    /* (0.6.95) EBM after Nitzer Ebb: relentless sixteenths on the root, the octave and the minor third thrown in, a
     * rest where the body hits, accents on the beat (Join in the Chant, Murderous, Let Your Body Learn) */
    [BA_DAF]      = {"RRRRRRR8RRRRRR8R", "a...a...a...a...", 1},   /* (gen6b) DAF, Nitzer Ebb: the 16ths never stop */
    [BA_CCARP]    = {"R878R878R878R878", "a...a...a...a...", 1},   /* Crystal Castles: root octave 7th octave */
    [BA_ROTHER]   = {"R.8.7.R.R.8.7.R.", "a.......a.......", 1},   /* Anthony Rother: the electro answer */
    [BA_VILLA]    = {"..R..RR...R..R.8", "..a.......a.....", 1},   /* (gen6b) tech house's roller: off the kick */
    [BA_NITZER1]  = {"R.R8R.R8R.R3R8R8", "a...a...a...a...", 1},
    [BA_NITZER2]  = {"RR8RRR8RRR8RRR3RRR8RRR8RRR8RR2R8", "a...a...a...a...a...a...a...a...", 2},
    [BA_NITZER3]  = {"R.RR.R8.R.RR.R3.", "a.....a.a.....a.", 1},
    /* (0.6.95) post rock after Radiohead: long roots, a walk up to the next chord at the phrase's end */
    [BA_RADIO]    = {"R===============R=======5===7===", 0, 2},
    /* (0.6.97) post punk after Peter Hook: the bass sings high on the neck, eighths over the root, fifth, octave and
     * seventh (Transmission, Love Will Tear Us Apart), Blue Monday's sequenced sixteenth octaves, Ceremony's run */
    [BA_HOOK2]    = {"R.5.8.7.R.5.8.5.R.5.8.7.8.7.5.3.", "a.......a.......a.......a.......", 2},
    [BA_BLUEMON]  = {"R8R8R8R8R8R8R8R8R8R8R8R8R8R85858", "a...a...a...a...a...a...a...a...", 2},
    [BA_CEREMONY] = {"R.R.R.3.5.5.5.3.R.R.R.3.5.5.7.8.", "a.......a.......a.......a.......", 2},
};
#endif
#include "gen_bass.h"
enum { KM_DOUBLE, KM_ANSWER, KM_SUSTAIN, KM_INTERLOCK };   /* the bass against the kick */

/* ------------------------------------------------------------------------------------------------- harmony --- */
/* a chord's quality: its notes as steps of the scale from its root (a stack); R9 rootless (3 5 7 9: the bass has the
 * root, jazz and deep house) */
enum { Q_TRI, Q_7, Q_9, Q_ADD9, Q_SUS2, Q_SUS4, Q_6, Q_QUART, Q_POWER, Q_11, Q_7NO5, Q_R9, Q_N };
static const int8_t QSTACK[Q_N][4] = {{0, 2, 4, 127}, {0, 2, 4, 6}, {0, 2, 6, 8}, {0, 2, 4, 8}, {0, 1, 4, 127}, {0, 3, 4, 127},
                                      {0, 2, 4, 5}, {0, 3, 6, 9}, {0, 4, 7, 127}, {0, 2, 6, 10}, {0, 2, 6, 127}, {2, 4, 6, 8}};
/* (0.8.3) a chord's colour beyond the mode, in the quality's high bits: F_MAJ a major triad on the degree (the minor
 * key's dominant, its leading tone raised), F_DOM a dominant seventh on it (a secondary dominant: V/ii, V/V ..), F_PAR
 * the parallel mode's chord on that degree (mixture: iv in major, IV in minor), F_FLAT the flat side's major chord
 * (bIII bVI bVII: the mode's own when it has it, else borrowed). Every one of them is checked against the piece's mode
 * and the genre's permissions (gen.c gplan_ok): a borrowed chord only where the genre borrows */
#define F_MAJ 0x10u
#define F_DOM 0x20u
#define F_PAR 0x30u
#define F_FLAT 0x40u
#define QQ(q) ((q) & 0x0Fu)
#define QF(q) ((q) & 0xF0u)
typedef struct { uint8_t n; int8_t deg[4]; uint8_t q[4]; } gprog_t;
/* (0.8.3) the progressions as scale degrees of the piece's mode (0 its tonic): a loop of one to four chords, its last
 * chord the one that leads back to the first (the loop's cadence: IV-I, bVII-i, V-i, bII-i ..). A mode where a chord
 * would come out diminished (or augmented) does not take that progression (gen.c tries the genre's next one) */
enum { PR_ONE, PR_I_VII, PR_I_VI, PR_I_IV, PR_I_V, PR_I_II, PR_I_III, PR_IV_I, PR_AEOL, PR_TRANCE, PR_MINOR4, PR_ANDAL,
       PR_EPIC, PR_50S, PR_CYCLE, PR_DEEP4, PR_DEEPVI, PR_JAZZ, PR_TURN, PR_JAZZ2, PR_BOSSA, PR_POP, PR_AXIS, PR_PLAGAL,
       PR_ROCK, PR_SECPOP, PR_BORROW, PR_MARIO, PR_SUS, PR_NEOCL, PR_RADIO, PR_PYRAMID, PR_PHRY, PR_ELEC, PR_N };
static const gprog_t PROG[PR_N] = {
    [PR_ONE]     = {1, {0}, {Q_7}},                                           /* i7 (a pedal) */
    [PR_I_VII]   = {2, {0, 6}, {Q_TRI, Q_TRI | F_FLAT}},                      /* i bVII (I bVII: borrowed) */
    [PR_I_VI]    = {2, {0, 5}, {Q_7, Q_7}},                                   /* i7 VImaj7 / I vi */
    [PR_I_IV]    = {2, {0, 3}, {Q_7, Q_7}},                                   /* i7 iv7 / i7 IV7 (dorian) / Imaj7 IVmaj7 */
    [PR_I_V]     = {2, {0, 4}, {Q_TRI, Q_TRI}},                               /* I V / i v */
    [PR_I_II]    = {2, {0, 1}, {Q_9, Q_9}},                                   /* I II (lydian) / i ii (dorian) */
    [PR_I_III]   = {2, {0, 2}, {Q_7, Q_7}},
    [PR_IV_I]    = {2, {3, 0}, {Q_7, Q_7}},                                   /* iv7 i7 (Kerri Chandler's cell) */
    [PR_AEOL]    = {4, {0, 5, 2, 6}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI}},           /* i VI III VII */
    [PR_TRANCE]  = {4, {0, 5, 6, 3}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI}},           /* i VI VII iv */
    [PR_MINOR4]  = {4, {0, 3, 6, 2}, {Q_7, Q_7, Q_TRI, Q_TRI}},               /* i iv VII III */
    [PR_ANDAL]   = {4, {0, 6, 5, 4}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI | F_MAJ}},   /* i VII VI V: the Andalusian cadence */
    [PR_EPIC]    = {4, {5, 6, 0, 0}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI}},           /* VI VII i i: the Aeolian cadence */
    [PR_50S]     = {4, {0, 5, 3, 4}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI}},           /* I vi IV V / i VI iv V */
    [PR_CYCLE]   = {4, {0, 3, 4, 0}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI}},           /* I IV V I / i iv V i */
    [PR_DEEP4]   = {4, {0, 2, 6, 3}, {Q_7, Q_7, Q_7, Q_7}},                   /* i7 IIImaj7 VII7 iv7 */
    [PR_DEEPVI]  = {4, {0, 5, 6, 0}, {Q_9, Q_7, Q_7, Q_9}},                   /* i9 VImaj7 VII7 i9 */
    [PR_JAZZ]    = {4, {1, 4, 0, 0}, {Q_7, Q_7, Q_9, Q_6}},                   /* ii7 V7 Imaj9 I6 */
    [PR_TURN]    = {4, {0, 5, 1, 4}, {Q_7, Q_7, Q_7, Q_7}},                   /* Imaj7 vi7 ii7 V7 */
    [PR_JAZZ2]   = {4, {2, 5, 1, 4}, {Q_7, Q_7, Q_7, Q_7}},                   /* iii7 vi7 ii7 V7 */
    [PR_BOSSA]   = {4, {0, 1, 1, 4}, {Q_7, Q_7 | F_DOM, Q_7, Q_7}},           /* Imaj7 II7 ii7 V7 (V/V) */
    [PR_POP]     = {4, {0, 4, 5, 3}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI}},           /* I V vi IV */
    [PR_AXIS]    = {4, {5, 3, 0, 4}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI}},           /* vi IV I V */
    [PR_PLAGAL]  = {4, {0, 3, 0, 4}, {Q_7, Q_7, Q_7, Q_7NO5}},
    [PR_ROCK]    = {4, {0, 6, 3, 0}, {Q_TRI, Q_TRI | F_FLAT, Q_TRI, Q_TRI}},  /* I bVII IV I */
    [PR_SECPOP]  = {4, {0, 2, 5, 3}, {Q_TRI, Q_7 | F_DOM, Q_TRI, Q_TRI}},     /* I V/vi vi IV */
    [PR_BORROW]  = {4, {0, 3, 3, 0}, {Q_TRI, Q_TRI, Q_TRI | F_PAR, Q_TRI}},   /* I IV iv I */
    [PR_MARIO]   = {4, {0, 5, 6, 0}, {Q_TRI, Q_TRI | F_FLAT, Q_TRI | F_FLAT, Q_TRI}},   /* I bVI bVII I */
    [PR_SUS]     = {2, {0, 4}, {Q_SUS4, Q_SUS2}},
    [PR_NEOCL]   = {4, {0, 4, 5, 3}, {Q_7, Q_7, Q_7, Q_ADD9}},
    [PR_RADIO]   = {4, {0, 5, 6, 3}, {Q_ADD9, Q_7, Q_SUS2, Q_ADD9}},          /* (0.6.95) i add9, VI7, VII sus2, iv add9 */
    [PR_PYRAMID] = {4, {0, 2, 5, 4}, {Q_7, Q_TRI, Q_7, Q_SUS4}},
    [PR_PHRY]    = {2, {0, 1}, {Q_TRI, Q_TRI}},                               /* i bII (phrygian) */
    [PR_ELEC]    = {4, {0, 6, 5, 6}, {Q_TRI, Q_TRI, Q_TRI, Q_TRI}},           /* (gen6) i VII VI VII: electro's */
};
/* (0.8.3) a genre's harmony: its family (how it composes) and what it may colour with */
enum { HF_LOOP, HF_JAZZY, HF_POP, HF_COLOR, HF_CINE, HF_JAZZ };   /* modal loops; 7th loops; functional pop; colour
                                                         * washes; film; jazz (ii-V, secondary dominants) */
#define HX_RAISE 0x08u                                  /* the minor key's dominant major at the cadence (leading tone) */
#define HX_SEC 0x10u                                    /* secondary dominants */
#define HX_BOR 0x20u                                    /* borrowed chords (mixture, the flat side in major) */
#define HX_LIFT 0x40u                                   /* FORM PARTS: the last section a whole step up (a real modulation) */
#define HX_APPR 0x80u                                   /* the lead's chromatic approach notes (a semitone into a chord tone) */
#define HXF(h) ((h) & 7u)
/* how the chords are voiced: PAD led to the nearest voicing in C3..G5 as written; PLAIN the same but triads whatever
 * the progression says; RICH sevenths and ninths added; SUS suspensions; QUART fourths; POWER root fifth octave;
 * PLANE the same shape moved in parallel (techno, impressionism) */
enum { VM_PAD, VM_PLAIN, VM_RICH, VM_SUS, VM_QUART, VM_POWER, VM_PLANE };
/* how the chord track plays: held, swelling, stabbed on offbeats, pulsed, dubbed (a stab or two a bar through the
 * delay), skanked (reggae's offbeat eighths), comped (jazz, syncopated), power (a push on the bar), a drone, sparse,
 * broken sixteenths (UKG), trance (gated sixteenths) */
enum { CS_HOLD, CS_SWELL, CS_STAB, CS_PULSE, CS_DUB, CS_SKANK, CS_COMP, CS_POWER, CS_DRONE, CS_SPARSE, CS_BROKEN,
       CS_TRANCE };
/* the arp: an order over the chord's notes (indices, +4 an octave up, -1 the end), its rate in steps, its gate */
enum { AP_UP, AP_UPDOWN, AP_DOWN, AP_ALBERTI, AP_GLASS, AP_PULSE, AP_SEQ, AP_WALK, AP_CYCLE3, AP_CYCLE5, AP_TRANCE,
       AP_BROKEN, AP_OCTAVES, AP_N };
typedef struct { int8_t ord[8]; uint8_t rate, gate; uint16_t mask; } garp_t;
static const garp_t ARP[AP_N] = {
    [AP_UP]      = {{0, 1, 2, 3, 4, 5, 6, 7}, 1, 50, 0xFFFF},
    [AP_UPDOWN]  = {{0, 1, 2, 3, 2, 1, -1}, 2, 60, 0xFFFF},
    [AP_DOWN]    = {{3, 2, 1, 0, 7, 6, 5, 4}, 1, 50, 0xFFFF},
    [AP_ALBERTI] = {{0, 3, 1, 3, -1}, 1, 70, 0xFFFF},
    [AP_GLASS]   = {{0, 1, 2, 0, 1, 2, 3, 2}, 1, 75, 0xFFFF},
    [AP_PULSE]   = {{0, 0, 0, 0, 0, 0, 0, 2}, 1, 40, 0xFFFF},
    [AP_SEQ]     = {{0, 3, 1, 3, -1}, 1, 45, 0xFFFF},
    [AP_WALK]    = {{0, 2, 1, 3, 2, 0, 3, 1}, 1, 55, 0xFFFF},
    [AP_CYCLE3]  = {{0, 2, 5, -1}, 1, 55, 0xFFFF},
    [AP_CYCLE5]  = {{0, 1, 2, 3, 6, -1}, 1, 55, 0xFFFF},
    [AP_TRANCE]  = {{0, 4, 1, 5, 2, 6, 3, 7}, 1, 35, 0xDB6D},   /* (a trance gate: x.xx.xx.xx.xx.xx) */
    [AP_BROKEN]  = {{0, 2, 1, 3, -1}, 2, 60, 0xB6DB},
    [AP_OCTAVES] = {{0, 4, 0, 4, 1, 5, 0, 4}, 1, 45, 0xFFFF},
};
/* (gen5) the club's chord stabs, off the records (gen.c gen_pads): bit p the step p of the bar. The organ / M1 offbeats,
 * the piano's 3 + 3 + 2, deep house's syncopations, Detroit's strings, techno's stab a bar or its answer; the figures a
 * genre draws from (HOUSE DEEP TECH HOUSE TECHNO DETROIT). (Nine: out of the merged constants, a table of its own) */
static const uint16_t STAB_FIG[9] = {0x4444u, 0x1249u, 0x4848u, 0x4141u, 0x2424u, 0x0001u, 0x0408u, 0x2492u, 0x4444u};
static const uint8_t STAB_SET[5][3] = {{0, 1, 4}, {2, 3, 4}, {0, 2, 6}, {5, 6, 2}, {1, 7, 3}};
/* the lead: a motif in phrases, an ostinato cell on an euclid, sparse long chord tones, Part's scale line, a machine's
 * figure, a two-bar hook (call and answer), none */
enum { LD_MOTIF, LD_OSTINATO, LD_SPARSE, LD_TINTI, LD_MECH, LD_HOOK, LD_NONE, LD_EBM, LD_KRAFT, LD_ELEC, LD_CC, LD_SUICIDE };
/* (gen6, the owner's archetypes) a fixed motif in the scale's degrees, a note every `every` steps held `hold`, the
 * chord's moves followed, its last note a step up every fourth bar (the answer): DAF's shout on the bar's one (the
 * step under it now and then); Kraftwerk's fourths and fifths; Rother's octave and fifth; Crystal Castles' octave glitch; Suicide's
 * one note and its step of tension. A genre with one is an archetype: its parts as written (no pools, no neighbours) */
typedef struct { int8_t d[4]; uint8_t every, hold; } gcell_t;
static const gcell_t LD_CELLS[5] = {{{0, 0, -1, 0}, 16, 3}, {{0, 3, 4, 3}, 4, 2}, {{0, 7, 0, 4}, 2, 1}, {{0, 7, 4, 0}, 2, 1},
                                    {{0, 0, 0, 1}, 16, 12}};

/* ------------------------------------------------------------------------------------------ sounds and mix --- */
#define EG_WH 7u                                        /* WHEEL (the others: song_dice.c) */
/* (0.9.9-gen1) a role's SOUND ARCHETYPE, packed (18 B; was 44): up to three (engine, factory preset) options (the first
 * the likeliest; a bass takes the first two only, so its engine is the archetype's), then what the role asks of the
 * sound over the preset, in the engine's real parameters: the amp envelope (GR: a range in eighths of 0..127), the
 * filter (type LP HP BP COMB COMB- MS20, 7: the engine's own; its cutoff and resonance: ANALOG's CUT RES, the part's
 * filter on the others), the filter envelope (GR4: in fourths of 0..63), the glide, ANALOG's WAVE (two: the second one
 * time in three; MIX 0: one oscillator), the LFO on the filter (its depth, a free rate or 1/4 1/8 1/8T 1/16 synced), the
 * part's drive and chorus, the voice mode */
typedef struct {
    uint8_t o[3][2];                                    /* (engine, preset); engine 255: none */
    uint8_t atk, dec, sus, rel, cut, res;               /* GR ranges */
    uint8_t envf;                                       /* GR4 */
    uint8_t ft;                                         /* bits 0..2 P_FTYPE (7: unset), 3..7 GLIDE / 4 */
    uint8_t wv;                                         /* ANALOG: bits 0..2 WAVE, 3..5 the other, 6 MIX 0, 7 none */
    uint8_t lfo;                                        /* high nibble depth / 4, low the rate (GLFO_RATE) */
    uint8_t dc;                                         /* high nibble DIST / 4, low CHORUS / 4 */
    uint8_t voice;
} grecipe_t;
#define GR(lo, hi) (uint8_t)((((lo) / 8) << 4) | ((hi) / 8))
#define GR4(lo, hi) (uint8_t)((((lo) / 4) << 4) | ((hi) / 4))
#define GFT(t, gl) (uint8_t)((t) | (((gl) / 4) << 3))
#define GWV(a, b, m) (uint8_t)((a) | ((b) << 3) | ((m) << 6))
#define GNOWV 0x80u
#define GLFO(d, r) (uint8_t)((((d) / 4) << 4) | (r))      /* r < 12: a free rate r * 2 + 2; 12 .. 15: 1/4 1/8 1/8T 1/16 */
#define GDC(d, c) (uint8_t)((((d) / 4) << 4) | ((c) / 4))
#define NO_ {255, 0}
enum { SR_SUB, SR_ACID, SR_REESE, SR_FMBASS, SR_ROCKBASS, SR_HOOK, SR_WOW, SR_PAD, SR_STRINGS, SR_CHOIR, SR_ORGAN,
       SR_EP, SR_STAB, SR_DUBCHORD, SR_PLUCK, SR_BELL, SR_LEAD, SR_VOXLEAD, SR_CHIP, SR_KEYS, SR_DRONE, SR_NOISE,
       SR_TEXTURE, SR_HOOVER, SR_WALL, SR_GTR, SR_SINEKEY, SR_EBMBASS, SR_808, SR_UPRIGHT, SR_TECHBASS, SR_SUPERSAW, SR_N };
/* the factory presets by their place (eng_analog.c, eng_phase.c .. : ANALOG 2 SQR BASS, 4 ACID, 7 SUB BASS ..) */
static const grecipe_t RECIPE[SR_N] = {
    /* ---- basses: their engine fixed */
    [SR_SUB]      = {{{EG_A, 7}, {EG_A, 7}, {EG_LO, 14}}, GR(0, 7), GR(56, 79), GR(88, 119), GR(16, 31), GR(32, 47), GR(0, 15), GR4(0, 7), GFT(0, 0), GNOWV, 0, 0, V_LEGATO},
    /* the 303 (0.9.9-gen2, after X0X's Open303: CUT ~48, RES ~100, ENV MOD ~85 of 127, DECAY ~200..600 ms, a 60 ms
     * slide, ACCENT on the filter's envelope and the level): one saw (or a square), the low-pass low (~400 Hz at rest)
     * with RES high (not ringing on its own) and the envelope high on it (~2 kHz at its peak; an ACCENT opens it ~2
     * octaves more: voice.c), its decay short (400..900 ms to the end: the filter's sweep) and a sustain under it
     * (ANALOG's one envelope is the level's too: the note's body), LEGATO, the SLIDEs gliding in TIME (gen_sound:
     * ~55 ms, no retrigger), driven after (DIST) */
    [SR_ACID]     = {{{EG_A, 4}, {EG_A, 2}, NO_}, GR(0, 7), GR(80, 88), GR(40, 48), GR(16, 31), GR(32, 40), GR(104, 112), GR4(52, 60), GFT(0, 24), GWV(0, 1, 1), 0, GDC(24, 0), V_LEGATO},
    [SR_REESE]    = {{{EG_A, 16}, {EG_A, 14}, NO_}, GR(0, 7), GR(72, 95), GR(104, 119), GR(32, 47), GR(40, 55), GR(16, 31), GR4(0, 11), GFT(0, 16), GNOWV, GLFO(12, 1), GDC(20, 28), V_LEGATO},
    /* (V10.54) the FM6 / WHEEL numbers below re-aimed after V10.42 replaced FM6 8..12 and 14 (ICE BELL DX KEYS FM PLUCK SOFT BELL FM
     * STRINGS WOOD) and WHEEL 5, 6 (DUB ORGAN SOFT KEYS): the roles got leads and basses (a SUB BASS on the bells) */
    [SR_FMBASS]   = {{{EG_FM, 2}, {EG_FM, 13}, {EG_PH, 7}}, GR(0, 7), GR(48, 71), GR(40, 87), GR(16, 31), GR(72, 103), GR(8, 23), GR4(8, 27), GFT(0, 0), GNOWV, 0, GDC(8, 0), V_LEGATO},
    [SR_ROCKBASS] = {{{EG_A, 2}, {EG_A, 14}, NO_}, GR(0, 7), GR(48, 71), GR(64, 111), GR(16, 39), GR(48, 71), GR(8, 23), GR4(8, 23), GFT(0, 0), GWV(1, 0, 0), 0, GDC(24, 16), V_LEGATO},
    /* the octave bass of italo, synth pop, electro, trance: a saw (or a square), plucked by the filter */
    [SR_HOOK]     = {{{EG_A, 2}, {EG_A, 14}, NO_}, GR(0, 7), GR(40, 63), GR(16, 55), GR(16, 31), GR(48, 71), GR(16, 39), GR4(20, 39), GFT(0, 0), GWV(0, 1, 0), 0, GDC(12, 16), V_LEGATO},
    /* dubstep's wobble: the fat saws, the resonant low-pass moved by an LFO on the beat */
    [SR_WOW]      = {{{EG_A, 14}, {EG_A, 16}, NO_}, GR(0, 7), GR(72, 95), GR(104, 127), GR(24, 39), GR(32, 47), GR(48, 71), GR4(0, 7), GFT(0, 8), GNOWV, GLFO(44, 13), GDC(40, 0), V_LEGATO},
    /* ---- chords, pads */
    [SR_PAD]      = {{{EG_A, 15}, {EG_FM, 4}, {EG_PH, 9}}, GR(56, 103), GR(80, 103), GR(104, 119), GR(80, 111), GR(40, 71), GR(8, 23), GR4(4, 19), GFT(0, 0), GNOWV, GLFO(12, 3), GDC(0, 40), V_POLY},
    [SR_STRINGS]  = {{{EG_A, 11}, {EG_FM, 4}, {EG_PH, 2}}, GR(40, 79), GR(80, 103), GR(104, 127), GR(72, 103), GR(48, 79), GR(8, 23), GR4(0, 11), GFT(0, 0), GNOWV, GLFO(12, 2), GDC(0, 56), V_POLY},
    [SR_CHOIR]    = {{{EG_VO, 0}, {EG_VO, 4}, {EG_FM, 4}}, GR(56, 103), GR(88, 103), GR(104, 119), GR(80, 111), GR(56, 95), GR(0, 7), GR4(0, 7), GFT(0, 0), GNOWV, GLFO(8, 2), GDC(0, 60), V_POLY},
    [SR_ORGAN]    = {{{EG_WH, 8}, {EG_WH, 1}, {EG_PH, 1}}, GR(0, 7), GR(64, 71), GR(120, 127), GR(24, 47), GR(56, 95), GR(0, 7), GR4(0, 3), GFT(0, 0), GNOWV, 0, GDC(0, 8), V_POLY},
    [SR_EP]       = {{{EG_FM, 0}, {EG_FM, 5}, {EG_PH, 8}}, GR(0, 7), GR(72, 95), GR(16, 55), GR(48, 79), GR(56, 95), GR(0, 15), GR4(4, 19), GFT(0, 0), GNOWV, 0, GDC(0, 20), V_POLY},
    [SR_STAB]     = {{{EG_A, 18}, {EG_PH, 10}, {EG_FM, 3}}, GR(0, 7), GR(56, 71), GR(24, 71), GR(24, 47), GR(48, 79), GR(16, 39), GR4(16, 39), GFT(0, 0), GNOWV, 0, GDC(8, 28), V_POLY},
    [SR_DUBCHORD] = {{{EG_A, 18}, {EG_WH, 2}, {EG_A, 3}}, GR(0, 7), GR(48, 71), GR(16, 63), GR(32, 55), GR(48, 71), GR(24, 47), GR4(12, 31), GFT(2, 0), GNOWV, 0, GDC(0, 32), V_POLY},
    /* ---- arps, leads */
    [SR_PLUCK]    = {{{EG_A, 20}, {EG_FM, 7}, {EG_A, 8}}, GR(0, 7), GR(40, 63), GR(0, 23), GR(32, 63), GR(56, 87), GR(24, 47), GR4(28, 55), GFT(0, 0), GNOWV, 0, GDC(0, 8), V_POLY},
    [SR_BELL]     = {{{EG_FM, 1}, {EG_FM, 5}, {EG_A, 13}}, GR(0, 7), GR(80, 103), GR(0, 7), GR(72, 103), GR(72, 103), GR(0, 15), GR4(0, 11), GFT(0, 0), GNOWV, 0, GDC(0, 8), V_POLY},
    [SR_LEAD]     = {{{EG_A, 21}, {EG_A, 0}, {EG_PH, 14}}, GR(0, 23), GR(48, 87), GR(56, 95), GR(32, 71), GR(56, 87), GR(16, 39), GR4(16, 39), GFT(0, 16), GNOWV, GLFO(4, 6), GDC(12, 12), V_LEGATO},
    [SR_VOXLEAD]  = {{{EG_VO, 1}, {EG_VO, 6}, {EG_A, 21}}, GR(0, 23), GR(56, 87), GR(56, 95), GR(32, 71), GR(56, 95), GR(8, 31), GR4(8, 23), GFT(0, 20), GNOWV, GLFO(4, 6), GDC(0, 20), V_LEGATO},
    [SR_CHIP]     = {{{EG_LO, 3}, {EG_LO, 13}, {EG_LO, 16}}, GR(0, 7), GR(40, 79), GR(24, 79), GR(16, 47), GR(72, 111), GR(8, 31), GR4(8, 27), GFT(0, 0), GNOWV, 0, 0, V_LEGATO},
    [SR_KEYS]     = {{{EG_A, 23}, {EG_PH, 15}, {EG_WH, 1}}, GR(0, 7), GR(64, 87), GR(16, 55), GR(40, 71), GR(56, 95), GR(0, 23), GR4(4, 19), GFT(0, 0), GNOWV, 0, GDC(0, 20), V_POLY},
    /* ---- textures */
    [SR_DRONE]    = {{{EG_A, 22}, {EG_A, 19}, {EG_FM, 4}}, GR(88, 119), GR(88, 103), GR(120, 127), GR(96, 127), GR(32, 63), GR(16, 39), GR4(0, 11), GFT(0, 0), GNOWV, GLFO(16, 1), GDC(0, 44), V_POLY},
    [SR_NOISE]    = {{{EG_NO, 4}, {EG_NO, 7}, {EG_NO, 5}}, GR(80, 119), GR(88, 103), GR(104, 127), GR(80, 119), 0, 0, 0, GFT(7, 0), GNOWV, 0, 0, V_POLY},
    [SR_TEXTURE]  = {{{EG_NO, 5}, {EG_NO, 6}, {EG_LO, 11}}, GR(64, 111), GR(88, 103), GR(104, 127), GR(88, 119), GR(40, 71), GR(16, 47), GR4(0, 11), GFT(2, 0), GNOWV, GLFO(20, 2), GDC(0, 32), V_POLY},
    [SR_HOOVER]   = {{{EG_A, 17}, {EG_A, 6}, {EG_PH, 6}}, GR(0, 15), GR(56, 95), GR(56, 95), GR(32, 63), GR(56, 95), GR(8, 23), GR4(8, 31), GFT(0, 0), GNOWV, 0, GDC(24, 48), V_POLY},
    [SR_WALL]     = {{{EG_A, 22}, {EG_A, 15}, {EG_A, 11}}, GR(24, 63), GR(88, 103), GR(104, 127), GR(80, 111), GR(48, 71), GR(0, 7), GR4(0, 7), GFT(0, 0), GNOWV, GLFO(8, 2), GDC(40, 56), V_POLY},
    [SR_GTR]      = {{{EG_PH, 13}, {EG_PH, 2}, {EG_A, 8}}, GR(0, 15), GR(64, 95), GR(56, 103), GR(48, 79), GR(56, 79), GR(16, 39), GR4(4, 19), GFT(2, 0), GNOWV, 0, GDC(12, 28), V_POLY},
    [SR_SINEKEY]  = {{{EG_A, 5}, {EG_FM, 0}, {EG_A, 23}}, GR(0, 7), GR(64, 95), GR(24, 63), GR(48, 79), GR(56, 95), GR(0, 15), GR4(0, 15), GFT(0, 0), GNOWV, 0, GDC(0, 8), V_POLY},
    /* ---- (0.9.9-gen1) the archetypes the genres lacked */
    /* EBM, darkwave: a square through the MS20 filter, short, the envelope on it, driven */
    [SR_EBMBASS]  = {{{EG_A, 2}, {EG_A, 14}, NO_}, GR(0, 7), GR(32, 47), GR(8, 31), GR(8, 23), GR(48, 71), GR(40, 63), GR4(28, 47), GFT(5, 0), GWV(1, 0, 1), 0, GDC(28, 0), V_LEGATO},
    /* trap's 808: a sine's long boom, gliding (the SLIDEs), driven */
    [SR_808]      = {{{EG_A, 7}, {EG_A, 7}, NO_}, GR(0, 7), GR(88, 111), GR(0, 15), GR(32, 47), GR(56, 79), GR(0, 7), GR4(0, 7), GFT(0, 48), GWV(3, 3, 1), 0, GDC(36, 0), V_LEGATO},
    /* jazz, bossa, boom bap: an upright's triangle, plucked */
    [SR_UPRIGHT]  = {{{EG_A, 7}, {EG_A, 7}, NO_}, GR(0, 7), GR(48, 63), GR(0, 23), GR(24, 39), GR(40, 63), GR(0, 15), GR4(8, 23), GFT(0, 0), GWV(2, 2, 1), 0, 0, V_LEGATO},
    /* techno's rolling bass: a saw (or a square), short, resonant, the envelope on it, driven */
    [SR_TECHBASS] = {{{EG_A, 14}, {EG_A, 2}, NO_}, GR(0, 7), GR(40, 55), GR(0, 31), GR(16, 31), GR(32, 55), GR(32, 55), GR4(20, 39), GFT(0, 0), GWV(0, 1, 1), 0, GDC(32, 0), V_LEGATO},
    /* trance's supersaw (SAW3), chorused */
    [SR_SUPERSAW] = {{{EG_A, 6}, {EG_A, 17}, {EG_A, 0}}, GR(0, 15), GR(64, 95), GR(64, 95), GR(40, 71), GR(72, 103), GR(8, 23), GR4(8, 23), GFT(0, 0), GWV(7, 7, 0), 0, GDC(0, 48), V_POLY},
};
/* (gen3) the amp envelope a role keeps whatever its archetype (gen.c gen_sound clamps ATK DEC SUS REL into these GR ranges):
 * a LEAD's sustain 44..75 % (never the organ's or the choir's full one: a medium decay to it, a short release; a bell's,
 * an EP's, a pluck's lead, its archetype's sustain under 56, a pluck's), an ARP's a pluck's (a fast attack, a short-medium decay, a low sustain), the CHORDS stabbed short (STAB PULSE DUB SKANK COMP
 * BROKEN TRANCE: GSTAB) or held slow in and out (HOLD SWELL DRONE), a BASS fast in; the rest as the archetype has it */
static const uint8_t ENV_RULE[6][4] = {
    {GR(0, 23), GR(48, 95), GR(56, 95), GR(16, 47)},    /* lead */
    {GR(0, 7), GR(32, 79), GR(0, 39), GR(16, 63)},      /* arp: a pluck */
    {GR(0, 7), GR(32, 71), GR(0, 47), GR(16, 47)},      /* chords stabbed */
    {GR(16, 127), GR(48, 127), GR(56, 127), GR(64, 127)},   /* chords held */
    {GR(0, 15), GR(24, 127), GR(0, 127), GR(8, 63)},    /* bass */
    {GR(0, 127), GR(0, 127), GR(0, 127), GR(0, 127)},   /* the others */
};
#define GSTAB(p) ((0xC7Cu >> (p)) & 1u)                 /* CS_STAB PULSE DUB SKANK COMP BROKEN TRANCE: the chords short */
/* (0.6.95) a throw its own: the neighbours a throw may take instead (the drum map's family, the chords' play, a role's
 * recipe), so two throws of a genre are not the same piece in another key */
static const uint8_t DM_ALT[DM_NONE][2] = {
    [DM_HOUSE] = {DM_DEEP, DM_TECHHOUSE}, [DM_DEEP] = {DM_HOUSE, DM_MICRO}, [DM_MICRO] = {DM_DEEP, DM_DUBTECH},
    [DM_TECHHOUSE] = {DM_HOUSE, DM_TECHNO}, [DM_TECHNO] = {DM_DETROIT, DM_HYPNO}, [DM_DETROIT] = {DM_TECHNO, DM_HOUSE},
    [DM_DUBTECH] = {DM_MICRO, DM_HYPNO}, [DM_HYPNO] = {DM_TECHNO, DM_DUBTECH}, [DM_HARD] = {DM_TECHNO, DM_EBM},
    [DM_ELECTRO] = {DM_RBOX, DM_KLING}, [DM_EBM] = {DM_HARD, DM_DARKWAVE}, [DM_ITALO] = {DM_DISCO, DM_SYNTHPOP},
    [DM_SYNTHPOP] = {DM_ITALO, DM_RBOX}, [DM_DARKWAVE] = {DM_POSTPUNK, DM_EBM}, [DM_POSTPUNK] = {DM_JOYDIV, DM_NEWORDER},
    [DM_JOYDIV] = {DM_POSTPUNK, DM_NEWORDER}, [DM_NEWORDER] = {DM_POSTPUNK, DM_JOYDIV},
    [DM_TRIBAL] = {DM_POSTPUNK, DM_AFRO}, [DM_RBOX] = {DM_SYNTHPOP, DM_KLING}, [DM_KLING] = {DM_RBOX, DM_MOTORIK},
    [DM_MOTORIK] = {DM_KLING, DM_INDIE}, [DM_INDIE] = {DM_MOTORIK, DM_SHOEGAZE}, [DM_SHOEGAZE] = {DM_INDIE, DM_SPARSE},
    [DM_FUNK] = {DM_AFRO, DM_BOOMBAP}, [DM_DISCO] = {DM_ITALO, DM_HOUSE}, [DM_BOOMBAP] = {DM_FUNK, DM_TRAP},
    [DM_TRAP] = {DM_BOOMBAP, DM_DUBSTEP}, [DM_UKG] = {DM_BREAKS, DM_HOUSE}, [DM_DUBSTEP] = {DM_TRAP, DM_DNB},
    [DM_BREAKS] = {DM_UKG, DM_DNB}, [DM_DNB] = {DM_BREAKS, DM_DUBSTEP}, [DM_ONEDROP] = {DM_STEPPERS, DM_ROCKERS},
    [DM_STEPPERS] = {DM_ONEDROP, DM_ROCKERS}, [DM_ROCKERS] = {DM_ONEDROP, DM_STEPPERS}, [DM_JAZZ] = {DM_BOSSA, DM_SPARSE},
    [DM_BOSSA] = {DM_JAZZ, DM_AFRO}, [DM_AFRO] = {DM_FUNK, DM_TRIBAL}, [DM_SPARSE] = {DM_JAZZ, DM_IDM}, [DM_IDM] = {DM_SPARSE, DM_BREAKS},
};
static const uint8_t CS_ALT[CS_TRANCE + 1][2] = {
    [CS_HOLD] = {CS_SWELL, CS_PULSE}, [CS_SWELL] = {CS_HOLD, CS_PULSE}, [CS_STAB] = {CS_PULSE, CS_DUB}, [CS_PULSE] = {CS_STAB, CS_HOLD},
    [CS_DUB] = {CS_STAB, CS_SPARSE}, [CS_SKANK] = {CS_DUB, CS_STAB}, [CS_COMP] = {CS_STAB, CS_BROKEN}, [CS_POWER] = {CS_PULSE, CS_HOLD},
    [CS_DRONE] = {CS_HOLD, CS_SWELL}, [CS_SPARSE] = {CS_HOLD, CS_DUB}, [CS_BROKEN] = {CS_COMP, CS_STAB}, [CS_TRANCE] = {CS_PULSE, CS_STAB},
};
/* the names the song generator never picks (the cheerful, the odd), whatever the role */
static const char *const GEN_NEVER[] = {"ARCADE", "BYTEBEAT", "GOSPEL", "ROCK DRIVE", "METAL", "BYTE VAR", "FLOAT FM",
                                        "FLOAT ARP", "BYTE TONE"};

/* -------------------------------------------------------------------------------------------------- forms --- */
enum { FM_LOOP, FM_PARTS, FM_CLUB, FM_ARC };            /* one loop; A B A C; intro build drop break; a slow arc */

/* -------------------------------------------------------------------------------------------------- genres --- */
#define SC_BLUE 12u                                     /* (the other scales: song_dice.c) */
#define SC_PEN 5u
typedef struct {
    const char *name;
    uint8_t bpm_lo, bpm_hi, swing;                      /* swing: G_SWING, 0..100 */
    uint8_t scale[4];                                   /* (0.8.3: a mode of seven is the key; PEN MPEN BLUES the lead's
                                                         * pentatonic over the mode that holds it) */
    uint8_t drum, drum_pct, kit, ghosts, hats, fills;   /* the map, how often drums play, the kit, snare ghosts a bar
                                                         * at full amount, the hats' cell, the fill */
    int8_t  sd_micro;                                   /* the snare laid back (sixteenths of a step) */
    uint8_t bass[3], bass_lo, kick_mode, bass_gate;     /* three archetypes, the root's lowest note, the kick rule, GATE */
    uint8_t prog[3], cbars, pads, voicing;              /* three progressions, bars a chord, the chords' play, voicing */
    uint8_t arp[2], lead;
    uint8_t amt[GP_N];                                  /* the parts' default amounts (0: the part sleeps) */
    uint8_t snd[GP_N];                                  /* the kit (GP_DRUMS) and a recipe a role */
    uint8_t tex;                                        /* the sixth track's recipe (TRACKS 6) */
    uint8_t rtype, dtime, wet, dtype;                   /* reverb type, delay time (N_DTIME), 0..100 wet, delay type bit 0
                                                         * ANALOG / DIGITAL, bit 1 PING-PONG */
    uint8_t form;
    uint8_t human;                                      /* 0 a machine .. 100 a band: HUMAN and TIMING */
    uint8_t hx;                                         /* (0.8.3) the harmony: its family HF_ and its HX_ colours */
} ggenre_t;
#define AMT(d, b, c, a, l) {d, b, c, a, l}
static const ggenre_t GENRE[] = {
    /* ---- the club: house */
    /* Chicago / New York house (Frankie Knuckles, Marshall Jefferson, Kerri Chandler): a 909 four on the floor, the
     * open hat off the beat, M1 piano / organ stabs on the chords, a synth bass answering the kick, swing ~56 % */
    {"HOUSE", 120, 126, 30, {SC_MIN, SC_DOR, SC_MAJ, SC_MIN}, DM_HOUSE, 100, KIT_909, 0, HC_HOUSE, FL_ROLL, 0,
     {BA_OFFBEAT, BA_BOUNCE, BA_OFFOCT}, 36, KM_ANSWER, 45, {PR_I_IV, PR_I_VI, PR_DEEP4}, 1, CS_STAB, VM_PLANE,
     {AP_BROKEN, AP_UP}, LD_HOOK, AMT(90, 75, 70, 0, 0), {KIT_909, SR_FMBASS, SR_KEYS, SR_PLUCK, SR_ORGAN}, SR_NOISE,
     0, 1, 40, 0, FM_CLUB, 10, HF_JAZZY},
    /* Larry Heard (Can You Feel It), Chez Damier, Kerri Chandler: an 808, a Rhodes on 7ths and 9ths through the
     * delay, a walking sub, swing ~57 % */
    {"DEEP HOUSE", 118, 124, 35, {SC_MIN, SC_DOR, SC_DOR, SC_MPEN}, DM_DEEP, 100, KIT_808, 0, HC_HOUSE, FL_DROP, 1,
     {BA_DEEPWALK, BA_OFFBEAT, BA_UKG}, 36, KM_ANSWER, 50, {PR_I_IV, PR_JAZZ, PR_IV_I}, 2, CS_STAB, VM_PLANE,
     {AP_BROKEN, AP_UPDOWN}, LD_SPARSE, AMT(85, 75, 75, 0, 0), {KIT_808, SR_SUB, SR_EP, SR_KEYS, SR_SINEKEY}, SR_TEXTURE,
     0, 1, 55, 0, FM_CLUB, 20, HF_JAZZY},
    /* Perlon, Villalobos, Akufen: clicks for a kit, a plucked loop that orbits, a dry FM bass, swing ~54 % */
    {"MICRO HOUSE", 122, 128, 20, {SC_MIN, SC_DOR, SC_MIN, SC_PHRY}, DM_MICRO, 100, KIT_CLICK, 0, HC_FLAT, FL_DROP, 0,
     {BA_OFFBEAT, BA_DEEPWALK, BA_ROLL16}, 36, KM_ANSWER, 40, {PR_ONE, PR_I_VII, PR_I_IV}, 2, CS_SPARSE, VM_QUART,
     {AP_CYCLE3, AP_WALK}, LD_SPARSE, AMT(70, 65, 40, 45, 20), {KIT_CLICK, SR_FMBASS, SR_PLUCK, SR_PLUCK, SR_CHIP}, SR_TEXTURE,
     0, 1, 45, 0, FM_CLUB, 0, HF_LOOP},
    /* (gen6b, the owner: "nothing like the real thing") Green Velvet, Hot Since 82, Solardo, Terry Francis: a 909
     * four on the floor that never lets go, the clap on 2 and 4, shuffled 16th hats, the open hat off the beat, a
     * syncopated rim; a rolling bass off the kick, one stab with its filter moving; no lead, swing ~55 % */
    {"TECH HOUSE", 124, 128, 22, {SC_MIN, SC_DOR, SC_MIN, SC_PHRY}, DM_TECHHOUSE, 100, KIT_909, 0, HC_HOUSE, FL_ROLL, 0,
     {BA_VILLA, BA_OFFOCT, BA_VILLA}, 36, KM_ANSWER, 40, {PR_ONE, PR_I_VII, PR_ONE}, 4, CS_STAB, VM_SUS,
     {AP_BROKEN, AP_SEQ}, LD_NONE, AMT(95, 90, 40, 0, 0), {KIT_909, SR_TECHBASS, SR_DUBCHORD, SR_PLUCK, SR_LEAD}, SR_NOISE,
     0, 1, 30, 0, FM_CLUB, 5, HF_LOOP},
    /* Phuture (Acid Tracks), DJ Pierre, Hardfloor: the TB-303 over a 909 (the owner: as it is) */
    {"ACID HOUSE", 122, 130, 8, {SC_MIN, SC_PHRY, SC_DOR, SC_MIN}, DM_HOUSE, 100, KIT_909, 0, HC_HOUSE, FL_ROLL, 0,
     {BA_ACID, BA_ACID, BA_ACID}, 36, KM_SUSTAIN, 60, {PR_ONE, PR_I_VII, PR_I_IV}, 2, CS_STAB, VM_SUS,
     {AP_UP, AP_SEQ}, LD_NONE, AMT(85, 90, 30, 20, 0), {KIT_909, SR_ACID, SR_STAB, SR_PLUCK, SR_LEAD}, SR_NOISE,
     0, 1, 35, 0, FM_CLUB, 0, HF_LOOP},
    /* ---- the club: techno */
    /* Detroit / Berlin (Jeff Mills, Robert Hood, Surgeon): a 909, a rolling bass, one hypnotic stab with its filter
     * swept over the bars */
    {"TECHNO", 128, 136, 4, {SC_MIN, SC_PHRY, SC_MIN, SC_MIN}, DM_TECHNO, 100, KIT_909, 0, HC_MACHINE, FL_NONE, 0,
     {BA_OFFBEAT, BA_PEDAL, BA_OFFBEAT}, 33, KM_SUSTAIN, 45, {PR_ONE, PR_PHRY, PR_I_VII}, 4, CS_STAB, VM_PLANE,
     {AP_PULSE, AP_SEQ}, LD_OSTINATO, AMT(95, 60, 25, 0, 0), {KIT_909, SR_TECHBASS, SR_STAB, SR_PLUCK, SR_LEAD}, SR_NOISE,
     0, 2, 35, 0, FM_CLUB, 0, HF_LOOP},
    /* Derrick May (Strings of Life), Carl Craig, UR: string stabs on minor 7ths, a melodic lead, a 909 with a light
     * swing */
    {"DETROIT", 124, 132, 12, {SC_MIN, SC_DOR, SC_MIN, SC_MAJ}, DM_DETROIT, 100, KIT_909, 0, HC_HOUSE, FL_ROLL, 0,
     {BA_DETROIT, BA_OCTBNC, BA_SYNTHW}, 36, KM_SUSTAIN, 50, {PR_I_VI, PR_MINOR4, PR_DEEPVI}, 1, CS_STAB, VM_PLANE,
     {AP_UPDOWN, AP_BROKEN}, LD_MOTIF, AMT(90, 80, 65, 0, 40), {KIT_909, SR_HOOK, SR_STRINGS, SR_PLUCK, SR_LEAD}, SR_PAD,
     0, 1, 45, 0, FM_CLUB, 5, HF_JAZZY},
    /* Basic Channel (Phylyps Trak), Maurizio, Deepchord: one chord stab through a long delay and reverb, its filter
     * swept, a sub, the hats soft */
    {"DUB TECHNO", 118, 126, 6, {SC_MIN, SC_DOR, SC_MIN, SC_MPEN}, DM_DUBTECH, 100, KIT_DUB, 0, HC_FLAT, FL_DUB, 0,
     {BA_DUB, BA_PEDAL, BA_OFFBEAT}, 31, KM_SUSTAIN, 70, {PR_ONE, PR_I_IV, PR_I_VI}, 4, CS_DUB, VM_RICH,
     {AP_PULSE, AP_CYCLE3}, LD_SPARSE, AMT(60, 60, 75, 20, 15), {KIT_DUB, SR_SUB, SR_DUBCHORD, SR_PLUCK, SR_SINEKEY}, SR_TEXTURE,
     4, 4, 85, 0, FM_ARC, 0, HF_LOOP},
    /* Ancient Methods, Perc, the Birmingham sound: an industrial kit hard, a rolling 16th bass, a hoover lead */
    {"HARD TECHNO", 138, 150, 0, {SC_PHRY, SC_MIN, SC_HARM, SC_PHRY}, DM_HARD, 100, KIT_INDUS, 0, HC_MACHINE, FL_DROP, 0,
     {BA_ROLL16, BA_PSY, BA_OCTBNC}, 33, KM_SUSTAIN, 40, {PR_ONE, PR_PHRY, PR_I_VII}, 4, CS_STAB, VM_POWER,
     {AP_PULSE, AP_OCTAVES}, LD_OSTINATO, AMT(100, 80, 20, 20, 0), {KIT_INDUS, SR_TECHBASS, SR_STAB, SR_PLUCK, SR_HOOVER}, SR_NOISE,
     1, 2, 25, 0, FM_CLUB, 0, HF_LOOP},
    /* Donato Dozzy, Voices From The Lake: a click kit, a 16th bass, a pluck cycling in fives and threes, everything
     * orbiting slowly */
    {"HYPNOTIC", 128, 134, 2, {SC_PHRY, SC_MIN, SC_DOR, SC_HARM}, DM_HYPNO, 100, KIT_CLICK, 0, HC_FLAT, FL_NONE, 0,
     {BA_ROLL16, BA_PSY, BA_OFFBEAT}, 33, KM_SUSTAIN, 40, {PR_ONE, PR_ONE, PR_I_II}, 4, CS_DUB, VM_PLANE,
     {AP_CYCLE5, AP_CYCLE3}, LD_OSTINATO, AMT(90, 80, 30, 50, 30), {KIT_CLICK, SR_TECHBASS, SR_STAB, SR_PLUCK, SR_PLUCK}, SR_TEXTURE,
     0, 4, 50, 0, FM_ARC, 0, HF_LOOP},
    /* Paul van Dyk (For an Angel), ATB, Ferry Corsten: supersaw chords gated in 16ths, a supersaw lead, the bass off
     * the beat */
    {"TRANCE", 134, 140, 0, {SC_MIN, SC_MIN, SC_HARM, SC_MIN}, DM_NEWORDER, 100, KIT_909, 0, HC_FLAT, FL_ROLL, 0,
     {BA_PSY, BA_PUMP8, BA_OFFBEAT}, 36, KM_ANSWER, 45, {PR_AEOL, PR_EPIC, PR_50S}, 2, CS_TRANCE, VM_RICH,
     {AP_TRANCE, AP_UP}, LD_MOTIF, AMT(85, 85, 70, 70, 50), {KIT_909, SR_HOOK, SR_SUPERSAW, SR_PLUCK, SR_SUPERSAW}, SR_NOISE,
     2, 4, 55, 3, FM_CLUB, 0, HF_POP | HX_RAISE},
    /* ---- electro, EBM, italo, synth pop, the wave */
    /* (gen6) Anthony Rother (Sex With The Machines, Hacker), DMX Krew, Cybotron: a pure 808 broken (the kick on 1, the
     * & of 2, 3's &), the clap on 2 and 4, 16th hats, cowbell; a syncopated bass (root octave seventh), i VII VI VII, a
     * saw lead in octaves and fifths; dry */
    {"ELECTRO", 128, 135, 0, {SC_MIN, SC_PHRY, SC_MIN, SC_PHRY}, DM_ELECTRO, 100, KIT_808, 0, HC_MACHINE, FL_TOM, 0,
     {BA_ROTHER, BA_SYNTHW, BA_ROTHER}, 36, KM_INTERLOCK, 40, {PR_ELEC, PR_I_VII, PR_ELEC}, 1, CS_STAB, VM_SUS,
     {AP_OCTAVES, AP_SEQ}, LD_ELEC, AMT(90, 90, 30, 30, 50), {KIT_808, SR_HOOK, SR_STAB, SR_CHIP, SR_LEAD}, SR_VOXLEAD,
     1, 2, 20, 1, FM_PARTS, 0, HF_LOOP},
    /* DAF (Der Mussolini), Nitzer Ebb: a sequenced mono MS-20 bass in driving 8ths / 16ths, a dry minimal kit, VOICE
     * shouts */
    {"EBM", 92, 104, 0,   /* (p4b, p6b: the owner's references, 94 and 101 BPM) */ {SC_PHRY, SC_MIN, SC_PHRY, SC_PHRY}, DM_EBM, 100, KIT_INDUS, 0, HC_MACHINE, FL_NONE, 1,
     {BA_NITZER3, BA_DARK8, BA_SUICIDE}, 33, KM_DOUBLE, 95, {PR_ONE, PR_ONE, PR_I_VI}, 4, CS_STAB, VM_POWER,
     {AP_SEQ, AP_OCTAVES}, LD_EBM, AMT(100, 100, 0, 20, 0),   /* (p3, the owner: the bass half as fast, with body, not
                                     * a pluck; no lead) */ {KIT_INDUS, SR_EBMBASS, SR_STAB, SR_PLUCK, SR_VOXLEAD}, SR_NOISE,
     1, 0, 8, 0, FM_PARTS, 0, HF_LOOP},   /* (gen6, the owner: DAF, Gesaffelstein (Viol, Aleph): a dry 909 four on the
                                     * floor, the clap on 2 and 4, accented 16th hats, a metal hit, no open hat; 16ths of
                                     * bass that never stop (gen6b), a second sequence an octave up, one chord;
                                     * a shout on the bar's one; no pads, no reverb) */
    /* Gazebo, Kano, Laserdance, Moroder: an 808, the octave bass, brassy stabs, a saw lead's hook */
    {"ITALO", 118, 124, 0,   /* (p6b: the owner's reference, 120) */ {SC_MIN, SC_MIN, SC_MAJ, SC_MIN}, DM_ITALO, 100, KIT_808, 0, HC_MACHINE, FL_TOM, 0,
     {BA_ITALO, BA_ITALO16, BA_SYNTHW}, 36, KM_DOUBLE, 40, {PR_AEOL, PR_ANDAL, PR_POP}, 2, CS_STAB, VM_PLAIN,   /* (p7b: the reference, ~2 bars a chord) */
     {AP_OCTAVES, AP_UP}, LD_HOOK, AMT(90, 90, 60, 60, 45), {KIT_808, SR_HOOK, SR_STAB, SR_PLUCK, SR_LEAD}, SR_PAD,
     0, 1, 40, 2, FM_PARTS, 0, HF_POP | HX_RAISE | HX_LIFT},
    /* Depeche Mode (Just Can't Get Enough), OMD, the Human League: an 808, a pad held, a plucked sequence, a saw
     * hook */
    {"SYNTH POP", 112, 126, 0, {SC_MIN, SC_MAJ, SC_MIN, SC_MAJ}, DM_SYNTHPOP, 100, KIT_808, 0, HC_MACHINE, FL_TOM, 0,
     {BA_ITALO, BA_HOOK, BA_ROCK8}, 36, KM_DOUBLE, 50, {PR_AEOL, PR_POP, PR_50S}, 1, CS_HOLD, VM_PLAIN,
     {AP_SEQ, AP_UPDOWN}, LD_HOOK, AMT(85, 85, 65, 55, 60), {KIT_808, SR_HOOK, SR_PAD, SR_PLUCK, SR_LEAD}, SR_STRINGS,
     0, 1, 45, 2, FM_PARTS, 5, HF_POP | HX_RAISE | HX_BOR | HX_LIFT},
    /* (gen6, the owner's reference) Crystal Castles (I, II), Pictureplane, Salem: a dirty kit, the kick long, 8th hats,
     * a 16th bass arpeggio (root octave seventh octave), a chiptune lead's octave glitch, a dark pad in a long reverb */
    {"DARKWAVE", 132, 140, 0,   /* (p6b: the owner's reference, 136) */ {SC_MIN, SC_PHRY, SC_MIN, SC_MIN}, DM_DARKWAVE, 100, KIT_INDUS, 0, HC_MACHINE, FL_NONE, 0,
     {BA_CCARP, BA_DARK8, BA_CCARP}, 31, KM_DOUBLE, 35, {PR_I_VI, PR_EPIC, PR_AEOL}, 2, CS_HOLD, VM_PLAIN,   /* (p7b: the reference changes chord every ~2 bars) */
     {AP_PULSE, AP_SEQ}, LD_CC, AMT(95, 90, 70, 0, 35), {KIT_INDUS, SR_EBMBASS, SR_PAD, SR_PLUCK, SR_CHIP}, SR_NOISE,
     2, 1, 75, 0,   /* (p6b: the reference sustained 74 % of the time: the pads held, wetter, the lead under them) */ FM_PARTS, 0, HF_POP},
    /* Joy Division and New Order: Hook's bass high on the neck, Morris's drums, Sumner's pulsed chords */
    {"POST PUNK", 118, 148, 0, {SC_MIN, SC_DOR, SC_MIN, SC_MAJ}, DM_POSTPUNK, 100, KIT_SOFT, 1, HC_FLAT, FL_TOM, 1,
     {BA_HOOK2, BA_BLUEMON, BA_CEREMONY}, 40, KM_DOUBLE, 70, {PR_I_VI, PR_I_IV, PR_AEOL}, 2, CS_PULSE, VM_PLAIN,
     {AP_OCTAVES, AP_UPDOWN}, LD_HOOK, AMT(95, 100, 55, 45, 50), {KIT_SOFT, SR_HOOK, SR_STRINGS, SR_PLUCK, SR_LEAD}, SR_CHOIR,
     1, 2, 45, 0, FM_PARTS, 35, HF_POP},   /* (0.6.97: after Joy Division and New Order: Hook's bass, Morris's drums, Sumner's
                                    * pulsed chords, a sequenced octave arp, strings and choir behind) */
    /* Adam and the Ants, Bow Wow Wow (the Burundi beat), Killing Joke: toms for a kit, an angular bass, a chanted
     * lead */
    {"TRIBAL PUNK", 140, 165, 0, {SC_MIN, SC_PHRY, SC_MIN, SC_DOR}, DM_TRIBAL, 100, KIT_SOFT, 0, HC_FLAT, FL_TOM, 1,
     {BA_ANGULAR, BA_ROCK8, BA_HOOK}, 36, KM_SUSTAIN, 60, {PR_ONE, PR_I_VII, PR_I_VI}, 4, CS_POWER, VM_POWER,
     {AP_PULSE, AP_UP}, LD_OSTINATO, AMT(95, 95, 40, 10, 45), {KIT_SOFT, SR_ROCKBASS, SR_GTR, SR_PLUCK, SR_VOXLEAD}, SR_NOISE,
     0, 1, 55, 0, FM_PARTS, 40, HF_LOOP},
    /* (gen6, the owner's reference) Suicide (Ghost Rider, Dream Baby Dream), Martin Rev: a rhythm box like a machine
     * (the kick on every beat, the snare on 2 and 4, 8th hats, no fills), a bass drone or Rev's pulse, a Farfisa drone,
     * one note and its step of tension, the delay's feedback high; minimal */
    {"MINIMAL SYNTH", 110, 130, 0, {SC_PHRY, SC_MIN, SC_PHRY, SC_MIN}, DM_RBOX, 100, KIT_SOFT, 0, HC_MACHINE, FL_NONE, 0,
     {BA_PEDAL, BA_SUICIDE, BA_PEDAL}, 33, KM_SUSTAIN, 40, {PR_ONE, PR_ONE, PR_PHRY}, 4, CS_DRONE, VM_POWER,
     {AP_PULSE, AP_SEQ}, LD_SUICIDE, AMT(80, 90, 50, 0, 45), {KIT_SOFT, SR_EBMBASS, SR_ORGAN, SR_CHIP, SR_VOXLEAD}, SR_NOISE,
     1, 2, 55, 0, FM_LOOP, 0, HF_LOOP},
    /* (gen6, the owner's reference) Kraftwerk (The Man Machine, Computer World, Numbers): a dry 808 on the grid (four on
     * the floor, the clap on 2 and 4, 8th hats, a zap), a 16th robot bass (root fifth octave fifth), a melody in fourths
     * and fifths, a three-note arp, a pad low; no swing, no reverb, an 8th digital delay */
    {"MAN MACHINE", 100, 118, 0, {SC_MIN, SC_MIN, SC_DOR, SC_MIN}, DM_KLING, 100, KIT_CLICK, 0, HC_MACHINE, FL_NONE, 0,
     {BA_SYNTHW, BA_SEQ16, BA_SYNTHW}, 36, KM_DOUBLE, 45, {PR_ONE, PR_I_IV, PR_CYCLE}, 4, CS_HOLD, VM_PLAIN,
     {AP_CYCLE3, AP_SEQ}, LD_KRAFT, AMT(80, 95, 35, 50, 65), {KIT_CLICK, SR_FMBASS, SR_PAD, SR_PLUCK, SR_LEAD}, SR_VOXLEAD,
     0, 1, 0, 1, FM_PARTS, 0, HF_LOOP},
    /* ---- kraut, indie, shoegaze, post rock */
    /* NEU! (Hallogallo), La Dusseldorf, Stereolab: the motorik beat, one chord, a droning guitar */
    {"MOTORIK", 118, 140, 0, {SC_MAJ, SC_MIX, SC_DOR, SC_MIN}, DM_MOTORIK, 100, KIT_SOFT, 0, HC_FLAT, FL_NONE, 0,
     {BA_MOTORIK, BA_ROCK8, BA_PUMP8}, 33, KM_DOUBLE, 60, {PR_ONE, PR_I_IV, PR_I_VII}, 4, CS_PULSE, VM_PLAIN,
     {AP_UPDOWN, AP_GLASS}, LD_TINTI, AMT(90, 95, 45, 50, 35), {KIT_SOFT, SR_ROCKBASS, SR_GTR, SR_PLUCK, SR_LEAD}, SR_DRONE,
     0, 1, 45, 0, FM_ARC, 30, HF_LOOP | HX_BOR},
    /* The Strokes, the Smiths, Pavement: a live kit, a root-eighths bass, guitars, a sung hook */
    {"INDIE", 110, 140, 0, {SC_MAJ, SC_MIN, SC_MIX, SC_MAJ}, DM_INDIE, 100, KIT_BREAKS, 1, HC_FLAT, FL_TOM, 1,
     {BA_INDIE, BA_ROOT5, BA_MCCART}, 36, KM_DOUBLE, 70, {PR_POP, PR_ROCK, PR_SECPOP}, 1, CS_POWER, VM_PLAIN,
     {AP_UPDOWN, AP_ALBERTI}, LD_HOOK, AMT(85, 90, 70, 20, 55), {KIT_BREAKS, SR_ROCKBASS, SR_GTR, SR_PLUCK, SR_KEYS}, SR_STRINGS,
     0, 1, 45, 0, FM_PARTS, 50, HF_POP | HX_SEC | HX_BOR | HX_LIFT},
    /* My Bloody Valentine (Soon), Slowdive, Ride: a wall of guitars, the drums under it, a voice in the haze */
    {"SHOEGAZE", 100, 130, 0, {SC_MAJ, SC_LYD, SC_MIX, SC_MIN}, DM_SHOEGAZE, 100, KIT_SOFT, 0, HC_FLAT, FL_ROLL, 1,
     {BA_ROCK8, BA_PUMP8, BA_ROOT5}, 36, KM_DOUBLE, 90, {PR_I_IV, PR_BORROW, PR_I_II}, 2, CS_SWELL, VM_RICH,
     {AP_UPDOWN, AP_GLASS}, LD_SPARSE, AMT(75, 85, 100, 30, 40), {KIT_SOFT, SR_ROCKBASS, SR_WALL, SR_PLUCK, SR_VOXLEAD}, SR_DRONE,
     2, 0, 90, 0, FM_ARC, 40, HF_COLOR | HX_BOR},
    /* Radiohead, Mogwai, Explosions in the Sky: add9 / sus chords, a clean arpeggio, long roots */
    {"POST ROCK", 84, 118, 0, {SC_MIN, SC_MAJ, SC_MIN, SC_MIN}, DM_INDIE, 85, KIT_BREAKS, 2, HC_FLAT, FL_TOM, 1,
     {BA_RADIO, BA_PUMP8, BA_FIFTHS}, 36, KM_SUSTAIN, 100, {PR_RADIO, PR_EPIC, PR_PYRAMID}, 2, CS_SWELL, VM_PAD,
     {AP_GLASS, AP_UPDOWN}, LD_MOTIF, AMT(60, 75, 80, 75, 45), {KIT_BREAKS, SR_ROCKBASS, SR_EP, SR_GTR, SR_VOXLEAD}, SR_STRINGS,
     4, 0, 70, 0, FM_ARC, 45, HF_COLOR},   /* (0.6.95: after Radiohead: add9 / sus chords, a clean arpeggio, long roots) */
    /* ---- funk, disco, hip hop, UK */
    /* James Brown (Funky Drummer), the Meters, Parliament: ghosted 16ths, the bass on the one, comped EP, swing ~59
     * % */
    {"FUNK", 96, 112, 22, {SC_DOR, SC_MIX, SC_MPEN, SC_BLUE}, DM_FUNK, 100, KIT_BREAKS, 4, HC_FUNK, FL_LINEAR, 1,
     {BA_JAMERSON, BA_SLAP, BA_AROUND}, 33, KM_INTERLOCK, 40, {PR_I_IV, PR_ONE, PR_IV_I}, 2, CS_COMP, VM_RICH,
     {AP_BROKEN, AP_WALK}, LD_HOOK, AMT(90, 100, 55, 20, 45), {KIT_BREAKS, SR_FMBASS, SR_EP, SR_PLUCK, SR_LEAD}, SR_ORGAN,
     0, 1, 30, 0, FM_PARTS, 60, HF_JAZZY | HX_APPR},
    /* Chic (Good Times), Donna Summer, Sylvester: the octave and walking bass, strings, the open hat off the beat */
    {"DISCO", 114, 124, 12, {SC_MIN, SC_DOR, SC_MAJ, SC_MIN}, DM_DISCO, 100, KIT_BREAKS, 1, HC_HOUSE, FL_ROLL, 0,
     {BA_ITALO, BA_GOODTIME, BA_BOUNCE}, 36, KM_DOUBLE, 40, {PR_I_IV, PR_TURN, PR_DEEP4}, 2, CS_COMP, VM_RICH,
     {AP_UP, AP_BROKEN}, LD_HOOK, AMT(90, 100, 65, 35, 45), {KIT_BREAKS, SR_ROCKBASS, SR_STRINGS, SR_PLUCK, SR_LEAD}, SR_STRINGS,
     0, 1, 40, 0, FM_PARTS, 30, HF_JAZZY | HX_RAISE | HX_SEC | HX_LIFT},
    /* DJ Premier, Pete Rock, J Dilla: a dusty kit with the MPC's swing (~58 %), an upright bass, Rhodes and keys, a
     * bell */
    {"BOOM BAP", 86, 96, 40, {SC_MIN, SC_DOR, SC_MPEN, SC_MIN}, DM_BOOMBAP, 100, KIT_SOFT, 1, HC_FUNK, FL_NONE, 2,
     {BA_BOOMBAP, BA_JUNGLE, BA_DUB}, 33, KM_DOUBLE, 65, {PR_I_IV, PR_MINOR4, PR_I_VI}, 2, CS_SPARSE, VM_RICH,
     {AP_BROKEN, AP_WALK}, LD_SPARSE, AMT(90, 85, 60, 20, 35), {KIT_SOFT, SR_UPRIGHT, SR_EP, SR_KEYS, SR_BELL}, SR_TEXTURE,
     0, 1, 40, 0, FM_LOOP, 50, HF_JAZZY},
    /* Metro Boomin, Zaytoven: an 808 gliding, rolling hats, a dark bell over a pad (half time) */
    {"TRAP", 132, 148, 0, {SC_MIN, SC_PHRY, SC_HARM, SC_MIN}, DM_TRAP, 100, KIT_808, 0, HC_FLAT, FL_HAT, 0,
     {BA_TRAP, BA_WOBBLE, BA_DUB}, 29, KM_SUSTAIN, 110, {PR_I_VI, PR_50S, PR_I_V}, 2, CS_SPARSE, VM_RICH,
     {AP_BROKEN, AP_UPDOWN}, LD_SPARSE, AMT(90, 100, 45, 40, 35), {KIT_808, SR_808, SR_PAD, SR_BELL, SR_BELL}, SR_CHOIR,
     2, 1, 55, 0, FM_PARTS, 0, HF_POP | HX_RAISE},
    /* MJ Cole, Todd Edwards, the Artful Dodger: the 2-step shuffle (~58 %), a reese / organ bass, EP chords, chopped
     * vocals */
    {"UK GARAGE", 130, 136, 40, {SC_MIN, SC_DOR, SC_MIN, SC_MAJ}, DM_UKG, 100, KIT_909, 2, HC_GARAGE, FL_KICK16, 1,
     {BA_UKG, BA_SPEEDG, BA_DEEPWALK}, 33, KM_INTERLOCK, 45, {PR_I_IV, PR_JAZZ, PR_DEEP4}, 1, CS_BROKEN, VM_RICH,
     {AP_BROKEN, AP_SEQ}, LD_HOOK, AMT(90, 90, 60, 35, 45), {KIT_909, SR_REESE, SR_EP, SR_PLUCK, SR_VOXLEAD}, SR_TEXTURE,
     0, 1, 40, 0, FM_CLUB, 10, HF_JAZZY},
    /* Skream (Midnight Request Line), Benga, Coki: half time, the wobble's LFO on the beat, sparse pads */
    {"DUBSTEP", 138, 142, 0, {SC_MIN, SC_PHRY, SC_HARM, SC_MIN}, DM_DUBSTEP, 100, KIT_808, 1, HC_FLAT, FL_BREAK, 0,
     {BA_WOBBLE, BA_DUB, BA_TRAP}, 29, KM_SUSTAIN, 110, {PR_ONE, PR_I_VI, PR_PHRY}, 2, CS_SPARSE, VM_SUS,
     {AP_PULSE, AP_BROKEN}, LD_SPARSE, AMT(85, 100, 40, 20, 30), {KIT_808, SR_WOW, SR_PAD, SR_PLUCK, SR_BELL}, SR_NOISE,
     2, 1, 55, 1, FM_CLUB, 0, HF_LOOP},
    /* The Chemical Brothers, Hardfloor, Plump DJs: a breakbeat, an acid bass, stabs */
    {"BREAKS", 126, 140, 6, {SC_MIN, SC_DOR, SC_PHRY, SC_MIN}, DM_BREAKS, 100, KIT_BREAKS, 3, HC_FUNK, FL_LINEAR, 1,
     {BA_ACID, BA_JUNGLE, BA_AROUND}, 33, KM_INTERLOCK, 50, {PR_ONE, PR_I_IV, PR_I_VI}, 2, CS_STAB, VM_RICH,
     {AP_UPDOWN, AP_BROKEN}, LD_OSTINATO, AMT(95, 90, 45, 30, 40), {KIT_BREAKS, SR_ACID, SR_STAB, SR_PLUCK, SR_LEAD}, SR_NOISE,
     1, 1, 40, 0, FM_PARTS, 20, HF_LOOP},
    /* Photek, Ed Rush & Optical (Wormhole), Goldie: the breakbeat, a reese bass, a dark pad */
    {"DRUM & BASS", 170, 176, 0, {SC_MIN, SC_DOR, SC_MIN, SC_MPEN}, DM_DNB, 100, KIT_BREAKS, 2, HC_FLAT, FL_LINEAR, 0,
     {BA_DNB, BA_JUNGLE, BA_WOBBLE}, 29, KM_DOUBLE, 90, {PR_I_VI, PR_DEEP4, PR_ONE}, 2, CS_SPARSE, VM_RICH,
     {AP_BROKEN, AP_CYCLE3}, LD_SPARSE, AMT(95, 95, 50, 20, 30), {KIT_BREAKS, SR_REESE, SR_PAD, SR_PLUCK, SR_LEAD}, SR_NOISE,
     2, 4, 50, 1, FM_CLUB, 0, HF_JAZZY},
    /* ---- dub, reggae */
    /* King Tubby, Lee Perry, Augustus Pablo: the one drop, a sub bass, the skank's organ through the delay, a
     * melodica-like lead */
    {"DUB", 70, 80, 20, {SC_MIN, SC_MAJ, SC_DOR, SC_MIN}, DM_ONEDROP, 100, KIT_DUB, 1, HC_FLAT, FL_DUB, 1,
     {BA_ONEDROP, BA_DUB, BA_ONEDROP2}, 33, KM_SUSTAIN, 50, {PR_I_VII, PR_I_IV, PR_ONE}, 2, CS_SKANK, VM_PLAIN,
     {AP_PULSE, AP_BROKEN}, LD_SPARSE, AMT(80, 100, 60, 10, 25), {KIT_DUB, SR_SUB, SR_DUBCHORD, SR_ORGAN, SR_LEAD}, SR_TEXTURE,
     1, 4, 80, 0, FM_ARC, 40, HF_LOOP},
    /* Jah Shaka, Mad Professor, the Twinkle Brothers: the kick on every beat, a heavy sub, the skank, a
     * melodica-like lead */
    {"STEPPERS", 76, 88, 16, {SC_MIN, SC_MIN, SC_DOR, SC_MAJ}, DM_STEPPERS, 100, KIT_DUB, 1, HC_FLAT, FL_DUB, 1,
     {BA_STEPPERS, BA_ROCKERS, BA_ONEDROP2}, 33, KM_DOUBLE, 45, {PR_I_VII, PR_I_IV, PR_ONE}, 2, CS_SKANK, VM_PLAIN,
     {AP_PULSE, AP_UP}, LD_SPARSE, AMT(90, 100, 65, 15, 30), {KIT_DUB, SR_SUB, SR_ORGAN, SR_ORGAN, SR_LEAD}, SR_TEXTURE,
     1, 4, 70, 0, FM_PARTS, 40, HF_LOOP},
    /* ---- jazz, latin */
    /* Bill Evans, Blue Note's trios: triplet swing (~66 %), a walking upright, comped 7ths, a voice-like lead */
    {"JAZZ", 110, 150, 40, {SC_MAJ, SC_DOR, SC_MIN, SC_MAJ}, DM_JAZZ, 90, KIT_BREAKS, 2, HC_FLAT, FL_NONE, 1,
     {BA_WALK, BA_WALK2, BA_TWOFEEL}, 33, KM_SUSTAIN, 95, {PR_JAZZ, PR_TURN, PR_JAZZ2}, 1, CS_COMP, VM_RICH,
     {AP_WALK, AP_UPDOWN}, LD_MOTIF, AMT(60, 95, 75, 20, 55), {KIT_BREAKS, SR_UPRIGHT, SR_EP, SR_EP, SR_VOXLEAD}, SR_ORGAN,
     0, 1, 40, 0, FM_LOOP, 70, HF_JAZZ | HX_RAISE | HX_SEC | HX_BOR | HX_APPR},
    /* Joao Gilberto, Jobim (Desafinado): the bossa's clave on a soft kit, a two-feel bass, nylon guitar chords */
    {"BOSSA", 120, 140, 6, {SC_MAJ, SC_LYD, SC_DOR, SC_MIN}, DM_BOSSA, 100, KIT_SOFT, 0, HC_FLAT, FL_NONE, 0,
     {BA_BOSSA, BA_TUMBAO, BA_TWOFEEL}, 33, KM_DOUBLE, 80, {PR_BOSSA, PR_I_II, PR_JAZZ}, 2, CS_COMP, VM_RICH,
     {AP_ALBERTI, AP_UPDOWN}, LD_MOTIF, AMT(75, 90, 70, 30, 50), {KIT_SOFT, SR_UPRIGHT, SR_GTR, SR_EP, SR_SINEKEY}, SR_STRINGS,
     0, 1, 40, 0, FM_LOOP, 50, HF_JAZZ | HX_RAISE | HX_SEC | HX_APPR},
    /* Fela Kuti, Tony Allen: the drums' interlocking parts, a riffing bass, organ and guitar figures, horns-like
     * lead */
    {"AFROBEAT", 110, 128, 16, {SC_DOR, SC_MIX, SC_MPEN, SC_MIN}, DM_AFRO, 100, KIT_BREAKS, 3, HC_FUNK, FL_LINEAR, 1,
     {BA_AROUND, BA_SKA, BA_ANGULAR}, 33, KM_INTERLOCK, 45, {PR_I_IV, PR_ONE, PR_I_VII}, 2, CS_COMP, VM_RICH,
     {AP_BROKEN, AP_CYCLE3}, LD_HOOK, AMT(100, 100, 55, 40, 45), {KIT_BREAKS, SR_ROCKBASS, SR_ORGAN, SR_GTR, SR_LEAD}, SR_ORGAN,
     0, 1, 30, 0, FM_ARC, 50, HF_JAZZY},
    /* ---- the night, the room */
    /* Brian Eno (Music for Airports), Aphex Twin (SAW II), Stars of the Lid: slow pads, sparse or no drums, bells */
    {"AMBIENT", 60, 90, 0, {SC_LYD, SC_MAJ, SC_MIN, SC_DOR}, DM_SPARSE, 25, KIT_SOFT, 0, HC_FLAT, FL_NONE, 0,
     {BA_PEDAL, BA_FIFTHS, BA_NEOARP}, 31, KM_SUSTAIN, 120, {PR_I_II, PR_I_IV, PR_SUS}, 4, CS_SWELL, VM_RICH,
     {AP_GLASS, AP_UPDOWN}, LD_TINTI, AMT(20, 50, 100, 40, 30), {KIT_SOFT, SR_SUB, SR_PAD, SR_BELL, SR_SINEKEY}, SR_DRONE,
     4, 0, 95, 0, FM_ARC, 20, HF_COLOR},
    /* Nils Frahm, Olafur Arnalds, Max Richter: a felt piano-like key, strings, no drums */
    {"NEO CLASSIC", 70, 112, 0, {SC_MIN, SC_DOR, SC_MAJ, SC_MEL}, DM_NONE, 0, KIT_SOFT, 0, HC_FLAT, FL_NONE, 0,
     {BA_NEOARP, BA_PEDAL, BA_FIFTHS}, 36, KM_SUSTAIN, 100, {PR_50S, PR_NEOCL, PR_CYCLE}, 1, CS_HOLD, VM_RICH,
     {AP_GLASS, AP_ALBERTI}, LD_TINTI, AMT(0, 60, 70, 90, 55), {KIT_SOFT, SR_SINEKEY, SR_STRINGS, SR_EP, SR_BELL}, SR_CHOIR,
     0, 1, 70, 0, FM_ARC, 30, HF_CINE | HX_RAISE | HX_SEC | HX_BOR},
    /* Debussy, Satie (Gymnopedies): lydian colours, parallel chords, a bell, no drums */
    {"IMPRESSION", 60, 92, 0, {SC_LYD, SC_MAJ, SC_LYD, SC_PEN}, DM_SPARSE, 25, KIT_SOFT, 0, HC_FLAT, FL_NONE, 0,
     {BA_FIFTHS, BA_PEDAL, BA_NEOARP}, 36, KM_SUSTAIN, 110, {PR_I_II, PR_I_III, PR_SUS}, 2, CS_SWELL, VM_PLANE,
     {AP_UPDOWN, AP_GLASS}, LD_TINTI, AMT(15, 50, 85, 75, 40), {KIT_SOFT, SR_SINEKEY, SR_PAD, SR_EP, SR_BELL}, SR_CHOIR,
     4, 0, 80, 0, FM_ARC, 30, HF_COLOR},
    /* Vangelis (Blade Runner), Hans Zimmer: strings swelling, a CS-80-like brass lead with vibrato, a choir behind */
    {"SOUNDTRACK", 64, 92, 0, {SC_MIN, SC_MIN, SC_MEL, SC_MPEN}, DM_SPARSE, 40, KIT_DUB, 0, HC_FLAT, FL_NONE, 0,
     {BA_PEDAL, BA_FIFTHS, BA_DUB}, 29, KM_SUSTAIN, 120, {PR_50S, PR_ANDAL, PR_EPIC}, 2, CS_SWELL, VM_RICH,
     {AP_ALBERTI, AP_GLASS}, LD_SPARSE, AMT(30, 60, 100, 30, 35), {KIT_DUB, SR_SUB, SR_STRINGS, SR_BELL, SR_LEAD}, SR_DRONE,
     4, 0, 90, 0, FM_ARC, 20, HF_CINE | HX_RAISE | HX_BOR},
    /* Aphex Twin (Xtal), Autechre (Tri Repetae), Boards of Canada: euclidean drums against each other, FM, bells */
    {"IDM", 90, 140, 10, {SC_PHRY, SC_HARM, SC_MIN, SC_DOR}, DM_IDM, 100, KIT_CLICK, 2, HC_FLAT, FL_KICK16, 0,
     {BA_JUNGLE, BA_ANGULAR, BA_DUST}, 33, KM_INTERLOCK, 50, {PR_MINOR4, PR_I_VI, PR_I_II}, 2, CS_SPARSE, VM_QUART,
     {AP_WALK, AP_CYCLE5}, LD_MOTIF, AMT(85, 70, 50, 60, 45), {KIT_CLICK, SR_FMBASS, SR_EP, SR_BELL, SR_CHIP}, SR_TEXTURE,
     4, 5, 60, 1, FM_PARTS, 10, HF_LOOP},
    /* Bugz in the Attic, IG Culture, 4hero: West London broken beat, a syncopated kit, Rhodes, a jazzy bass */
    {"BROKEN", 110, 124, 20, {SC_DOR, SC_MIN, SC_MEL, SC_DOR}, DM_BREAKS, 90, KIT_SOFT, 3, HC_FUNK, FL_LINEAR, 2,
     {BA_AROUND, BA_DUST, BA_JUNGLE}, 33, KM_INTERLOCK, 55, {PR_I_IV, PR_JAZZ, PR_MINOR4}, 2, CS_COMP, VM_QUART,
     {AP_WALK, AP_BROKEN}, LD_MOTIF, AMT(80, 85, 60, 30, 45), {KIT_SOFT, SR_FMBASS, SR_EP, SR_KEYS, SR_VOXLEAD}, SR_TEXTURE,
     4, 4, 65, 0, FM_PARTS, 50, HF_JAZZY},
};
#define NGENRE (sizeof GENRE / sizeof GENRE[0])
/* (0.9.9-gen1) each genre's PATTERN GRAMMAR past its maps and archetypes (GENRE's order): on the bass, how many ACCENTs
 * (off the kick), SLIDEs (a note tied into the next: its glide), octave jumps (the offbeat sixteenths), 0 .. 3 each;
 * DRUM-X GLUE (0 .. 3: x 4; 0.9.9-gen2: 2 at most, moderate); the closed hats ratcheted now and then (a condition: every other loop); MUTATE on the arp
 * or the chords (WHAT PATTERN, LOOP 4, ORBIT: a pattern orbiting the piece); (gen3) LIFT 0 .. 7: the bass, chords, arp and
 * lead's level over the drums, (LIFT - 2) x 1.5 dB (GG_LIFT: the drums at most 60 % of the parts' energy, gen_test). The
 * 303's own rules (gen_acid) are the acid bass's */
#define GG(acc, sld, oct, glue, rat, mut, lift) (uint16_t)((acc) | (sld) << 2 | (oct) << 4 | (glue) << 6 | (rat) << 8 | (mut) << 9 | (lift) << 10)
#define GG_ACC(g) ((g) & 3u)
#define GG_SLD(g) (((g) >> 2) & 3u)
#define GG_OCT(g) (((g) >> 4) & 3u)
#define GG_GLUE(g) (((g) >> 6) & 3u)
#define GG_RAT(g) (((g) >> 8) & 1u)
#define GG_MUT(g) (((g) >> 9) & 1u)
#define GG_LIFT(g) ((int32_t)(((g) >> 10) & 7u) * 3 - 6)   /* (gen3) the bass, chords, arp, lead's LEVEL over the drums: -6 ..
                                                         * +15 (half dB), by how the genre's kit weighs (gen.c gen_mix) */
static const uint16_t GGRAM[] = {   /* (gen3: + LIFT, 2 = none; MUTATE on TECHNO and TECH HOUSE too) */
    GG(1, 0, 1, 0, 0, 0, 6), GG(0, 1, 0, 0, 0, 0, 4), GG(1, 0, 0, 1, 0, 1, 0), GG(1, 0, 1, 1, 0, 1, 5),   /* HOUSE DEEP HOUSE MICRO HOUSE TECH HOUSE */
    GG(0, 0, 0, 1, 0, 0, 2), GG(1, 0, 0, 2, 0, 1, 5), GG(1, 0, 1, 1, 0, 0, 4), GG(0, 0, 0, 1, 0, 1, 2),   /* ACID HOUSE TECHNO DETROIT DUB TECHNO */
    GG(2, 0, 0, 2, 1, 0, 7), GG(1, 0, 0, 2, 0, 1, 0), GG(1, 0, 1, 1, 0, 0, 2), GG(2, 0, 2, 1, 1, 0, 4),   /* HARD TECHNO HYPNOTIC TRANCE ELECTRO */
    GG(2, 0, 0, 2, 0, 0, 5), GG(1, 0, 2, 0, 0, 0, 5), GG(1, 0, 2, 0, 0, 0, 4), GG(1, 0, 1, 1, 0, 0, 6),   /* EBM ITALO SYNTH POP DARKWAVE */
    GG(1, 0, 1, 0, 0, 0, 4), GG(1, 0, 0, 0, 0, 0, 3), GG(1, 0, 1, 0, 0, 1, 3), GG(1, 0, 1, 0, 0, 1, 1),   /* POST PUNK TRIBAL PUNK MINIMAL SYNTH MAN MACHINE */
    GG(0, 0, 1, 0, 0, 1, 2), GG(1, 0, 0, 0, 0, 0, 1), GG(0, 1, 0, 0, 0, 0, 1), GG(0, 1, 0, 0, 0, 0, 1),   /* MOTORIK INDIE SHOEGAZE POST ROCK */
    GG(2, 1, 2, 0, 0, 0, 2), GG(1, 0, 3, 0, 0, 0, 2), GG(1, 1, 0, 1, 0, 0, 3), GG(1, 3, 0, 1, 1, 0, 2),   /* FUNK DISCO BOOM BAP TRAP */
    GG(1, 1, 1, 0, 1, 0, 2), GG(1, 2, 0, 2, 0, 0, 2), GG(1, 0, 0, 1, 1, 0, 2), GG(1, 2, 0, 2, 1, 0, 2),   /* UK GARAGE DUBSTEP BREAKS DRUM & BASS */
    GG(0, 1, 0, 0, 0, 0, 2), GG(0, 0, 0, 0, 0, 0, 2), GG(1, 1, 0, 0, 0, 0, 1), GG(0, 1, 0, 0, 0, 0, 6),   /* DUB STEPPERS JAZZ BOSSA */
    GG(2, 0, 1, 0, 0, 0, 2), GG(0, 1, 0, 0, 0, 1, 2), GG(0, 0, 0, 0, 0, 0, 2), GG(0, 0, 0, 0, 0, 0, 2),   /* AFROBEAT AMBIENT NEO CLASSIC IMPRESSION */
    GG(0, 0, 0, 0, 0, 1, 2), GG(1, 1, 1, 1, 1, 1, 1), GG(1, 1, 1, 0, 0, 0, 2),   /* SOUNDTRACK IDM BROKEN */
};
_Static_assert(sizeof GGRAM / sizeof GGRAM[0] == NGENRE, "GGRAM: a grammar a genre");

/* ------------------------------------------------------------------------------------------- (0.8.6) EXPLORE --- */
/* GENRE's first choice and the dice's default: no genre's template, every one of its rules thrown (gen.c gx_genre).
 * gp.genre keeps its values (0 RANDOM, 1 + a genre's index) and EXPLORE comes after them, so every number that names a
 * genre (the web emulator's web_gen, the tests, the tools) still names it; the knob shows it first (gen_knob) */
#define GEN_EXPLORE ((uint32_t)NGENRE + 1u)
#define GEN_NPOS ((uint32_t)NGENRE + 2u)
#define GEN_TOP 4u                                      /* (0.9.9-gen2) GENRE[4], ACID HOUSE: the knob's first genre (gen.c
                                                         * gen_knob; its number kept: projects, the emulator, the tools) */                /* the knob's places: EXPLORE, RANDOM, the genres */
/* its keys: the SCALE page's modes of seven, the exotic ones likeliest (LOC too: its tonic chord a diminished one the
 * plan never takes, the other six degrees do) */
static const uint8_t GX_SCALE[9] = {SC_MAJ, SC_MIN, SC_DOR, SC_MIX, SC_HARM, SC_PHRY, SC_LYD, 10u /* LOC */, SC_MEL};
static const uint8_t GX_SCALE_W[9] = {4, 4, 8, 8, 13, 14, 14, 8, 13};
/* the lead's degrees over the mode: all seven, the pentatonics, hexatonics, a gapped one, an odd five */
static const uint8_t GX_PAL[8] = {0x7Fu, 0x5Du, 0x37u, 0x6Fu, 0x3Bu, 0x77u, 0x2Du, 0x5Bu};
/* the recipes a role may take (beyond its usual ones: a plucked bass, a bell lead, an organ texture ..) */
static const uint8_t GX_BASS[14] = {SR_SUB, SR_ACID, SR_REESE, SR_FMBASS, SR_ROCKBASS, SR_HOOK, SR_WOW, SR_EBMBASS, SR_PLUCK, SR_CHIP, SR_GTR,
                                    SR_808, SR_UPRIGHT, SR_TECHBASS};
static const uint8_t GX_PADS[13] = {SR_PAD, SR_STRINGS, SR_CHOIR, SR_ORGAN, SR_EP, SR_STAB, SR_DUBCHORD, SR_HOOVER, SR_WALL, SR_GTR, SR_KEYS, SR_BELL, SR_DRONE};
static const uint8_t GX_ARPS[8] = {SR_PLUCK, SR_BELL, SR_CHIP, SR_KEYS, SR_SINEKEY, SR_EP, SR_GTR, SR_STAB};
static const uint8_t GX_LEADS[8] = {SR_LEAD, SR_VOXLEAD, SR_CHIP, SR_HOOVER, SR_BELL, SR_PLUCK, SR_WOW, SR_ORGAN};
static const uint8_t GX_TEX[8] = {SR_NOISE, SR_TEXTURE, SR_DRONE, SR_CHOIR, SR_PAD, SR_ORGAN, SR_BELL, SR_STRINGS};
/* any engine on any role (the weights a role, RL_BASS .. RL_TEX; NOISE never a bass, a lead or an arp: unpitched) */
#define GX_NENG 8u
static const uint8_t GX_ENG[GX_NENG] = {EG_A, EG_FM, EG_PH, EG_LO, EG_VO, EG_WH, EG_NO, ENGI_ANIMAL};
static const uint8_t GX_ENG_W[RL_N][GX_NENG] = {
    {0},
    {20, 14, 14, 12, 8, 8, 0, 8},                       /* bass */
    {14, 12, 12, 8, 14, 12, 4, 8},                      /* chords */
    {14, 10, 12, 12, 16, 6, 0, 16},                     /* lead */
    {12, 14, 14, 14, 8, 8, 0, 14},                      /* arp */
    {8, 6, 8, 8, 14, 8, 20, 16},                        /* texture */
};
/* a track's own loop (polyrhythm: odd and irregular lengths against the piece's) and its DIV (N_DIV: 0 1/4, 1 1/8,
 * 2 1/16, 3 1/32, 4 8T, 5 16T); how often a role keeps the piece's loop, a role's DIV weights */
static const uint8_t GX_LEN[24] = {5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 18, 20, 21, 24, 27, 28, 30, 36, 40, 44, 48, 52, 56, 60};
static const uint8_t GX_KEEP[RL_N] = {60, 70, 80, 45, 45, 50};
static const uint8_t GX_DIV_W[RL_N][6] = {{0, 8, 80, 0, 0, 12}, {0, 15, 70, 0, 5, 10}, {5, 25, 60, 0, 10, 0},
                                         {0, 20, 55, 0, 10, 15}, {0, 15, 50, 15, 0, 20}, {20, 30, 50, 0, 0, 0}};
/* the step components it scatters (gen.c gx_comps): their weights on a melodic track */
static const uint8_t GX_COMP[12] = {CP_RAND, CP_OCT, CP_BEND, CP_ECHO, CP_PULSE, CP_WALK, CP_CLIMB, CP_ARPQ, CP_ARPN,
                                    CP_TONAL, CP_JUMP, CP_REV};
static const uint8_t GX_COMP_W[12] = {14, 12, 8, 12, 8, 12, 6, 8, 6, 6, 4, 4};
/* a DRUM-X sound's own way on a step (LM_X2 .. LM_FLAM) */
static const uint8_t GX_LM_W[LM_N] = {0, 30, 14, 4, 20, 22, 10};
