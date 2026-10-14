#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Affine layout proof, domain rejection, every decoder width and final guard."""
import hashlib
import random
import re
import subprocess
import sys
import tempfile
import struct
import unittest
from pathlib import Path
from exp_packing_test import g


def decode(desc, blob, n):
    words = []
    for j in range(0, len(desc), 10):
        d = desc[j:j + 10]
        base, slope, offset, width = (int.from_bytes(d[:4], "little"),
            int.from_bytes(d[4:7], "little"), int.from_bytes(d[7:9], "little"), d[9])
        for i in range(n):
            bit = i * width
            start = offset + bit // 8
            assert start + 4 <= len(blob)
            residual = (int.from_bytes(blob[start:start + 4], "little") >> (bit % 8)) & ((1 << width) - 1)
            word = base + slope * i + residual
            assert word <= 0x7f7fffff
            words.append(word)
    return words


class Affine(unittest.TestCase):
    def test_original_words_and_selection(self):
        rows = [[struct.unpack("<I", struct.pack("<f", g.pot_exp(lo, hi, p)))[0]
                 for p in range(128)] for _, lo, hi in g.EXP_RANGES]
        expected = [w for row in rows for w in row]
        for n, total in [(16,7075),(32,6757),(64,6685),(128,6799)]:
            d, b = g.pack_exp_affine(rows, n)
            self.assertEqual(len(d) + len(b), total)
            actual = decode(d,b,n)
            self.assertEqual(actual, expected)
            self.assertEqual(hashlib.sha256(b"".join(struct.pack("<I",w) for w in actual)).hexdigest(),
                             "c8c351223b0c805e68f88c917c5ed0a8b4983277681e2a1b43251627ffbd3faf")

    def test_every_width_offset_guard_slope(self):
        rng = random.Random(81973)
        for n in (16,32,64,128):
            for width in range(1,26):
                for slope in (0,1,0xffffff):
                    row = []
                    for block in range(128//n):
                        residuals = [rng.randrange(1 << width) for _ in range(n)]
                        residuals[0],residuals[1],residuals[-1] = 0,0,(1<<width)-1
                        row += [0x01000000 + slope*i + residuals[i] for i in range(n)]
                    if max(row)>0x7f7fffff: continue
                    d,b=g.pack_exp_affine([row],n)
                    self.assertEqual(decode(d,b,n),row)
                    self.assertEqual(list(d[9::10]),[width]*(128//n))
                    logical=(128//n)*((n*width+7)//8)
                    guard=max(0,((n-1)*width//8)+4-((n*width+7)//8))
                    self.assertEqual(len(b),logical+guard)
                    self.assertEqual(b[logical:],bytes(guard))

    def test_compiled_every_width_and_final_guard(self):
        # Exercise the production accessor from the real generated header with
        # adversarial valid tables: each residual width, byte phase, slope,
        # and constant final block. Expected bytes never use the decoder.
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            oldargv = sys.argv
            try:
                sys.argv = ["gen", str(root / "x0x_drum_samples.h")]
                g.main()
            finally:
                sys.argv = oldargv
            original = (root / "x0x_drum_tables.h").read_text()
            for width in range(1,26):
                rows = []
                for curve in range(19):
                    row=[]
                    for block in range(2):
                        base=0x01000000 + curve * 0x10000
                        slope=0xffffff if curve % 2 else 0
                        residuals=[(i*7919) & ((1<<width)-1) for i in range(64)]
                        residuals[0],residuals[1],residuals[-1]=0,0,(1<<width)-1
                        # Narrow final block makes the object require guards.
                        if curve==18 and block==1: residuals=[0]*64
                        row += [base+slope*i+residuals[i] for i in range(64)]
                    rows.append(row)
                d,b=g.pack_exp_affine(rows)
                header=original
                for name,values in [("x0x_exp_blocks",d),("x0x_exp_packed",b)]:
                    header=re.sub(r"static const uint8_t "+name+r"\[\d+\] = \{.*?\};",
                                  g.arr("uint8_t",name,values),header,flags=re.S)
                (root / "x0x_drum_tables.h").write_text(header)
                exe=root / "oracle"
                subprocess.run(["cc","-O2","-std=c99","-Wall","-Wextra","-Werror",
                                "-fsanitize=address,undefined","-I"+str(root),
                                str(Path(__file__).parent / "host/exp_table_test.c"),
                                "-o",str(exe)],check=True)
                actual=subprocess.run([str(exe)],capture_output=True,check=True).stdout
                expected=b"".join(struct.pack("<I",w) for row in rows for w in row)
                self.assertEqual(actual,expected,width)

    def test_rejected_inputs(self):
        bad=[[],[[0]*127],[[0]*129],[[-1]*128],[[True]*128],
             [[0x7f800000]*128],[[1]+[0]*127],
             [[0,0]+[1<<25]*126], [[i*0x1000000 for i in range(128)]],
             [[0]*128]*4097]
        for rows in bad:
            with self.subTest(rows=len(rows)):
                with self.assertRaises(ValueError):g.pack_exp_affine(rows)
        for n in (0,15,True,256):
            with self.assertRaises(ValueError):g.pack_exp_affine([[0]*128],n)

if __name__ == "__main__": unittest.main()
