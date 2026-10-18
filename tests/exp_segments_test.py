#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact segmented predictor bounds and deterministic resource proof."""
import hashlib
import struct
import re
import subprocess
import sys
import tempfile
from pathlib import Path
import unittest
from exp_packing_test import g


def decode(starts, desc, blob):
    rows=[]
    for first in starts:
        row=[]
        block=first
        start=0
        while start<128:
            d=desc[14*block:14*block+14]
            assert len(d)==14
            base=int.from_bytes(d[:4],"little")
            slope=int.from_bytes(d[4:7],"little")
            curvature=int.from_bytes(d[7:10],"little")
            offset=int.from_bytes(d[10:12],"little")
            width,end=d[12:14]
            assert 1<=width<=25 and start+2<=end<=128
            for i in range(end-start):
                bit=i*width
                pos=offset+bit//8
                assert pos+4<=len(blob)
                residual=(int.from_bytes(blob[pos:pos+4],"little")>>(bit%8))&((1<<width)-1)
                predicted=base+(slope+curvature*i)*i
                word=predicted+residual
                assert predicted<=word<=0x7f7fffff
                row.append(word)
            start=end
            block+=1
        assert len(row)==128 and block-first<=64
        rows.append(row)
    return rows


class Segments(unittest.TestCase):
    def test_original_words_layout_and_determinism(self):
        rows=[[struct.unpack("<I",struct.pack("<f",g.pot_exp(lo,hi,p)))[0]
               for p in range(128)] for _,lo,hi in g.EXP_RANGES]
        packed=g.pack_exp_segments(rows)
        self.assertEqual(tuple(map(len,packed)),(19,756,5247))
        self.assertEqual(packed,g.pack_exp_segments(rows))
        self.assertEqual(decode(*packed),rows)
        self.assertEqual(hashlib.sha256(b"".join(struct.pack("<I",w) for row in rows for w in row)).hexdigest(),
                         "c8c351223b0c805e68f88c917c5ed0a8b4983277681e2a1b43251627ffbd3faf")
        self.assertTrue(any(int.from_bytes(packed[1][j+7:j+10],"little") for j in range(0,len(packed[1]),14)))

    def test_exact_polynomial_and_final_guards(self):
        for slope,curvature in [(0,0),(1,1),(100,10000),(0xffffff,0)]:
            row=[0x01000000+slope*i+curvature*i*i for i in range(128)]
            if max(row)>0x7f7fffff:continue
            packed=g.pack_exp_segments([row])
            self.assertEqual(decode(*packed),[row])
            self.assertEqual(packed[2][-3:],bytes(3))

    def test_compiled_maximum_scan_and_coefficients(self):
        # Every curve shares 64 two-entry segments, forcing the last pot to
        # traverse the full maximum. Coefficients reach the 24-bit domain;
        # each index remains 0 or 1, so every expected IEEE word stays finite.
        descriptors=bytearray()
        expected=[]
        for block in range(64):
            base=0x10000000+block
            descriptors.extend(base.to_bytes(4,"little")+bytes([255])*6+
                               block.to_bytes(2,"little")+bytes([1,2*block+2]))
            expected += [base,base+2*0xffffff]
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            oldargv=sys.argv
            try:
                sys.argv=["gen",str(root/"x0x_drum_samples.h")]
                g.main()
            finally:sys.argv=oldargv
            header=(root/"x0x_drum_tables.h").read_text()
            for name,values in [("x0x_exp_starts",bytes(19)),
                                ("x0x_exp_segments",descriptors),
                                ("x0x_exp_packed",bytes(67))]:
                header=re.sub(r"static const uint8_t "+name+r"\[\d+\] = \{.*?\};",
                              g.arr("uint8_t",name,values),header,flags=re.S)
            (root/"x0x_drum_tables.h").write_text(header)
            exe=root/"oracle"
            subprocess.run(["cc","-O2","-std=c99","-Wall","-Wextra","-Werror",
                            "-fsanitize=address,undefined","-I"+str(root),
                            str(Path(__file__).parent/"host/exp_table_test.c"),
                            "-o",str(exe)],check=True)
            actual=subprocess.run([str(exe)],capture_output=True,check=True).stdout
            self.assertEqual(actual,b"".join(struct.pack("<I",w) for w in expected)*19)

    def test_compiled_every_width_and_final_guard(self):
        # Forge each legal residual width in the actual segmented accessor.
        # A narrow last segment forces its final cross-byte read to use guards.
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);oldargv=sys.argv
            try:
                sys.argv=['gen',str(root/'x0x_drum_samples.h')];g.main()
            finally:sys.argv=oldargv
            original=(root/'x0x_drum_tables.h').read_text()
            for width in range(1,26):
                descriptors=bytearray();blob=bytearray();expected=[];read_end=0
                for block in range(64):
                    w=1 if block==63 else width;base=0x10000000+block;offset=len(blob)
                    value=(1<<w)-1;packed=value<<w
                    descriptors.extend(base.to_bytes(4,'little')+bytes(6)+offset.to_bytes(2,'little')+bytes([w,block*2+2]))
                    blob.extend(packed.to_bytes((2*w+7)//8,'little'))
                    read_end=max(read_end,offset+w//8+4);expected.extend([base,base+value])
                blob.extend(bytes(max(0,read_end-len(blob))))
                header=original
                for name,values in [('x0x_exp_starts',bytes(19)),('x0x_exp_segments',descriptors),('x0x_exp_packed',blob)]:
                    header,count=re.subn(r'static const uint8_t '+name+r'\[\d+\] = \{.*?\};',g.arr('uint8_t',name,values),header,flags=re.S)
                    self.assertEqual(count,1,name)
                (root/'x0x_drum_tables.h').write_text(header)
                exe=root/'oracle'
                subprocess.run(['cc','-O2','-std=c99','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root),str(Path(__file__).parent/'host/exp_table_test.c'),'-o',str(exe)],check=True)
                actual=subprocess.run([str(exe)],capture_output=True)
                self.assertEqual(actual.returncode,0,actual.stderr.decode('utf-8','replace'))
                self.assertEqual(actual.stdout,b''.join(struct.pack('<I',w) for w in expected)*19,width)

    def test_invalid_input(self):
        for rows in [[],[[0]*127],[[0]*129],[[True]*128],[[-1]*128],
                     [[0x7f800000]*128],[[128-i for i in range(128)]]]:
            with self.subTest(rows=len(rows)):
                with self.assertRaises(ValueError):g.pack_exp_segments(rows)

if __name__=="__main__":unittest.main()
