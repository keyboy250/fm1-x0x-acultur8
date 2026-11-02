#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fail-closed target gate regressions; no SDK, compiler or device required."""
from contextlib import redirect_stderr, redirect_stdout
import copy
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("target_budget", Path(__file__).with_name("target_budget.py"))
budget = importlib.util.module_from_spec(spec)
spec.loader.exec_module(budget)


class TargetBudgetTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.dis = self.root / "x0x.dis"
        self.base = self.root / "budget.json"
        self.report = self.root / "report.json"
        self.write_dis()

    def write_dis(self, names=budget.FUNCS, count=3):
        lines = ["Disassembly of section .text:"]
        for n, name in enumerate(names):
            start = 0x2000000 + n * 0x100
            lines += [name + ":"]
            lines += [f" {start + i * 2:x}:  00 00  r0 = r0 + 1" for i in range(count)]
        self.dis.write_text("\n".join(lines), encoding="utf-8")

    def run_gate(self, *args):
        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            return budget.main([str(self.dis), str(self.base), *args])

    def make_baseline(self):
        self.assertEqual(self.run_gate("--update", "--compiler-id", "synthetic test compiler"), 0)

    def test_missing_disassembly_fails_and_reports(self):
        self.dis.unlink()
        self.assertEqual(self.run_gate("--report", str(self.report)), 1)
        report = json.loads(self.report.read_text())
        self.assertEqual(report["status"], "fail")
        self.assertTrue(report["errors"])

    def test_missing_baseline_fails(self):
        self.assertEqual(self.run_gate(), 1)

    def test_baseline_roundtrip_and_read_only_comparison(self):
        self.make_baseline()
        before = self.base.read_bytes()
        self.assertEqual(self.run_gate("--report", str(self.report)), 0)
        self.assertEqual(self.base.read_bytes(), before)
        self.assertEqual(json.loads(self.report.read_text())["status"], "pass")

    def test_partial_coverage_cannot_overwrite_baseline(self):
        self.make_baseline()
        before = self.base.read_bytes()
        self.write_dis(budget.FUNCS[:-1])
        self.assertEqual(self.run_gate("--update", "--compiler-id", "broken compiler"), 1)
        self.assertEqual(self.base.read_bytes(), before)

    def test_update_requires_compiler_identity(self):
        self.assertEqual(self.run_gate("--update"), 1)
        self.assertFalse(self.base.exists())

    def test_empty_and_duplicate_function_are_missing_coverage(self):
        self.dis.write_text(budget.FUNCS[0] + ":\n", encoding="utf-8")
        self.assertEqual(self.run_gate(), 1)
        self.write_dis()
        with self.dis.open("a", encoding="utf-8") as output:
            output.write("\n" + budget.FUNCS[0] + ":\n 2000000: 00 00 rts\n")
        self.assertEqual(self.run_gate(), 1)

    def test_missing_invalid_or_unknown_baseline_rows_fail(self):
        self.make_baseline()
        good = json.loads(self.base.read_text())
        malformed = []
        missing = copy.deepcopy(good)
        del missing["functions"][budget.FUNCS[0]]
        malformed.append(missing)
        extra = copy.deepcopy(good)
        extra["functions"]["obsolete_felucca_render"] = extra["functions"][budget.FUNCS[0]]
        malformed.append(extra)
        for value in (-1, 3.0, True, "3"):
            bad = copy.deepcopy(good)
            bad["functions"][budget.FUNCS[0]]["cost"] = value
            malformed.append(bad)
        malformed.extend([[], {**good, "model": {}}, {**good, "schema": 99}])
        for bad in malformed:
            with self.subTest(bad=bad):
                self.base.write_text(json.dumps(bad), encoding="utf-8")
                self.assertEqual(self.run_gate(), 1)

    def test_straight_line_growth_cannot_hide_behind_zero_loop_cost(self):
        self.make_baseline()
        self.write_dis(count=4)
        self.assertEqual(self.run_gate(), 1)

    def test_local_labels_and_jump_table_data(self):
        self.dis.write_text("render:\n 100: 00 00 r0 = 0\n.LBB0_1:\n"
                            " 102: 00 00 < 0 : 0x100 >\n 104: 00 00 goto -4 <render: 100 >\n"
                            "other:\n 106: 00 00 rts\n", encoding="utf-8")
        result = budget.measure(self.dis, ("render",))["render"]
        self.assertEqual(result, {"insns": 2, "loop": 2, "div": 0, "call": 0, "cost": 2})

    def test_nested_loops_divides_and_calls(self):
        instructions = [(0x100, "r0 = 0"), (0x102, "r1 = 0"), (0x104, "r2 = r1 / r0"),
                        (0x106, "if r1 != 0 goto -4 <render+0x2 : 102 >"),
                        (0x108, "call 2 <helper : 200 >"),
                        (0x10a, "goto -10 <render : 100 >")]
        self.assertEqual(budget.cost(instructions),
                         {"insns": 6, "loop": 6, "div": 1, "call": 1, "cost": 47})

    def test_invalid_loop_target_rejected(self):
        with self.assertRaisesRegex(ValueError, "not an instruction"):
            budget.cost([(0x100, "r0 = 0"), (0x104, "goto -2 <render+0x2 : 102 >")])

    def test_wrong_cpu_disassembly_is_rejected(self):
        self.dis.write_text("render:\n 100: 3f e5 <unknown instruction>\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "mcpu=r3"):
            budget.measure(self.dis, ("render",))

    def test_regression_limits_and_zero_baseline(self):
        before = {"f": {"insns": 100, "loop": 0, "div": 0, "call": 0, "cost": 0}}
        after = copy.deepcopy(before)
        after["f"]["insns"] = 110
        self.assertEqual(budget.compare(after, before), [])
        after["f"]["insns"] = 111
        self.assertTrue(budget.compare(after, before))
        for metric in ("cost", "div", "call"):
            after = copy.deepcopy(before)
            after["f"][metric] = 1
            self.assertTrue(budget.compare(after, before))

    def test_report_cannot_overwrite_inputs(self):
        self.make_baseline()
        for path in (self.base, self.dis):
            before = path.read_bytes()
            with self.assertRaises(SystemExit) as result:
                self.run_gate("--report", str(path))
            self.assertEqual(result.exception.code, 2)
            self.assertEqual(path.read_bytes(), before)


if __name__ == "__main__":
    unittest.main()
