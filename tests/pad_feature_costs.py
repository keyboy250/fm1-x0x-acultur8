#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Additional external-pad static evidence, preserving the original budget model.
python3 tests/pad_feature_costs.py build/x0x.dis --report FILE --source-sha256 BUILD_SHA256
"""
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import target_budget as budget
from bass_feature_costs import parse, frame, DIRECT, TAIL

FEATURES = ("pad_publish", "pad_sync", "seq_pad_desired", "seq_notes_clear", "seq_midi_prepare", "seq_midi_sync",
 "s_midi_batch", "plat_midi_batch", "plat_midi_connected", "engine_notes_clear", "engine_arrangement_refresh",
 "engine_midi_channel_allowed", "project_apply_io", "harmony_resolve")
ROOTS = ("fm1_alnk0_irq", "engine_render", "seq_advance", "seq_advance_arrangement", "engine_notes_clear",
 "engine_init", "project_defaults", "project_load", "project_save", "pref_set", "undo_arrangement_intent", "ui_undo_preflight")
NOT_EMITTED = ("pad_release", "midi_out_batch", "midi_ring_try_batch", "cq_put", "do_stop",
 "engine_arrangement_global", "variant_command", "variant_snapshot", "host_web_midi_ready", "host_web_midi_batch",
 "host_web_midi_packet", "host_web_midi_ack", "host_web_midi_cancel", "web_midi_ready", "web_midi_cancel",
 "web_buttons_tagged", "web_input_ticket")
# Source alternatives from the immutable production SINK / per-render selector.
# Physical cancellation callback is null; browser callbacks are not target evidence.
INDIRECT = {"engine_render": ("seq_advance", "seq_advance_arrangement"),
 "pad_publish": ("s_midi_batch",), "seq_notes_clear": ("s_boff",),
 "midi_try": ("s_midi_try", "s_midi"), "bass_midi_on": ("s_midi_pair", "s_midi_try", "s_midi"),
 "bass_release": ("s_boff",), "fire_bass": ("s_bon_checked", "s_bon"), "bass_retune_held": ("s_bretune", "s_bon")}

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument("dis",type=Path);ap.add_argument("--report",type=Path,required=True)
    ap.add_argument("--source-sha256",required=True);ap.add_argument("--wasm",type=Path,default=Path("build/emu/x0x.wasm"));ap.add_argument("--gate-report",type=Path,default=Path("build/target-cost-report.json"))
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
    report={"schema":1,"kind":"additional-pad-static-costs","dis_sha256":hashlib.sha256(args.dis.read_bytes()).hexdigest(),
     "build_source_sha256":args.source_sha256,"wasm_sha256":hashlib.sha256(args.wasm.read_bytes()).hexdigest(),
     "original_gate_report_sha256":hashlib.sha256(args.gate_report.read_bytes()).hexdigest(),"model":budget.MODEL,"rows":measured,"caller_chains":chains,
     "not_emitted":[n for n in NOT_EMITTED if n not in byname],
     "limits":"Original target_budget model/limits unchanged. Static metrics exclude callees; all_calls includes straight-line and indirect calls. Frames are prologue allocations only, not worst stack. Source callback alternatives are conservative annotations, not resolved executed paths. Shortest chains do not prove cycles, branch frequency, interrupt nesting, deadline margin or receiver delivery. Lower caller counts moved work into helpers."}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8",newline="\n")
    print(f"pad ledger: {len(measured)} address-qualified rows, {len(chains)} root/helper chains, dis {report['dis_sha256']}")
if __name__=="__main__":main()
