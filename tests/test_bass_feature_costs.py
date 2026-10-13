# SPDX-License-Identifier: GPL-3.0-only
"""Independent disassembly fixtures for duplicate labels, frames and call metrics."""
import tempfile
import unittest
from pathlib import Path
import bass_feature_costs as ledger

class LedgerTest(unittest.TestCase):
    def parse(self,text):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/"fixture.dis";p.write_text(text,encoding="utf-8");return ledger.parse(p)
    def test_address_qualified_duplicates_and_frame(self):
        rows=self.parse("""helper:
 1000:    7f 04             [--sp] = {rets, r15-r4}
 1002:    e2 95             sp += -44
.Llocal:
 1004:    80 00             r0 = 1
 1006:    c4 00             call r4
helper:
 2000:    80 00             rts
""")
        self.assertEqual(set(rows),{0x1000,0x2000})
        self.assertEqual(rows[0x1000]["id"],"helper@0x1000")
        self.assertEqual(ledger.frame(rows[0x1000])[0],96)
        self.assertEqual(ledger.frame(rows[0x2000])[0],0)
        metrics=ledger.budget.cost([(a,t) for a,t,_ in rows[0x1000]["instructions"]])
        self.assertEqual(metrics,{"insns":4,"loop":0,"div":0,"call":0,"cost":0})
        self.assertEqual(sum(t.startswith("call") for _,t,_ in rows[0x1000]["instructions"]),1)
    def test_unknown_rejected_and_jump_table_excluded(self):
        with self.assertRaises(ValueError):self.parse("bad:\n 1000:    80 00             <unknown instruction>\n")
        rows=self.parse("ok:\n 1000:    80 00             rts\n 1002:    80 00             < 1 : 1000 >\n")
        self.assertEqual(len(rows[0x1000]["instructions"]),1)
    def test_call_and_tail_targets(self):
        self.assertEqual(int(ledger.DIRECT.match("call -20 <helper : 1000 >")[1],16),0x1000)
        self.assertEqual(int(ledger.TAIL.match("goto -20 <helper : 1000 >")[1],16),0x1000)
        self.assertIsNone(ledger.DIRECT.match("call r4"))
if __name__=="__main__":unittest.main()
