# SPDX-License-Identifier: GPL-3.0-only
"""Use real native command51 replies, then reject wire damage and mismatches."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from fm1_generation_read import decode,request,GenerationReadError

def main():
    checks=0
    for source in range(3):
        for page in (0,1,2,4):
            frame=(Path('build/midi-303')/f'query-{source}-{page}.bin').read_bytes()
            assert frame[0]==240 and frame[-1]==247
            data=list(frame[1:-1]);result=decode(data,source,page,77)
            assert result and result['read_only'] and result['source']==source
            assert decode(data,source,page,76) is None
            changed=list(data);changed[-1]^=1
            try:decode(changed,source,page,77)
            except GenerationReadError:pass
            else:raise AssertionError('CRC damage accepted')
            try:decode(data[:-1],source,page,77)
            except GenerationReadError:pass
            else:raise AssertionError('Truncation accepted')
            checks+=4
    assert decode(list(frame[1:-1]),2,4,77)['state_name']=='applied'
    for args in ((3,0,0),(0,11,0),(0,0,128),(True,0,0)):
        try:request(*args)
        except ValueError:pass
        else:raise AssertionError('Invalid request accepted')
        checks+=1
    print(f'GENERATION READBACK PASS: {checks} real wire/CRC/context/ABI validation checks; no MIDI ports opened')
if __name__=='__main__':main()
