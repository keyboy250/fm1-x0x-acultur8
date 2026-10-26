#!/usr/bin/env python3
"""Produce offline public wire plans for every compatible scope route."""
from pathlib import Path
import json
from fm1_copy_plan import plan,group
count=0
for source_scope in range(25):
    for destination_scope in range(25):
        if group(source_scope)!=group(destination_scope):continue
        data=plan(1,2,source_scope,destination_scope,1)
        Path(f'build/midi-copy/plan-{source_scope}-{destination_scope}.json').write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
        count+=1
assert count==489
print('COPY PLANS PASS:489 compatible routes;no ports')
