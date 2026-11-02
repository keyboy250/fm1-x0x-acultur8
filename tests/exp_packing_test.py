#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Variable-width EXP layout and guarded read contract; no altered float oracle."""
import hashlib
import importlib.util
import random
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("gen_drum_samples", ROOT / "tools/gen_drum_samples.py")
g = importlib.util.module_from_spec(spec)
spec.loader.exec_module(g)


def decode(descriptors, blob):
    rows = []
    for base, offset, width in descriptors:
        row = []
        for pot in range(128):
            bit = pot * width
            start = offset + (bit >> 3)
            assert start + 4 <= len(blob)
            word = int.from_bytes(blob[start:start + 4], "little")
            row.append(base + ((word >> (bit & 7)) & ((1 << width) - 1)))
        rows.append(row)
    return rows


class ExpPacking(unittest.TestCase):
    def test_all_original_words_and_layout(self):
        rows = [[struct.unpack("<I", struct.pack("<f", g.pot_exp(lo, hi, p)))[0]
                 for p in range(128)] for _, lo, hi in g.EXP_RANGES]
        descriptors, blob = g.pack_exp_curves(rows)
        self.assertEqual([d[2] for d in descriptors],
                         [26,25,25,24,25,23,26,23,26,23,26,24,24,25,26,26,26,26,26])
        self.assertEqual(len(blob), 7600)
        self.assertEqual(decode(descriptors, blob), rows)
        bits = b"".join(struct.pack("<I", w) for row in decode(descriptors, blob) for w in row)
        self.assertEqual(hashlib.sha256(bits).hexdigest(),
                         "c8c351223b0c805e68f88c917c5ed0a8b4983277681e2a1b43251627ffbd3faf")

    def test_every_width_offset_and_final_guard(self):
        rng = random.Random(8721)
        rows = []
        for width in range(1, 27):
            base = 0x30000000
            row = [base + rng.randrange(1 << width) for _ in range(128)]
            row[0], row[-1] = base, base + (1 << width) - 1
            rows.append(row)
        for end in range(1, 27):
            wanted = rows[:end]
            descriptors, blob = g.pack_exp_curves(wanted)
            logical = sum(16 * d[2] for d in descriptors)
            guard = max(0, (127 * end // 8) + 4 - 16 * end)
            self.assertEqual(len(blob), logical + guard)
            self.assertEqual(decode(descriptors, blob), wanted)
            self.assertEqual(blob[logical:], bytes(guard))
            self.assertEqual([d[1] for d in descriptors],
                             [16 * sum(range(1, n)) for n in range(1, end + 1)])

    def test_constant_row(self):
        rows = [[0x3f800000] * 128]
        descriptors, blob = g.pack_exp_curves(rows)
        self.assertEqual(descriptors, [(0x3f800000, 0, 1)])
        self.assertEqual(len(blob), 19)
        self.assertEqual(decode(descriptors, blob), rows)

    def test_compiled_final_guard(self):
        original, argv = g.EXP_RANGES, sys.argv
        try:
            with tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                for name, lo, hi in [("last23",50.0,90.0), ("last24",130.0,320.0),
                                     ("last25",0.43,4.7), ("last26",100.0,4000.0),
                                     ("constant",1.0,1.0)]:
                    target = root / name
                    g.EXP_RANGES = original[:-1] + [("TEST", lo, hi)]
                    sys.argv = ["gen", str(target / "x0x_drum_samples.h")]
                    g.main()
                    expected = b"".join(struct.pack("<f", g.pot_exp(lo,hi,p))
                                        for _,lo,hi in g.EXP_RANGES for p in range(128))
                    for mode in ["packed", "raw"]:
                        exe = target / mode
                        command = ["cc", "-O2", "-std=c99", "-Wall", "-Wextra", "-Werror",
                                   "-fsanitize=address,undefined"]
                        if mode == "raw":
                            command += ["-DX0X_EXP_RAW_REFERENCE"]
                        subprocess.run(command + ["-I" + str(target),
                            str(ROOT / "tests/host/exp_table_test.c"), "-o", str(exe)], check=True)
                        result = subprocess.run([str(exe)], capture_output=True)
                        self.assertEqual(result.returncode, 0, result.stderr.decode())
                        self.assertEqual(result.stdout, expected, (name, mode))
        finally:
            g.EXP_RANGES, sys.argv = original, argv

    def test_corrupt_or_unrepresentable_input_rejected(self):
        for rows in [[], [[0] * 127], [[0] * 129], [[-1] * 128], [[True] * 128],
                     [[0x7f800000] * 128], [[0] + [1 << 26] * 127],
                     [[0] + [(1 << 26) - 1] * 127] * 159]:
            with self.subTest(rows=len(rows)):
                with self.assertRaises(ValueError):
                    g.pack_exp_curves(rows)

if __name__ == "__main__":
    unittest.main()
