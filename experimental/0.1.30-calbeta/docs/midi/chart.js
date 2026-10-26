'use strict';
(()=>{
 const $=id=>document.getElementById(id), data=JSON.parse($('chart-data').textContent), rows=data.controls;
 const dom=new Map([...document.querySelectorAll('#controls tbody tr')].map(el=>[el.dataset.id,el]));
 const byId=new Map(rows.map(r=>[r.id,r])), inputs=['search','group','channel','support'];let visible=rows, current=null;
 function state(){return Object.fromEntries(inputs.map(k=>[k,$(k).value]));}
 function termsMatch(r,q){return q.toLowerCase().trim().split(/\s+/).filter(Boolean).every(t=>{
  const m=t.match(/^(ch|channel|cc|id):(\d+)$/);
  if(m)return r[{ch:'channel',channel:'channel',cc:'cc',id:'parameter'}[m[1]]]===Number(m[2]);
  return r.search.includes(t);
 });}
 function writeHash(extra={}){const s=state(), p=new URLSearchParams({...s,...extra});for(const[k,v]of [...p])if(!v)p.delete(k);history.replaceState(null,'',location.pathname+location.search+(p.size?'#'+p.toString():''));}
 function filter(update=true){
  const s=state();visible=[];
  for(const r of rows){const match=termsMatch(r,s.search)&&(!s.group||r.group===s.group)&&(!s.channel||(s.channel==='none'?r.channel===null:r.channel===Number(s.channel)))&&(!s.support||(s.support==='new'?r.new:r.support===s.support));dom.get(r.id).hidden=!match;if(match)visible.push(r);}
  $('result-count').textContent=`${visible.length} of ${rows.length} entries · ${visible.filter(r=>r.channel!==null).length} assigned addresses`;
  $('empty').hidden=visible.length!==0;if(update)writeHash();
 }
 function clear(){for(const k of inputs)$(k).value='';filter();}
 function reveal(id){const r=byId.get(id);if(!r)return;
  for(const k of inputs)$(k).value='';filter(false);
  document.querySelectorAll('tr.selected').forEach(x=>x.classList.remove('selected'));
  const tr=dom.get(id);tr.classList.add('selected');tr.querySelector('details').open=true;
  writeHash({row:id});tr.scrollIntoView({block:'center',behavior:'instant'});tr.querySelector('summary').focus({preventScroll:true});
 }
 function fromHash(){const p=new URLSearchParams(location.hash.slice(1));for(const k of inputs)$(k).value=p.get(k)||'';filter(false);if(p.get('row'))reveal(p.get('row'));}
 inputs.forEach(k=>$(k).addEventListener(k==='search'?'input':'change',()=>filter()));
 $('reset').addEventListener('click',clear);window.addEventListener('hashchange',fromHash);
 document.addEventListener('click',e=>{const link=e.target.closest('a[href^="#row="]');if(link){e.preventDefault();reveal(new URLSearchParams(link.getAttribute('href').slice(1)).get('row'));}});
 const csvCell=v=>'"'+String(v??'').replaceAll('"','""')+'"';
 $('export').addEventListener('click',()=>{
  const cols=['id','name','group','target','channel','cc','parameter','support','new','range','default','choices','context','scaling','notes','readback','source'];
  const text='\uFEFF'+[cols.map(csvCell).join(','),...visible.map(r=>cols.map(k=>csvCell(Array.isArray(r[k])?JSON.stringify(r[k]):r[k])).join(','))].join('\r\n');
  const url=URL.createObjectURL(new Blob([text],{type:'text/csv;charset=utf-8'})),a=document.createElement('a');a.href=url;a.download='fm1-0.1.22-midi-filtered.csv';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
 });
 $('print').addEventListener('click',()=>window.print());
 function valueResult(){
  if(!current)return;const raw=Number($('raw').value);
  if($('raw').value===''||!Number.isInteger(raw)||raw<0||raw>127){$('raw-result').textContent='Enter a whole number0–127.';return;}
  const r=current;
  if(!r.wire_values){$('raw-result').textContent=r.id==='motion.value'?'Target-dependent; read the active motion domain.':`Legacy: ${Math.floor((raw*3+63)/127)} · independent clocks: ${Math.floor((raw*7+63)/127)}`;return;}
  const value=r.wire_values[raw];let label='';
  if(Array.isArray(r.choices)&&r.choices[value]!==undefined)label=` · ${r.choices[value]}`;
  if(r.id==='remote.action_arm')label=value===2?' · ARM':value===0?' · DISARM':' · REJECTED arm code';
  if(r.id==='pref.SQ_TEMPO')label=` · ${value+20} BPM`;
  if(r.id==='pref.SQ_DLEN'||r.id==='pref.SQ_BLEN')label=` · ${value+1} steps`;
  if(r.id==='pref.SQ_BTRANS')label=` · ${value-24} semitones`;
  if(r.id==='ui.random_options.length')label=value===0?' · KEEP':` · ${value} steps`;
  if(r.id==='ui.random_options.note_step')label=value===0?' · ALL':` · NOTE step ${value}`;
  $('raw-result').textContent=`Raw ${raw} → canonical ${value}${label}`;
 }
 document.querySelectorAll('.inspect').forEach(b=>b.addEventListener('click',()=>{
  current=byId.get(b.dataset.id);const r=current;
  $('inspect-title').textContent=r.name;$('inspect-meta').textContent=`${r.target} · channel ${r.channel}, CC ${r.cc} · parameter ${r.parameter}`;
  $('inspect-rule').textContent=r.scaling;$('inspect-note').textContent=r.notes;$('raw').value=64;valueResult();
  const container=$('conversion-table');container.replaceChildren();
  if(r.wire_values){const table=document.createElement('table'),head=document.createElement('thead'),tr=document.createElement('tr');for(const text of ['Raw CC','Canonical value']){const th=document.createElement('th');th.textContent=text;tr.append(th);}head.append(tr);table.append(head);const body=document.createElement('tbody');r.wire_values.forEach((v,i)=>{const row=document.createElement('tr');for(const x of [i,v]){const td=document.createElement('td');td.textContent=x;row.append(td);}body.append(row);});table.append(body);container.append(table);}else container.textContent='Conversion depends on the committed context or clock mode.';
  $('inspector').showModal();
 }));
 $('raw').addEventListener('input',valueResult);$('close-inspector').addEventListener('click',()=>$('inspector').close());
 function map(){const n=Number($('map-channel').value),grid=$('address-map');grid.replaceChildren();for(let cc=0;cc<128;cc++){
  const r=rows.find(x=>x.channel===n&&x.cc===cc),el=document.createElement(r?'a':'span');el.textContent=cc;
  if(r){el.href='#row='+encodeURIComponent(r.id);el.title=`CC${cc}: ${r.target} · ${r.name}`;el.setAttribute('aria-label',el.title);}else{el.className=cc>=120||(cc>=32&&cc<64)?'reserved':'unused';el.title='CC'+cc+': '+(el.className==='reserved'?'reserved':'unassigned');}
  grid.append(el);
 }}
 $('map-channel').addEventListener('change',map);map();fromHash();
})();
