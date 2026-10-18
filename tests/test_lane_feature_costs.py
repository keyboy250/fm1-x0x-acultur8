# SPDX-License-Identifier: GPL-3.0-only
import tempfile
import unittest
from pathlib import Path
from bass_feature_costs import parse, frame

class LedgerParser(unittest.TestCase):
    def test_duplicate_labels_and_tail_frames(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"sample.dis"
            p.write_text("helper:\n 100: 00 00 [--sp] = {rets,r7-r4}\n 102: 00 00 sp += -16\n 104: 00 00 goto 1 <helper: 200 >\nhelper:\n 200: 00 00 rts\n")
            rows=parse(p)
            self.assertEqual([r["id"] for r in rows.values()],["helper@0x100","helper@0x200"])
            self.assertEqual(frame(rows[0x100])[0],36)
    def test_unknown_instruction_fails(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"sample.dis";p.write_text("helper:\n 100: 00 00 <unknown instruction>\n")
            with self.assertRaises(ValueError):parse(p)
if __name__=="__main__":unittest.main()
