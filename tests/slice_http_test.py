# SPDX-License-Identifier: GPL-3.0-only
"""Actual dashboard Handler HTTP routing/MIME; no live project/storage writes."""
import importlib.util
from pathlib import Path
import sys,tempfile,threading,unittest,urllib.request,urllib.error
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'lab'))
spec=importlib.util.spec_from_file_location('slice_test_server',ROOT/'lab/server.py')
server=importlib.util.module_from_spec(spec);spec.loader.exec_module(server)
class SliceHTTPTest(unittest.TestCase):
 def test_dashboard_module_allowlist(self):
  with tempfile.TemporaryDirectory() as temp:
   saved=server.WORK;server.WORK=Path(temp);out=server.WORK/'build/emu';out.mkdir(parents=True)
   names=['slice-ui.js','slice-control.mjs','slice-worklet.mjs','slice-import.js','worklet.js','cc-input-router.mjs']
   for name in names:(out/name).write_bytes((ROOT/'web/emu'/name).read_bytes())
   (out/'private-sdk.txt').write_text('not public',encoding='utf-8')
   http=server.ThreadingHTTPServer(('127.0.0.1',0),server.Handler);thread=threading.Thread(target=http.serve_forever,daemon=True);thread.start()
   try:
    base=f'http://127.0.0.1:{http.server_port}'
    for name in names:
     with urllib.request.urlopen(base+'/emu/'+name) as response:
      self.assertEqual(response.status,200);self.assertEqual(response.headers.get_content_type(),'text/javascript');self.assertEqual(response.read(),(out/name).read_bytes())
    for name in ['private-sdk.txt','../../firmware/src/app/ui.c','%2e%2e/private-sdk.txt']:
     with self.assertRaises(urllib.error.HTTPError) as error:urllib.request.urlopen(base+'/emu/'+name)
     self.assertEqual(error.exception.code,404)
   finally:http.shutdown();http.server_close();thread.join();server.WORK=saved
if __name__=='__main__':unittest.main()
