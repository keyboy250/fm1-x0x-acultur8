// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {readWav,parseMarkers,exportMarkers} from '../../web/emu/slice-import.js';
const rate=44100,frames=rate*2,buffer=new ArrayBuffer(44+frames*4),d=new DataView(buffer);
const str=(p,s)=>[...s].forEach((c,i)=>d.setUint8(p+i,c.charCodeAt(0)));
str(0,'RIFF');d.setUint32(4,buffer.byteLength-8,true);str(8,'WAVE');str(12,'fmt ');d.setUint32(16,16,true);d.setUint16(20,1,true);d.setUint16(22,2,true);d.setUint32(24,rate,true);d.setUint32(28,rate*4,true);d.setUint16(32,4,true);d.setUint16(34,16,true);str(36,'data');d.setUint32(40,frames*4,true);
for(let i=0;i<frames;i++){d.setInt16(44+i*4,Math.round(Math.sin(i/10)*10000),true);d.setInt16(46+i*4,-Math.round(Math.sin(i/10)*10000),true);}
const a=readWav(buffer);
const csv='Slice,Start,End,Time\n1,0,.13,0\n2,.13,.51,.13\n3,.51,2,.51\n';
const m=parseMarkers(csv,'irregular.csv',a);
assert.deepEqual(m.slices.map(s=>s.start),[0,5733,22491]);
assert.deepEqual(m.slices.map(s=>s.time),[0,5733,22491]);assert.equal(a.channels,2);
assert.equal(a.pcm[3],-a.pcm[2]);
const rt=parseMarkers(JSON.stringify(exportMarkers(a,m)),'roundtrip.json',a);assert.deepEqual(rt,m);
assert.throws(()=>parseMarkers('Slice,Duration,Total\n0,.5,0\n1,.5,.5','rex.csv',a),/cumulative/);
assert.throws(()=>parseMarkers(JSON.stringify({slices:[{start:1},{start:.5}]}),'bad.json',a),/out-of-range|increase/);
assert.throws(()=>parseMarkers(JSON.stringify({sampleRate:48000,slices:[{start:0}]}),'bad.json',a),/sampleRate/);
assert.throws(()=>parseMarkers(JSON.stringify({slices:Array.from({length:65},(_,i)=>({start:i/100}))}),'bad.json',a),/1–64/);
assert.throws(()=>readWav(new ArrayBuffer(8)),/RIFF/);
const many=parseMarkers(JSON.stringify({slices:Array.from({length:64},(_,i)=>({start:i/32}))}),'64.json',a);assert.equal(many.slices.length,64);
const delayed=parseMarkers(JSON.stringify({loopDuration:3,slices:[{start:0,end:.1,time:.2},{start:.5,end:.6,time:1.2}]}),'delayed.json',a);
assert.equal(delayed.slices[0].time,8820);assert.equal(delayed.slices[1].time,52920);
const out=new URL('../../build/host/',import.meta.url);fs.mkdirSync(out,{recursive:true});fs.writeFileSync(new URL('marked-demo.wav',out),Buffer.from(buffer));fs.writeFileSync(new URL('marked-demo.csv',out),csv);
console.log('marker import: WAV stereo, unequal cuts, exact times, 64 slices, roundtrip and invalid/legacy rejection passed');
