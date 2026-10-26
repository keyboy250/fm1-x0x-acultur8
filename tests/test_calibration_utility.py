#!/usr/bin/env python3
"""Offline analysis plus real C-handler round trips. Never imports mido."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
import wave
import zlib
import numpy as np

ROOT=Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('cal',ROOT/'tools/fm1_calibrate.py')
cal=importlib.util.module_from_spec(spec);spec.loader.exec_module(cal)

class Tests(unittest.TestCase):
    def setUp(self):
        base=ROOT/'build/calibration';base.mkdir(parents=True,exist_ok=True)
        self.folder=Path(tempfile.mkdtemp(prefix='utility-',dir=base)).resolve()
        assert self.folder.is_relative_to(base.resolve())
        self.proc=None

    def fixture(self,bpm=134.0594,seconds=125):
        fs=16000;x=np.zeros(int(seconds*fs))
        t=np.arange(int(fs*.06))/fs
        hit=.8*np.sin(2*np.pi*57*t)*np.exp(-t*70)
        for i in range(int(seconds*bpm/60)):
            p=round(i*fs*60/bpm);n=min(len(hit),len(x)-p)
            if n>0:x[p:p+n]+=hit[:n]
        p=self.folder/('fixture-'+str(bpm)+'-'+str(seconds)+'.wav')
        with wave.open(str(p),'wb') as w:
            w.setparams((1,2,fs,0,'NONE','not compressed'));w.writeframes((x*32767).astype('<i2').tobytes())
        return p

    def server(self):
        command=json.loads(os.environ['CALIBRATION_WIRE_COMMAND']) if 'CALIBRATION_WIRE_COMMAND' in os.environ else [str(ROOT/'build/calibration/wire-test'),'--wire']
        self.proc=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
        def close():
            self.proc.stdin.close()
            try:self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:self.proc.terminate();self.proc.wait(timeout=10)
            self.proc.stdout.close()
        self.addCleanup(close)
        def exchange(inp,out,wire):
            self.assertEqual((inp,out),('synthetic-in','synthetic-out'))
            self.assertEqual(wire[:4],cal.HEADER)
            self.proc.stdin.write(bytes(wire).hex()+'\n');self.proc.stdin.flush()
            line=self.proc.stdout.readline();self.assertTrue(line)
            return cal.decode(bytes.fromhex(line),wire[5],wire[6])
        return exchange

    def test_wave_measurement(self):
        p=self.fixture();r=cal.analyze(p,134)
        self.assertAlmostEqual(r['measured_bpm'],134.0594,places=3)
        self.assertEqual(r['proposed_rate'],44120)
        self.assertFalse(r['verified']);self.assertNotIn('mido',sys.modules)
        seal=r.pop('proposal_sha256');self.assertEqual(seal,cal.digest(r))

    def test_verify_and_short_rejection(self):
        r=cal.analyze(self.fixture(134),134,current_rate=44120);self.assertTrue(r['verified'])
        with self.assertRaises(cal.CalibrationError):cal.analyze(self.fixture(seconds=20),134)
        with self.assertRaises(cal.CalibrationError):cal.analyze(self.fixture(),134,max_ppm=float('nan'))

    def test_apply_reset_actual_handler(self):
        exchange=self.server();original=cal.exchange;cal.exchange=exchange;self.addCleanup(setattr,cal,'exchange',original)
        state=exchange('synthetic-in','synthetic-out',cal.request(7));self.assertEqual(state['active_hz'],44100)
        snap=self.folder/'before.json';cal.save_new(snap,dict(input_port='synthetic-in',output_port='synthetic-out',state=state))
        report=cal.analyze(self.fixture(),134)
        proposal=self.folder/'proposal.json';cal.save_new(proposal,report)
        args=SimpleNamespace(snapshot=snap,proposal=proposal,receipt=self.folder/'apply.json',input_port='synthetic-in',output_port='synthetic-out')
        cal.change(args,1);receipt=cal.load(args.receipt);self.assertEqual(receipt['status'],'APPLIED_READBACK_MATCHES')
        self.assertEqual(receipt['after']['stored_hz'],44120);self.assertEqual(receipt['after']['generation'],1)
        self.assertEqual(receipt['mutation_attempts'],1);self.assertEqual(receipt['requests_attempted'],3)
        self.proc.stdin.write('REBOOT\n');self.proc.stdin.flush();self.assertEqual(self.proc.stdout.readline().strip(),'REBOOTED')
        after=exchange('synthetic-in','synthetic-out',cal.request(8));self.assertEqual(after['active_hz'],44120)
        args.receipt=self.folder/'stale.json'
        with self.assertRaises(cal.CalibrationError):cal.change(args,1)
        self.assertFalse(args.receipt.exists())
        snap2=self.folder/'after.json';cal.save_new(snap2,dict(input_port='synthetic-in',output_port='synthetic-out',state=after))
        args.snapshot=snap2;args.receipt=self.folder/'reset.json';cal.change(args,2)
        reset=cal.load(args.receipt);self.assertEqual(reset['after']['active_hz'],44100)
        self.assertEqual(reset['after']['stored_hz'],0);self.assertEqual(reset['after']['unit'],after['unit'])
        self.assertEqual(reset['after']['generation'],2)
        self.assertNotIn('mido',sys.modules)

    def test_lost_ack_is_not_retried(self):
        actual=self.server();calls=[]
        before=actual('synthetic-in','synthetic-out',cal.request(1))
        snapshot=self.folder/'before.json';cal.save_new(snapshot,dict(input_port='synthetic-in',output_port='synthetic-out',state=before))
        proposal=self.folder/'proposal.json';cal.save_new(proposal,cal.analyze(self.fixture(),134))
        def lost(inp,out,wire):
            calls.append(wire[6]);reply=actual(inp,out,wire)
            if wire[6]==1:raise cal.CalibrationError('Simulated lost write acknowledgement')
            return reply
        original=cal.exchange;cal.exchange=lost;self.addCleanup(setattr,cal,'exchange',original)
        args=SimpleNamespace(snapshot=snapshot,proposal=proposal,receipt=self.folder/'uncertain.json',input_port='synthetic-in',output_port='synthetic-out')
        with self.assertRaises(cal.CalibrationError):cal.change(args,1)
        self.assertEqual(calls,[0,1]);receipt=cal.load(args.receipt)
        self.assertEqual(receipt['status'],'FAILED_OR_UNCERTAIN_NO_RETRY')
        self.assertEqual(receipt['mutation_attempts'],1)
        self.assertEqual(actual('synthetic-in','synthetic-out',cal.request(2))['generation'],1)

    def test_existing_receipt_blocks_mutation(self):
        actual=self.server();calls=[]
        before=actual('synthetic-in','synthetic-out',cal.request(1))
        snapshot=self.folder/'before.json';cal.save_new(snapshot,dict(input_port='synthetic-in',output_port='synthetic-out',state=before))
        proposal=self.folder/'proposal.json';cal.save_new(proposal,cal.analyze(self.fixture(),134))
        receipt=self.folder/'receipt.json';cal.save_new(receipt,{'preserve':True})
        def record(inp,out,wire):calls.append(wire[6]);return actual(inp,out,wire)
        original=cal.exchange;cal.exchange=record;self.addCleanup(setattr,cal,'exchange',original)
        args=SimpleNamespace(snapshot=snapshot,proposal=proposal,receipt=receipt,input_port='synthetic-in',output_port='synthetic-out')
        with self.assertRaises(FileExistsError):cal.change(args,1)
        self.assertEqual(calls,[0]);self.assertEqual(cal.load(receipt),{'preserve':True})

    def test_wire_crc_refusal_and_snapshot(self):
        exchange=self.server();state=exchange('synthetic-in','synthetic-out',cal.request(1))
        request=cal.request(2,1,44119,44100,0,0,42);request[-1]^=1
        refused=exchange('synthetic-in','synthetic-out',request)
        self.assertEqual(refused['status'],1);self.assertEqual(refused['generation'],0)
        self.assertEqual(state['active_hz'],refused['active_hz'])
        bad=cal.HEADER+[1,5,0,0,10]+sum([cal.u35(v) for v in (44100,0,0,0,1,43900,44300,44100,0,1)],[])
        bad+=cal.u35(zlib.crc32(bytes(bad)))
        self.assertEqual(cal.decode(bad,5,0)['active_hz'],44100)
        bad[-1]^=1
        with self.assertRaises(cal.CalibrationError):cal.decode(bad,5,0)
        with self.assertRaises(cal.CalibrationError):cal.request(1,1,45000,44100,0,0,42)

if __name__=='__main__':unittest.main()
