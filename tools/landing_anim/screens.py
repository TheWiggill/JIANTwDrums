# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
"""Packs tools/landing_anim/screens.c's recording into web/landing/screens/<scene>.webp (animated, looping).
  screens.py SCREENS.raw OUT_DIR [--fps 15] [--scale 2] [--quality 82]"""
import sys
from pathlib import Path

from PIL import Image


def frames(path):
    with open(path, "rb") as f:
        while True:
            hdr = f.readline()
            if not hdr:
                return
            _, name, n = hdr.decode().split()
            out = []
            for _ in range(int(n)):
                raw = f.read(240 * 240 * 2)
                rgb = bytearray(240 * 240 * 3)
                for i in range(240 * 240):
                    v = raw[2 * i] | raw[2 * i + 1] << 8
                    r, g, b = v >> 11, (v >> 5) & 63, v & 31
                    rgb[3 * i], rgb[3 * i + 1], rgb[3 * i + 2] = r * 255 // 31, g * 255 // 63, b * 255 // 31
                out.append(Image.frombytes("RGB", (240, 240), bytes(rgb)))
            yield name, out


def main():
    src, dst = sys.argv[1], Path(sys.argv[2])
    arg = lambda k, d: type(d)(sys.argv[sys.argv.index(k) + 1]) if k in sys.argv else d
    fps, scale, q = arg("--fps", 15), arg("--scale", 2), arg("--quality", 82)
    dst.mkdir(parents=True, exist_ok=True)
    for name, fr in frames(src):
        if scale != 1:
            fr = [f.resize((240 * scale, 240 * scale), Image.NEAREST) for f in fr]
        fr[0].save(dst / f"{name}.webp", save_all=True, append_images=fr[1:], duration=round(1000 / fps), loop=0,
                   quality=q, method=6)
        print("%-12s %3d frames %7d B" % (name, len(fr), (dst / f"{name}.webp").stat().st_size))


main()
