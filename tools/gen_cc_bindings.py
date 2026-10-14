# SPDX-License-Identifier: GPL-3.0-only
"""Generate compact native CC lookup and exhaustive implementation coverage ledger."""
import argparse,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
KINDS=['UNSUPPORTED','SOUND','STRIP','SEQ','GEN','BRKGEN','CHANNEL','PREF','SONG','ACTION','SELECTOR','STEP','MOTION','PROB','PERF','GENACTION','SEED','ARR','FOLLOW','PAD','EXACT','MASTER','TRANSPORT','LANE','HISTORY']

def binding(row):
    key=row['id'];parts=key.split('.');a=b=c=0;kind='UNSUPPORTED'
    if parts[0]=='sound':kind='SOUND';a=row['target']['engine'];b=row['target']['voice'];c=row['target']['index']
    elif parts[0]=='strip':kind='STRIP';a=int(parts[1]);c=['highpass','lowpass','boost','ceiling','release','flags','enabled','highpass_enabled','lowpass_enabled','limiter_enabled'].index(parts[2])
    elif key.startswith('pref.SQ_'):kind='SEQ';c=['SWING','DLEN','DRATE','BLEN','BRATE','BDIR','BTRANS','TEMPO','ACCENT','CLK','NOTES','THEME','KEYLED','KEYSOUND','AUTOSAVE'].index(parts[1][3:])
    elif parts[0]=='generator' and parts[1].isdigit():kind='GEN';c=int(parts[1])
    elif key.startswith('break.generator.'):kind='BRKGEN';c=int(parts[2])
    elif key.startswith('part.channel.'):kind='CHANNEL';a=int(parts[2])
    elif key.startswith('pref.'):
        names=['OUT_P','USBA_P','FREEPLAY_P','SREP_P','STALLS_P','BATT_P','break_slot_a','break_slot_b','song_edit_block','tape_macro']
        if parts[1] in names:kind='PREF';c=names.index(parts[1])
    elif parts[0]=='song':kind='SONG';c=int(parts[1])
    elif parts[0]=='action':kind='ACTION';c=int(parts[1])
    elif parts[0]=='selector':kind='SELECTOR';c=['pattern','part','drum','bass','step','block_low','block_high','motion_low','motion_high','kit','strip','seed_byte','commit','parameter_low','parameter_high'].index(parts[1])
    elif parts[0]=='step':kind='STEP';c=['drum_hit','drum_accent','bass_note','bass_gate','bass_accent','bass_slide','break_allowed','break_slice'].index(parts[1])
    elif parts[0]=='motion':kind='MOTION';c=['value','target','voice','param','pattern','part','record','hold','clear'].index(parts[1])
    elif parts[0]=='probability':kind='PROB'
    elif parts[0]=='performance':kind='PERF';c=['mute','solo','pattern_cue','chain_start','chain_end','song_enabled','song_record'].index(parts[1])
    elif key=='generator.seed_byte':kind='SEED'
    elif parts[0]=='generator':kind='GENACTION';c=['new','mutate'].index(parts[1])
    elif parts[0]=='arrangement':kind='ARR';c=['enabled','length','key','mode','parent','degree','duration','pattern','mute','queue'].index(parts[1])
    elif parts[0]=='follow':kind='FOLLOW';c=['mode','low','high'].index(parts[1])
    elif parts[0]=='pad':kind='PAD';c=['channel','root','low','high'].index(parts[1])
    elif parts[0]=='value':kind='EXACT';c=['exact_low7','exact_mid7','exact_high2','exact_commit','exact_high7'].index(parts[1])
    elif key=='output.master_knob':kind='MASTER'
    elif parts[0]=='transport':kind='TRANSPORT';c=['play','stop','record'].index(parts[1])
    elif parts[0]=='lane':kind='LANE';c=['length','rate','mode'].index(parts[1])
    elif key in ('editor.undo','editor.redo'):kind='HISTORY';c=int(key=='editor.redo')
    return kind,a,b,c

def generate(root=ROOT):
    data=json.loads((root/'docs/ACULTUR8_CC_INVENTORY.json').read_text(encoding='utf-8'));rows=[];coverage=[]
    for row in data['entries']:
        kind,a,b,c=binding(row);address=row['address']
        slot=(address['channel']-1)*88+(address['cc'] if address['cc']<32 else address['cc']-32) if address else None
        if slot is not None:
            # 5-bit kind + 5-bit target + 4-bit voice + 5-bit index in uint32.
            if a>31 or b>15 or c>31:raise ValueError('Packed binding exceeded')
            rows.append((slot,KINDS.index(kind)|(a<<5)|(b<<10)|(c<<14)))
        status='native candidate; focused acceptance required' if kind not in ('UNSUPPORTED','ACTION') else 'unimplemented'
        reason='runtime Q12 gain, source-owned ADC pickup; physical acceptance pending' if kind=='MASTER' else 'storage/destructive actions require separate preflight; no flash per CC' if kind=='ACTION' else 'browser/nonmusical callback absent' if kind=='UNSUPPORTED' else ''
        if kind=='HISTORY':reason='stopped shared project history; rejects owned/recording gestures; global proj.set settings remain outside journal'
        if slot is None:status='explicitly unmapped/read-only/nonmusical';reason='no writable direct CC address in reviewed registry'
        coverage.append({'id':row['id'],'parameter_number':slot,'binding':kind,'status':status,'reason':reason})
    rows.sort()
    # Exact regular strip grid; every allocated cell is proven against inventory.
    strips=[r for r in data['entries'] if r['id'].startswith('strip.')]
    first=min(r['parameter_number'] for r in strips);stride=12;voices=25;fields=10
    for voice in range(voices):
        for field in range(fields):
            expected=(first+voice*stride+field,KINDS.index('STRIP')|(voice<<5)|(field<<14))
            if expected not in rows:raise ValueError('Strip grid wire map changed')
    if len(strips)!=voices*fields:raise ValueError('Strip grid extent changed')
    rest=[r for r in rows if not first<=r[0]<first+voices*stride]
    if len(rows)-len(rest)!=len(strips):raise ValueError('Non-strip grid collision')
    runs=[];at=0
    while at<len(rest):
        start,word=rest[at];count=1;shift=255
        if at+1<len(rest) and rest[at+1][0]==start+1:
            delta=rest[at+1][1]-word
            if delta>=0 and (not delta or not delta&(delta-1)):
                shift=delta.bit_length()-1 if delta else 255
                while at+count<len(rest) and count<255 and rest[at+count]==(start+count,word+(count*delta)):
                    count+=1
        runs.append((start,count,shift,word));at+=count
    # No unchecked runtime expansion: every occupied number and every hole has
    # equivalent semantics before emitting any production artifact.
    def decode(number):
        if first<=number<first+voices*stride:
            voice,field=divmod(number-first,stride)
            return KINDS.index('STRIP')|(voice<<5)|(field<<14) if field<fields else None
        for start,count,shift,word in runs:
            if start<=number<start+count:return word+(0 if shift==255 else (number-start)<<shift)
        return None
    oracle=dict(rows)
    if any(decode(n)!=oracle.get(n) for n in range(65536)):raise ValueError('Generated decoder wire/holes mismatch')
    header=['/* SPDX-License-Identifier: GPL-3.0-only */','/* Generated by tools/gen_cc_bindings.py; no ROM strings. */','#pragma once']
    header.append('enum {'+','.join('CCK_'+k+'='+str(i) for i,k in enumerate(KINDS))+'};')
    header.append('typedef struct {uint16_t first;uint8_t count,shift;uint32_t word;} cc_binding_run_t;')
    header.append('typedef char cc_binding_run_size_8[(sizeof(cc_binding_run_t)==8)?1:-1];')
    header.append('static const cc_binding_run_t cc_binding_runs[]={'+','.join('{'+','.join(str(v) for v in r)+'}' for r in runs)+'};')
    header.append('#define CC_RUN_COUNT '+str(len(runs)))
    header.append('#define CC_STRIP_FIRST '+str(first)+'u')
    header.append('#define CC_STRIP_LIMIT '+str(first+voices*stride)+'u')
    header.append('#define CC_STRIP_STRIDE '+str(stride)+'u')
    header.append('#define CC_STRIP_FIELDS '+str(fields)+'u')
    # Test-only traversal interface. Production firmware retains only ranges.
    header.append('#ifdef CC_REFERENCE_TABLES')
    header.append('static const uint16_t cc_numbers[]={'+','.join(str(x[0]) for x in rows)+'};')
    header.append('static const uint32_t cc_binding_words[]={'+','.join(str(x[1]) for x in rows)+'};')
    header.append('#endif')
    header.append('static const uint16_t cc_cutoff_hz[]={'+','.join(str(round(20*1000**(v/127))) for v in range(128))+'};')
    header.append('#define CC_BINDINGS_COUNT '+str(len(rows)))
    return '\n'.join(header)+'\n',json.dumps({'registry_map':data['map_version'],'scope':'native implementation candidate; no browser/target/device acceptance','rows':coverage},indent=2)+'\n'

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args();h,j=generate()
    for path,text in [('firmware/src/app/cc_bindings.h',h),('docs/ACULTUR8_CC_RUNTIME_COVERAGE.json',j)]:
        p=ROOT/path
        if args.check:
            if not p.exists() or p.read_text(encoding='utf-8')!=text:raise SystemExit('CC binding drift: '+path)
        else:p.write_text(text,encoding='utf-8',newline='\n')
