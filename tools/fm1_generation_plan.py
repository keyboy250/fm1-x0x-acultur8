#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build a deterministic303 MIDI event plan. No ports are opened or messages sent."""
import argparse
import json

def coarse(number,value,maximum):
    raw=next((i for i in range(128) if (i*maximum+63)//127==value),None)
    if raw is None:raise ValueError('Value requires exact transaction')
    ch,offset=divmod(number,88)
    return [176+ch,offset if offset<32 else offset+32,raw]

def exact(number,value):
    if not 0<=value<1<<21:raise ValueError('Exact transaction range exceeded')
    return [coarse(1072,number&127,127),coarse(1073,number>>7,15),
            coarse(1074,value&127,127),coarse(1075,(value>>7)&127,127),
            coarse(1130,(value>>14)&127,127),coarse(1077,1,1)]

def plan(pattern,bass,mode,block,length,step,seed,request_id):
    args=(pattern,bass,mode,block,length,step,seed,request_id)
    if any(type(v) is not int for v in args) or not (0<=pattern<32 and 0<=bass<2 and 0<=mode<5 and 0<=block<192 and 0<=length<=64 and 0<=step<=64 and 0<seed<=0xffffffff and 0<=request_id<16384):raise ValueError('Invalid generation target/recipe/request')
    events=[]
    for n,v,m in ((1010,pattern,31),(1011,2+bass,4),(1013,bass,1),(1069,22+bass,24),(1071,1,1)):
        events.append(coarse(n,v,m))
    for n,v in ((1203,mode),(1204,block),(1223,length),(1224,step)):
        events+=exact(n,v)
    for i in range(4):events+=exact(1227+i,(seed>>(8*i))&255)
    events += [coarse(1231,request_id&127,127),coarse(1232,request_id>>7,127),coarse(1233,2,7),coarse(1205,0,1),coarse(1205,1,1)]
    return dict(schema=1,mode='plan_only',feature='303_generation',required_capability=1,
        pattern_zero_based=pattern,bass=bass,request_id=request_id,seed32=seed,
        preconditions=['matching local candidate/capability','stopped/drained transport','disposable fixture or explicitly authorized project','new monotonically increasing request ID'],
        events=events,verify='Read command51 source result matching request; verify command49 musical changes independently. Never blindly replay timed-out mutations.')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key,default in (('pattern',0),('bass',0),('mode',4),('block',0),('length',32),('step',0),('seed',305419896),('request_id',1)):
        p.add_argument('--'+key.replace('_','-'),type=int,default=default)
    print(json.dumps(plan(**vars(p.parse_args())),indent=2))
if __name__=='__main__':main()
