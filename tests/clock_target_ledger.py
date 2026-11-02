#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Supplement original gates with named caller/helper and preserving-guard evidence."""
from pathlib import Path
import argparse, hashlib, json, re
import target_budget as budget

p=argparse.ArgumentParser();p.add_argument('dis',type=Path);p.add_argument('--report',required=True,type=Path);args=p.parse_args()
root=Path(__file__).resolve().parents[1]
extras=('seq_advance_arrangement','seq_until_event_arrangement','advance_common','clock_prepare','clock_transport','clock_tick','run_request','seq_start','seq_continue','plat_control_enter','plat_control_leave','drain')
names=budget.FUNCS+extras
rows=budget.measure(args.dis,names)
# Follow every named direct call/tail-jump callee for the expanded scheduling/storage roots.
# Inlined work is counted in its caller; sink function-pointer calls remain an
# explicit physical/runtime boundary rather than an invented static call sum.
listing=args.dis.read_text(encoding='utf-8')
blocks={}
for match in re.finditer(r'^([A-Za-z_][A-Za-z_0-9.]*):$',listing,re.M):
    blocks[match.group(1)]=match.start()
ordered_blocks=sorted(blocks.items(),key=lambda item:item[1])
body_text={name:listing[start:ordered_blocks[i+1][1] if i+1<len(ordered_blocks) else len(listing)] for i,(name,start) in enumerate(ordered_blocks)}
closure=set();todo=['seq_advance_poly','seq_until_event_poly','engine_rate_set','poly_expand','rf_sweep']
while todo:
    name=todo.pop()
    if name in closure:continue
    assert name in body_text,'Missing expanded direct callee: '+name
    closure.add(name)
    todo.extend(re.findall(r'\b(?:call|goto)\s+-?\d+\s+<([A-Za-z_][A-Za-z_0-9.]*)\s*:',body_text[name]))
expanded_rows=budget.measure(args.dis,tuple(sorted(closure)))
reference=json.loads((root/'tests/clock_entry_family_reference.json').read_text(encoding='utf-8'))
assert reference['model']==budget.MODEL
assert reference['originalTargetBudgetSHA256']==hashlib.sha256((root/'tests/target_budget.json').read_bytes()).hexdigest()
advancement=('seq_advance','seq_advance_arrangement','advance_common','clock_prepare','clock_transport','clock_tick')
def aggregate(values):
    values=tuple(values)
    return {k:sum(row[k] for row in values) for k in budget.METRICS}
actual=aggregate(rows[name] for name in advancement)
prior=aggregate(reference['functions'].values())
family_errors=budget.compare({'advancement_family':actual},{'advancement_family':prior})
assert not family_errors,family_errors
bodies=budget.functions(args.dis,names)
guards={name:[text for _,text in bodies[name]] for name in ('plat_control_enter','plat_control_leave')}
assert not any(re.search(r'\b(?:sti|cli)\b',text) for body in guards.values() for text in body),'Control guard must restore exact ICFG state without unconditional STI/CLI'
sources=['firmware/src/seq/sequencer.c','firmware/src/app/plat_fm1.c','firmware/hal/fm1_irq.h','firmware/src/app/engine.c','firmware/src/app/project.c','firmware/src/seq/sequencer.h','firmware/src/usb.c','firmware/src/app/main_fm1.c','tools/gen_drum_samples.py']
sources+=['firmware/src/app/cc.c','firmware/src/app/ui.c','firmware/src/app/variant.c','firmware/src/dsp/drum909.c','firmware/src/seq/lanes.c','firmware/src/seq/lanes.h','firmware/src/seq/rate_families.c','firmware/src/seq/rate_families.h','firmware/src/seq/rate_slice_codec.c','firmware/src/seq/rate_slice_codec.h']
report={'status':'pass','kind':'static-caller-helper-coverage','model':budget.MODEL,'disassembly_sha256':hashlib.sha256(args.dis.read_bytes()).hexdigest(),'rows':rows,'matching_family':{'current':actual,'reference':prior,'status':'pass'},'guard_bodies':guards,'source_sha256':{name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in sources},'original_budget_sha256':hashlib.sha256((root/'tests/target_budget.json').read_bytes()).hexdigest(),'claims':'Bodies counted once, not a dynamic call sum. Original15 gates remain separately required. No cycles, IRQ deadlines, physical stack or delivery claim.'}
report['expanded_direct_callee_coverage']={'roots':['seq_advance_poly','seq_until_event_poly','engine_rate_set','poly_expand','rf_sweep'],'rows':expanded_rows,'distinct_body_totals':aggregate(expanded_rows.values()),'comparison':'Additional new-feature coverage; no historical full-closure baseline is substituted. Existing15 and six-body reference gate remain unchanged. Indirect sink callbacks require runtime/physical measurement.'}
default_roots=['ui_strip_default_action','project_strip_default_save','project_strip_default_get','engine_strip_restore_prepare','engine_strip_restore_commit']
default_closure=set();todo=default_roots[:]
while todo:
    name=todo.pop()
    if name in default_closure:continue
    assert name in body_text,'Missing defaults direct callee: '+name
    default_closure.add(name)
    todo.extend(re.findall(r'\b(?:call|goto)\s+-?\d+\s+<([A-Za-z_][A-Za-z_0-9.]*)\s*:',body_text[name]))
default_rows=budget.measure(args.dis,tuple(sorted(default_closure)))
report['strip_defaults_direct_callee_coverage']={'roots':default_roots,'rows':default_rows,'distinct_body_totals':aggregate(default_rows.values()),'comparison':'New stopped defaults path; original15 and clock-family gates unchanged. Indirect flash calls/physical IRQ time remain separate.'}
for name in ['firmware/src/storage.c','firmware/src/app/project.c','firmware/src/app/engine.h','firmware/src/app/plat.h','firmware/src/app/variant.h']:
    report['source_sha256'][name]=hashlib.sha256((root/name).read_bytes()).hexdigest()
args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'status':'pass','functions':len(rows),'guards':guards,'run_request':rows['run_request'],'drain':rows['drain'],'event_arrangement':rows['seq_until_event_arrangement']}))
