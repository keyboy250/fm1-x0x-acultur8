"""Generate pure wire plans for the independent local fixtures;never sends MIDI."""
from pathlib import Path
import json
from fm1_freeze_plan import plan as freeze
from fm1_variation_plan import plan as variation
out=Path('build/midi-freeze')
for lane in range(24):
    captured=json.loads((out/f'capture-{lane}.json').read_text())
    plans=[('variation',variation(1,lane,0,3,0,42,0,0,0,0,0,1)),('freeze',freeze(1,lane,2)),('commit',freeze(1,lane,3,commit=True,destination=6,capture_id=captured['capture_id']))]
    for name,value in plans:(out/f'{name}-{lane}.json').write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
print('FREEZE FIXTURE PLANS PASS:72 explicit public plans;no ports.')
