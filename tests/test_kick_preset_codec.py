import sys,json,struct,hashlib,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import fm1_kick_presets as K
from fm1_generation_read import decode,request,GenerationReadError
class KickPresetCodec(unittest.TestCase):
 def test_native_wire_andportable_record(self):
  for page in range(11,20):
   for index in range(3 if page in(12,13,19) else 4 if page==14 else 2 if page==17 else 1):
    frame=(ROOT/f'build/kick-tests/query-{page}-{index}.bin').read_bytes();data=list(frame[1:-1]);result=decode(data,0,page,77,index);self.assertTrue(result['read_only']);self.assertIsNone(decode(data,0,page,76,index));data[-1]^=1
    with self.assertRaises(GenerationReadError):decode(data,0,page,77,index)
  data=(ROOT/'build/kick-tests/query-18-0.bin').read_bytes()[1:-1];q=decode(data,0,18,77);self.assertEqual(q['slot_status'],0);raw=b''.join(struct.pack('<I',q['word_'+str(i)])for i in range(17));p=K.from_record(raw,'Native kick');self.assertEqual(K.raw_record(p),raw);self.assertEqual(p['hybrid'][2],73)
 def test_exact_bounds_andbad_files(self):
  p=K.from_record(b'\x01'+bytes([0,1]+[0]*37)+bytes(4+16)+struct.pack('<HHBBBB',20,20000,0,2,80,1),'test')
  for key,index in [('hybrid',0),('hybrid',38),('original_bd',6)]:
   bad=json.loads(json.dumps(p));bad[key][index]=999
   with self.assertRaises(ValueError):K.validate(bad)
  with self.assertRaises(ValueError):K.from_record(bytes(67),'bad')
  for page,index in [(12,3),(13,3),(14,4),(15,16),(17,2),(18,16)]:
   with self.assertRaises(ValueError):request(0,page,0,index)
if __name__=='__main__':unittest.main()
