import assert from 'node:assert/strict';
import {PRESETS,detectSlices,demoAudio,exportActive,validateSlices,validateProfile} from './slice-core.mjs';
import {parseMarkers} from './wav-import.mjs';
const audio=demoAudio(), before=audio.pcm.slice(),full=detectSlices(audio,PRESETS[0]);
assert.equal(full.length,8,'eight known synthetic attacks');
for(let i=0;i<8;i++)assert.ok(Math.abs(full[i].start/audio.rate-(.2+i*.45))<.025,'boundary preserves attack with bounded lookback');
const kick=detectSlices(audio,PRESETS[1]),snare=detectSlices(audio,PRESETS[2]);
assert.ok(kick.length>=4);assert.ok(snare.length>=4);
// A narrow band separates alternating low and high sine bursts, without modifying PCM.
const narrowKick=detectSlices(audio,{...PRESETS[1],bandwidth:30,thresholdDb:-10});
const narrowSnare=detectSlices(audio,{...PRESETS[2],bandwidth:80,thresholdDb:-10});
assert.equal(narrowKick.length,4);assert.equal(narrowSnare.length,4);
assert.ok(narrowKick.every((s,i)=>Math.abs(s.start/audio.rate-(.2+i*.9))<.03));
assert.ok(narrowSnare.every((s,i)=>Math.abs(s.start/audio.rate-(.65+i*.9))<.03));
assert.deepEqual(audio.pcm,before,'analysis leaves original untouched');
assert.deepEqual(detectSlices({...audio,pcm:new Float32Array(audio.frames)},PRESETS[0]),[]);
const stereo=new Float32Array(audio.frames*2);for(let i=0;i<audio.frames;i++){stereo[2*i]=audio.pcm[i];stereo[2*i+1]=-audio.pcm[i];}
assert.equal(detectSlices({...audio,pcm:stereo,channels:2},PRESETS[0]).length,8,'opposite stereo polarity does not erase attacks');
const edited=full.map((s,i)=>({...s,active:i!==1})),markers=exportActive(audio,edited);
assert.equal(markers.slices.length,7);assert.equal(edited.length,8);assert.equal(edited[1].active,false);
const parsed=parseMarkers(JSON.stringify(markers),'demo.json',audio);assert.equal(parsed.slices.length,7);
assert.equal(markers.slices[1].time,full[2].start/audio.rate,'inactive gaps preserved');
assert.throws(()=>exportActive(audio,full.map(s=>({...s,active:false}))),/Activate/);
assert.throws(()=>validateSlices([{...full[0],end:audio.frames+1}],audio),/boundaries/);
assert.throws(()=>validateSlices([full[1],full[0]],audio),/boundaries/);
assert.throws(()=>validateProfile({...PRESETS[0],thresholdDb:NaN}),/thresholdDb/);
assert.throws(()=>detectSlices(demoAudio(8000),PRESETS[4]),/frequency/);
console.log(JSON.stringify({status:'PASS',syntheticOnsets:full.length,narrowKick:narrowKick.length,narrowSnare:narrowSnare.length,compatibleExport:parsed.slices.length,checks:['onsets','frequency-selection','stereo-phase','silence','source-preservation','active-filter','original-timing','existing-importer','bounds','invalid-profile','sample-rate-limit']}));
