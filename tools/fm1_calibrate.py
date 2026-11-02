#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline quarter-note WAV calibration and explicit command52 operations.

Analysis/verification never opens a MIDI port. No subcommand starts transport,
changes a project, installs firmware, or sends notes/CC. Apply/Reset are explicit.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import secrets
import time
import wave
import zlib

HEADER = [0x7d, 0x46, 0x4c, 52]
FIELDS = ('active_hz', 'stored_hz', 'generation', 'unit', 'record_status',
          'min_hz', 'max_hz', 'default_hz', 'busy', 'capabilities')
MIN_HZ, MAX_HZ = 43900, 44300
class CalibrationError(RuntimeError): pass

def u35(value):
    if type(value) is not int or not 0 <= value <= 0xffffffff:
        raise CalibrationError('Expected unsigned 32-bit integer')
    return [(value >> (7*i)) & 127 for i in range(5)]

def number(values):
    n = sum(v << (7*i) for i, v in enumerate(values))
    if n > 0xffffffff: raise CalibrationError('Oversize protocol word')
    return n

def request(token, op=0, rate=0, current=0, generation=0, unit=0, new_unit=0):
    if type(token) is not int or not 0 <= token < 128 or op not in (0, 1, 2):
        raise CalibrationError('Invalid token/operation')
    wire = HEADER + [1, token, op]
    if op:
        if (op == 1 and not MIN_HZ <= rate <= MAX_HZ) or (op == 2 and rate):
            raise CalibrationError('Out-of-range Apply or nonzero Reset')
        for v in (rate, current, generation, unit, new_unit): wire += u35(v)
        wire += u35(zlib.crc32(bytes(wire)))
    return wire

def decode(data, token, op):
    d = list(data)
    if d[:4] != HEADER: return None
    if len(d) != 64 or any(type(x) is not int or not 0 <= x < 128 for x in d):
        raise CalibrationError('Malformed calibration reply')
    if d[4] != 1: raise CalibrationError('Unsupported calibration ABI')
    if d[5:7] != [token, op]: return None
    if zlib.crc32(bytes(d[:-5])) != number(d[-5:]):
        raise CalibrationError('Calibration reply CRC mismatch')
    if d[8] != len(FIELDS): raise CalibrationError('Unknown field count')
    w = dict(zip(FIELDS, [number(d[9+5*i:14+5*i]) for i in range(len(FIELDS))]))
    if (w['min_hz'], w['max_hz'], w['default_hz']) != (MIN_HZ, MAX_HZ, 44100):
        raise CalibrationError('Unknown calibration bounds/default')
    if not MIN_HZ <= w['active_hz'] <= MAX_HZ or w['busy'] not in (0, 1):
        raise CalibrationError('Invalid calibration state')
    if w['stored_hz'] and not MIN_HZ <= w['stored_hz'] <= MAX_HZ:
        raise CalibrationError('Invalid stored rate')
    if w['record_status'] > 4 or w['capabilities'] != 1:
        raise CalibrationError('Unsupported calibration record/capabilities')
    w['status'] = d[7]
    return w

def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()

def save_new(path, value):
    with Path(path).open('x', encoding='utf-8') as f:
        json.dump(value, f, indent=2); f.write('\n')

def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))

def analyze(path, bpm, current_rate=44100, channel=1, max_ppm=25):
    import numpy as np
    from scipy.signal import correlate, find_peaks
    if not 20 <= bpm <= 300 or not MIN_HZ <= current_rate <= MAX_HZ:
        raise CalibrationError('BPM20..300 and current rate43900..44300 required')
    if not math.isfinite(max_ppm) or not 1 <= max_ppm <= 100:
        raise CalibrationError('Verification tolerance must be1..100ppm')
    with wave.open(str(path), 'rb') as w:
        fs, channels, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
        if not 8000 <= fs <= 192000 or channels > 8 or w.getnframes()/fs > 1800:
            raise CalibrationError('Use an8–192kHz PCM recording no longer than30minutes')
        if not 1 <= channel <= channels or width not in (2, 3, 4):
            raise CalibrationError('Select an existing channel in a16/24/32-bit PCM WAV')
        raw = w.readframes(w.getnframes())
    if width == 3:
        a = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        values = a[:, 0] | a[:, 1] << 8 | a[:, 2] << 16
        values = (values ^ 0x800000) - 0x800000
    else:
        values = np.frombuffer(raw, dtype='<i'+str(width))
    x = values.reshape(-1, channels)[:, channel-1].astype(float) / (2**(width*8-1))
    hop = max(1, fs//1000)
    envelope = np.max(np.abs(x[:len(x)//hop*hop].reshape(-1, hop)), axis=1)
    if not len(envelope) or envelope.max() < .01: raise CalibrationError('No usable signal')
    peaks, _ = find_peaks(envelope, height=.25*envelope.max(),
                         distance=max(1, int(.65*60/bpm*fs/hop)), prominence=.1*envelope.max())
    if len(peaks) < 40: raise CalibrationError('Need40or more clean quarter-note kicks')
    pre, post, margin = int(fs*.004), int(fs*.036), int(fs*.014)
    ref = int(peaks[5]*hop); template = x[ref-pre:ref+post]
    positions, quality = [], []
    for sample in peaks*hop:
        lo, hi = max(0, int(sample)-pre-margin), min(len(x), int(sample)+post+margin)
        segment = x[lo:hi]
        if len(segment) < len(template): continue
        c = correlate(segment, template, mode='valid', method='fft'); j = int(np.argmax(c))
        norm = np.sqrt(np.sum(template**2)*np.sum(segment[j:j+len(template)]**2))
        positions.append(lo+j+pre); quality.append(float(c[j]/norm) if norm else 0)
    accepted = np.asarray(quality) >= .95
    if accepted.sum() < 40 or (~accepted).sum() > max(2, len(accepted)*.02):
        raise CalibrationError('Pattern is not sufficiently repeatable')
    times = np.asarray(positions)[accepted]/fs
    if times[-1]-times[0] < 120: raise CalibrationError('Need at least120seconds of active kicks')
    gaps = np.diff(times); median = float(np.median(gaps))
    if abs(median-60/bpm)/(60/bpm) > .01 or np.max(abs(gaps-median)) > .003:
        raise CalibrationError('Missing/extra kicks or unstable intervals;use quarter-note-only pattern')
    indices = np.arange(len(times)); period, intercept = np.polyfit(indices, times, 1)
    residual = (times-(intercept+indices*period))*1000
    if np.max(abs(residual)) > 1: raise CalibrationError('Nonlinear timing or transient instability')
    measured = float(60/period); ppm = (measured/bpm-1)*1e6
    proposed = int(round(current_rate*measured/bpm))
    if not MIN_HZ <= proposed <= MAX_HZ: raise CalibrationError('Proposed rate outside firmware bounds')
    report = dict(schema=1, pattern='quarter-note-kicks', wav_sha256=hashlib.sha256(Path(path).read_bytes()).hexdigest(),
                  sample_rate=fs, channel=channel, duration_seconds=float(times[-1]-times[0]),
                  matched_kicks=len(times), target_bpm=bpm, measured_bpm=measured, error_ppm=ppm,
                  current_rate=current_rate, proposed_rate=proposed,
                  max_residual_ms=float(np.max(abs(residual))), full_scale_samples=int((abs(x)>=.99999).sum()),
                  verification_limit_ppm=max_ppm, verified=abs(ppm)<=max_ppm,
                  limits='Relative to recording-interface clock;requires clean quarter-note pattern,stretch disabled,and unchanged tempo. No device write.')
    report['proposal_sha256'] = digest(report)
    return report

def exchange(input_port, output_port, wire):
    # Imported only by explicit live operations. No live operation is run by local tests.
    import mido
    token, op = wire[5:7]
    with mido.open_input(input_port) as incoming, mido.open_output(output_port) as outgoing:
        while incoming.poll() is not None: pass
        outgoing.send(mido.Message('sysex', data=wire))
        deadline = time.monotonic()+3
        while time.monotonic() < deadline:
            msg = incoming.poll()
            if msg is not None and msg.type == 'sysex':
                state = decode(msg.data, token, op)
                if state is not None: return state
            time.sleep(.01)
    raise CalibrationError('No valid reply;unsupported firmware or uncertain operation. Do not retry a write blindly.')

def checked(state):
    if state['status']: raise CalibrationError('Firmware refused operation:status'+str(state['status']))
    return state

def change(args, op):
    before = load(args.snapshot)
    if (before['input_port'], before['output_port']) != (args.input_port, args.output_port):
        raise CalibrationError('Ports differ from saved snapshot')
    previous = checked(before['state'])
    live = checked(exchange(args.input_port, args.output_port, request(secrets.randbelow(128))))
    keys = ('active_hz', 'stored_hz', 'generation', 'unit', 'record_status')
    if any(previous[k] != live[k] for k in keys): raise CalibrationError('Stale snapshot;read status again')
    if live['busy'] or live['record_status'] > 2: raise CalibrationError('Device busy or record unavailable')
    rate = 0
    if op == 1:
        report = load(args.proposal); seal = report.pop('proposal_sha256', None)
        if seal != digest(report): raise CalibrationError('Proposal hash mismatch')
        if (report.get('schema') != 1 or report.get('pattern') != 'quarter-note-kicks'
                or len(report.get('wav_sha256','')) != 64
                or report.get('duration_seconds',0) < 120 or report.get('matched_kicks',0) < 40
                or not 0 <= report.get('max_residual_ms',float('inf')) <= 1):
            raise CalibrationError('Incomplete or unsuitable measurement proposal')
        if not 20 <= report['target_bpm'] <= 300 or not math.isfinite(report['measured_bpm']):
            raise CalibrationError('Invalid measured tempo')
        if report['current_rate'] != live['active_hz']: raise CalibrationError('Measurement used a different calibration rate')
        rate = report['proposed_rate']
        if type(rate) is not int or not MIN_HZ <= rate <= MAX_HZ or rate != round(live['active_hz']*report['measured_bpm']/report['target_bpm']):
            raise CalibrationError('Proposal rate does not match measurement')
        if report.get('verified'):
            raise CalibrationError('Measurement already passes its verification tolerance;no Apply needed')
    new_unit = live['unit'] or secrets.randbelow(0xffffffff)+1
    wire = request(secrets.randbelow(128), op, rate, live['active_hz'], live['generation'], live['unit'], new_unit)
    # Reserve the receipt before any mutation, and never overwrite earlier evidence.
    receipt = dict(schema=1, operation='apply' if op == 1 else 'reset', before=before,
                   requested_rate=rate, requested_unit=new_unit, expected_generation=live['generation'],
                   status='PENDING', requests_attempted=1,
                   read_attempts=1, mutation_attempts=0)
    save_new(args.receipt, receipt)
    try:
        receipt['requests_attempted'] += 1
        receipt['mutation_attempts'] = 1
        reply = exchange(args.input_port, args.output_port, wire)
        receipt['reply'] = reply
        after = checked(reply)
        expected = rate or live['default_hz']
        if after['active_hz'] != expected or after['stored_hz'] != rate:
            raise CalibrationError('Write acknowledgement does not match request')
        changed = rate != live['stored_hz']
        if after['generation'] != live['generation']+int(changed) or after['unit'] != (new_unit if changed else live['unit']):
            raise CalibrationError('Write acknowledgement generation/unit does not match request')
        receipt['requests_attempted'] += 1
        receipt['read_attempts'] += 1
        confirm = checked(exchange(args.input_port, args.output_port, request(secrets.randbelow(128))))
        if any(after[k] != confirm[k] for k in keys): raise CalibrationError('Post-write status changed')
        receipt.update(status='APPLIED_READBACK_MATCHES', after=after, confirmed=confirm)
    except Exception as exc:
        receipt.update(status='FAILED_OR_UNCERTAIN_NO_RETRY', error=str(exc)); raise
    finally:
        Path(args.receipt).write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')

def main():
    ap = argparse.ArgumentParser(description=__doc__); sub = ap.add_subparsers(dest='command', required=True)
    sub.add_parser('ports', help='List MIDI port names without opening a port')
    for name in ('analyze', 'verify'):
        p = sub.add_parser(name); p.add_argument('wav', type=Path);p.add_argument('--bpm',type=float,required=True)
        p.add_argument('--current-rate',type=int,default=44100);p.add_argument('--channel',type=int,default=1)
        p.add_argument('--max-ppm',type=float,default=25);p.add_argument('--out',type=Path,required=True)
    for name in ('status', 'apply', 'reset'):
        p=sub.add_parser(name);p.add_argument('--input-port',required=True);p.add_argument('--output-port',required=True)
        if name=='status': p.add_argument('--out',type=Path,required=True)
        else:
            p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--receipt',type=Path,required=True)
            if name=='apply':p.add_argument('--proposal',type=Path,required=True)
    args=ap.parse_args()
    if args.command=='ports':
        import mido
        print(json.dumps(dict(inputs=mido.get_input_names(),outputs=mido.get_output_names()),indent=2))
    elif args.command in ('analyze','verify'):
        result=analyze(args.wav,args.bpm,args.current_rate,args.channel,args.max_ppm);save_new(args.out,result)
        print(json.dumps(result,indent=2))
        if args.command=='verify' and not result['verified']:raise CalibrationError('Timing outside verification tolerance')
    elif args.command=='status':
        if args.out.exists():raise CalibrationError('Output already exists')
        state=checked(exchange(args.input_port,args.output_port,request(secrets.randbelow(128))))
        save_new(args.out,dict(schema=1,input_port=args.input_port,output_port=args.output_port,state=state));print(json.dumps(state,indent=2))
    else:change(args,1 if args.command=='apply' else 2)

if __name__=='__main__':
    try:main()
    except ImportError as exc:raise SystemExit('Missing dependency: '+str(exc)+';see calibration-requirements.txt')
    except (CalibrationError,ValueError,OSError,KeyError) as exc:raise SystemExit(str(exc))
