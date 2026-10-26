"""Actual native command51 capture frames and bounded coherent scan;no MIDI ports."""
from pathlib import Path
import sys,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from fm1_generation_read import decode,request,GenerationReadError
from fm1_capture_read import scan
checks=0
def frame(source,page,index=0):
    data=(Path('build/midi-freeze')/f'query-{source}-{page}-{index}.bin').read_bytes()
    assert data[0]==240 and data[-1]==247
    return list(data[1:-1])
for src in range(3):
    header=decode(frame(src,5),src,5,77);assert header['complete']==1 and header['owner_lane']==23 and header['length']==64 and header['note_data_valid']==1;checks+=1
    for step in range(64):
        data=frame(src,6,step);row=decode(data,src,6,77,step);assert row['capture_id']==header['capture_id'] and row['step_zero_based']==step;checks+=1
        assert decode(data,src,6,76,step) is None;checks+=1
        for bad in [data[:-1],data[:-1]+[data[-1]^1]]:
            try:decode(bad,src,6,77,step)
            except GenerationReadError:pass
            else:raise AssertionError('Damaged capture frame accepted')
            checks+=1
    reader=lambda page,index:decode(frame(src,page,index),src,page,77,index)
    out=scan(reader);assert out['coherent'] and len(out['steps'])==64;checks+=1
    calls=[]
    def torn(page,index):
        calls.append((page,index));v=reader(page,index)
        if page==6:v['capture_id']^=1
        return v
    try:scan(torn)
    except GenerationReadError:pass
    else:raise AssertionError('Torn scan accepted')
    assert len(calls)==6;checks+=1
    fault=[True]
    def recover(page,index):
        v=reader(page,index)
        if page==6 and fault[0]:fault[0]=False;v['capture_id']^=1
        return v
    assert scan(recover)['coherent'];checks+=1
for index in [-1,64,128]:
    try:request(0,6,1,index)
    except ValueError:pass
    else:raise AssertionError('Invalid capture index accepted')
    checks+=1
# Recompute CRC after semantic damage:decoder must validate data as well as framing.
for offset,value in [(2,128),(3,3),(6,255),(7,0)]:
    d=frame(0,6);at=11+5*(5+offset);d[at:at+5]=[(value>>(7*i))&127 for i in range(5)];crc=zlib.crc32(bytes(d[:-5]));d[-5:]=[(crc>>(7*i))&127 for i in range(5)]
    try:decode(d,0,6,77,0)
    except GenerationReadError:pass
    else:raise AssertionError('Invalid captured step domain accepted')
    checks+=1
for path in Path('build/midi-freeze').glob('*-unavailable.bin'):
    _,src,page,index,*_=path.stem.split('-');d=list(path.read_bytes()[1:-1])
    try:decode(d,int(src),int(page),77,int(index))
    except GenerationReadError:pass
    else:raise AssertionError('Unavailable capture accepted')
    checks+=1
print(f'FREEZE READBACK PASS: {checks} actual frame/CRC/token/domain/refusal checks;64steps/3sources,coherent scans,bounded torn retries;no ports.')
