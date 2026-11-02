#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only kick snapshots and named JSON preset export. No notes,CC,save ortransport sends."""
import argparse,hashlib,json,struct
from pathlib import Path
from fm1_generation_read import read_snapshot,STAMP,GenerationReadError
MAP_ID=0x4b503031
HYBRID_MAX=(1,4,100,100,127,127,1,127,127,127,127,127,127,127,127,100,40,1,100,127,200,127,100,127,1,100,127,127,127,1,100,127,127,127,60,127,80,100,100)
BASE_MAX=(127,127,127,127,127,127,6,127,127,127)
STRIP_KEYS=('highpass_hz','lowpass_hz','boost_db','ceiling_attenuation_db','release_ms','flags')
def validate(preset):
    if preset.get('schema')!='acultur8-kick-preset-1':raise ValueError('Unsupported preset format')
    for name,maxima in [('hybrid',HYBRID_MAX),('original_bd',BASE_MAX)]:
        values=preset.get(name)
        if not isinstance(values,list) or len(values)!=len(maxima) or any(type(v)is not int or not 0<=v<=m for v,m in zip(values,maxima)):raise ValueError('Invalid '+name)
    s=preset.get('strip',{})
    for key,lo,hi in zip(STRIP_KEYS,(20,20,0,0,10,0),(20000,20000,24,48,250,15)):
        if type(s.get(key))is not int or not lo<=s[key]<=hi:raise ValueError('Invalid strip '+key)
    crc=preset.get('sample_fingerprint32',0)
    if type(crc)is not int or not 0<=crc<=0xffffffff or (preset['hybrid'][1]==4 and not crc) or (preset['hybrid'][1]!=4 and crc):raise ValueError('Invalid custom sample reference')
    return preset

def raw_record(preset):
    validate(preset);s=preset['strip'];return bytes([1]+preset['hybrid'])+struct.pack('<I',preset['sample_fingerprint32'])+bytes(preset['original_bd']+[0]*6)+struct.pack('<HHBBBB',*(s[k] for k in STRIP_KEYS))
def from_record(raw,name):
    if len(raw)!=68 or raw[0]!=1 or any(raw[54:60]):raise ValueError('Invalid native kick record')
    return validate(dict(schema='acultur8-kick-preset-1',name=name,hybrid=list(raw[1:40]),sample_fingerprint32=struct.unpack_from('<I',raw,40)[0],original_bd=list(raw[44:54]),strip=dict(zip(STRIP_KEYS,struct.unpack_from('<HHBBBB',raw,60)))))
def from_capture(capture):
    s=capture['strip_capture'];preset=dict(schema='acultur8-kick-preset-1',name=capture['name'],hybrid=capture['raw_values'],sample_fingerprint32=0,original_bd=capture['original_bd_capture']['first16_bytes'][:10],strip={k:(-s['ceiling_dbfs'] if k=='ceiling_attenuation_db' else s[k]) for k in STRIP_KEYS})
    preset['capture_provenance']={k:capture[k] for k in ('device','capture_method','original_project_sha256')};return validate(preset)
def read_current(ip,op,source,message):
    frames=[]
    def read(page,index=0):
        answer=read_snapshot(ip,op,source,page,message=message,index=index);frames.append(answer);return answer
    meta=read(11)
    if meta['kick_map_identity']!=MAP_ID or meta['kick_control_count']!=39 or meta['preset_slots']!=16 or meta['kick_query_abi']!=1:raise GenerationReadError('Unsupported kick capability/map')
    desired=[];audio_next=[];current_hit=[]
    for page,out in [(12,desired),(13,audio_next),(19,current_hit)]:
        for index in range(3):
            frame=read(page,index);out.extend(frame['field_'+str(index*13+i)] for i in range(13))
    base=read(17);strip=read(17,1);end=read(11)
    stamps={tuple(frame[k] for k in STAMP) for frame in frames}
    if len(stamps)!=1 or any(frame['queue_written']!=frame['queue_read'] for frame in frames):raise GenerationReadError('State changed orqueue pending;no coherent snapshot')
    preset=validate(dict(schema='acultur8-kick-preset-1',name='Current kick',hybrid=desired,original_bd=[base['base_'+str(i)] for i in range(10)],strip={k:strip[k] for k in STRIP_KEYS},sample_fingerprint32=meta['current_sample_fingerprint32']))
    validate({**preset,'hybrid':audio_next})
    return dict(preset=preset,audio_next=audio_next,current_hit=current_hit,desired_audio_next_equal=desired==audio_next,capabilities=end,frames=frames)
def export_slot(ip,op,source,slot,name,message):
    meta=read_snapshot(ip,op,source,11,message=message)
    if meta['kick_map_identity']!=MAP_ID:raise GenerationReadError('Unsupported map')
    answer=read_snapshot(ip,op,source,18,message=message,index=slot)
    if answer['slot_status']!=0:raise GenerationReadError('Preset slot empty orunsupported')
    raw=b''.join(struct.pack('<I',answer['word_'+str(i)]) for i in range(17))
    preset=from_record(raw,name);preset['native_record_sha256']=hashlib.sha256(raw).hexdigest();return preset

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('mode',choices=('read','export-slot','convert-capture'));p.add_argument('--source',type=int,default=0);p.add_argument('--slot',type=int);p.add_argument('--name',default='Kick');p.add_argument('--capture',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--port');p.add_argument('--output-port');a=p.parse_args()
    if a.output.exists():p.error('Output exists;choose a new file')
    if a.mode=='convert-capture':result=from_capture(json.loads(a.capture.read_text(encoding='utf-8')))
    else:
        import mido
        from fm1_strip_read import choose_ports
        inp,out=choose_ports(mido.get_input_names(),mido.get_output_names(),a.port,a.output_port)
        with mido.open_input(inp)as ip,mido.open_output(out)as op:
            if a.mode=='read':result=read_current(ip,op,a.source,mido.Message)
            else:
                if a.slot is None or not 1<=a.slot<=16:p.error('slot must be1..16')
                result=export_slot(ip,op,a.source,a.slot-1,a.name,mido.Message)
    with a.output.open('x',encoding='utf-8')as f:json.dump(result,f,indent=2);f.write('\n')
    print('Saved '+str(a.output))
if __name__=='__main__':main()
