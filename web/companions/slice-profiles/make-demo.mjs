import {writeFile} from 'node:fs/promises';
import {demoAudio} from './slice-core.mjs';
const a=demoAudio(),buf=Buffer.alloc(44+a.frames*2);
buf.write('RIFF',0);buf.writeUInt32LE(buf.length-8,4);buf.write('WAVEfmt ',8);buf.writeUInt32LE(16,16);buf.writeUInt16LE(1,20);buf.writeUInt16LE(1,22);buf.writeUInt32LE(a.rate,24);buf.writeUInt32LE(a.rate*2,28);buf.writeUInt16LE(2,32);buf.writeUInt16LE(16,34);buf.write('data',36);buf.writeUInt32LE(a.frames*2,40);
for(let i=0;i<a.frames;i++)buf.writeInt16LE(Math.round(a.pcm[i]*32767),44+i*2);
await writeFile(new URL('./demo.wav',import.meta.url),buf);
