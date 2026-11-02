#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Acultur8: X0X coverage, fail-closed baseline validation and reproducible reports.
"""Compare X0X pi32v2 disassembly with an explicitly reviewed static cost baseline.

  python3 tests/target_budget.py [DIS [BUDGET]] [--report FILE]
  python3 tests/target_budget.py DIS BUDGET --update --compiler-id DESCRIPTION

Missing artifacts, required functions or baseline rows are failures. This estimates
compiled code size and loop complexity, not cycles, real-time deadlines or CPU load.
Inspect the disassembly before intentionally updating a baseline. Never auto-update
it in CI to accept a regression. See docs/ACULTUR8_TARGET_BUILD.md.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

# These are present in the baseline target image, including the actual inner drum
# renders and reverb/delay functions. render_sub is inlined into engine_render.
# A renamed/inlined function must be reviewed here, never silently omitted.
FUNCS = (
    "d9_render", "d8_render", "hat_tick", "bass303_render", "breaks_render",
    "fx_reverb", "fx_delay", "fxbus_process_st", "master_process_st",
    "core2_job", "engine_render", "seq_until_event", "seq_advance",
    "motion_tick", "fm1_alnk0_irq",
)
DIV_W = 8
NEST = 4
MAXD = 4
SCHEMA = 1
MODEL = {"nest_weight": NEST, "divide_extra_weight": DIV_W, "max_depth": MAXD}
METRICS = ("insns", "loop", "div", "call", "cost")
LABEL = re.compile(r"^([A-Za-z_][A-Za-z_0-9.]*):$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)$")
TARGET = re.compile(r"goto -?\d+ <[^>]*: ([0-9a-f]+) >")


def functions(path, required=FUNCS):
    """Keep local .L labels in their function; exclude jump-table data."""
    out, cur = {}, None
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        m = LABEL.match(line.strip())
        if m:
            name = m.group(1)
            if not name.startswith("."):
                if name in required:
                    if name in out:
                        raise ValueError(f"duplicate function label: {name}")
                    cur = out[name] = []
                else:
                    cur = None
            continue
        m = INSN.match(line)
        if cur is not None and m:
            instruction = m.group(3).strip()
            if "<unknown instruction>" in instruction:
                raise ValueError("unknown target instruction; regenerate with objdump --mcpu=r3")
            if not re.match(r"<\s*\d+\s*:", instruction):
                cur.append((int(m.group(1), 16), instruction))
    return out


def cost(insns):
    """Backward spans approximate loops. Weight divides and nested loop bodies.

    Count all instructions as a second gate so added straight-line work or a new
    call cannot disappear behind a zero loop cost. This does not traverse callees.
    """
    if not insns:
        raise ValueError("function has no instructions")
    heads = {}
    addresses = {a for a, _ in insns}
    for a, t in insns:
        m = TARGET.search(t)
        if m and insns[0][0] <= int(m.group(1), 16) < a:
            h = int(m.group(1), 16)
            if h not in addresses:
                raise ValueError(f"loop target {h:x} is not an instruction")
            heads[h] = max(heads.get(h, a), a)
    n = divs = calls = weighted = 0
    for a, t in insns:
        depth = min(sum(h <= a <= end for h, end in heads.items()), MAXD)
        if not depth:
            continue
        k = NEST ** (depth - 1)
        n += 1
        if re.search(r"= r\d+ / r\d+", t):
            divs += 1
            k *= 1 + DIV_W
        calls += bool(re.match(r"call\b", t))
        weighted += k
    return {"insns": len(insns), "loop": n, "div": divs, "call": calls, "cost": weighted}


def measure(path, required=FUNCS):
    fns = functions(path, required)
    missing = [name for name in required if not fns.get(name)]
    if missing:
        raise ValueError("required functions missing/empty: " + ", ".join(missing))
    return {name: cost(fns[name]) for name in required}


def read_baseline(path, required=FUNCS):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(data, dict) or data.get("schema") != SCHEMA or data.get("model") != MODEL:
        raise ValueError("baseline schema/cost model mismatch")
    if not isinstance(data.get("compiler_id"), str) or not data["compiler_id"].strip():
        raise ValueError("baseline compiler identity is missing")
    if not isinstance(data.get("disassembly_sha256"), str) or not re.fullmatch(r"[0-9a-f]{64}", data["disassembly_sha256"]):
        raise ValueError("baseline disassembly hash is missing/invalid")
    rows = data.get("functions")
    if not isinstance(rows, dict) or set(rows) != set(required):
        raise ValueError("baseline function coverage differs from required X0X functions")
    for name, row in rows.items():
        if not isinstance(row, dict) or set(row) != set(METRICS):
            raise ValueError(f"{name}: missing/unknown baseline metrics")
        if any(type(row[k]) is not int or row[k] < 0 for k in METRICS) or row["insns"] == 0:
            raise ValueError(f"{name}: baseline metrics must be nonnegative integers with instructions")
        if any(row[k] > row["insns"] for k in ("loop", "div", "call")):
            raise ValueError(f"{name}: invalid baseline instruction counts")
    return data


def compare(actual, baseline):
    errors = []
    for name, row in actual.items():
        previous = baseline[name]
        # Integer comparison handles a zero baseline without division by zero.
        for metric in ("insns", "cost"):
            if row[metric] * 100 > previous[metric] * 110:
                errors.append(f"{name}: {metric} {row[metric]} exceeds {previous[metric]} +10%")
        for metric in ("div", "call"):
            if row[metric] > previous[metric]:
                errors.append(f"{name}: loop {metric} count grew {previous[metric]} -> {row[metric]}")
    return errors


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dis", nargs="?", type=Path, default=Path("build/x0x.dis"))
    ap.add_argument("budget", nargs="?", type=Path, default=Path("tests/target_budget.json"))
    ap.add_argument("--update", action="store_true", help="intentionally replace a reviewed baseline")
    ap.add_argument("--compiler-id", help="compiler version, toolchain archive and flags used for the baseline")
    ap.add_argument("--report", type=Path, help="write machine-readable validation evidence")
    args = ap.parse_args(argv)
    if args.report and args.report.resolve() in (args.dis.resolve(), args.budget.resolve()):
        ap.error("--report must not overwrite the disassembly or baseline")
    if args.update and args.budget.resolve() == args.dis.resolve():
        ap.error("baseline must not overwrite the disassembly")
    report = {"schema": SCHEMA, "kind": "static-target-cost", "status": "fail", "errors": []}
    try:
        # Coverage is checked before any baseline write, including --update.
        actual = measure(args.dis)
        digest = hashlib.sha256(args.dis.read_bytes()).hexdigest()
        report.update(functions=actual, disassembly_sha256=digest, model=MODEL)
        if args.update:
            if not args.compiler_id or not args.compiler_id.strip():
                raise ValueError("--update requires --compiler-id; inspect compiler changes before rebasing")
            data = {"schema": SCHEMA, "compiler_id": args.compiler_id,
                    "disassembly_sha256": digest, "model": MODEL, "functions": actual}
            args.budget.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
            report["status"] = "baseline-updated"
            print(f"target: baseline updated ({len(actual)} functions); rerun without --update to check")
        else:
            baseline = read_baseline(args.budget)
            report["compiler_id"] = baseline["compiler_id"]
            report["errors"] = compare(actual, baseline["functions"])
            report["status"] = "fail" if report["errors"] else "pass"
        for name, row in actual.items():
            print(f"target: {name:20s} {row['insns']:5d} instructions, {row['loop']:4d} in loops, "
                  f"{row['div']:2d} divides, {row['call']:2d} calls, cost {row['cost']:6d}")
    except (OSError, ValueError, TypeError, KeyError) as error:
        report["errors"] = [str(error)]
    for error in report["errors"]:
        print("target: FAIL " + error, file=sys.stderr)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if report["status"] == "pass":
        print("target: static budgets passed; device timing and headroom remain separate checks")
    return 1 if report["status"] == "fail" else 0


if __name__ == "__main__":
    sys.exit(main())
