"""Decode actual command51 variation/live frames;never touches a device."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from fm1_generation_read import decode,request,GenerationReadError
checks=0
for source in range(3):
 for page in (8,9):
    frame=(Path('build/midi-303')/f'query-{source}-{page}.bin').read_bytes();assert frame[0]==240 and frame[-1]==247
    data=list(frame[1:-1]);result=decode(data,source,page,77);assert result and result['read_only'];assert decode(data,source,page,76) is None
    for bad in (data[:-1],data[:-1]+[data[-1]^1]):
        try:decode(bad,source,page,77)
        except GenerationReadError:pass
        else:raise AssertionError('Damaged frame accepted')
    checks+=4
    if page==8:assert result['every']==2 and result['seed']==255 and result['amount']==8 and result['seed_valid']==1
    else:assert result['lane']==23 and result['context_matches']==1 and result['notes_mask']!=0
for page in (11,12,13):
 try:request(0,page,1)
 except ValueError:pass
 else:raise AssertionError('Unsupported page accepted')
 checks+=1
print(f'VARIATION READBACK PASS: {checks} real frame/CRC/token/refusal checks;staged/live domains and actual captured mask;no ports.')
