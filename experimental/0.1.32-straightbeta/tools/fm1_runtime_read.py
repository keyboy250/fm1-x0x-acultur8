#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded command50 runtime snapshots. Sends only read-only SysEx command50.
No CC, notes, transport, save/export, reset, selection or read-clear operations.
"""
import argparse,json,secrets,time,zlib
from pathlib import Path
from fm1_pattern_read import choose_ports
HEADER=(0x7d,0x46,0x4c,50)
FIELDS=('revision', 'queue_written', 'queue_read', 'transport_epoch', 'playing', 'request_start', 'request_stop', 'request_continue', 'transport_hold', 'transport_stage', 'requested_lane', 'playing_pattern', 'lane_clocks_enabled', 'lane_storage', 'expanded_rates', 'arrangement_flags', 'song_on', 'song_position', 'chain_a', 'chain_b', 'recipe_lane', 'recipe_pattern', 'every_raw', 'change', 'seed', 'lock', 'mode', 'options', 'notes_mask', 'low_note', 'high_note', 'step_zero_based', 'cycle', 'active_length', 'active_rate', 'stored_part_length', 'part_cycle', 'freeze_state', 'capture_count', 'capture_length', 'capture_prefix_valid', 'capture_owner_matches', 'base_low', 'base_high', 'accepted_low', 'accepted_high', 'mute_mask', 'voice_mute_mask', 'probability', 'predicted_low', 'predicted_high', 'variation_length')
class DiagnosticError(RuntimeError):pass
def request(lane,token):
    if type(lane) is not int or not 0<=lane<24 or type(token) is not int or not 0<=token<128:raise ValueError('lane0..23,token0..127 required')
    return list(HEADER)+[1,token,lane]
def number(a):return sum(v<<(7*i) for i,v in enumerate(a))
def steps(lo,hi,length):return [i+1 for i in range(min(length,64)) if ((lo if i<32 else hi)>>(i%32))&1]
def decode(data,lane,token):
    d=tuple(data)
    if d[:4]!=HEADER:return None
    if len(d)<13 or any(type(x) is not int or not 0<=x<128 for x in d):raise DiagnosticError('Malformed frame')
    if d[4]!=1:raise DiagnosticError('Unsupported version')
    if d[5:7]!=(token,lane):return None
    crc=number(d[-5:])
    if crc>0xffffffff or zlib.crc32(bytes(d[:-5]))!=crc:raise DiagnosticError('CRC mismatch')
    if d[7]:
        if len(d)!=13 or d[7] not in (1,2,3):raise DiagnosticError('Malformed refusal')
        raise DiagnosticError({1:'Invalid request',2:'Runtime unavailable',3:'Unsupported protocol'}[d[7]])
    if len(d)!=9+5*len(FIELDS)+5 or d[8]!=len(FIELDS):raise DiagnosticError('Unexpected field count/length')
    values=[number(d[9+5*i:14+5*i]) for i in range(len(FIELDS))]
    if any(x>0xffffffff for x in values):raise DiagnosticError('Oversize integer')
    w=dict(zip(FIELDS,values))
    if w['requested_lane']!=lane or w['playing_pattern']>=32 or w['recipe_lane'] not in (*range(24),255):raise DiagnosticError('Invalid snapshot context')
    for k in ('active_length','stored_part_length','variation_length','capture_count','capture_length'):
        if w[k]>64:raise DiagnosticError('Invalid length/count')
    if w['capture_prefix_valid'] and (w['capture_count']>w['capture_length'] or not w['capture_owner_matches']):raise DiagnosticError('Invalid capture validity')
    owner=w['capture_owner_matches']!=0
    w.update(playing_pattern_number=w['playing_pattern']+1,recipe_pattern_number=w['recipe_pattern']+1,
      cadence_passes=2<<w['every_raw'] if w['every_raw']<3 else None,
      base_hits=steps(w['base_low'],w['base_high'],w['active_length']),
      predicted_hits=steps(w['predicted_low'],w['predicted_high'],w['active_length']) if lane<22 else None,
      accepted_prefix_hits=steps(w['accepted_low'],w['accepted_high'],w['capture_count']) if w['capture_prefix_valid'] else None,
      capture_scope='existing selected-lane accepted-gate prefix; no prior-pass history or external MIDI-delivery proof',
      predicted_scope='calculated intent before probability/mute; not observed triggers',
      recipe_matches_playing_lane=owner)
    w['observations']=dict(recipe_disabled=w['recipe_lane']==255,recipe_slot_mismatch=not owner,
      frozen=owner and w['freeze_state']==2,transport_pending=bool(w['request_start'] or w['request_stop'] or w['request_continue'] or w['transport_hold'] or w['transport_stage']),
      queue_drained=w['queue_written']==w['queue_read'],clock_length_differs_from_variation_length=w['active_length']!=w['variation_length'],
      muted=bool((w['mute_mask']|w['voice_mute_mask'])&(1<<lane)),capture_has_observed_prefix=bool(w['capture_prefix_valid'] and w['capture_count']))
    return w
def read_snapshot(ip,op,lane,*,message,timeout=1.5):
    token=secrets.randbelow(128);op.send(message('sysex',data=request(lane,token)));end=time.monotonic()+timeout
    while time.monotonic()<end:
        pkt=ip.poll()
        if pkt is not None and pkt.type=='sysex':
            answer=decode(pkt.data,lane,token)
            if answer is not None:return answer
        time.sleep(.002)
    raise DiagnosticError('No command50 reply; no fallback/save/export was sent')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--lane',type=int,required=True);p.add_argument('--samples',type=int,default=1);p.add_argument('--interval',type=float,default=.25);p.add_argument('--port');p.add_argument('--output-port');p.add_argument('--output',type=Path);a=p.parse_args()
    try:
        request(a.lane,0)
        if not 1<=a.samples<=240 or not .25<=a.interval<=5:raise ValueError('samples1..240; interval0.25..5 seconds required')
        if a.output and a.output.exists():raise ValueError('Output already exists')
        import mido
        inp,out=choose_ports(mido.get_input_names(),mido.get_output_names(),a.port,a.output_port)
        rows=[]
        with mido.open_input(inp) as ip,mido.open_output(out) as op:
            for i in range(a.samples):
                start=time.monotonic();row=read_snapshot(ip,op,a.lane,message=mido.Message);row['host_monotonic_seconds']=time.monotonic();rows.append(row)
                if i+1<a.samples:time.sleep(max(0,a.interval-(time.monotonic()-start)))
        result=dict(schema=1,read_only=True,command=50,input=inp,output=out,snapshots=rows)
        if a.output:
            with a.output.open('x',encoding='utf-8') as f:f.write(json.dumps(result,indent=2)+'\n')
        print(json.dumps(result,indent=2))
    except (OSError,ImportError,ValueError,DiagnosticError) as e:p.exit(1,'Runtime query failed: '+str(e)+'\n')
if __name__=='__main__':main()
