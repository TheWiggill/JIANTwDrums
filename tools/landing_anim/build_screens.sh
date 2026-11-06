#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
# The front page's screens: tools/landing_anim/screens.c (the firmware itself, nothing patched, on the browser
# emulator's host build) records its pages moving, screens.py packs them -> web/landing/screens/*.webp.
#   [JIANT_FW=TREE] tools/landing_anim/build_screens.sh
# JIANT_FW: the firmware tree to record (V10: the public line's checkout, web/STABLE), its build/gen made once
# (tools/build.py's generate()); this tree by default.
set -e
cd "$(dirname "$0")/../.."
FW=${JIANT_FW:-.}
T=$(mktemp -d)
VER=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' "$FW/firmware/src/felucca.c")
cc -O1 -w -std=gnu99 "-DFELUCCA_VERSION=\"$VER\"" -I"$FW/build/gen" -I"$FW/firmware/src" -I"$FW" -o "$T/screens" \
    tools/landing_anim/screens.c -lm
"$T/screens" "$T/screens.raw"
python3 tools/landing_anim/screens.py "$T/screens.raw" web/landing/screens --scale 1
rm -rf "$T"
echo "screens: web/landing/screens ($VER)"
