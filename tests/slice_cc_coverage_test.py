# SPDX-License-Identifier: GPL-3.0-only
"""Browser overlay never changes native slice bindings or stable wire registry."""
from pathlib import Path
import importlib.util,json,unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('slice_cc_generator',ROOT/'tools/gen_cc_bindings.py');g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
class SliceCoverage(unittest.TestCase):
 def test_native_and_browser_are_separate(self):
  h,j=g.generate(ROOT);self.assertEqual(h,(ROOT/'firmware/src/app/cc_bindings.h').read_text(encoding='utf-8'));self.assertEqual(j,(ROOT/'docs/ACULTUR8_CC_RUNTIME_COVERAGE.json').read_text(encoding='utf-8'))
  inv=json.loads((ROOT/'docs/ACULTUR8_CC_INVENTORY.json').read_text(encoding='utf-8'));coverage=json.loads(j)['rows'];rows={r['id']:r for r in coverage}
  slices=[r for r in inv['entries'] if r['id'].startswith('slice.')];self.assertEqual(len(slices),13)
  for n,r in enumerate(slices):
   self.assertEqual(r['parameter_number'],1104+n);self.assertEqual(r['address'],{'channel':13,'cc':80+n});self.assertEqual(g.binding(r)[0],'UNSUPPORTED')
   c=rows[r['id']];self.assertEqual(c['binding'],'UNSUPPORTED');self.assertEqual(c['status'],'unimplemented');self.assertEqual(c['browser']['source'],2);self.assertEqual(c['browser']['abi'],65537)
   self.assertEqual(c['browser']['parameter_number'],r['parameter_number'])
   for key in ['physical_usb_trs','real_daw','real_audio_context','download_delivery']:self.assertFalse(c['browser'][key])
  self.assertEqual(sum('browser' in r for r in coverage),13,'browser qualification must not leak into unrelated native rows')
if __name__=='__main__':unittest.main()
