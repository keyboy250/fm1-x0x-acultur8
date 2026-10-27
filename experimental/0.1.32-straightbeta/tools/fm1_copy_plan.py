#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Print scoped/cross-part copy wire plan;no ports,transport or storage actions."""
import argparse,json
from fm1_generation_plan import coarse
def group(scope):return 0 if scope==0 else 1 if scope<23 else 2
def plan(source,destination,source_scope,destination_scope,request_id):
    if any(type(v) is not int for v in (source,destination,source_scope,destination_scope,request_id)) or not (0<=source<32 and 0<=destination<32 and 0<=source_scope<25 and 0<=destination_scope<25 and 0<=request_id<16384) or group(source_scope)!=group(destination_scope):raise ValueError('Explicit compatible slots/scopes and14-bit request required')
    events=[coarse(n,v,m) for n,v,m in [(1241,source,31),(1242,destination,31),(1243,source_scope,24),(1244,destination_scope,24),(1231,request_id&127,127),(1232,request_id>>7,127),(1245,1,1),(1246,0,1),(1246,1,1)]]
    return dict(schema=1,mode='plan_only',feature='scoped_cross_part_copy',required_capability=32,source_pattern_zero_based=source,destination_pattern_zero_based=destination,source_scope=source_scope,destination_scope=destination_scope,request_id=request_id,events=events,preview_event_count=7,preconditions=['CC MODE FULL','stopped/drained,writable and unowned project','explicit approved destination','fresh monotonic request ID'],verify='First send preview prefix,read command51 page10 with preview_valid=1 and page4 ACCEPTED matching request. Then execute suffix and require matching APPLIED/NO_CHANGE;independently query command49 both slots and motion,Undo/Redo/save recall. No automatic save/start/installation.')
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('source','destination','source_scope','destination_scope','request_id'):p.add_argument('--'+name.replace('_','-'),type=int,required=True)
    print(json.dumps(plan(**vars(p.parse_args())),indent=2))
if __name__=='__main__':main()
