#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Account for upstream main-thread helpers; original audio-root gates stay mandatory."""
import argparse,hashlib,json
from pathlib import Path
import target_budget as budget
p=argparse.ArgumentParser();p.add_argument('dis',type=Path);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
listing=a.dis.read_text();assert '\nbrk_filter:' not in listing,'Deferred filter must not consume lane storage'
rows=budget.measure(a.dis,('breaks_render','load_stream_recover','ann_send','fm1_main'))
r={'status':'pass','model':budget.MODEL,'rows':rows,'disassembly_sha256':hashlib.sha256(a.dis.read_bytes()).hexdigest(),'scope':'New recovery/backup/accessibility main-thread work recorded; original15 audio gates remain separately required. BREAK one-knob filter deferred to preserve the frozen independent-lane slab. No cycles, physical stack or IRQ deadline claim.'}
a.report.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({'status':'pass','rows':rows}))
