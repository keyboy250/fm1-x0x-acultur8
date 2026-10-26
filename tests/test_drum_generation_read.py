#!/usr/bin/env python3
"""Decode actual generation and Undo replies through the production query client."""
import json
from pathlib import Path
import struct
import unittest
from test_pattern_read import Q, Port, Message
ROOT=Path(__file__).resolve().parents[1]
def capture(name):
    data=(ROOT/'build/drum-generation'/name).read_bytes();rows={}
    while data:
        n=struct.unpack_from('<H',data)[0];frame,data=data[2:n+2],data[n+2:]
        assert len(frame)==n and frame[0]==240 and frame[-1]==247
        body=tuple(frame[1:-1]);key=body[6],body[7],Q.number(body[8:10]);assert Q.decode(body,*key,body[5])['status']=='ok';rows[key]=body
    port=Port(rows);return Q.capture(port,port,(2,3),message=Message)
class DrumGenerationReadTests(unittest.TestCase):
    def test_all_22_drum_lanes_and_undo(self):
        reports=[]
        for kit in range(2):
            for voice in range(11):
                with self.subTest(kit=kit,voice=voice):
                    before=capture(f'before-{kit}-{voice}.bin');after=capture(f'after-{kit}-{voice}.bin');undo=capture(f'undo-{kit}-{voice}.bin')
                    self.assertTrue(Q.compare(before,undo)['equal']);self.assertEqual(before['stamp']['project_fingerprint'],undo['stamp']['project_fingerprint'])
                    a,b=Q.semantic(before),Q.semantic(after);self.assertEqual(a['3'],b['3']);self.assertEqual(a['2']['motion'],b['2']['motion']);self.assertEqual(a['2']['swing_raw'],b['2']['swing_raw'])
                    for i in range(25):
                        if i!=kit*11+voice:self.assertEqual(a['2']['lanes'][i],b['2']['lanes'][i])
                    old=a['2']['lanes'][kit*11+voice];new=b['2']['lanes'][kit*11+voice]
                    self.assertEqual({k:v for k,v in old.items() if k!='hits'},{k:v for k,v in new.items() if k!='hits'})
                    reports.append(dict(kit=kit,voice=voice,other_lanes_unchanged=True,accent_swing_unchanged=True,undo_exact=True))
        (ROOT/'build/drum-generation/query-client-results.json').write_text(json.dumps(reports,indent=2)+'\n')
if __name__=='__main__':unittest.main(verbosity=2)
