# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import sys,zlib,types
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import fm1_runtime_read as R
wire=(Path("build/runtime-diagnostics/native-frame.bin").read_bytes());d=list(wire[1:-1]);answer=R.decode(d,1,77);assert answer and answer["requested_lane"]==1
assert R.decode(d,2,77) is None
assert R.decode([1,2,3,4],1,77) is None
for index in range(8,len(d)):
 bad=d.copy();bad[index]^=1
 try:R.decode(bad,1,77)
 except R.DiagnosticError:pass
 else:raise AssertionError(("accepted corrupt frame",index))
for lane in [-1,24,True,1.2]:
 try:R.request(lane,1)
 except ValueError:pass
 else:raise AssertionError(lane)
class Link:
 def __init__(self):self.sent=[];self.reply=None
 def send(self,m):
  self.sent.append(m);assert tuple(m.data[:4])==R.HEADER and len(m.data)==7
  a=d.copy();a[5]=m.data[5];crc=zlib.crc32(bytes(a[:-5]));a[-5:]=[(crc>>(7*i))&127 for i in range(5)];self.reply=types.SimpleNamespace(type="sysex",data=a)
 def poll(self):r=self.reply;self.reply=None;return r
l=Link();got=R.read_snapshot(l,l,1,message=lambda kind,**kw:types.SimpleNamespace(type=kind,**kw));assert got["requested_lane"]==1 and len(l.sent)==1
print("RUNTIME CLIENT PASS: native-wire CRC/decoder, every payload/CRC-byte corruption refused, token/lane matching, invalid requests and sole command50 transmission")
