// SPDX-License-Identifier: GPL-3.0-only
// Shared browser sound renderer; no native firmware or WASM changes.
import {renderSlice,playbackSettings,validateSlices,validateProfile,PRESETS} from './slice-core.mjs';
import {parseMarkers} from './slice-import.js';
export async function hashAudio(a){
 const head=new TextEncoder().encode(`${a.rate}:${a.channels}:`),pcm=new Uint8Array(a.pcm.buffer,a.pcm.byteOffset,a.pcm.byteLength),all=new Uint8Array(head.length+pcm.length);
 all.set(head);all.set(pcm,head.length);
 return [...new Uint8Array(await crypto.subtle.digest('SHA-256',all))].map(v=>v.toString(16).padStart(2,'0')).join('');
}
export async function importSettings(text,name,audio){
 let raw;try{raw=JSON.parse(text);}catch{}
 if(raw?.type==='acultur8-slice-review'){
  if(![1,2].includes(raw.version))throw Error('Unsupported review project version.');
  if(raw.audio?.rate!==audio.rate||raw.audio?.channels!==audio.channels||raw.audio?.frames!==audio.frames||raw.audio?.sha256!==await hashAudio(audio))throw Error('This review project belongs to a different WAV. Current import is unchanged.');
  const profile=validateProfile(raw.profile),slices=validateSlices(raw.slices,audio);
  const loopDuration=raw.loopDuration??audio.duration;
  if(!Number.isFinite(loopDuration)||loopDuration<=0||loopDuration>60)throw Error('Invalid loop duration.');
  const markers={slices:slices.map((s,i)=>({...s,time:Math.round((raw.slices[i].time??s.start/audio.rate)*44100)})),length:Math.round(loopDuration*44100),loopDuration,bpm:raw.bpm??null};
  const model={audio,markers,profile,fadeMs:profile.fadeMs};compileBank(model);return model;
 }
 const markers=parseMarkers(text,name,audio);
 markers.slices=markers.slices.map(s=>({...s,active:true,playback:playbackSettings()}));
 return {audio,markers,profile:{...PRESETS[0],fadeMs:0},fadeMs:0};
}
export function compileBank(model){
 const {audio,markers}=model;
 if(!markers.slices.length)throw Error('Import at least one slice.');
 const validated=validateSlices(markers.slices,audio);
 const fadeMs=model.fadeMs??0;
 if(!Number.isFinite(fadeMs)||fadeMs<0||fadeMs>20)throw Error('Invalid edge fade.');
 const rendered=[],all=[];let frames=0,lastTime=-1;
 for(let i=0;i<validated.length;i++){
  const s=validated[i],time=markers.slices[i].time;
  if(!Number.isInteger(time)||time<0||time>=markers.length||time<lastTime)throw Error('Invalid or unordered trigger time.');lastTime=time;
  const sound=renderSlice(audio,s,fadeMs),size=Math.max(1,sound.frames);
  if(frames+size>1440000)throw Error('Shaped slice bank exceeds 1,440,000 source frames. Shorten overlapping slice lengths; nothing was truncated.');
  all.push({start:frames,end:frames+size,time,active:s.active,label:s.label});rendered.push(sound);frames+=size;
 }
 const pcm=new Float32Array(frames*audio.channels);let offset=0;
 for(let i=0;i<rendered.length;i++){pcm.set(rendered[i].pcm,offset*audio.channels);offset=all[i].end;}
 // Validate active tails at original timing. The sync engine also guards overlaps at changed tempos.
 const active=all.filter(s=>s.active);
 if(active.length)parseMarkers(JSON.stringify({sampleRate:audio.rate,loopDuration:markers.loopDuration,slices:active.map(s=>({startSample:s.start,endSample:s.end,time:s.time/44100}))}),'bank.json',{...audio,frames,duration:frames/audio.rate});
 return {pcm,frames,rate:audio.rate,channels:audio.channels,slices:all,length:markers.length};
}
export async function exportProject(model){
 const {audio,markers}=model;
 return {type:'acultur8-slice-review',version:2,audio:{name:model.name,sha256:await hashAudio(audio),rate:audio.rate,channels:audio.channels,frames:audio.frames},profile:{...model.profile,fadeMs:model.fadeMs},loopDuration:markers.loopDuration,bpm:markers.bpm,
 slices:markers.slices.map(s=>({...s,time:s.time/44100,playback:playbackSettings(s.playback)}))};
}
