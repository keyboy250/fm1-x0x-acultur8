"""Decode real drum/diagnostic editor frames and refuse damage; no device."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from fm1_generation_read import decode,request,GenerationReadError
checks=0
for source in range(3):
    for page in (3,7):
        frame=(Path('build/midi-303')/f'query-{source}-{page}.bin').read_bytes()
        data=list(frame[1:-1]);r=decode(data,source,page,77)
        assert r and r['read_only']
        assert decode(data,source,page,76) is None
        for broken in (data[:-1],data[:-1]+[data[-1]^1]):
            try:decode(broken,source,page,77)
            except GenerationReadError:pass
            else:raise AssertionError('Damaged frame accepted')
        checks+=4
        if page==3:assert r['seed_index']==255 and r['effective_seed']==256 and r['seed_valid']==1
for page in (0,1,2,3,4,7):assert request(0,page,1)
for page in (11,12,13):
    try:request(0,page,1)
    except ValueError:pass
    else:raise AssertionError('Unsupported page accepted')
    checks+=1
print(f'DRUM READBACK PASS: {checks} real frame/CRC/context/refusal checks, recipe seed255=>256, diagnostic pages; no ports.')
