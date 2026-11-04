#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# (cpu5) tests/fx_rest_test.c both ways: BIN_REST (fx.c as built) against BIN_REF (built with -DFX_REST_OFF): the same
# output until the gate shut (the hash), exact zeros through the rest both ways (the C checks), the rest engaged.
#   python3 tests/fx_rest_test.py BIN_REST BIN_REF
import re, subprocess, sys
out = {}
for mode, b in (("rest", sys.argv[1]), ("reference", sys.argv[2])):
    r = subprocess.run([b, mode], capture_output=True, text=True)
    print(r.stdout.strip())
    if r.returncode:
        raise SystemExit(f"fx-rest: {mode} failed ({r.returncode})")
    out[mode] = dict(re.findall(r"(\w+)=(\w+)", r.stdout))
if out["rest"]["gate_shut_at"] != out["reference"]["gate_shut_at"]:
    raise SystemExit("fx-rest: the gate shut at different blocks")
if out["rest"]["pre_hash"] != out["reference"]["pre_hash"]:
    raise SystemExit("fx-rest: the output differed before the rest")
r, f = int(out["rest"]["wake_peak"]), int(out["reference"]["wake_peak"])
if abs(r - f) > 512:
    raise SystemExit(f"fx-rest: the wake's level differs ({r} vs {f})")
print("fx-rest: the same until the gate shuts, exact zeros through the rest both ways, the rest engaged, the wake at level PASS")
