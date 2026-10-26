#!/usr/bin/env python3
"""Decode actual before/after variation replies through the production query client."""
import json
from pathlib import Path
import struct
import unittest
from test_pattern_read import Q, Port, Message
ROOT=Path(__file__).resolve().parents[1]
def capture(name):
    data=(ROOT/'build/303-variation'/name).read_bytes();rows={}
    while data:
        n=struct.unpack_from('<H',data)[0];frame,data=data[2:n+2],data[n+2:]
        assert len(frame)==n and frame[0]==240 and frame[-1]==247
        body=tuple(frame[1:-1]);key=body[6],body[7],Q.number(body[8:10]);assert Q.decode(body,*key,body[5])['status']=='ok';rows[key]=body
    port=Port(rows);return Q.capture(port,port,(1,2),message=Message)
class VariationReadTests(unittest.TestCase):
    def test_base_readback_unchanged(self):
        before=capture('before.bin');after=capture('after.bin')
        self.assertTrue(Q.compare(before,after)['equal']);self.assertEqual(before['stamp']['project_fingerprint'],after['stamp']['project_fingerprint'])
        for bass in range(2):
            for mode in range(4):
                a=json.loads((ROOT/f'build/303-variation/baseline-{bass}-{mode}.json').read_text())
                for state in ('armed','off'):
                    self.assertEqual(a,json.loads((ROOT/f'build/303-variation/{state}-{bass}-{mode}.json').read_text()))
        (ROOT/'build/303-variation/query-client-results.json').write_text(json.dumps(dict(target_pattern=1,other_pattern=2,all_lanes_motion_accent_swing_unchanged=True,project_fingerprint_unchanged=True,save_bytes_identical=True),indent=2)+'\n')
if __name__=='__main__':unittest.main(verbosity=2)
