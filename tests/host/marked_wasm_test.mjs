// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';import assert from 'node:assert/strict';
const bytes=fs.readFileSync(process.argv[2]);let mem;
const {instance}=await WebAssembly.instantiate(bytes,{wasi_snapshot_preview1:{clock_time_get:(a,b,p)=>{new DataView(mem.buffer).setBigUint64(p,0n,true);return 0;}}});
const e=instance.exports;mem=e.memory;e._initialize();e.web_boot();
const pcm=new Float32Array(mem.buffer,e.web_marked_pcm(),128);for(let i=0;i<64;i++){pcm[i*2]=i/100;pcm[i*2+1]=-i/100;}
new Uint32Array(mem.buffer,e.web_marked_meta(),9).set([0,4,0,8,13,11,16,21,23]);
assert.equal(e.web_marked_load(64,44100,2,3,32),0);e.web_marked_play(0);e.web_marked_render(32);
const l=new Float32Array(mem.buffer,e.web_marked_l(),32),r=new Float32Array(mem.buffer,e.web_marked_r(),32);
assert.equal(e.web_marked_triggers(),3);assert.equal(e.web_marked_cursor(),32);assert.equal(e.web_marked_playing(),0);
assert.equal(l[4],0);assert.equal(l[10],0);assert.ok(Math.abs(l[11]-.08)<1e-6);assert.ok(Math.abs(r[23]+.16)<1e-6);assert.equal(l[28],0);
e.web_marked_play(1);e.web_marked_render(64);assert.equal(e.web_marked_triggers(),6);e.web_marked_stop();e.web_marked_render(32);assert.ok(new Float32Array(mem.buffer,e.web_marked_l(),32).every(v=>v===0));
e.web_marked_pad(2);e.web_marked_render(8);assert.ok(Math.abs(new Float32Array(mem.buffer,e.web_marked_l(),8)[0]-.16)<1e-6);
console.log('marked WebAssembly: exact off-grid trigger samples, stereo slice boundaries, repeat, stop and pad passed');
