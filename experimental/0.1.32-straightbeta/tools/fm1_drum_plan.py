#!/usr/bin/env python3
"""Plan deterministic drum-generation wire CCs; opens no MIDI port."""
import argparse,json
from fm1_generation_plan import coarse,exact
def plan(pattern,kit,voice,density,positions,anchors,seed_index,length,request_id):
    values=(pattern,kit,voice,density,positions,anchors,seed_index,length,request_id)
    maxima=(31,1,10,100,3,5,255,64,16383)
    if any(type(v) is not int or not 0<=v<=m for v,m in zip(values,maxima)):raise ValueError('Invalid drum target/recipe/request')
    events=[coarse(n,v,m) for n,v,m in ((1010,pattern,31),(1011,kit,4),(1068,kit,1),(1012,voice,10),(1069,kit*11+voice,24),(1071,1,1))]
    for n,v in ((1206,density),(1207,positions),(1208,anchors),(1209,seed_index),(1223,length)):events+=exact(n,v)
    events +=[coarse(1231,request_id&127,127),coarse(1232,request_id>>7,127),coarse(1233,3,7),coarse(1210,0,1),coarse(1210,1,1)]
    return dict(schema=1,mode='plan_only',feature='drum_generation',required_capability=2,pattern_zero_based=pattern,kit=kit,voice=voice,request_id=request_id,seed_index=seed_index,effective_seed=seed_index+1,events=events,preconditions=['CC MODE FULL','stopped/drained transport','explicit safe fixture target','new monotonically increasing14-bit request ID'],verify='Command51 pages3/4 matching request, command49 actual rhythm/length and unrelated-state preservation; never replay blindly after timeout.')
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name,default in [('pattern',1),('kit',0),('voice',0),('density',50),('positions',0),('anchors',1),('seed_index',255),('length',0),('request_id',1)]:parser.add_argument('--'+name.replace('_','-'),type=int,default=default)
    print(json.dumps(plan(**vars(parser.parse_args())),indent=2))
if __name__=='__main__':main()
