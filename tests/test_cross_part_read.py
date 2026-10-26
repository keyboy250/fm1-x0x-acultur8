#!/usr/bin/env python3
"""Production command49 client decodes actual cross-part C service replies."""
import copy
import json
from pathlib import Path
import struct
import unittest
from test_pattern_read import Q, Port, Message
ROOT=Path(__file__).resolve().parents[1]

def capture(name):
    data=(ROOT/'build/cross-copy'/name).read_bytes(); rows={}
    while data:
        n=struct.unpack_from('<H',data)[0]; frame,data=data[2:n+2],data[n+2:]
        assert len(frame)==n and frame[0]==240 and frame[-1]==247
        body=tuple(frame[1:-1]); key=body[6],body[7],Q.number(body[8:10])
        assert Q.decode(body,*key,body[5])['status']=='ok'
        rows[key]=body
    port=Port(rows)
    return Q.capture(port,port,(2,3),message=Message)

class CrossReadTests(unittest.TestCase):
    def test_actual_cross_results_and_undo(self):
        reports=[]
        for source,destination in ((2,4),(2,6),(12,1),(23,24),(24,23)):
            with self.subTest(source=source,destination=destination):
                before=capture(f'before-{source}-{destination}.bin')
                after=capture(f'after-{source}-{destination}.bin')
                undo=capture(f'undo-{source}-{destination}.bin')
                self.assertTrue(Q.compare(before,undo)['equal'])
                self.assertEqual(before['stamp']['project_fingerprint'],undo['stamp']['project_fingerprint'])
                a=Q.semantic(before); b=Q.semantic(after)
                self.assertEqual(a['2'],b['2'])
                src=a['2']['lanes'][source-1]; old=a['3']['lanes'][destination-1]; new=b['3']['lanes'][destination-1]
                self.assertEqual(a['3']['swing_raw'],b['3']['swing_raw'])
                for i in range(25):
                    if i!=destination-1:self.assertEqual(a['3']['lanes'][i],b['3']['lanes'][i])
                for key in ('length','rate'):self.assertEqual(src[key],new[key])
                if source<23:
                    self.assertEqual(src['hits'],new['hits']);self.assertEqual(old['shared_accent'],new['shared_accent'])
                else:
                    for key in ('direction','transpose_raw','transpose_semitones'):self.assertEqual(src[key],new[key])
                    gen=copy.deepcopy(src['generator']);gen['accent']=old['generator']['accent'];self.assertEqual(gen,new['generator'])
                    for s,d,n in zip(src['steps'],old['steps'],new['steps']):
                        self.assertEqual(n['note'],s['note']);self.assertEqual(n['flags'],(s['flags']&~4)|(d['flags']&4))
                reports.append(dict(source=source,destination=destination,source_unchanged=True,destination_accent_swing_preserved=True,undo_exact=True))
        (ROOT/'build/cross-copy/query-client-results.json').write_text(json.dumps(reports,indent=2)+'\n')

if __name__=='__main__':unittest.main(verbosity=2)
