#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Report additional bass/MIDI static costs; never changes target_budget limits.
python3 tests/bass_feature_costs.py build/x0x.dis --report FILE --source-sha256 BUILD_SHA256
"""
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import target_budget as budget

FEATURES = ("midi_try", "bass_midi_shared", "bass_midi_off", "bass_release", "fire_bass", "s_bon_checked",
 "bass_midi_on", "bass_follow_note", "bass_retune_held", "seq_notes_clear", "seq_midi_prepare", "seq_midi_sync",
 "s_bon", "s_boff", "s_bretune", "s_midi", "s_midi_try", "s_midi_pair", "plat_midi_out", "plat_midi_pair",
 "plat_midi_connected", "engine_notes_clear", "engine_arrangement_refresh", "harmony_resolve_note")
ROOTS = ("fm1_alnk0_irq", "engine_render", "seq_advance", "seq_advance_arrangement", "engine_notes_clear",
 "engine_init", "project_defaults", "project_load", "project_save")
NOT_EMITTED = ("midi_ring_try_put", "midi_ring_try_pair", "cq_put", "engine_arrangement_global",
 "variant_command", "variant_snapshot", "variant_schema2_valid")
# Conservative alternatives from immutable production engine SINK / render selection.
# These are source annotations, not disassembler-resolved dynamic targets.
INDIRECT = {"engine_render": ("seq_advance", "seq_advance_arrangement"),
 "midi_try": ("s_midi_try", "s_midi"), "bass_midi_on": ("s_midi_pair", "s_midi_try", "s_midi"),
 "bass_release": ("s_boff",), "seq_notes_clear": ("s_boff",),
 "fire_bass": ("s_bon_checked", "s_bon"), "bass_retune_held": ("s_bretune", "s_bon")}
DIRECT = re.compile(r"^call\s+-?\d+\s+<[^>]*: ([0-9a-f]+) >")
TAIL = re.compile(r"^goto\s+-?\d+\s+<[^>]*: ([0-9a-f]+) >")

def parse(path):
    rows=[]; cur=None
    for line in path.read_text(encoding="utf-8").splitlines():
        m=budget.LABEL.match(line.strip())
        if m:
            if not m[1].startswith("."):
                cur={"name":m[1],"instructions":[]};rows.append(cur)
            continue
        m=budget.INSN.match(line)
        if cur is not None and m:
            t=m[3].strip()
            if "<unknown instruction>" in t: raise ValueError("unknown instruction")
            if not re.match(r"<\s*\d+\s*:",t):cur["instructions"].append((int(m[1],16),t,len(m[2].split())))
    out={}
    for row in rows:
        if not row["instructions"]:continue
        row["address"]=row["instructions"][0][0]
        row["id"]=f"{row['name']}@0x{row['address']:x}"
        out[row["address"]]=row
    return out

def frame(row):
    pro=[]; total=0
    for a,t,_ in row["instructions"]:
        if re.match(r"(?:call|goto|if\b|ifs\b)",t):break
        pro.append([hex(a),t])
        push=re.search(r"\[--sp\] = \{([^}]+)\}",t)
        if push:
            for item in push[1].split(","):
                item=item.strip(); ran=re.fullmatch(r"r(\d+)-r(\d+)",item)
                total+=4*(abs(int(ran[1])-int(ran[2]))+1) if ran else 4
        adjust=re.fullmatch(r"sp \+= -(\d+)",t)
        if adjust:total+=int(adjust[1])
    return total,pro

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
    report={"schema":1,"kind":"additional-bass-static-costs","dis_sha256":hashlib.sha256(args.dis.read_bytes()).hexdigest(),
     "build_source_sha256":args.source_sha256,"wasm_sha256":hashlib.sha256(args.wasm.read_bytes()).hexdigest(),
     "original_gate_report_sha256":hashlib.sha256(args.gate_report.read_bytes()).hexdigest(),"model":budget.MODEL,"rows":measured,"caller_chains":chains,
     "not_emitted":[n for n in NOT_EMITTED if n not in byname],
     "limits":"Original target_budget model/limits unchanged. Static metrics exclude callees; all_calls includes straight-line and indirect calls. Frames are prologue allocations only, not worst stack. Source callback alternatives are conservative annotations, not resolved executed paths. Shortest chains do not prove cycles, branch frequency, interrupt nesting, deadline margin or receiver delivery. Lower caller counts moved work into helpers."}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8",newline="\n")
    print(f"bass ledger: {len(measured)} address-qualified rows, {len(chains)} root/helper chains, dis {report['dis_sha256']}")
if __name__=="__main__":main()
