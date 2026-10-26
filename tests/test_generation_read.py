#!/usr/bin/env python3
"""Decode actual generation and Undo replies through the production query client."""
import json
from pathlib import Path
import struct
import unittest
from test_pattern_read import Q, Port, Message
ROOT=Path(__file__).resolve().parents[1]
def capture(name):
    data=(ROOT/'build/generation'/name).read_bytes();rows={}
    while data:
        n=struct.unpack_from('<H',data)[0];frame,data=data[2:n+2],data[n+2:]
        assert len(frame)==n and frame[0]==240 and frame[-1]==247
        body=tuple(frame[1:-1]);key=body[6],body[7],Q.number(body[8:10]);assert Q.decode(body,*key,body[5])['status']=='ok';rows[key]=body
    port=Port(rows);return Q.capture(port,port,(2,3),message=Message)
class GenerationReadTests(unittest.TestCase):
    def test_all_modes_both_parts_and_undo(self):
        reports=[]
        masks=[{9,11,0,2,4,5,7},{0,2,4,5,7,9,11},{7,11,2},{7,11,2},set(range(12))]
        for bass in range(2):
            for mode in range(5):
                with self.subTest(bass=bass,mode=mode):
                    before=capture(f'before-{bass}-{mode}.bin');after=capture(f'after-{bass}-{mode}.bin');undo=capture(f'undo-{bass}-{mode}.bin')
                    self.assertTrue(Q.compare(before,undo)['equal']);self.assertEqual(before['stamp']['project_fingerprint'],undo['stamp']['project_fingerprint'])
                    a,b=Q.semantic(before),Q.semantic(after);self.assertEqual(a['3'],b['3']);self.assertEqual(a['2']['motion'],b['2']['motion']);self.assertEqual(a['2']['swing_raw'],b['2']['swing_raw'])
                    for i in range(25):
                        if i!=22+bass:self.assertEqual(a['2']['lanes'][i],b['2']['lanes'][i])
                    old=a['2']['lanes'][22+bass];new=b['2']['lanes'][22+bass]
                    self.assertEqual({k:v for k,v in old.items() if k!='steps'},{k:v for k,v in new.items() if k!='steps'})
                    low=12*(old['generator']['base_octave']+1);high=low+12*old['generator']['octave_range']-1
                    for step in new['steps']:
                        self.assertIn(step['note']%12,masks[mode]);self.assertGreaterEqual(step['note'],low);self.assertLessEqual(step['note'],high)
                    reports.append(dict(bass=bass,mode=mode,constrained_notes=True,other_lanes_unchanged=True,undo_exact=True))
        (ROOT/'build/generation/query-client-results.json').write_text(json.dumps(reports,indent=2)+'\n')
if __name__=='__main__':unittest.main(verbosity=2)
