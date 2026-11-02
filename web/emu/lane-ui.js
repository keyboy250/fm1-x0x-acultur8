// SPDX-License-Identifier: GPL-3.0-only
// Explicit editor selection, canonical C readback and one closed gesture per edit.
const $=id=>document.getElementById(id);
const sounds=['909 BD','909 SD','909 LT','909 MT','909 HT','909 RS','909 CP','909 CH','909 OH','909 CR','909 RD',
  '808 BD','808 SD','808 LT','808 MT','808 HT','808 RS / Clave','808 CP / Maracas','808 CB','808 CY','808 OH','808 CH','303 A','303 B','BREAK'];
const rates=['1/16','1/16 triplet','1/32','1/8 triplet','1/4','1/8','1/4 triplet','1/32 triplet','1/1','1/2','1/64','1/1 dotted','1/2 dotted','1/4 dotted','1/8 dotted','1/16 dotted','1/32 dotted','1/64 dotted','1/1 triplet','1/2 triplet','1/64 triplet'];
const rateKinds=[1,4,1,4,1,1,4,4,1,1,1,2,2,2,2,2,2,2,4,4,4],familyMasks=[1,3,5,7];
function options(select,labels){select.replaceChildren(...labels.map((label,i)=>{const o=document.createElement('option');o.value=String(i);o.textContent=label;return o;}));}
options($('lane-pattern'),Array.from({length:32},(_,i)=>`Pattern ${i}`));options($('lane-sound'),sounds);options($('lane-rate'),rates);
let pattern=0,lane=0,page=0,viewRevision=0,request=0;
let available=false,protectedProject=false,playing=true,caps=0,info=null,settings=null,mode=null,breaks=null;
let selected=false,draft=null,transaction=null,checkingSupport=true;
const pending=new Map(),rows=[];
const reached=(a,b)=>((a-b)|0)>=0;
function say(message,failed=false){checkingSupport=false;$('lane-status').textContent=message;$('lane-status').classList.toggle('lane-error',failed);}
function busy(){return !!transaction||pending.size>0;}
function editable(){return available&&selected&&!protectedProject&&!busy()&&typeof window.fm1SliceSend==='function';}
function send(op,target=0,voice=0,index=0,value=0,stage='direct'){
  request=(request+1)>>>0;if(!request)request=1;
  const command={type:'lane-command',abi:1,op,request,target,voice,index,value};
  pending.set(request,{...command,stage});window.fm1SliceSend(command);return request;
}
function selectView(){
  viewRevision=(viewRevision+1)>>>0;selected=false;settings=breaks=null;
  if(typeof window.fm1SliceSend==='function')window.fm1SliceSend({type:'lane-select',pattern,lane,page,viewRevision});
  render();
}
function edit(op,index,value){
  if(!editable()||draft!==null||!mode)return render();
  if(op===18&&!mode[0])return render();
  if(op===19&&(!mode[0]&&playing||mode[3]))return render();
  transaction={op,index,value,target:op===18?pattern:0,voice:op===18?lane:0,stage:'begin',revision:null,ticket:null,cancelled:false,failure:null};
  send(3,0,0,0,0,'begin');say('Opening one undo gesture…');render();
}
function action(op){
  if(!editable()||draft!==null||playing||!(caps&(op===7?8:4)))return render();
  transaction={op,stage:'direct',revision:null,ticket:null,failure:null};send(op);say(op===7?'Requesting project save…':'Waiting for history acknowledgement…');render();
}
function failure(result,op){
  return ({'-1':'The engine rejected this value. The last readback is restored.','-2':'This operation is unavailable in this engine.',
    '-3':op===19?'The clock-mode request is busy. First initialization requires stopped playback; try again after the pending transition.':op===5||op===6||op===7?'Stop playback before Undo, Redo or Save.':'The engine is busy. Try the edit again.',
    '-4':'Project is protected. Load a supported project before editing.','-5':'History or queue capacity is full. Try again after pending work finishes.','-6':'Project save failed.'})[result]||`Engine rejected the operation (${result}).`;
}
function settle(){
  const t=transaction;if(!t||t.stage!=='readback'||!info||!selected)return;
  if(t.ticket!==null&&!reached(info.values[4]>>>0,t.ticket))return;
  const snapshot=t.op===19?modeSnapshot:t.op===18&&t.index===2?breakSnapshot:settingsSnapshot;
  if(!snapshot||t.revision!==null&&!reached(snapshot.revision>>>0,t.revision))return;
  transaction=null;
  if(t.failure)say(t.failure,true);
  else if(t.cancelled)say('Edit cancelled before its value was sent.');
  else if(t.op===7)say('Project save acknowledged and read back.');
  else if(t.op===5||t.op===6)say(`${t.op===5?'Undo':'Redo'} acknowledged and read back.`);
  else say('Edit acknowledged and read back. Timing status below shows when playback adopts it.');
}
let settingsSnapshot=null,modeSnapshot=null,breakSnapshot=null;
function reply(m){
  const sent=pending.get(m.request);
  if(!sent||!Number.isInteger(m.result)||m.result< -6||m.result>1||!Number.isInteger(m.revision)||m.revision<0||m.revision>0xffffffff||!Number.isInteger(m.ticket)||m.ticket<0||m.ticket>0xffffffff)return;
  pending.delete(m.request);const t=transaction;if(!t)return;
  if(m.result===-4)protectedProject=true;
  if(sent.stage==='begin'){
    if(m.result<0){transaction=null;render();say(failure(m.result,3),true);return;}
    if(t.cancelled){t.stage='end';send(4,0,0,0,0,'end');}
    else {t.stage='edit';send(t.op,t.target,t.voice,t.index,t.value,'edit');say('Waiting for engine acknowledgement…');}
  }else if(sent.stage==='edit'){
    if(m.result<0)t.failure=failure(m.result,t.op);
    else {t.revision=m.revision>>>0;if(m.result===1)t.ticket=m.ticket>>>0;}
    t.stage='end';send(4,0,0,0,0,'end');
  }else {
    if(m.result<0)t.failure=failure(m.result,sent.op);
    if(sent.stage==='direct'&&m.result>=0){t.revision=m.revision>>>0;if(m.result===1)t.ticket=m.ticket>>>0;}
    t.stage='readback';
    if(t.failure)say(t.failure,true);else say('Acknowledged · waiting for applied ticket and selected C readback.');
    settle();
  }
  render();
}
for(let i=0;i<32;i++){
  const cell=document.createElement('div');cell.className='lane-step';
  const title=document.createElement('strong'),gate=document.createElement('button'),slice=document.createElement('select');
  gate.type='button';options(slice,['Generator',...Array.from({length:8},(_,s)=>`Slice ${s+1}`)]);
  gate.addEventListener('click',()=>{if(!breaks||!editable()||draft!==null)return;const on=(breaks[0]>>>i)&1,s=(breaks[1+(i>>>3)]>>>((i&7)*4))&15;edit(18,2,(page*32+i)|((on^1)<<6)|(s<<7));});
  slice.addEventListener('change',()=>{const value=Number(slice.value);if(!breaks||!Number.isInteger(value)||value<0||value>8)return render();edit(18,2,(page*32+i)|(((breaks[0]>>>i)&1)<<6)|(value<<7));});
  cell.append(title,gate,slice);$('lane-grid').append(cell);rows.push({cell,title,gate,slice});
}
for(const [id,key] of [['lane-pattern','pattern'],['lane-sound','lane']])$(id).addEventListener('change',()=>{
  const value=Number($(id).value);if(!available||busy()||draft!==null||!Number.isInteger(value)||value<0||value>=(key==='pattern'?32:25))return render();
  if(key==='pattern')pattern=value;else lane=value;selectView();
});
for(const [id,next] of [['lane-previous',0],['lane-next',1]])$(id).addEventListener('click',()=>{if(!available||busy()||draft!==null||lane!==24||page===next)return;page=next;selectView();});
$('lane-length').addEventListener('input',()=>{if(!editable()||!mode?.[0])return render();draft=$('lane-length').value;controls();});
function commitLength(){
  if(draft===null)return;const text=draft;draft=null;const value=Number(text);
  if(text.trim()===''||!Number.isInteger(value)||value<1||value>64){render();say('Length must be a whole number from 1 to 64.',true);return;}
  edit(18,0,value);
}
$('lane-length').addEventListener('change',commitLength);$('lane-length').addEventListener('blur',commitLength);
$('lane-rate').addEventListener('change',()=>{const value=Number($('lane-rate').value);if(!Number.isInteger(value)||value<0||value>=(mode?.[5]??8))return render();edit(18,mode?.[5]===21?3:1,value);});
$('lane-rate-enable').addEventListener('click',()=>{if(!playing&&mode?.[0]&&mode[5]===8)edit(18,8,1);});
for(const [id,index] of [['lane-rate-family',4],['lane-rate-boundary',5],['lane-rate-hysteresis',6]])$(id).addEventListener('change',()=>edit(18,index,Number($(id).value)));
$('lane-mode').addEventListener('click',()=>edit(19,0,mode?.[1]?0:1));
for(const [id,op] of [['lane-undo',5],['lane-redo',6],['lane-save',7]])$(id).addEventListener('click',()=>action(op));
for(const event of ['blur','pagehide','pointercancel'])window.addEventListener(event,()=>{
  // Never leave BEGIN acknowledged without END. If value was already sent, close
  // it after its reply; a cancellation does not pretend an accepted edit vanished.
  if(transaction?.stage==='begin')transaction.cancelled=true;
  if(draft!==null){draft=null;render();say('Uncommitted length draft discarded.');}
});
function controls(){
  const lock=!editable(),drafting=draft!==null;
  $('lane-selection').disabled=!available||busy()||drafting;
  $('lane-mode').disabled=lock||drafting||!mode||!!mode[3]||(!mode[0]&&playing);
  $('lane-rate-enable').disabled=lock||drafting||playing||!mode?.[0]||mode[5]===21;
  $('lane-rate-policy').disabled=lock||drafting||mode?.[5]!==21;
  $('lane-timing').disabled=lock||!mode?.[0];$('lane-rate').disabled=drafting;
  $('lane-break-controls').disabled=lock||drafting||!mode?.[0]||!breaks;
  $('lane-previous').disabled=!available||busy()||drafting||page===0;$('lane-next').disabled=!available||busy()||drafting||page===1;
  for(const [id,bit] of [['lane-undo',4],['lane-redo',4],['lane-save',8]])$(id).disabled=lock||drafting||playing||!(caps&bit);
}
function render(){
  $('lane-pattern').value=String(pattern);$('lane-sound').value=String(lane);
  $('lane-break-panel').hidden=lane!==24;$('lane-page-label').textContent=`Steps ${page*32+1}–${page*32+32} of 64`;
  if(mode){
    $('lane-mode').textContent=!mode[0]?'Initialize independent lanes':mode[1]?'Use grouped kit clocks':'Use independent sound clocks';
    $('lane-mode-state').textContent=`Saved choice: ${mode[1]?'independent':'grouped'} · Effective: ${mode[2]?'independent':'grouped'}${mode[3]?' · Pending next musical bar':''}${!mode[0]?' · Not initialized; stop to initialize':''}`;
  }
  if(settings){
    if(draft===null)$('lane-length').value=String(settings[1]);
    if($('lane-rate').children.length!==(mode?.[5]??8)){if(mode?.[5]===21)$('lane-rate').replaceChildren(...[11,8,12,18,9,13,19,4,14,6,5,15,3,0,16,1,2,17,7,10,20].map(i=>{const o=document.createElement('option');o.value=String(i);o.textContent=rates[i];return o;}));else options($('lane-rate'),rates.slice(0,8));}
    if(settings.length===13){$('lane-rate-family').value=String(settings[9]);$('lane-rate-boundary').value=String(settings[10]);$('lane-rate-hysteresis').value=String(settings[11]);for(const o of $('lane-rate').options)o.disabled=!(familyMasks[settings[9]]&rateKinds[Number(o.value)])&&Number(o.value)!==settings[2];}
    $('lane-rate').value=String(settings[2]);
    $('lane-position').textContent=`Editing pattern ${pattern} · ${sounds[lane]} · Playing pattern ${settings[3]} · ${playing&&settings[4]!==255?`step ${settings[4]+1}`:'stopped / no local step'} · cycles ${settings[5]}`;
    $('lane-timing-state').textContent=!mode?.[0]?'Initialize the lanes to edit their timing.':settings[3]!==pattern?'Prepared settings for this pattern; it is not the playing pattern.':settings[8]?settings.length!==13?'Timing read back · waiting for this sound’s next local step.':`Selected ${rates[settings[2]]}; effective ${rates[settings[6]]}. Waiting for ${settings[10]?'the next sound step after the next bar':'this sound’s next local step'}.`:`Timing ready · effective step rate ${rates[settings[6]]??'unknown'}${!settings[7]&&lane<22?' (kit BD conductor; this sound’s individual timing stays saved)':''}.`;
  }else {$('lane-position').textContent=`Editing pattern ${pattern} · ${sounds[lane]} · waiting for selected readback.`;$('lane-timing-state').textContent='Waiting for selected timing.';}
  for(const [i,row] of rows.entries()){
    const step=page*32+i,on=breaks?!!((breaks[0]>>>i)&1):false,slice=breaks?(breaks[1+(i>>>3)]>>>((i&7)*4))&15:0;
    row.title.textContent=`Step ${step+1}`;row.gate.textContent=on?'Gate on':'Gate off';row.gate.setAttribute('aria-pressed',String(on));row.gate.setAttribute('aria-label',`Step ${step+1} gate`);row.slice.value=String(slice);row.slice.setAttribute('aria-label',`Step ${step+1} slice`);
    row.cell.classList.toggle('lane-outside',!!settings&&step>=settings[1]);row.cell.classList.toggle('lane-playing',!!settings&&playing&&settings[3]===pattern&&settings[4]===step);
  }
  controls();
}
function validSnapshot(s,section,count){
  if(!(s?.abi===1&&s.section===section&&Number.isInteger(s.revision)&&s.revision>=0&&s.revision<=0xffffffff&&Array.isArray(s.values)&&s.values.length===count&&s.values.every(v=>Number.isInteger(v)&&v>=0&&v<=0xffffffff)))return false;
  const v=s.values;
  if(section===11)return v.slice(0,4).every(x=>x<=1)&&v[4]===25&&(v[5]===8||v[5]===21)&&v[6]===64;
  if(section===10)return v[0]<=1&&v[1]>=1&&v[1]<=64&&v[2]<(count===13?21:8)&&v[3]<32&&(v[4]<64||v[4]===255)&&v[6]<(count===13?21:8)&&v[7]<=1&&v[8]<=1&&(count!==13||(v[9]<4&&v[10]<2&&v[11]<3&&v[12]===1));
  if(section===12)return v.slice(1).every(word=>Array.from({length:8},(_,i)=>(word>>>(i*4))&15).every(x=>x<=8));
  return true;
}
window.addEventListener('fm1-ready',()=>{
  pending.clear();transaction=null;draft=null;available=selected=false;protectedProject=false;playing=true;caps=0;info=settings=mode=breaks=null;settingsSnapshot=modeSnapshot=breakSnapshot=null;
  $('lane-mode-state').textContent='Clock mode unavailable.';$('lane-project').textContent='';say('Checking lane support…');checkingSupport=true;selectView();
});
window.addEventListener('fm1-marked',event=>{
  const m=event.detail;if(!m||typeof m!=='object')return;if(m.type==='lane-reply'){reply(m);return;}if(m.type!=='frame')return;
  const v=m.variant;info=validSnapshot(v?.info,0,8)?v.info:null;caps=info?.values[0]??0;protectedProject=!!info?.values[2];playing=m.sync?.playing!==false;
  const view=v?.laneView;
  available=!!(caps&1024)&&validSnapshot(view?.mode,11,7);
  modeSnapshot=available?view.mode:null;mode=modeSnapshot?.values??null;
  selected=available&&view.pattern===pattern&&view.lane===lane&&view.page===page&&view.viewRevision===viewRevision&&validSnapshot(view.settings,10,view.mode.values[5]===21?13:9);
  settingsSnapshot=selected?view.settings:null;settings=settingsSnapshot?.values??null;
  breakSnapshot=selected&&validSnapshot(view.breaks,12,5)?view.breaks:null;breaks=breakSnapshot?.values??null;
  $('lane-availability').textContent=protectedProject?'Project is protected. Load a supported project before editing.':available?'All 25 sound lanes are supported by this engine.':'Lane editing is unavailable in this engine. Existing instrument controls remain available.';
  $('lane-project').textContent=info?`Project ${info.values[1]?'has unsaved changes':'matches its saved state'} · revision ${info.revision}`:'';
  settle();
  // Support alone is insufficient: wait for this selection's validated readback.
  // Any transaction, draft or error message retires the initial checking status.
  if(checkingSupport&&selected)say('Lane support checked. Selected sound timing read back.');
  render();
});
render();
