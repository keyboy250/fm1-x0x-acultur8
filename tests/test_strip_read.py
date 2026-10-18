# SPDX-License-Identifier: GPL-3.0-only
import importlib.util
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("strip_read",ROOT/"tools/fm1_strip_read.py")
R=importlib.util.module_from_spec(spec);spec.loader.exec_module(R)

class Message:
    def __init__(self,kind, data=()):self.type=kind;self.data=list(data)

class ReaderTest(unittest.TestCase):
    def setUp(self):
        raw=(ROOT/"build/strip-query-replies.bin").read_bytes()
        self.assertEqual(len(raw),401*31)
        self.frames=[raw[i+1:i+30] for i in range(0,len(raw),31)]
    def test_native_encoder_python_decoder_all_fields(self):
        for strip in range(25):
            for flags in range(16):
                row=R.decode(self.frames[strip*16+flags],strip,strip+flags)
                self.assertEqual(row["highpass_hz"],20000-strip if strip else 20)
                self.assertEqual(row["lowpass_hz"],20+strip if strip else 20000)
                self.assertEqual((row["boost_db"],row["ceiling_dbfs"],row["release_ms"],row["flags"]),(24,-48,250,flags))
                self.assertEqual([row[k] for k in ("strip_enabled","highpass_enabled","lowpass_enabled","limiter_enabled")],[bool(flags&(1<<i)) for i in range(4)])
                self.assertFalse(row["audio_dsp_readback"])
                self.assertEqual(row["configuration_source"],"project")
        row=R.decode(self.frames[-1],23,77)
        self.assertEqual([row[k] for k in ("highpass_hz","lowpass_hz","boost_db","ceiling_dbfs","release_ms","flags")],[146,10240,0,-1,80,7])
    def test_validation_and_correlation(self):
        d=list(self.frames[-1]);self.assertIsNone(R.decode(d,22,77));self.assertIsNone(R.decode(d,23,76))
        self.assertIsNone(R.decode([1,2,3],23,77))
        for bad in (d[:-1],d+[0],d[:5],d[:4]+[2]+d[5:],d[:24]+[25]+d[25:],d[:-1]+[16],d[:8]+[127]*5+d[13:]):
            with self.assertRaises(R.ReadbackError):R.decode(bad,23,77)
        for status,name in [(1,"invalid"),(2,"busy"),(3,"unsupported")]:
            self.assertEqual(R.decode(d[:7]+[status],23,77)["status"],name)
        for strip,token in [(-1,1),(25,1),(1,128),(1,-1),(True,1)]:
            with self.assertRaises(ValueError):R.request(strip,token)
    def test_busy_stale_reply_and_only_read_commands(self):
        source=list(self.frames[-1]);sent=[];pending=[]
        class In:
            def poll(self):return pending.pop(0) if pending else None
        class Out:
            def send(self,m):
                sent.append(m);token=m.data[5]
                payload=source[:];payload[5]=token
                if len(sent)==1:pending.append(Message("sysex",payload[:7]+[2]))
                else:
                    stale=payload[:];stale[5]=(token-1)&127
                    pending.extend([Message("clock"),Message("sysex",stale),Message("sysex",payload)])
        row=R.read_strip(In(),Out(),23,message=Message)
        self.assertEqual(row["highpass_hz"],146);self.assertEqual(len(sent),2)
        self.assertTrue(all(m.type=="sysex" and m.data[:4]==list(R.HEADER) and len(m.data)==7 for m in sent))
    def test_no_reply_fails_without_write_fallback(self):
        sent=[]
        class In:
            def poll(self):return None
        class Out:
            def send(self,m):sent.append(m)
        with self.assertRaises(R.ReadbackError):R.read_strip(In(),Out(),23,message=Message,timeout=0)
        self.assertEqual(len(sent),1);self.assertEqual(sent[0].data[:4],list(R.HEADER))
    def test_windows_pairing_and_ambiguity(self):
        self.assertEqual(R.choose_ports(["X0X FM-1 18"],["X0X FM-1 19"]),("X0X FM-1 18","X0X FM-1 19"))
        with self.assertRaises(R.ReadbackError):R.choose_ports(["FM-1 1","FM-1 2"],["FM-1 1","FM-1 2"])
        self.assertEqual(R.choose_ports(["FM-1 1","FM-1 2"],["FM-1 1","FM-1 2"],"FM-1 2","FM-1 1"),("FM-1 2","FM-1 1"))
        with self.assertRaises(R.ReadbackError):R.choose_ports(["Other synth"],["Other synth"])

if __name__=="__main__":unittest.main()
