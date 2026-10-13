// SPDX-License-Identifier: GPL-3.0-only
// Check a real standalone conversion through the browser importer and WASM scheduler.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {readWav,parseMarkers} from '../../web/emu/slice-import.js';
const [wasmPath,jsonPath]=process.argv.slice(2);
const raw=JSON.parse(fs.readFileSync(jsonPath,'utf8'));
const bytes=fs.readFileSync(path.join(path.dirname(jsonPath),raw.audioFile));
assert.equal(createHash('sha256').update(bytes).digest('hex'),raw.audioSha256);
const audio=readWav(bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength));
const markers=parseMarkers(JSON.stringify(raw),'original.json',audio);
const csv=parseMarkers(fs.readFileSync(jsonPath.replace(/\.json$/,'')+'.csv','utf8'),'original.csv',audio);
assert.deepEqual(csv.slices.map(s=>[s.start,s.end,s.time]),markers.slices.map(s=>[s.start,s.end,s.time]));
assert.throws(()=>parseMarkers(JSON.stringify({...raw,audioFrames:raw.audioFrames+1}),'wrong.json',audio),/length/i);
assert.throws(()=>parseMarkers(JSON.stringify({...raw,channels:raw.channels===2?1:2}),'wrong.json',audio),/channel/i);
let memory;
const {instance}=await WebAssembly.instantiate(fs.readFileSync(wasmPath),{wasi_snapshot_preview1:{clock_time_get:(a,b,p)=>{new DataView(memory.buffer).setBigUint64(p,0n,true);return 0;}}});
const e=instance.exports;memory=e.memory;e._initialize();e.web_boot();
new Float32Array(memory.buffer,e.web_marked_pcm(),audio.pcm.length).set(audio.pcm);
new Uint32Array(memory.buffer,e.web_marked_meta(),markers.slices.length*3).set(markers.slices.flatMap(s=>[s.start,s.end,s.time]));
assert.equal(e.web_marked_load(audio.frames,audio.rate,audio.channels,markers.slices.length,markers.length),0);
e.web_marked_play(0);let cursor=0,peak=0;
function render(n){while(n){const count=Math.min(n,128);e.web_marked_render(count);for(const v of new Float32Array(memory.buffer,e.web_marked_l(),count))peak=Math.max(peak,Math.abs(v));n-=count;cursor+=count;}}
for(let i=0;i<markers.slices.length;i++){
  const when=markers.slices[i].time;
  render(Math.max(0,when-cursor));
  // Multiple simultaneous slices may fire together; this fixture has distinct onsets.
  assert.equal(e.web_marked_triggers(),i,'no early trigger at '+when);
  render(1);assert.equal(e.web_marked_triggers(),i+1,'trigger on exact output sample '+when);
}
render(markers.length-cursor);assert.equal(e.web_marked_playing(),0);assert.equal(e.web_marked_cursor(),markers.length);assert.ok(peak>0);
console.log(JSON.stringify({slices:markers.slices.length,loopSamples:markers.length,peak,exactOriginalTriggerSamples:markers.slices.map(s=>s.time),csvJsonEquivalent:true}));
