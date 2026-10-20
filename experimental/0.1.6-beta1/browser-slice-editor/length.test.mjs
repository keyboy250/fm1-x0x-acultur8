import assert from 'node:assert/strict';
import {PRESETS,demoAudio,analyseDetection,setSliceLength,exportActive,validateProfile} from './slice-core.mjs';
import {parseMarkers} from './wav-import.mjs';
const audio=demoAudio(),before=audio.pcm.slice(),p={...PRESETS[2],bandwidth:80,thresholdDb:-10};
const full=analyseDetection(audio,p),capped=analyseDetection(audio,{...p,lengthMs:150});
assert.equal(full.slices.length,4);assert.deepEqual(capped.matches,full.matches,'length cannot alter detection or trigger timing');
for(let i=0;i<capped.slices.length;i++){
 const a=full.slices[i],b=capped.slices[i];assert.equal(a.start,b.start);assert.ok(b.end-b.start<=Math.round(.15*audio.rate));assert.ok(b.end<=a.end);
}
const kickFrame=Math.round(1.1*audio.rate);
assert.ok(full.slices[0].start<kickFrame&&full.slices[0].end>kickFrame,'uncapped slice includes intervening kick');
assert.ok(capped.slices[0].end<kickFrame,'150ms length excludes intervening kick');
const edited=setSliceLength(audio,capped.slices,0,80);
assert.equal(edited[0].end-edited[0].start,Math.round(audio.rate*.08));assert.deepEqual(edited.slice(1),capped.slices.slice(1));
assert.equal(capped.slices[0].end-capped.slices[0].start,Math.round(audio.rate*.15),'manual edit does not mutate input');
const long=setSliceLength(audio,capped.slices,0,30000);assert.equal(long[0].end,capped.slices[1].start);
const last=setSliceLength(audio,capped.slices,3,30000);assert.equal(last[3].end,audio.frames);
assert.throws(()=>setSliceLength(audio,capped.slices,0,0),/length/);assert.throws(()=>setSliceLength(audio,capped.slices,0,NaN),/length/);
const inactive=capped.slices.map((s,i)=>({...s,active:i!==1})),result=setSliceLength(audio,inactive,1,25);assert.equal(result[1].active,false);
const markers=exportActive(audio,edited),parsed=parseMarkers(JSON.stringify(markers),'markers.json',audio);assert.equal(parsed.slices[0].end,edited[0].end);assert.equal(markers.slices[0].time,edited[0].start/audio.rate);
const old={...p};delete old.lengthMs;assert.equal(validateProfile(old).lengthMs,0);assert.deepEqual(analyseDetection(audio,old).slices,full.slices);
assert.deepEqual(audio.pcm,before);
console.log(JSON.stringify({status:'PASS',checks:['intervening-kick-excluded','match-and-trigger-times-preserved','preview-length-cap','individual-length-edit','next-start-source-end-clamps','active-flags-preserved','invalid-length-refusal','marker-import-compatibility','legacy-profile-uncapped','source-PCM-unchanged']}));
