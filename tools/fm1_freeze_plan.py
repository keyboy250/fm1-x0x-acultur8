#!/usr/bin/env python3
"""Print explicit Freeze/Commit MIDI plans;never opens ports or starts playback."""
import argparse,json
from fm1_generation_plan import coarse,exact

def plan(pattern,lane,request_id,*,commit=False,destination=None,capture_id=None):
    if any(type(x) is not int for x in (pattern,lane,request_id)) or not 0<=pattern<32 or not 0<=lane<24 or not 0<=request_id<16384 or type(commit) is not bool:raise ValueError('Invalid explicit context/request')
    if commit and (type(destination) is not int or not 0<=destination<32 or destination==pattern or type(capture_id) is not int or not 0<capture_id<=0xffffffff):raise ValueError('Commit requires different destination and captured-pass token')
    if not commit and (destination is not None or capture_id is not None):raise ValueError('Freeze takes no destination/capture token')
    part=lane//11 if lane<22 else lane-20
    context=[(1010,pattern,31),(1011,part,4)]
    context += [(1068,lane//11,1),(1012,lane%11,10)] if lane<22 else [(1013,lane-22,1)]
    context += [(1069,lane,24),(1071,1,1)]
    events=[coarse(n,v,m) for n,v,m in context]
    if commit:
        events+=exact(1220,destination)
        for byte in range(4):events+=exact(1237+byte,(capture_id>>(8*byte))&255)
    execute=1222 if commit else 1221
    events += [coarse(1231,request_id&127,127),coarse(1232,request_id>>7,127),coarse(1233,7 if commit else 6,7),coarse(execute,0,1),coarse(execute,1,1)]
    return dict(schema=1,mode='plan_only',feature='freeze_commit',required_capability=16,pattern_zero_based=pattern,lane=lane,request_id=request_id,operation='commit' if commit else 'freeze',destination_zero_based=destination,expected_capture_id=capture_id,events=events,preconditions=['CC MODE FULL','matching armed live variation and playback pattern','no competing capture owner','Commit:stopped/drained,complete HELD pass,explicit matching capture token'],verify='Command51 page4 matching request;Freeze PENDING until full pass becomes APPLIED. Capture scan pages5/6 must be complete/coherent;Commit must change only approved destination lane,with Undo/Redo and command49 actual musical readback. No save/start/installation is sent.')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pattern',type=int,required=True);p.add_argument('--lane',type=int,required=True);p.add_argument('--request-id',type=int,required=True)
    p.add_argument('--commit',action='store_true');p.add_argument('--destination',type=int);p.add_argument('--capture-id',type=lambda s:int(s,0));print(json.dumps(plan(**vars(p.parse_args())),indent=2))
if __name__=='__main__':main()
