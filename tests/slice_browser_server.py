# SPDX-License-Identifier: GPL-3.0-only
"""Disposable slice acceptance routes only; no arbitrary source/private files."""
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
MODULES={'worklet.js','midi-worklet.mjs','cc-input-worklet.mjs','slice-worklet.mjs','slice-control.mjs','slice-import.js','cc-input-router.mjs'}
class Handler(BaseHTTPRequestHandler):
 def do_GET(self):
  path=self.path.split('?',1)[0]
  if path=='/tests/host/slice_browser_test.html':file=ROOT/'tests/host/slice_browser_test.html';mime='text/html; charset=utf-8'
  elif path=='/build/emu/x0x.wasm':file=ROOT/'build/emu/x0x.wasm';mime='application/wasm'
  elif path.startswith('/web/emu/') and path[9:] in MODULES:file=ROOT/'web/emu'/path[9:];mime='text/javascript'
  else:self.send_error(404);return
  content=file.read_bytes();self.send_response(200);self.send_header('Content-Type',mime);self.send_header('Content-Length',str(len(content)));self.end_headers();self.wfile.write(content)
if __name__=='__main__':
 port=int(sys.argv[1]) if len(sys.argv)>1 else 8119
 print(f'http://127.0.0.1:{port}/tests/host/slice_browser_test.html',flush=True)
 ThreadingHTTPServer(('127.0.0.1',port),Handler).serve_forever()
