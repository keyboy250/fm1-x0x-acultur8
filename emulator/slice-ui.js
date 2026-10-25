// SPDX-License-Identifier: GPL-3.0-only
import {readWav,parseMarkers} from './slice-import.js';
import {SliceService,selectedImport,syntheticImport} from './slice-control.mjs';
const $=id=>document.getElementById(id);
let imported=null,ready=false,restoring=false,restoreEpoch=-1;
const say=(text,error=false)=>{ $('slice-status').textContent=text;$('slice-status').classList.toggle('slice-error',error); };
function send(message){window.fm1SliceSend(message);}
function controls(){for(const id of ['slice-preview-play','slice-export','slice-save'])$(id).disabled=!imported||!ready||service.busy;$('slice-preview-stop').disabled=!service.ready();$('slice-sync-play').disabled=!ready||service.busy;$('slice-list').querySelectorAll('button').forEach(b=>b.disabled=!ready||service.busy||!!service.state?.owned||!!(service.state?.gate&28));}
const service=new SliceService({send,files:()=>[$('slice-audio').files[0],$('slice-markers').files[0]],notify:say,
 changed:s=>{const changed=imported!==s.imported;imported=s.imported;ready=s.ready()&&!!s.state.loaded;if(changed&&imported)draw();controls();},
 download:(json,cc)=>{$('slice-export-json').value=json;$('slice-export-output').hidden=false;$('slice-export-output').open=true;if(cc)return false;
 const url=URL.createObjectURL(new Blob([json],{type:'application/json'})),a=document.createElement('a');a.href=url;a.download='fm1-slice-markers.json';document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),2000);return true;}});
window.fm1SliceControl=service;
async function act(index,value){try{const result=await service.perform(index,value);say(result.reason);}catch(e){say(e.reason||e.message,true);}}
async function adopt(candidate){if(service.powered){await service.replace(candidate);say('Ready: exact markers loaded.');}else{service.imported=candidate;service.update();say('Import prepared. Switch on the FM1 panel to load it.');}}
async function restore(){if(restoring||restoreEpoch===service.epoch||!service.ready()||service.state.loaded||!service.imported||service.state.gate||service.state.owned)return;restoring=true;restoreEpoch=service.epoch;try{await service.replace(service.imported);say('Retained import loaded into the engine.');}catch(e){say(e.reason||e.message,true);}finally{restoring=false;}}
function draw(){
  if(!imported)return;
  const {audio:a,markers:m}=imported;
  $('slice-info').textContent=`${imported.name} · ${m.slices.length} original slices · ${a.rate} Hz · ${a.channels===2?'stereo':'mono'} · ${m.loopDuration.toFixed(3)} s${m.bpm?' · '+m.bpm+' BPM':''}`;
  const canvas=$('slice-wave'),ctx=canvas.getContext('2d'),w=canvas.width,h=canvas.height;
  ctx.fillStyle='#0b0e12';ctx.fillRect(0,0,w,h);ctx.strokeStyle='#81b9bd';ctx.beginPath();
  for(let x=0;x<w;x++){
    const lo=Math.floor(x*a.frames/w),hi=Math.max(lo+1,Math.floor((x+1)*a.frames/w));let peak=0;
    for(let i=lo;i<hi;i++)for(let c=0;c<a.channels;c++)peak=Math.max(peak,Math.abs(a.pcm[i*a.channels+c]));
    ctx.moveTo(x,h/2-peak*(h/2-10));ctx.lineTo(x,h/2+peak*(h/2-10));
  }ctx.stroke();ctx.strokeStyle='#f0a043';
  for(const s of m.slices){const x=s.start/a.frames*w;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();}
  $('slice-list').replaceChildren(...m.slices.map((s,i)=>{
    const b=document.createElement('button');b.type='button';b.dataset.slice=String(i);b.disabled=!ready;
    b.textContent=`${i+1} · ${s.label} · ${(s.start/a.rate).toFixed(3)}–${(s.end/a.rate).toFixed(3)} s · trigger ${(s.time/44100).toFixed(3)} s`;
    b.onclick=()=>act(4,i);return b;
  }));controls();
}
$('slice-import-button').onclick=async()=>{try{await adopt(await selectedImport($('slice-audio').files[0],$('slice-markers').files[0]));}catch(e){say(e.reason||e.message,true);}};
$('slice-demo').onclick=()=>adopt(syntheticImport()).catch(e=>say(e.reason||e.message,true));
$('slice-preview-play').onclick=()=>act(5,1);
$('slice-preview-stop').onclick=()=>act(6,1);
$('slice-sync-play').onclick=()=>act(7,1);
$('slice-source-bpm').onchange=()=>{const bpm=Number($('slice-source-bpm').value);if(!Number.isFinite(bpm)||bpm<20||bpm>300){say('Source BPM must be 20–300.',true);return;}act(3,Math.round(bpm*1000));};
$('slice-repeat').onchange=()=>act(1,Number($('slice-repeat').checked));
$('slice-layer').onchange=()=>act(2,Number($('slice-layer').checked));
$('slice-volume').oninput=()=>act(0,Math.round(Number($('slice-volume').value)*100));
$('slice-tempo-set').onclick=()=>{const bpm=Number($('slice-seq-bpm').value);if(!Number.isFinite(bpm)||bpm<20||bpm>275){say('FM1 tempo must be 20–275 BPM.',true);return;}if(!service.powered){say('Switch on the emulator first.',true);return;}send({type:'seq-tempo',bpm});};
$('slice-save').onclick=()=>act(10,1);$('slice-reopen').onclick=()=>act(11,1);$('slice-export').onclick=()=>act(12,1);
window.addEventListener('fm1-ready',()=>{service.powered=true;service.update();});
window.addEventListener('fm1-marked',event=>{const m=event.detail;if(m?.type==='cc-input-reset'){service.engineReset();return;}service.onMessage(m);
 if(m?.type==='frame'){
  if(m.ccInput?.slice){const s=m.ccInput.slice;if(s.version===0x10001){service.powered=true;service.update();restore();}
   if(document.activeElement!==$('slice-source-bpm'))$('slice-source-bpm').value=s.sourceBpm??service.imported?.markers.bpm??'';
   $('slice-repeat').checked=!!s.repeat;$('slice-layer').checked=!!s.layer;if(document.activeElement!==$('slice-volume'))$('slice-volume').value=s.gain;}
  if(m.sync){const s=m.sync;$('slice-sync-status').textContent=s.state<0?'Sync stopped: slice overlap exceeds capacity.':`FM1 ${s.bpm.toFixed(1)} BPM · ${s.playing?'Playing':'Stopped'} · ${s.state===1?'Waiting for next 4-beat bar':s.state===2?'Slices follow sequencer · original pitch':'Slice sync off'}`;if(document.activeElement!==$('slice-seq-bpm'))$('slice-seq-bpm').value=Math.round(s.bpm*10)/10;}
  if(m.marked&&imported){const p=m.marked;$('slice-playhead').textContent=`${p.playing?'Playing':'Stopped'} · ${(p.cursor/44100).toFixed(3)} s · ${p.triggers} triggers`;$('slice-list').querySelectorAll('button').forEach(b=>b.classList.toggle('slice-active',Number(b.dataset.slice)===p.slice));}
 }
});controls();


if(new URLSearchParams(location.search).has('rex')) { say('Import the matching WAV and JSON files here. Raw REX conversion is available in the local workbench.', true); }
