#!/usr/bin/env python3
"""Build explicit variation MIDI plans;never opens ports or sends transport."""
import argparse,json
from fm1_generation_plan import coarse,exact
def plan(pattern,lane,every,amount,anchor_lock,seed,mode,shape,region,constraint,chord_block,request_id,off=False):
    values=(pattern,lane,every,amount,anchor_lock,seed,mode,shape,region,constraint,chord_block,request_id)
    maxima=(31,23,2,8,3,255,2,1,3,3,191,16383)
    if any(type(v) is not int or not 0<=v<=m for v,m in zip(values,maxima)) or type(off) is not bool:raise ValueError('Invalid variation context,recipe or request')
    part=lane//11 if lane<22 else lane-20
    context=[(1010,pattern,31),(1011,part,4)]
    context += [(1068,lane//11,1),(1012,lane%11,10)] if lane<22 else [(1013,lane-22,1)]
    context += [(1069,lane,24),(1071,1,1)]
    events=[coarse(n,v,m) for n,v,m in context]
    for n,v in zip(range(1211,1218),(every,amount,anchor_lock,seed,mode,shape,region)):events+=exact(n,v)
    events+=exact(1235,constraint)+exact(1236,chord_block)
    execute=1219 if off else 1218
    events +=[coarse(1231,request_id&127,127),coarse(1232,request_id>>7,127),coarse(1233,5 if off else 4,7),coarse(execute,0,1),coarse(execute,1,1)]
    return dict(schema=1,mode='plan_only',feature='repeating_variation',required_capability=8,pattern_zero_based=pattern,lane=lane,passes=2<<every,request_id=request_id,off=off,events=events,preconditions=['CC MODE FULL','stopped/drained and unowned runtime','explicit approved fixture','monotonically increasing request ID'],verify='Command51 page8 staging,page4 matching action receipt,page9 installed runtime;command50 accepted prefix and actual MIDI/audio playback evidence. Project/Undo/save bytes must remain unchanged.')
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key,default in [('pattern',1),('lane',0),('every',0),('amount',3),('anchor_lock',3),('seed',42),('mode',0),('shape',0),('region',0),('constraint',0),('chord_block',0),('request_id',1)]:p.add_argument('--'+key.replace('_','-'),type=int,default=default)
    p.add_argument('--off',action='store_true');print(json.dumps(plan(**vars(p.parse_args())),indent=2))
if __name__=='__main__':main()
