#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Object-only history cost ledger: original target model, qualified relocations.
Extract existing official objects with build/history-cost-experiment/extract.sh.
python tests/cc_history_feature_costs.py --report tests/cc_history_feature_costs.json
Does not link, compile, update baselines or claim cycles/worst stack.
"""
import argparse,hashlib,json,re,struct
from pathlib import Path
import target_budget as budget
ROOTS=('ui_cc_undo','do_undo','ui_undo_preflight','undo_arrangement_intent','engine_transport_reserve','engine_transport_release')
ALTERNATIVES={'do_undo':('undo_undo','undo_redo'),'ud_swap':('undo_changed',)}

def read_object(path):
 b=path.read_bytes()
 if b[:6]!=b'\x7fELF\x01\x01':raise ValueError('requires little-endian ELF32')
 h=struct.unpack_from('<16sHHIIIIIHHHHHH',b)
 secs=[struct.unpack_from('<IIIIIIIIII',b,h[6]+i*h[11]) for i in range(h[12])]
 ns=secs[h[13]];strings=b[ns[4]:ns[4]+ns[5]]
 def string(b,o):return b[o:b.index(0,o)].decode()
 names=[string(strings,s[0]) for s in secs];symbols=[];relocs={}
 for idx,s in enumerate(secs):
  if s[1]==2:
   tab=secs[s[6]];stringsym=b[tab[4]:tab[4]+tab[5]]
   rows=[]
   for off in range(s[4],s[4]+s[5],s[9]):
    x=struct.unpack_from('<IIIBBH',b,off);rows.append({'name':string(stringsym,x[0]),'value':x[1],'size':x[2],'type':x[3]&15,'section':x[5]})
   symbols.append((idx,rows))
 symtabs=dict(symbols)
 for s in secs:
  if s[1]!=4:continue
  relocs.setdefault(names[s[7]],{})
  for off in range(s[4],s[4]+s[5],s[9]):
   addr,info,add=struct.unpack_from('<IIi',b,off);relocs[names[s[7]]][addr]=symtabs[s[6]][info>>8]
 dis=Path(str(path)+'.dis').read_text(encoding='utf-8');instructions={};section=None
 for line in dis.splitlines():
  m=re.match(r'Disassembly of section (\S+):',line)
  if m:section=m[1];instructions[section]=[];continue
  m=budget.INSN.match(line)
  if m and section:
   text=m[3].strip()
   if '<unknown instruction>' in text:raise ValueError('unknown instruction')
   if not re.match(r'<\s*\d+\s*:',text):instructions[section].append((int(m[1],16),text))
 rows={}
 for _,syms in symbols:
  for symbol in syms:
   if symbol['type']!=2 or not symbol['size'] or not (0<symbol['section']<len(names)):continue
   section_index=symbol['section'];section=names[section_index];start=symbol['value'];end=start+symbol['size']
   ins=[(a,t) for a,t in instructions.get(section,[]) if start<=a<end]
   if not ins:continue
   key=f"{path.name}:{symbol['name']}@{section}+0x{start:x}"
   frame=0;pro=[]
   for addr,t in ins:
    if re.match(r'(?:call|goto|if\b|ifs\b)',t):break
    pro.append([hex(addr),t]);push=re.search(r'\[--sp\] = \{([^}]+)\}',t)
    if push:
     for reg in push[1].split(','):
      reg=reg.strip();r=re.fullmatch(r'r(\d+)-r(\d+)',reg)
      frame+=4*(abs(int(r[1])-int(r[2]))+1) if r else 4
    if re.fullmatch(r'\[--sp\] = rets',t):frame+=4
    adjust=re.fullmatch(r'sp \+= -(\d+)',t)
    if adjust:frame+=int(adjust[1])
   edges=[]
   for addr,t in ins:
    if not re.match(r'(?:call|goto)\b',t):continue
    symbol=relocs.get(section,{}).get(addr)
    if symbol:
     edges.append({'site':hex(addr),'kind':'direct-call' if t.startswith('call') else 'tail-transfer','targetName':symbol['name']})
    elif t.startswith('call'):
     edges.append({'site':hex(addr),'kind':'unresolved-indirect-call','targetName':None})
   for name in ALTERNATIVES.get(symbol_name:=key.split(':',1)[1].split('@',1)[0],()):
    edges.append({'site':None,'kind':'source-callback-alternative','targetName':name})
   normalized=[];allocations=[]
   for addr,t in ins:
    relocation=relocs.get(section,{}).get(addr)
    if relocation and re.match(r'(?:call|goto)\b',t) and (relocation['section']!=section_index or not start<=relocation['value']<end):
     t=re.sub(r'<[^>]*>','<external-relocation>',t)
    normalized.append((addr,t))
    push=re.search(r'\[--sp\] = \{([^}]+)\}',t)
    if push:
     count=0
     for reg in push[1].split(','):
      reg=reg.strip();r=re.fullmatch(r'r(\d+)-r(\d+)',reg)
      count+=4*(abs(int(r[1])-int(r[2]))+1) if r else 4
     allocations.append({'site':hex(addr),'bytes':count,'kind':'register-push'})
    if re.fullmatch(r'\[--sp\] = rets',t):allocations.append({'site':hex(addr),'bytes':4,'kind':'return-address-push'})
    adjust=re.fullmatch(r'sp \+= -(\d+)',t)
    if adjust:allocations.append({'site':hex(addr),'bytes':int(adjust[1]),'kind':'local-allocation'})
   rows[key]={'name':symbol_name,'metrics':budget.cost(normalized),'allCallSites':sum(t.startswith('call') for _,t in ins),'unconditionalPrologueBytes':frame,'stackAllocationSites':allocations,'allocationSumBytesNotWorstStack':sum(x['bytes'] for x in allocations),'prologue':pro,'edges':edges}
 return rows

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--object',type=Path,action='append');ap.add_argument('--report',type=Path,required=True)
 a=ap.parse_args();objects=a.object or [Path('build/history-cost-experiment/target-x0x.o'),Path('build/history-cost-experiment/target-engine.o')]
 rows={};identity={}
 for p in objects:
  rows.update(read_object(p));identity[str(p)]={'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'dis_sha256':hashlib.sha256(Path(str(p)+'.dis').read_bytes()).hexdigest()}
 byname={}
 for key,row in rows.items():byname.setdefault(row['name'],[]).append(key)
 missing=[x for x in ROOTS if x not in byname]
 if missing:raise ValueError('missing required functions: '+','.join(missing))
 for row in rows.values():
  for edge in row['edges']:edge['targets']=byname.get(edge['targetName'],[])
 selected=set(k for name in ROOTS for k in byname[name]);todo=list(selected)
 while todo:
  for edge in rows[todo.pop()]['edges']:
   for target in edge['targets']:
    if target not in selected:selected.add(target);todo.append(target)
 report={'model':budget.MODEL,'objectLoopNormalization':'External relocation-qualified calls/tail transfers cannot form intra-function loops; only their printed target labels are suppressed before original cost(). All instructions/calls remain counted.','scope':'Existing official pre-optimization history target objects; original15gates and limits untouched. XIP failed1592B, no target acceptance.','limits':'Static instruction/loop/frame estimates, not cycles or whole-chain worst stack. Object addresses qualify duplicates. Unresolvedexternal/indirect callbacks and IRQ nesting remain unmeasured. All outlined callee work is listed; lower caller counts are not CPU improvement.','objects':identity,'sourceManifest':'build/cc-history-checkpoint/manifest.json','sourceCallbackAlternatives':ALTERNATIVES,'rootIds':{n:byname[n] for n in ROOTS},'rows':{k:rows[k] for k in sorted(selected)}}
 a.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n');print(f'ledger {len(selected)} emitted caller/callee rows; '+str(a.report))
 for name in ROOTS:
  for key in byname[name]:
   r=rows[key];print(name,r['metrics'],'allcalls',r['allCallSites'],'allocation-sites-sum',r['allocationSumBytesNotWorstStack'])
if __name__=='__main__':main()
