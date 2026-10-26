"""Production command51 frames and pure public plans;never opens MIDI ports."""
from pathlib import Path
import sys,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from fm1_generation_read import decode,request,GenerationReadError
from fm1_copy_plan import plan,group
checks=0
for src in range(3):
    d=list(Path(f'build/midi-303/query-{src}-10.bin').read_bytes()[1:-1])
    v=decode(d,src,10,77)
    assert [v[k] for k in ('source_pattern','destination_pattern','source_scope','destination_scope','recipe_valid_mask','preview_valid','same_target')]==[1,2,2,4,15,1,0];checks+=1
    assert v['source_motion_count']==v['destination_motion_count']==1;checks+=1
    assert decode(d,src,10,76) is None;checks+=1
    for bad in (d[:-1],d[:-1]+[d[-1]^1]):
        try:decode(bad,src,10,77)
        except GenerationReadError:pass
        else:raise AssertionError('Damaged frame accepted')
        checks+=1
    for offset,value in ((0,32),(1,32),(2,25),(3,25),(4,16),(5,2),(7,193),(8,161),(9,161),(10,2)):
        bad=d.copy();at=11+5*(5+offset);bad[at:at+5]=[(value>>(7*i))&127 for i in range(5)];crc=zlib.crc32(bytes(bad[:-5]));bad[-5:]=[(crc>>(7*i))&127 for i in range(5)]
        try:decode(bad,src,10,77)
        except GenerationReadError:pass
        else:raise AssertionError('Invalid copy domain accepted')
        checks+=1
for source_scope in range(25):
    for destination_scope in range(25):
        try:v=plan(1,2,source_scope,destination_scope,1)
        except ValueError:assert group(source_scope)!=group(destination_scope)
        else:
            assert group(source_scope)==group(destination_scope) and len(v['events'])==9 and v['preview_event_count']==7
            for n,event,canonical,maximum in zip((1241,1242,1243,1244),v['events'],(1,2,source_scope,destination_scope),(31,31,24,24)):
                ch,cc=n//88,n%88;assert event[:2]==[176+ch,cc if cc<32 else cc+32];assert (event[2]*maximum+63)//127==canonical
        checks+=1
for bad in ((-1,2,1,1,1),(1,32,1,1,1),(1,2,25,1,1),(1,2,1,1,16384),(True,2,1,1,1)):
    try:plan(*bad)
    except ValueError:pass
    else:raise AssertionError('Invalid plan accepted')
    checks+=1
print(f'COPY READBACK PASS: {checks} frame/CRC/domain/compatibility/CC roundtrip checks;3sources;native capability/receipt readback in SIMPLE/OFF;no ports.')
