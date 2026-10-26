#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded read-only command51 ABI1. Never sends controls, transport or saves."""
import argparse
import json
import secrets
import time
import zlib
from pathlib import Path

HEADER=(125,70,76,51)
STAMP=('revision','queue_written','queue_read','transport_epoch','remote_generation')
PAGES={
 0:('cc_map','query_abi','feature_bits','source_count','firmware_identity'),
 1:('pattern','part','lane','context_ready','armed','staged_request','seed_mask'),
 2:('mode','chord_block','length','note_step','seed32'),
 3:('density','positions','anchors','seed_index','effective_seed','length','seed_valid'),
 5:('capture_id','owner_lane','owner_pattern','state','length','rate','count','complete','note_data_valid','gate_data_valid','context_matches','destination','destination_valid','operation','expected_capture_id'),
 6:('capture_id','step_zero_based','note','gate','accent','slide','flags','valid'),
 8:('every','amount','anchor_lock','seed','variation_mode','shape','range','constraint_mode','chord_block','seed_valid','off_request'),
 10:('source_pattern','destination_pattern','source_scope','destination_scope','recipe_valid_mask','preview_valid','current_fingerprint','referenced_bars','source_motion_count','destination_motion_count','same_target'),
 9:('lane','pattern','every','amount','seed','anchor_lock','variation_mode','shape','range','notes_mask','low_note','high_note','context_matches'),
 7:('cc_mode','recipe_kind','accepted','rejected','unsupported','busy','coalesced','overflow'),
 4:('receipt_valid','result_request','staged_request','state','reason','before_revision','after_revision','before_fingerprint','recipe_changed')}
STATES=('idle','accepted','pending','applied','no_change','rejected','unsupported','cancelled')
REASONS=('ok','invalid_context','stale_context','stopped_required','queue_busy','protected','owner_busy','invalid_value','missing_notes','unsupported_note_or_auto_mutate','undo_capacity','cancelled','capture_incomplete','unsupported_capture_direction_or_layout','same_or_missing_destination','copy_motion_or_undo_capacity')
class GenerationReadError(RuntimeError):pass

def request(source,page,token,index=0):
    if any(type(v) is not int for v in (source,page,token,index)) or not 0<=source<3 or page not in PAGES or not 0<=token<128 or (not 0<=index<64 if page==6 else index!=0):
        raise ValueError('source0..2, page0..10, token0..127;index0 except captured-step page6 index0..63 required')
    return list(HEADER)+( [1,token,source,page,index])

def number(values):return sum(v<<(7*i) for i,v in enumerate(values))

def decode(data,source,page,token,index=0):
    request(source,page,token,index)
    d=tuple(data)
    if d[:4]!=HEADER:return None
    if len(d)<15 or any(type(v) is not int or not 0<=v<128 for v in d):raise GenerationReadError('Malformed frame')
    if d[4]!=1:raise GenerationReadError('Unsupported ABI')
    if d[5:9]!=(token,source,page,index):return None
    crc=number(d[-5:])
    if crc>0xffffffff or zlib.crc32(bytes(d[:-5]))!=crc:raise GenerationReadError('CRC mismatch')
    if d[9]:
        if len(d)!=15 or d[9] not in (1,3):raise GenerationReadError('Malformed refusal')
        raise GenerationReadError('Invalid request' if d[9]==1 else 'Unsupported page/version or unavailable state')
    names=STAMP+PAGES[page]
    if len(d)!=16+5*len(names) or d[10]!=len(names):raise GenerationReadError('Unexpected field count/length')
    vals=[number(d[11+5*i:16+5*i]) for i in range(len(names))]
    if any(v>0xffffffff for v in vals):raise GenerationReadError('Oversize integer')
    result=dict(zip(names,vals))
    result.update(schema=1,command=51,abi=1,source=source,page=page,read_only=True)
    if page==0 and (result['query_abi']!=1 or result['source_count']!=3):raise GenerationReadError('Unsupported capability ABI')
    if page==1 and (result['pattern']>=32 or result['part']>=5 or result['lane']>=25 or result['seed_mask']>15):raise GenerationReadError('Invalid context')
    if page==2 and (result['mode']>4 or result['chord_block']>=192 or result['length']>64 or result['note_step']>64):raise GenerationReadError('Invalid recipe')
    if page==3 and (result['density']>100 or result['positions']>3 or result['anchors']>5 or result['seed_index']>255 or result['length']>64 or result['seed_valid']>1 or result['effective_seed']!=(result['seed_index']+1 if result['seed_valid'] else 0)):raise GenerationReadError('Invalid drum recipe')
    if page==7 and (result['cc_mode']>2 or result['recipe_kind']>4):raise GenerationReadError('Invalid diagnostic mode')
    if page in (8,9) and (result['every']>2 or result['amount']>8 or result['anchor_lock']>3 or result['seed']>255 or result['variation_mode']>2 or result['shape']>1 or result['range']>3):raise GenerationReadError('Invalid variation domain')
    if page==8 and (result['constraint_mode']>3 or result['chord_block']>191 or result['seed_valid']>1 or result['off_request']>1):raise GenerationReadError('Invalid staged variation')
    if page==9 and (result['lane'] not in (*range(24),255) or result['pattern']>31 or result['notes_mask']>4095 or result['low_note']>127 or result['high_note']>127 or result['context_matches']>1):raise GenerationReadError('Invalid live variation')
    if page==5:
        if result['owner_lane'] not in (*range(24),255) or result['owner_pattern']>31 or result['state']>2 or result['length']>64 or result['count']>64 or result['rate']>20 or result['destination']>31 or result['operation'] not in (0,1,255):raise GenerationReadError('Invalid capture header')
        if any(result[k]>1 for k in ('complete','note_data_valid','gate_data_valid','context_matches','destination_valid')):raise GenerationReadError('Invalid capture validity')
        if bool(result['capture_id'])!=bool(result['complete']) or (result['complete'] and (result['state']!=2 or not result['length'] or result['count']!=result['length'] or not result['gate_data_valid'])):raise GenerationReadError('Incomplete capture advertised')
    if page==6 and (not result['capture_id'] or result['step_zero_based']!=index or result['note']>127 or result['gate']>2 or result['accent']>1 or result['slide']>1 or result['flags']>15 or result['valid']!=1 or (result['flags']&3)!=result['gate'] or bool(result['flags']&4)!=bool(result['accent']) or bool(result['flags']&8)!=bool(result['slide'])):raise GenerationReadError('Invalid captured step')
    if page==10 and (result['source_pattern']>31 or result['destination_pattern']>31 or result['source_scope']>24 or result['destination_scope']>24 or result['recipe_valid_mask']>15 or result['preview_valid']>1 or result['referenced_bars']>192 or result['source_motion_count']>160 or result['destination_motion_count']>160 or result['same_target']>1):raise GenerationReadError('Invalid scoped-copy recipe')
    if page==4:
        if result['state']>=len(STATES) or result['reason']>=len(REASONS) or result['result_request']>=16384 or result['staged_request']>=16384:raise GenerationReadError('Invalid result')
        result['state_name']=STATES[result['state']];result['reason_name']=REASONS[result['reason']]
        result['applied']=bool(result['receipt_valid'] and result['state'] in (3,4))
        result['scope']='Last source result only; confirm matching request and independent command49 state. No physical output proof.'
    return result

def read_snapshot(ip,op,source,page,*,message,timeout=1.5,index=0):
    token=secrets.randbelow(128);op.send(message('sysex',data=request(source,page,token,index)));end=time.monotonic()+timeout
    while time.monotonic()<end:
        pkt=ip.poll()
        if pkt is not None and pkt.type=='sysex':
            answer=decode(pkt.data,source,page,token,index)
            if answer is not None:return answer
        time.sleep(.002)
    raise GenerationReadError('No command51 reply; no fallback/control/save was sent')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=int,required=True);p.add_argument('--page',type=int,required=True);p.add_argument('--index',type=int,default=0)
    p.add_argument('--port');p.add_argument('--output-port');p.add_argument('--output',type=Path);a=p.parse_args()
    try:
        request(a.source,a.page,0,a.index)
        if a.output and a.output.exists():raise ValueError('Output already exists')
        import mido
        from fm1_strip_read import choose_ports
        inp,out=choose_ports(mido.get_input_names(),mido.get_output_names(),a.port,a.output_port)
        with mido.open_input(inp) as ip,mido.open_output(out) as op:result=read_snapshot(ip,op,a.source,a.page,message=mido.Message,index=a.index)
        text=json.dumps(result,indent=2)+'\n'
        if a.output:
            with a.output.open('x',encoding='utf-8') as f:f.write(text)
        print(text,end='')
    except (ValueError,OSError,ImportError,GenerationReadError) as e:p.exit(1,'Generation readback failed: '+str(e)+'\n')
if __name__=='__main__':main()
