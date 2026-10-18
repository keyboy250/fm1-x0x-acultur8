#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Additional individual-lane static evidence, preserving the original budget model.
python3 tests/lane_feature_costs.py build/x0x.dis --report FILE --source-sha256 BUILD_SHA256
"""
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import target_budget as budget
from bass_feature_costs import parse, frame, DIRECT, TAIL

FEATURES = ("drain", "poly_sound_offset", "time_cell", "timing", "poly_valid", "poly_set", "poly_length", "poly_rate", "poly_break_gate", "poly_break_slice", "poly_break_set", "row_reset", "row_fire", "fire_step", "part_rate", "next_pos", "swing_q8", "ext_ticks", "poly_musical_tick", "poly_mode_boundary", "seq_lane_cycles", "seq_lane_position", "engine_lane_set")
ROOTS = ("fm1_alnk0_irq", "engine_render", "seq_advance", "seq_advance_arrangement", "seq_advance_poly", "engine_init", "project_defaults", "project_load", "project_save", "pref_set")
NOT_EMITTED = ("poly_enable", "seq_lane_latched_rate", "engine_lane_mode", "variant_command", "variant_snapshot", "web_variant_command", "web_variant_snapshot")
# Source alternatives from the immutable production SINK / per-render selector.
# Physical cancellation callback is null; browser callbacks are not target evidence.
INDIRECT = {"row_fire": ("s_drum_seq", "s_step"), "fire_step": ("s_drum_seq", "s_brk", "s_brkstop", "s_step"), "engine_render": ("seq_advance", "seq_advance_arrangement", "seq_advance_poly"),
 "pad_publish": ("s_midi_batch",), "seq_notes_clear": ("s_boff",),
 "midi_try": ("s_midi_try", "s_midi"), "bass_midi_on": ("s_midi_pair", "s_midi_try", "s_midi"),
 "bass_release": ("s_boff",), "fire_bass": ("s_bon_checked", "s_bon"), "bass_retune_held": ("s_bretune", "s_bon")}

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument("dis",type=Path);ap.add_argument("--report",type=Path,required=True)
    ap.add_argument("--baseline-dis",type=Path,required=True);ap.add_argument("--source-sha256",required=True);ap.add_argument("--wasm",type=Path,default=Path("build/emu/x0x.wasm"));ap.add_argument("--gate-report",type=Path,default=Path("build/target-cost-report.json"))
    args=ap.parse_args()
    if not re.fullmatch(r"[0-9a-f]{64}",args.source_sha256):raise ValueError("invalid build source identity")
    digest=hashlib.sha256(args.dis.read_bytes()).hexdigest();gates=json.loads(args.gate_report.read_text(encoding="utf-8"))
    if gates["status"]!="pass" or gates["model"]!=budget.MODEL or gates["disassembly_sha256"]!=digest:raise ValueError("exact original gate report/disassembly mismatch")
    rows=parse(args.dis);byname={}
    for a,row in rows.items():byname.setdefault(row["name"],[]).append(a)
    missing=[n for n in FEATURES+ROOTS if n not in byname]
    if missing:raise ValueError("required emitted functions missing: "+", ".join(missing))
    edges={a:[] for a in rows}
    for a,row in rows.items():
        for site,t,_ in row["instructions"]:
            match=DIRECT.match(t) or TAIL.match(t)
            if match and int(match[1],16) in rows and int(match[1],16)!=a:
                target=int(match[1],16)
                # Internal goto targets are not separate function entries.
                edges[a].append((target,"direct-call" if DIRECT.match(t) else "tail-transfer",hex(site)))
        for name in INDIRECT.get(row["name"],()):
            for target in byname.get(name,[]):edges[a].append((target,"source-callback-alternative",None))
    selected=set(a for n in FEATURES+ROOTS for a in byname[n]);todo=list(a for n in FEATURES for a in byname[n])
    while todo:
        for target,_,_ in edges[todo.pop()]:
            if target not in selected:selected.add(target);todo.append(target)
    # One reproducible shortest caller chain per root/helper; alternatives stay in edges.
    chains=[]
    for rootname in ROOTS:
        for root in byname[rootname]:
            paths={root:[rows[root]["id"]]};q=deque([root])
            while q:
                a=q.popleft()
                for target,_,_ in edges[a]:
                    if target not in paths:paths[target]=paths[a]+[rows[target]["id"]];q.append(target)
            for name in FEATURES:
                for target in byname[name]:
                    if target in paths:chains.append({"root":rows[root]["id"],"helper":rows[target]["id"],"chain":paths[target]})
    measured={}
    for a in sorted(selected):
        row=rows[a];ins=row["instructions"];f,pro=frame(row)
        measured[row["id"]]={"metrics":budget.cost([(site,t) for site,t,_ in ins]),
         "instruction_bytes":sum(n for _,_,n in ins),"frame_bytes":f,"prologue":pro,
         "all_calls":[[hex(site),t] for site,t,_ in ins if t.startswith("call")],
         "edges":[{"target":rows[target]["id"],"kind":kind,"site":site} for target,kind,site in edges[a]]}
    report={"schema":1,"kind":"additional-lane-static-costs","dis_sha256":hashlib.sha256(args.dis.read_bytes()).hexdigest(),
     "build_source_sha256":args.source_sha256,"wasm_sha256":hashlib.sha256(args.wasm.read_bytes()).hexdigest(),
     "original_gate_report_sha256":hashlib.sha256(args.gate_report.read_bytes()).hexdigest(),"model":budget.MODEL,"rows":measured,"caller_chains":chains,
     "not_emitted":[n for n in NOT_EMITTED if n not in byname],
     "limits":"Original target_budget model/limits unchanged. Static metrics exclude callees; all_calls includes straight-line and indirect calls. Frames are prologue allocations only, not worst stack. Source callback alternatives are conservative annotations, not resolved executed paths. Shortest chains do not prove cycles, branch frequency, interrupt nesting, deadline margin or receiver delivery. Lower caller counts moved work into helpers."}
    baseline=parse(args.baseline_dis)
    def comparison(table, name):
        return [{"id": r["id"], "metrics": budget.cost([(a,t) for a,t,_ in r["instructions"]]), "frame_bytes":frame(r)[0]} for r in table.values() if r["name"]==name]
    report["drain_comparison"]={"baseline_dis_sha256":hashlib.sha256(args.baseline_dis.read_bytes()).hexdigest(), "baseline_engine_render":comparison(baseline,"engine_render"), "baseline_drain":comparison(baseline,"drain"), "current_engine_render":comparison(rows,"engine_render"), "current_drain":comparison(rows,"drain"), "interpretation":"Baseline drain is inlined into engine_render; no separate baseline frame is inferred. Current drain is called once before the outer audio/event loop at its unchanged position. Caller metrics exclude outlined work: compare the listed body, callback alternatives and transitive edges; these are not physical cycles or worst stack."}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8",newline="\n")
    print(f"lane ledger: {len(measured)} address-qualified rows, {len(chains)} root/helper chains, dis {report['dis_sha256']}")
if __name__=="__main__":main()
