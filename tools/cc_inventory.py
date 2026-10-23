# SPDX-License-Identifier: GPL-3.0-only
"""Generate the source-derived CC design inventory; does not compile or edit runtime.

The deliberately narrow C reader fails closed on unknown descriptor syntax.
Run --write to refresh design artifacts, or --check to reject source/artifact drift.
"""
import argparse
import ast
import hashlib
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JSON_PATH = Path('docs/ACULTUR8_CC_INVENTORY.json')
MD_PATH = Path('docs/ACULTUR8_CC_INVENTORY.md')
FILES = ['firmware/src/app/engine.c', 'firmware/src/app/engine.h', 'firmware/src/app/ui.c',
         'firmware/src/app/x0x.h', 'firmware/src/app/block_bridge.inc', 'firmware/src/app/variant.c', 'firmware/src/app/variant.h',
         'firmware/src/seq/pattern.h', 'firmware/src/seq/lanes.h', 'firmware/src/seq/sequencer.h', 'firmware/src/seq/motion.h', 'firmware/src/seq/motion.c',
         'firmware/src/seq/tb3po.h', 'firmware/src/seq/harmony.h', 'firmware/src/dsp/drum909.c', 'firmware/src/dsp/drum808.c',
         'firmware/src/dsp/bass303.c', 'firmware/src/dsp/fxbus.c', 'firmware/src/dsp/master.c',
         'firmware/src/dsp/breaks.c', 'firmware/src/dsp/sound_strip.h',
         'firmware/src/app/main_fm1.c', 'firmware/src/midi_uart.c', 'firmware/src/usb.c',
         'web/emu/index.html', 'web/emu/slice-panel.html', 'web/emu/pad-panel.html']
CCS = list(range(32)) + list(range(64, 120))
RUNTIME_NOTE = 'design contract only; implementation status is in ACULTUR8_CC_RUNTIME_COVERAGE.json'
STRUCTS = {
    'tb3po_cfg_t':['density','accent','slide','oct_range','root','scale','base_oct','mutate_bars','seed'],
    'bpart_t':['step','len','rate','dir','transpose','gen'],
    'dpart_t':['hit','accent','len','rate','rsv'],
    'brkpart_t':['steps','set','slot_a','slot_b','rsv','slice'],
    'pattern_t':['drum','bass','brk','swing','rsv'],
    'arrangement_settings_t':['flags','key','mode','parent','bass_low','bass_high','pad_channel','pad_root','pad_low','pad_high','degree'],
    'sound_strip_config_t':['highpass_hz','lowpass_hz','boost_db','ceiling_db','release_ms','flags'],
    'settings_t':['magic','format','palette','keyled','clk_out','notes_out','bpm_x10','accent_q7','keysound','autosave_off','rsv_bright','stereo','snd_rev','midi_ch','out_loud','usb_full','one_core'],
    'lane_t':['used','gen','pat','part','t','v','i','rsv','val'],
}


def clean(text):
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/', '', text)


def split(text):
    out, start, depth, quoted, escape = [], 0, 0, False, False
    for i, c in enumerate(text):
        if quoted:
            if escape: escape = False
            elif c == '\\': escape = True
            elif c == '"': quoted = False
        elif c == '"': quoted = True
        elif c in '({[': depth += 1
        elif c in ')}]': depth -= 1
        elif c == ',' and depth == 0:
            if text[start:i].strip(): out.append(text[start:i].strip())
            start = i + 1
    if quoted or depth: raise ValueError('Unbalanced C initializer')
    if text[start:].strip(): out.append(text[start:].strip())
    return out


def initializer(text, name):
    found = re.search(r'\b' + re.escape(name) + r'\s*(?:\[[^;]*?\])?\s*=\s*\{', text)
    if not found: raise ValueError('Missing initializer: ' + name)
    start, depth, quoted = found.end(), 1, False
    for i in range(start, len(text)):
        c = text[i]
        if c == '"' and text[i-1] != '\\': quoted = not quoted
        if not quoted:
            if c == '{': depth += 1
            elif c == '}':
                depth -= 1
                if not depth: return text[start:i]
    raise ValueError('Unclosed initializer: ' + name)


def integer(expr, symbols):
    expr = re.sub(r'\b(0[xX][0-9A-Fa-f]+|\d+)[uUlL]+\b', r'\1', expr.strip())
    tree = ast.parse(expr, mode='eval')
    def visit(node):
        if isinstance(node, ast.Expression): return visit(node.body)
        if isinstance(node, ast.Constant) and type(node.value) is int: return node.value
        if isinstance(node, ast.Name) and node.id in symbols: return symbols[node.id]
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub): return -visit(node.operand)
        if isinstance(node, ast.BinOp):
            a, b = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add): return a+b
            if isinstance(node.op, ast.Sub): return a-b
            if isinstance(node.op, ast.Mult): return a*b
            if isinstance(node.op, ast.BitOr): return a|b
            if isinstance(node.op, ast.LShift): return a<<b
        raise ValueError('Unsupported integer expression: ' + expr)
    return visit(tree)


def symbols_from(texts):
    symbols = {}
    for _ in range(5):
        for text in texts.values():
            for name, value in re.findall(r'^#define\s+(\w+)\s+([^\n]+)', text, re.M):
                try: symbols[name] = integer(value, symbols)
                except (ValueError, SyntaxError): pass
            for body in re.findall(r'\benum\s*(?:\w+)?\s*\{([^}]+)\}', text):
                val = -1
                for item in split(body):
                    fields = item.split('=', 1); name = fields[0].strip()
                    try: val = integer(fields[1], symbols) if len(fields)>1 else val+1
                    except (ValueError, SyntaxError): break
                    symbols[name] = val
    return symbols


def descriptors(text, name, symbols):
    body = initializer(text, name)
    # Macro definitions themselves are primary source: no guessed defaults/counts.
    macros={m[0]:(m[1],m[2]) for m in re.findall(r'^#define\s+(\w+)(\([^)]*\))?[ \t]+([^\n]+)',text,re.M)}
    allowed=['LIN','EXP','PP','DRIVE','LEVEL','DIST','SENDS','PAN','P_DRIVE','P_DIST','P_REV','P_DLY']
    for _ in range(5):
        old=body
        for macro in allowed:
            if macro not in macros:continue
            params,value=macros[macro]
            if params:
                def expand(m):
                    args=split(m.group(1));formals=split(params[1:-1]);result=value
                    if len(args)!=len(formals):raise ValueError('Macro argument mismatch')
                    for formal,arg in zip(formals,args):result=re.sub(r'\b'+re.escape(formal)+r'\b',lambda _:arg,result)
                    return result
                body=re.sub(r'\b'+macro+r'\(([^()]*)\)',expand,body)
            else:body=re.sub(r'\b'+macro+r'\b',lambda _:value,body)
        if body==old:break
    rows = []
    for item in split(body):
        item = re.sub(r'^\[\w+\]\s*=\s*', '', item)
        if not item.startswith('{'): raise ValueError('Unknown descriptor syntax: ' + item)
        fields = split(item[1:-1])
        if fields[0].startswith('{'): fields = split(fields[0][1:-1])
        if len(fields)!=4 or not fields[0].startswith('"'): raise ValueError('Invalid descriptor: ' + item)
        label, maximum, default, names = fields
        choices = None
        if names != '0':
            try: choices = [json.loads(x) for x in split(initializer(text,names))]
            except ValueError:
                if names != 'TB3PO_SCALE_NAMES': raise
                choices = 'extern TB3PO_SCALE_NAMES'
        try: decoded=json.loads(label)
        except ValueError as e:raise ValueError(f'{name}: invalid label {label!r}') from e
        rows.append({'name':decoded, 'max':integer(maximum,symbols),
                     'default':integer(default,symbols), 'choices':choices})
    return rows


def enum_names(text, first):
    for body in re.findall(r'\benum\s*(?:\w+)?\s*\{([^}]+)\}', text):
        names=[x.split('=')[0].strip() for x in split(body)]
        if first in names:return names
    raise ValueError('Missing enum: '+first)


def build(root=ROOT):
    raw={p:(root/p).read_text(encoding='utf-8') for p in FILES}
    texts={p:clean(t) for p,t in raw.items()}; symbols=symbols_from(texts)
    structured={}
    for name,expected in STRUCTS.items():
        matches=[m for text in texts.values() for m in re.findall(r'typedef\s+struct\s*\{([^{}]*)\}\s*'+name+r'\s*;',text)]
        if len(matches)!=1:raise ValueError('Structured coverage missing: '+name)
        fields=[]
        for declaration in matches[0].split(';'):
            declaration=declaration.strip()
            if not declaration:continue
            tail=re.sub(r'^\w+\s+', '', declaration)
            fields.extend(re.sub(r'\[.*','',field).strip() for field in split(tail))
        if fields!=expected:raise ValueError('Structured field coverage changed: '+name)
        structured[name]={field:'reserved/read-only metadata' if field in ('rsv','rsv_bright','magic','format','stereo','snd_rev') or name=='lane_t' and field in ('used','gen') else 'editable registry field or recursively covered structure' for field in fields}
    for key,value in [('NPAT',32),('NSTEPS',64),('NSONG',192),('NLANE',160),('VARIANT_STRIPS',25),('POLY_LANES',25),('NRATES',8)]:
        if symbols.get(key)!=value:raise ValueError('Explicit context extent changed: '+key)
    rows=[]
    def add(key, name, maximum, slot, category='musical', minimum=0, default=None,
            source='firmware/src/app/ui.c', context=None, **extra):
        row={'id':key,'name':name,'category':category,'range':[minimum,maximum],
             'default':default,'source':source,'context':context or {},
             'runtime_cc':RUNTIME_NOTE,'live_requirement':'while playing',
             'canonical_requirement':'validated intent + dirty/undo; audio-owner application',
             'address':None,'scaling':'round(min + cc7 * (max-min) / 127)'}
        if slot is not None:
            channel,cc=divmod(slot,len(CCS));row['address']={'channel':channel+1,'cc':CCS[cc]}
            if CCS[cc]<32:row['optional_cc14']={'msb':CCS[cc],'lsb':CCS[cc]+32,'channel':channel+1,
                'status':'optional proposal; not implemented','scaling':'round(min + cc14 * (max-min) / 16383)'}
        row.update(extra);rows.append(row)
    # Stable 16-slot voice blocks preserve addresses when a descriptor is appended.
    engine=texts['firmware/src/app/engine.c']; bases=[0,192,384,416,432,464,544]
    exposed=engine[engine.index('int engine_nparams'):engine.index('const x0x_param_t *engine_param')]
    if set(re.findall(r'case (T_\w+):',exposed))!=set(['T_909','T_808','T_303','T_BRK','T_FX','T_MIX','T_MST']):raise ValueError('Engine family coverage changed')
    families=[('909','drum909.c','d9_specs'),('808','drum808.c','k_tracks'),
              ('303','bass303.c','params'),('BREAK','breaks.c','BRK_PARAMS'),
              ('FX','fxbus.c','fx_p'),('MIX',None,'MIX_P'),('MASTER','master.c','MST_P')]
    counts={}
    for target,(family,file,array) in enumerate(families):
        source='firmware/src/dsp/'+file if file else 'firmware/src/app/engine.c'; text=texts[source]
        if target==0: arrays=split(initializer(text,array))
        elif target==1: arrays=re.findall(r'TRK\((\w+),',initializer(text,array))
        elif target==2: arrays=[array]*symbols['NBASS']
        elif target==5: arrays=[array]*symbols['NPARTS']
        else: arrays=[array]
        if target in (0,1):
            voice_names=[json.loads(x) for x in split(initializer(engine,'N909' if target==0 else 'N808'))]
            if len(arrays)!=len(voice_names):raise ValueError('Voice map mismatch')
        else: voice_names=['A','B'] if target==2 else ['909','808','303A','303B','BREAK'] if target==5 else ['GLOBAL']
        counts[family]=[]
        for voice,arr in enumerate(arrays):
            ds=descriptors(text,arr,symbols)
            # engine_nparams exposes BREAK level/pitch only; pattern controls separately below.
            if target==3:
                engine_np=engine[engine.index('int engine_nparams'):engine.index('const x0x_param_t *engine_param')]
                match=re.search(r'case T_BRK:\s*return ([^;]+);',engine_np)
                if not match:raise ValueError('BREAK engine descriptor boundary missing')
                ds=ds[symbols['BRK_NSET']:symbols['BRK_NSET']+integer(match.group(1),symbols)]
            if len(ds)>16:raise ValueError('Stable voice block exceeded')
            counts[family].append(len(ds))
            for index,d in enumerate(ds):
                add(f'sound.{family}.{voice_names[voice]}.{index}',d['name'],d['max'],bases[target]+voice*16+index,
                    default=d['default'],source=source,target={'engine':target,'voice':voice,'index':index},choices=d['choices'])
    # Exact ABI field order and actual checked ranges, plus independently editable stage bits.
    vp=enum_names(texts['firmware/src/app/variant.h'],'VP_HIGHPASS')
    if vp!=['VP_HIGHPASS','VP_LOWPASS','VP_BOOST','VP_CEILING','VP_RELEASE','VP_FLAGS','VP_COUNT']:raise ValueError('Strip ABI changed')
    strip=[('highpass',20,20000),('lowpass',20,20000),('boost',0,24),('ceiling',0,48),('release',10,250),('flags',0,15),
           ('enabled',0,1),('highpass_enabled',0,1),('lowpass_enabled',0,1),('limiter_enabled',0,1)]
    for voice in range(symbols['VARIANT_STRIPS']):
        for i,(name,low,high) in enumerate(strip):
            add(f'strip.{voice}.{name}',name,high,576+voice*12+i,minimum=low,source='firmware/src/app/variant.c',
                target={'strip':voice,'field':i if i<6 else 5,'mask':None if i<6 else 1<<(i-6)},
                scaling='log-frequency proposal; endpoints exact' if i<2 else 'round(min + cc7 * (max-min) / 127)')
    ui=texts['firmware/src/app/ui.c']
    refs=enum_names(ui,'R_NONE')
    expected='R_NONE R_ENG R_SWING R_DLEN R_DRATE R_BLEN R_BRATE R_BDIR R_BTRANS R_GEN R_BRKSET R_BRKSLOT R_TEMPO R_ACCENT R_CLKOUT R_NOTEOUT R_PALETTE R_KEYLED R_KEYSOUND R_AUTOSAVE R_ACT R_SBAR R_SPAT R_SLEN R_STALLS R_HDR R_TAPE R_MIDICH R_OUTLOUD R_SREP R_BATT R_USBFULL R_FREEPLAY R_RATEENABLE R_RATEFAMILY R_RATEBOUNDARY R_RATEHYST R_RATEEXACT R_STRIP R_STRIPFLAG R_STRIPDEFAULT R_CORES R_BMODE R_BDEGREE R_BMUTE'.split()
    expected+=['R_AKEY','R_ASCALE','R_AFOLLOW','R_COPYCFG'] # native aliases of existing harmony fields
    if refs!=expected:raise ValueError('UI reference coverage must be updated')
    # Every pref kind accounted for, including nonmusical/read-only rows.
    seq=descriptors(ui,'SEQ_P',symbols); sq=enum_names(ui,'SQ_SWING')[:-1]
    for i,(symbol,d) in enumerate(zip(sq,seq)):
        if len(seq)!=len(sq):raise ValueError('Sequence descriptor count mismatch')
        category='nonmusical' if symbol in ('SQ_THEME','SQ_KEYLED','SQ_AUTOSAVE') else 'musical'
        context={'pattern':[0,31]} if symbol=='SQ_SWING' else {'pattern':[0,31],'kit':[0,1]} if symbol in ('SQ_DLEN','SQ_DRATE') else {'pattern':[0,31],'bass':[0,1]} if symbol in ('SQ_BLEN','SQ_BRATE','SQ_BDIR','SQ_BTRANS') else {}
        add('pref.'+symbol,d['name'],d['max'],900+i,category,default=d['default'],context=context,
            semantic_offset=1 if symbol in ('SQ_DLEN','SQ_BLEN') else 20 if symbol=='SQ_TEMPO' else -24 if symbol=='SQ_BTRANS' else 0,choices=d['choices'])
    for i,d in enumerate(descriptors(ui,'GEN_P',symbols)):
        add(f'generator.{i}',d['name'],d['max'],920+i,default=d['default'],context={'pattern':[0,31],'bass':[0,1]},choices=d['choices'],semantic_offset=1 if i==symbols['G_OCTS'] else 0)
    brk=descriptors(texts['firmware/src/dsp/breaks.c'],'BRK_PARAMS',symbols)
    for i,d in enumerate(brk[:symbols['BRK_NSET']]):
        add(f'break.generator.{i}',d['name'],d['max'],936+i,default=d['default'],context={'pattern':[0,31]},choices=d['choices'],source='firmware/src/dsp/breaks.c')
    for i,d in enumerate(descriptors(ui,'MIDICH_P',symbols)):
        add(f'part.channel.{i}',d['name'],d['max'],952+i,default=d['default'],cleanup='old-channel Off before new attacks; pad collision rejection')
    for i,array in enumerate(['OUT_P','USBA_P','FREEPLAY_P','SREP_P','STALLS_P','BATT_P']):
        # Scalar descriptor initializers use the same reader through a wrapper.
        scalar=initializer(ui,array);d=descriptors('static x0x_param_t scalar[]={'+ '{'+scalar+'}' +'}; '+ui,'scalar',symbols)[0]
        add('pref.'+array,d['name'],d['max'],None if array=='BATT_P' else 960+i,
            'read_only' if array=='BATT_P' else 'diagnostic' if array=='STALLS_P' else 'musical',default=d['default'],
            context={'bass':[0,1]} if array=='FREEPLAY_P' else {'block':[0,191]} if array=='SREP_P' else {})
    for i,d in enumerate(descriptors(ui,'SONG_P',symbols)):
        add(f'song.{i}',d['name'],d['max'],972+i,default=d['default'],context={'block':[0,191]} if i<5 else {})
    for i,d in enumerate(descriptors(ui,'ACT_P',symbols)):
        add(f'action.{i}',d['name'] or 'No action',127,None if i==0 or i>=12 else 984+i,'action',
            action_policy='edge-trigger; explicit safety/preflight; no storage operation in audio callback',context={'pattern':[0,31],'part':[0,4],'block':[0,191]})
    add('system.one_core','Second core disabled at next power-on',1,None,'nonmusical',source='firmware/src/app/x0x.h',runtime_parameter='Native settings control only; no CC address; next power-on')
    # Structured data and controls not represented by x0x_param_t.
    specs=[('selector.pattern',31,{}),('selector.part',4,{}),('selector.drum',10,{}),('selector.bass',1,{}),
           ('selector.step',63,{}),('selector.block_low',127,{}),('selector.block_high',1,{}),('selector.motion_low',127,{}),('selector.motion_high',1,{}),
           ('pref.break_slot_a',127,{'pattern':[0,31]}),('pref.break_slot_b',127,{'pattern':[0,31]}),('pref.song_edit_block',191,{}),('pref.tape_macro',127,{}),
           ('step.drum_hit',1,{'pattern':[0,31],'kit':[0,1],'drum':[0,10],'step':[0,63]}),
           ('step.drum_accent',1,{'pattern':[0,31],'kit':[0,1],'step':[0,63]}),
           ('step.bass_note',127,{'pattern':[0,31],'bass':[0,1],'step':[0,63]}),('step.bass_gate',2,{'pattern':[0,31],'bass':[0,1],'step':[0,63]}),
           ('step.bass_accent',1,{'pattern':[0,31],'bass':[0,1],'step':[0,63]}),('step.bass_slide',1,{'pattern':[0,31],'bass':[0,1],'step':[0,63]}),
           ('step.break_allowed',1,{'pattern':[0,31],'step':[0,63]}),('step.break_slice',8,{'pattern':[0,31],'step':[0,63]}),
           ('motion.value',255,{'lane':[0,159],'step':[0,63]}),('motion.target',7,{'lane':[0,159]}),('motion.voice',127,{'lane':[0,159]}),
           ('motion.param',15,{'lane':[0,159]}),('motion.pattern',31,{'lane':[0,159]}),('motion.part',4,{'lane':[0,159]}),
           ('motion.record',1,{'lane':[0,159]}),('motion.hold',1,{'lane':[0,159]}),('motion.clear',1,{'lane':[0,159]}),
           ('probability.value',135,{'pattern':[0,31],'part':[0,4],'drum':[0,10],'step':[0,63]}),
           ('performance.mute',1,{'strip':[0,24]}),('performance.solo',1,{'strip':[0,24]}),('performance.pattern_cue',31,{'part':[0,4]}),
           ('performance.chain_start',31,{}),('performance.chain_end',31,{}),('performance.song_enabled',1,{}),('performance.song_record',1,{}),
           ('generator.new',1,{'pattern':[0,31],'bass':[0,1]}),('generator.mutate',1,{'pattern':[0,31],'bass':[0,1]}),
           ('generator.seed_byte',255,{'pattern':[0,31],'bass':[0,1],'seed_byte':[0,3]}),
           ('arrangement.enabled',1,{}),('arrangement.length',192,{}),('arrangement.key',11,{}),('arrangement.mode',9,{}),('arrangement.parent',255,{}),
           ('arrangement.degree',7,{'block':[0,191]}),('arrangement.duration',32,{'block':[0,191]}),('arrangement.pattern',31,{'block':[0,191],'part':[0,4]}),
           ('arrangement.mute',1,{'block':[0,191],'part':[0,4]}),('arrangement.queue',191,{}),
           ('follow.mode',3,{'bass':[0,1]}),('follow.low',127,{'bass':[0,1]}),('follow.high',127,{'bass':[0,1]}),
           ('pad.channel',255,{}),('pad.root',127,{}),('pad.low',127,{}),('pad.high',127,{}),
           ('selector.kit',1,{}),('selector.strip',24,{}),('selector.seed_byte',3,{}),
           ('selector.commit',1,{}),('selector.parameter_low',127,{}),('selector.parameter_high',15,{}),
           ('value.exact_low7',127,{}),('value.exact_mid7',127,{}),('value.exact_high2',3,{}),('value.exact_commit',1,{})]
    for i,(key,maximum,context) in enumerate(specs):
        add(key,key,maximum,1010+i,context=context,source='firmware/src/seq/pattern.h' if key.startswith(('step.','generator.seed')) else 'firmware/src/seq/sequencer.h',
            validity='semantic validation required: enum holes, range ordering, slot availability, channel conflicts and selected extent',
            cleanup='reconcile old-channel owned notes before any replacement attack' if key.startswith(('follow.','pad.')) else None)
    # Parse enum bounds, rather than invent the harmony mode count.
    for row in rows:
        if row['id'] in ('pref.SQ_DRATE','pref.SQ_BRATE'):row['dynamic_max']='7 with initialized typed lane storage; original descriptor max3 for legacy projects; next local step'
        if row['id']=='arrangement.mode': row['range'][1]=symbols['HARMONY_CHROMATIC']
        if row['id']=='arrangement.duration':row['range'][0]=1
        if row['id'] in ('arrangement.duration','follow.mode'):
            row['coarse_range']=[1,8] if row['id']=='arrangement.duration' else [0,2]
            row['scaling']='Preserve legacy coarse range; full semantic range is available through exact-value transactions and native/browser controls'
        if row['id']=='arrangement.parent':row['allowed_values']=list(range(7))+[symbols['HARMONY_NO_PARENT']]
        if row['id']=='pad.channel':row['allowed_values']=list(range(16))+[255]
        if row['id']=='motion.value':
            row['value_domains']={'requires_explicit_parameter_target':True,
                'by_parameter':{sound['id']:{'range':sound['range'],'sentinels':[symbols['MOT_NONE']]} for sound in rows if sound['id'].startswith('sound.')},
                'T_PROB':{'range':[0,100],'condition_start':symbols['PROB_COND'],'condition_count':symbols['PROB_NCOND'],'sentinels':[symbols['MOT_NONE']],
                    'ui_probability_subset':list(range(5,101,5)),'source':'engine.c probability evaluator; sequencer.h PROB_COND/PROB_NCOND; ui.c prob_turn'}}
            row['scaling']='target_domain[round(cc7 * (len(target_domain)-1) / 127)]; target required; exact transaction validates membership'
            if 'optional_cc14' in row:row['optional_cc14']['scaling']='target_domain[round(cc14 * (len(target_domain)-1) / 16383)]; target required'
        if 'allowed_values' in row:
            row['scaling']='allowed_values[round(cc7 * (len(allowed_values)-1) / 127)]; exact-value transaction validates canonical value'
            if 'optional_cc14' in row:row['optional_cc14']['scaling']='allowed_values[round(cc14 * (len(allowed_values)-1) / 16383)]'
        if row['id'].startswith('pref.break_slot'):row['dynamic_max']='engine_brk_nslots()-1; validate against loaded slot registry'
        if row['address']:row['parameter_number']=(row['address']['channel']-1)*len(CCS)+CCS.index(row['address']['cc'])
    rate_names=[json.loads(x) for x in split(initializer(ui,'RATE_N'))]
    if len(rate_names)!=symbols['NRATES']:raise ValueError('Rate enum/label coverage mismatch')
    add('lane.length','Individual sound-lane length',64,1128,minimum=1,context={'pattern':[0,31],'strip':[0,24]},
        source='firmware/src/seq/lanes.h',runtime_parameter='engine_lane_set; next local step; prepared Off/On')
    add('lane.rate','Individual sound-lane rate',7,1129,context={'pattern':[0,31],'strip':[0,24]},
        source='firmware/src/seq/lanes.h',runtime_parameter='engine_lane_set; next local step; prepared Off/On',
        choices=rate_names)
    add('value.exact_high7','21-bit exact value high chunk',127,1130,
        source='CC transaction extension: same low/mid/commit IDs, new high7 address; old high2 retained')
    add('lane.mode','Prepared independent clock mode',1,1131,source='firmware/src/seq/lanes.h',runtime_parameter='engine_lane_mode; first initialization stopped; later Off/On next96-tick bar')
    add('selector.destination_pattern','Explicit pattern-copy destination',31,1132,source='firmware/src/app/cc.c',action_policy='source draft destination; selector.commit freezes it; never UI focus')
    add('action_control.arm','Arm destructive native action',11,1133,'action',source='firmware/src/app/cc.c',action_policy='0 cancels; action2/3/4/5/7/8/9 binds source/context/project revision')
    add('action_control.cancel','Cancel source action arm',1,1134,'action',source='firmware/src/app/cc.c',action_policy='1 cancels source arm; 0 does nothing')
    for field,label,maximum in [('enable','Enable expanded rates (stopped)',1),('exact','Expanded exact division',20),('family','Rate sweep family',3),('boundary','Rate latch boundary',1),('hysteresis','Rate sweep hysteresis CC units',2),('sweep_v1','Family-safe rate sweep ABI1',127)]:
        number=1135+['enable','exact','family','boundary','hysteresis','sweep_v1'].index(field)
        add('rate.'+field,label,maximum,number,context={'pattern':[0,31],'strip':[0,24]},source='firmware/src/seq/rate_families.h',runtime_parameter='native shared engine; legacy lane.rate1129 unchanged')
    add('output.master_knob','MASTER gain Q12',4096,1100,source='web/emu/index.html',
        canonical_requirement='define live remote gain vs physical ADC pickup; current pot is runtime, not project intent')
    for i,(key,maximum,category) in enumerate([
        ('slice.level_percent',100,'browser_musical'),('slice.repeat',1,'browser_musical'),('slice.layer',1,'browser_musical'),
        ('slice.source_bpm_milli',300000,'browser_musical'),('slice.audition_index',63,'browser_musical'),
        ('slice.play',1,'browser_action'),('slice.stop',1,'browser_action'),('slice.arm_sync',1,'browser_action'),
        ('slice.import',1,'browser_action'),('slice.demo',1,'browser_action'),('slice.save_import',1,'browser_action'),
        ('slice.reopen_import',1,'browser_action'),('slice.export',1,'browser_action'),
        ('editor.undo',1,'action'),('editor.redo',1,'action'),('transport.play',1,'action'),('transport.stop',1,'action'),('transport.record',1,'action')]):
        add(key,key,maximum,1104+i,category,minimum=20000 if key=='slice.source_bpm_milli' else 0,
            source='web/emu/slice-panel.html' if key.startswith('slice.') else 'firmware/src/app/ui.c',
            canonical_requirement='browser import/preview lane; separate storage and hardware capability' if key.startswith('slice.') else 'canonical action with explicit owner and acknowledgement')
    for key in ['route.connect_permission','route.output_selection','route.activate','route.retry_cleanup','route.disconnect',
                'file.audio_selection','file.marker_selection','file.hardware_slot_upload','file.hardware_slot_clear',
                'ui.navigation','ui.keyboard_latch','ui.factory_reset']:
        add(key,key,1,None,'nonmusical_action',source='web/emu/index.html',
            action_policy='explicitly inventoried; no automatic permission/file/dialog/device selection over CC; runtime policy not implemented')
    for row in rows:
        if row['address']:row['parameter_number']=(row['address']['channel']-1)*len(CCS)+CCS.index(row['address']['cc'])
    covered={name:'descriptor or structured field' for name in refs}
    for field,maximum in [('source',31),('destination',31),('scope',24)]:
        add('ui.pattern_copy.'+field,'Stopped copy '+field,maximum,None,'nonmusical_action',source='firmware/src/app/ui.c',action_policy='Native UI only; confirmed staged paste; no new remote CC binding')
    covered.update({'R_COPYCFG':'native-only source/destination/scope; no MIDI binding', 'R_NONE':'sentinel','R_HDR':'non-selectable header','R_BATT':'read-only', 'R_ACT':'all ACT_P actions; block actions 12-15 and Undo action 16 and scoped paste action 17 and copy submenu action 18 native only', 'R_BMODE':'alias arrangement.enabled', 'R_BDEGREE':'alias arrangement.degree', 'R_BMUTE':'alias arrangement.mute'})
    result={'schema':1,'map_version':'draft-1-actions-prototype-1','runtime_cc':RUNTIME_NOTE,
            'source_hashes':{p:hashlib.sha256(t.encode()).hexdigest() for p,t in raw.items()},
            'descriptor_counts':counts,'ui_ref_coverage':covered,'structured_field_coverage':structured,
            'ingress_context_proof':{'USB':'firmware/src/usb.c:4,960; main_fm1.c:146 TIMER5 ISR, not main loop',
                'TRS':'firmware/src/midi_uart.c:87; main_fm1.c:148 TIMER5 ISR, not main loop',
                'conclusion':'Do not call canonical UI/undo setters directly in packet producers. Ordered main-context ring dispatch or bounded CC mailbox needs ownership/resource proof.'},
            'address_model':{'channel_numbering':'1..16 human; MIDI status low nibble is channel-1','coarse_cc':CCS,
                'reserved_channel_mode_cc':list(range(120,128)), 'reserved_fine_cc':list(range(32,64)),
                'selectors':'Explicit input-source-owned pattern/part/kit/drum/bass/strip/step/block/lane/seed-byte selectors. Never UI focus. Reset selectors on input disconnect/replacement. Split low/high selectors commit only on explicit target-confirm barrier.',
                'version_policy':'IDs and occupied addresses immutable after adoption; new entries use holes/new version. Reordering source descriptors requires explicit identity migration.',
                'optional_14bit':'Only registered coarse CC0..31; CC32..63 reserved companions. Every field has an ordinary direct CC7 address, which can quantize wide ranges. Exact canonical integers use the additional explicit selector/value transaction below, never NRPN-only.',
                'exact_values':'selector.parameter_low/high identify immutable parameter_number (7-bit chunks); selector.commit freezes complete context. value.exact_low7/mid7/high7 stages a 21-bit unsigned canonical value; value.exact_commit applies atomically. Old exact_high2 remains a compatible 16-bit high chunk. Reject incomplete/stale transactions. All four seed bytes separately addressable. Browser source BPM milli fits 21 bits; its browser setter remains unimplemented. This extra protocol uses ordinary CCs, not NRPN.',
                'feedback':'No MIDI echo; browser snapshots separate from outbound MIDI. Explicit feedback opt-in needs independent routing/loop tests.',
                'overflow':'Bounded ingress/coalescing for values, ordering barriers for selectors/actions; priority/reserve for Note-Off/Stop/clock; observable rejection/drop counters, never blocking audio.'},
            'entries':rows}
    validate(result);return result


def validate(data):
    ids=set();addresses=set()
    for row in data['entries']:
        if row['id'] in ids:raise ValueError('Duplicate logical ID: '+row['id'])
        ids.add(row['id']);low,high=row['range']
        if low>high:raise ValueError('Invalid range')
        if row['default'] is not None and not low<=row['default']<=high:raise ValueError('Invalid default')
        a=row['address']
        if a:
            pair=(a['channel'],a['cc'])
            if not 1<=pair[0]<=16 or pair[1] not in CCS:raise ValueError('Invalid/reserved CC address')
            if pair in addresses:raise ValueError('CC address collision: '+str(pair))
            addresses.add(pair)
            fine=row.get('optional_cc14')
            if fine and (fine['channel']!=pair[0] or fine['msb']!=pair[1] or pair[1]>=32 or fine['lsb']!=pair[1]+32):raise ValueError('Invalid 14-bit companion')
        if row['runtime_cc']!=RUNTIME_NOTE:raise ValueError('False runtime claim')


def value_domain(row, target=None):
    """Pure draft-map reference, not a runtime MIDI implementation."""
    domains=row.get('value_domains')
    if domains:
        selected=domains.get('T_PROB') if target=='T_PROB' else domains['by_parameter'].get(target)
        if selected is None:raise ValueError('Explicit valid motion target required')
        low,high=selected['range'];values=list(range(low,high+1))
        if 'condition_count' in selected:values.extend(range(selected['condition_start'],selected['condition_start']+selected['condition_count']))
        return sorted(set(values+selected['sentinels']))
    if 'allowed_values' in row:return row['allowed_values']
    low,high=row['range'];return list(range(low,high+1))


def scaled_value(row, raw, bits=7, target=None):
    if bits not in (7,14) or type(raw) is not int or not 0<=raw<1<<bits:raise ValueError('Invalid CC precision/value')
    values=value_domain(row,target);den=(1<<bits)-1
    if bits==7 and 'coarse_range' in row:
        low,high=row['coarse_range'];values=list(range(low,high+1))
    return values[(raw*(len(values)-1)+den//2)//den]


def exact_value(row, raw, target=None):
    if type(raw) is not int or not 0<=raw<=2097151 or raw not in value_domain(row,target):raise ValueError('Exact transaction outside target domain')
    return raw


def markdown(data):
    counts=Counter(row['category'] for row in data['entries'])
    out=['# Acultur8 complete CC design inventory','',
         '**Inventory and draft mapping only. Native candidate support is recorded in ACULTUR8_CC_RUNTIME_COVERAGE.json; this inventory does not certify browser/DAW/device acceptance.**','',
         'Generated with `python tools/cc_inventory.py --write`; validate with `--check`. No firmware compilation is performed.', '',
         f"{len(data['entries'])} inventory rows: {sum(r['address'] is not None for r in data['entries'])} addressed field templates/descriptor rows and {sum(r['address'] is None for r in data['entries'])} explicitly unmapped/read-only/nonmusical rows. Repeated contexts enumerate the full pattern, step, block and lane extents in JSON. Categories: {dict(counts)}.",'',
         '## Source-derived engine descriptor counts','', '| Family | Per-voice counts | Total |','| --- | --- | --- |']
    for family,counts in data['descriptor_counts'].items():out.append(f'| {family} | {counts} | {sum(counts)} |')
    out+=['','## Mapping contract','',
          'Fixed channel/CC addresses are independent of configurable note channels. CC120–127 are reserved for channel-mode messages. CC32–63 are reserved fine companions; optional 14-bit precision supplements ordinary 7-bit CC and is not NRPN-only.',
          '','Structured fields use explicit MIDI-owned selectors, never the open UI page or editing focus. Pattern 0–31, part 0–4, step 0–63, block 0–191 and motion lane 0–159 are explicit contexts. Selectors must be scoped per input source and reset on detach. Split-address transactions need an explicit commit barrier; its final wire protocol is not accepted yet.',
          '','Ranges describe stored encodings. Length pots add one, tempo adds 20 BPM, transpose subtracts 24. Enum holes and 255 sentinels require semantic validation, not numeric clamping. Frequency scaling is a draft logarithmic proposal; exact cutoff resolution and 14-bit pairing need acceptance. Direct CC7 quantizes wide ranges. The additional ordinary-CC explicit parameter/context/value transaction has three 7-bit chunks (21 bits), sufficient for browser source-BPM milli (19 bits); the browser setter remains unimplemented. Old high2 retains the original 16-bit compatibility path. No false claim of exact 7-bit representation.',
          '','Sparse parent/pad-channel domains use allowed-value ordinal scaling for both CC7 and optional CC14, so intermediate values remain valid. Motion values require an explicit parameter target: each sound descriptor has its own 0..max domain plus MOT_NONE=255; T_PROB supports probability 0..100, all 35 condition codes 101..135, and MOT_NONE. The original panel edits its 5%-step probability subset, while the evaluator accepts the wider domain. Exact-value transactions test target membership and preserve every condition code.',
          '','`lane.length` and `lane.rate` address all25 lanes in each of32 patterns; eight rates preserve IDs0..3. `lane.mode` appends address1131. First initialization requires stopped transport; subsequent mode changes are live at the next96-tick musical boundary. Legacy part descriptor addresses remain stable and dispatch through packed lane setters when initialized. BREAK step contexts now cover all64 steps; uninitialized legacy projects reject steps16..63 until initialized.',
          '','## Runtime design and acceptance still required','',
          'The original audio `midi_in` handler ignores CC. The isolated native candidate now intercepts it in an ordered main-context handoff; consult the runtime ledger and native checkpoint. Browser input listeners, compiled worklet/WASM acceptance and complete live coverage remain open. Original stopped-only follow/pad/output/length commands alone cannot establish live DAW automation.',
          '','Do not call UI/canonical/undo functions from the MIDI audio callback. Validate and publish canonical intent with audio-owner application, dirty/readback acknowledgement and bounded undo gestures. Reconcile motion hold/record. Live mode/channel/range changes retain original-channel Off debt before replacement attacks. Define musical boundaries for length/rate/block changes.',
          '','Context proof: `usb_poll` is TIMER5 ISR (`usb.c:4,960`, called by `main_fm1.c:146`); `uart_midi_poll` is also TIMER5 ISR (`midi_uart.c:87`, `main_fm1.c:148`). “Outside the render” does not mean main context. Reject direct canonical UI setters in either packet producer. Ordered main-context ring dispatch or a bounded CC mailbox needs further ownership/resource/order proof.',
          '','No incoming CC echo. Bound/coalesce automation without reordering selectors/actions; protect Note-Off/Stop/clock and expose rejection/overflow counters. Device IDs and input routes are not project data. Storage/destructive actions remain explicitly inventoried with preflight rather than silently omitted.',
          '','Acceptance: exhaustive registry coverage and endpoint/enum scaling; collision/reservation checks; every editable field while playing; canonical readback/motion/undo/save-reopen; saturated ingress and output cleanup; source-replacement races; real DAW/browser GUI; unchanged target resource gates and physical timing.',
          '','## Complete logical field registry','', '| ID | Name | Range | Channel / CC | Context | Category |','| --- | --- | --- | --- | --- | --- |']
    for row in data['entries']:
        a=row['address'];address=f"{a['channel']} / {a['cc']}" if a else 'none'
        out.append(f"| {row['id']} | {row['name']} | {row['range']} | {address} | {json.dumps(row['context'],sort_keys=True)} | {row['category']} |")
    return '\n'.join(out)+'\n'


def check_artifacts(root,data):
    expected={JSON_PATH:json.dumps(data,indent=2,ensure_ascii=False)+'\n',MD_PATH:markdown(data)}
    for path,text in expected.items():
        target=root/path
        if not target.exists() or target.read_text(encoding='utf-8')!=text:raise ValueError('CC inventory drift: '+str(path))


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path,default=ROOT)
    parser.add_argument('--write',action='store_true');parser.add_argument('--check',action='store_true');args=parser.parse_args()
    data=build(args.root);outputs={JSON_PATH:json.dumps(data,indent=2,ensure_ascii=False)+'\n',MD_PATH:markdown(data)}
    for path,text in outputs.items():
        target=args.root/path
        if args.write:target.write_text(text,encoding='utf-8',newline='\n')
    if args.check:check_artifacts(args.root,data)
    print(f"CC inventory: {len(data['entries'])} rows; descriptor/map/reservation checks PASS; runtime acceptance is separate")


if __name__=='__main__':main()
