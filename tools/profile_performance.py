#!/usr/bin/env python3
"""Diagnostic host timings. No pass/fail CPU threshold or FM1 % conversion.
Run AFTER compiling the same performance_probe.c against each source revision:
  python3 tools/profile_performance.py BEFORE AFTER [REPEATS]
"""
import json, re, statistics, subprocess, sys
before, after = sys.argv[1:3]
repeats = int(sys.argv[3]) if len(sys.argv) > 3 else 9
cases = [(name, ['track', str(e), str(p), '1', '4', '0', '0', '6'])
         for name,e,p in [('ANALOG',0,0),('PHASE',2,0),('LOFI',3,0),('VOICE vowels',5,0),
                         ('VOICE words',5,7),('WHEEL',7,0),('DRUM',10,0),('NOISE',11,0),('FM6',12,0),('ANIMAL',14,0)]]
cases += [('FM6 '+str(n)+' operators', ['fmops','12','0','1','4','0','0',str(n)]) for n in (6,4,2)]
cases += [('FX '+name, ['fx','0','0','1','0',str(mask),str(rt),'6'])
          for name,mask,rt in [('no input',0,0),('chorus input',1,0),('delay input',2,0),
                               ('ROOM',4,0),('SPRING',4,1),('SHIMMER',4,2),('RESO',4,3),('CLOUD',4,4)]]
cases += [('mixer idle',['idle','0','0','1','0','0','0','6']),
          ('six FM6 tracks dry',['mix','12','0','6','2','0','0','6']),
          ('six VOICE tracks dry',['mix','5','0','6','2','0','0','6']),
          ('four ANALOG tracks all sends',['mix','0','0','4','2','7','0','6'])]
for name,args in cases:
    times=[[],[]]; checks=[set(),set()]; live=set()
    for rep in range(repeats):
        for idx in ([0,1] if rep%2 else [1,0]):
            result=subprocess.check_output([[before,after][idx]]+args,text=True)
            times[idx].append(float(re.search(r'ns_per_sample=([\d.]+)',result)[1]))
            checks[idx].add(re.search(r'checksum=(\d+)',result)[1])
            live.add(int(re.search(r' live=(\d+)',result)[1]))
    if checks[0] != checks[1] or len(checks[0]) != 1:
        raise SystemExit('audio checksum changed or nondeterministic: '+name)
    med=[statistics.median(t) for t in times]
    print(json.dumps(dict(scene=name,before_ns=med[0],after_ns=med[1],delta_pct=100*(med[1]/med[0]-1),
                          checksum=next(iter(checks[0])),live_end=sorted(live),repeats=repeats)),flush=True)
