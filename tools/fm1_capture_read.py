#!/usr/bin/env python3
"""Read a bounded coherent HELD capture;no controls,transport,save or fallback."""
import argparse,json
from pathlib import Path
from fm1_generation_read import read_snapshot,GenerationReadError,STAMP

def scan(reader,attempts=3):
    if type(attempts) is not int or not 1<=attempts<=3:raise ValueError('At most3 bounded attempts')
    last='No complete capture'
    for _ in range(attempts):
        try:
            first=reader(5,0)
            if not first['complete'] or not first['gate_data_valid'] or not first['capture_id']:raise GenerationReadError('Capture not complete/HELD')
            if first['owner_lane']>=22 and not first['note_data_valid']:raise GenerationReadError('Captured303 notes unavailable')
            steps=[]
            for i in range(first['length']):
                row=reader(6,i)
                if row['capture_id']!=first['capture_id'] or any(row[k]!=first[k] for k in STAMP) or row['step_zero_based']!=i or row['valid']!=1:raise GenerationReadError('Capture changed during scan')
                steps.append(row)
            last_header=reader(5,0)
            if last_header!=first:raise GenerationReadError('Capture header changed during scan')
            return dict(schema=1,read_only=True,coherent=True,header=first,steps=steps,scope='Captured accepted gates/final303 pitches;not physical MIDI/audio delivery proof. Capture ID is a content fingerprint,not a monotonic session ID.')
        except GenerationReadError as e:last=str(e)
    raise GenerationReadError('Bounded capture scan failed: '+last)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=int,required=True);p.add_argument('--port');p.add_argument('--output-port');p.add_argument('--output',type=Path);a=p.parse_args()
    try:
        if not 0<=a.source<3:raise ValueError('Source0..2 required')
        if a.output and a.output.exists():raise ValueError('Output already exists')
        import mido
        from fm1_strip_read import choose_ports
        inp,out=choose_ports(mido.get_input_names(),mido.get_output_names(),a.port,a.output_port)
        with mido.open_input(inp) as ip,mido.open_output(out) as op:
            result=scan(lambda page,index:read_snapshot(ip,op,a.source,page,message=mido.Message,index=index))
        text=json.dumps(result,indent=2)+'\n'
        if a.output:
            with a.output.open('x',encoding='utf-8') as f:f.write(text)
        print(text,end='')
    except (ValueError,OSError,ImportError,GenerationReadError) as e:p.exit(1,'Capture readback failed: '+str(e)+'\n')
if __name__=='__main__':main()
