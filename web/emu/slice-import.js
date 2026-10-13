// SPDX-License-Identifier: GPL-3.0-only
// WAV and exact timing metadata. No REX decoding and no transient guessing.
export function readWav(buffer) {
  const d=new DataView(buffer), ascii=(p,n)=>String.fromCharCode(...new Uint8Array(buffer,p,n));
  if(d.byteLength<44||ascii(0,4)!=='RIFF'||ascii(8,4)!=='WAVE') throw Error('Choose a RIFF WAV file. Direct REX decoding is not included.');
  let fmt=null, data=null;
  for(let p=12;p+8<=d.byteLength;) {
    const size=d.getUint32(p+4,true),end=p+8+size;
    if(end>d.byteLength) throw Error('Truncated WAV chunk.');
    const id=ascii(p,4);
    if(id==='fmt ') { if(size<16) throw Error('Invalid WAV format.'); fmt={type:d.getUint16(p+8,true),channels:d.getUint16(p+10,true),rate:d.getUint32(p+12,true),align:d.getUint16(p+20,true),bits:d.getUint16(p+22,true)}; }
    if(id==='data') data={offset:p+8,size};
    p=end+(size&1);
  }
  if(!fmt||!data) throw Error('WAV needs fmt and data chunks.');
  const {type,channels,rate,align,bits}=fmt;
  if(![1,2].includes(channels)||rate<8000||rate>48000||!([8,16,24,32].includes(bits)&&type===1||bits===32&&type===3)||align!==channels*(bits/8)) throw Error('Supported WAV: mono/stereo PCM 8/16/24/32-bit or float32, 8–48 kHz.');
  if(data.size%align) throw Error('WAV data has an incomplete frame.');
  const frames=data.size/align;
  if(!frames||frames>1440000||frames/rate>30) throw Error('Emulator preview supports WAVs up to 30 seconds. Nothing is truncated.');
  const pcm=new Float32Array(frames*channels),bytes=bits/8;
  for(let i=0;i<pcm.length;i++) {
    const p=data.offset+i*bytes;
    let v;
    if(type===3) v=d.getFloat32(p,true);
    else if(bits===8) v=(d.getUint8(p)-128)/128;
    else if(bits===16) v=d.getInt16(p,true)/32768;
    else if(bits===24) { let q=d.getUint8(p)|(d.getUint8(p+1)<<8)|(d.getUint8(p+2)<<16); if(q&0x800000) q-=0x1000000; v=q/8388608; }
    else v=d.getInt32(p,true)/2147483648;
    if(!Number.isFinite(v)||Math.abs(v)>1) throw Error('WAV contains non-finite or out-of-range audio samples.');
    pcm[i]=v;
  }
  return {pcm,frames,rate,channels,duration:frames/rate};
}
function number(value,label) { if(value===null||value===undefined||value===''||!Number.isFinite(Number(value))) throw Error('Invalid '+label+'.'); return Number(value); }
function csvRows(text) {
  const rows=[];let row=[],cell='',quoted=false;
  for(let i=0;i<=text.length;i++) {
    const c=text[i]??'\n';
    if(c==='"') { if(quoted&&text[i+1]==='"'){cell+='"';i++;}else quoted=!quoted; }
    else if(!quoted&&(c===','||c==='\n')) {row.push(cell.trim());cell='';if(c==='\n'){if(row.some(Boolean))rows.push(row);row=[];}}
    else if(c!=='\r')cell+=c;
  }
  if(quoted)throw Error('Unclosed CSV quote.');return rows;
}
export function parseMarkers(text,name,audio) {
  if(text.length>262144)throw Error('Marker file exceeds 256 KiB.');
  let raw, warnings=[];
  if(name.toLowerCase().endsWith('.json')||text.trim().startsWith('{')) raw=JSON.parse(text);
  else {
    const rows=csvRows(text.replace(/^\uFEFF/,'')),header=rows.shift()?.map(s=>s.toLowerCase());
    if(!header) throw Error('Marker CSV is empty.');
    if(header.includes('total')&&!header.includes('start')) throw Error('This rex2wav CSV stores cumulative durations, not original REX onsets. Use exact Start markers (seconds) for the full WAV; original groove cannot be recovered from Total alone.');
    const index=k=>header.indexOf(k), get=(r,k)=>index(k)<0?undefined:r[index(k)];
    if(index('start')<0)throw Error('CSV needs Start in seconds, plus optional End, Duration, Time and Slice columns.');
    const slices=[];let loop;
    for(const r of rows) {
      if(r[0].toLowerCase()==='loop'){loop=number(r[r.length-1],'loop duration');continue;}
      const start=number(get(r,'start'),'Start');
      const end=get(r,'end'),duration=get(r,'duration'),time=get(r,'time');
      slices.push({start,end:end?number(end,'End'):duration?start+number(duration,'Duration'):undefined,time:time?number(time,'Time'):start,label:get(r,'slice')||String(slices.length+1)});
    }
    raw={slices,loopDuration:loop};
  }
  if(!raw||!Array.isArray(raw.slices)||raw.slices.length<1||raw.slices.length>64)throw Error('Provide 1–64 slices. Nothing is reduced to eight slices.');
  if(raw.sampleRate!==undefined&&number(raw.sampleRate,'sampleRate')!==audio.rate)throw Error('Marker sampleRate does not match the WAV.');
  if(raw.audioFrames!==undefined&&number(raw.audioFrames,'audioFrames')!==audio.frames)throw Error('This marker file belongs to a different WAV length. Choose the matching _slices.wav file.');
  if(raw.channels!==undefined&&number(raw.channels,'channels')!==audio.channels)throw Error('Marker channel count does not match the WAV.');
  const loopDuration=raw.loopDuration===undefined?audio.duration:number(raw.loopDuration,'loopDuration');
  if(loopDuration<=0||loopDuration>60)throw Error('Loop duration must be greater than zero and at most 60 seconds.');
  const length=Math.round(loopDuration*44100), slices=raw.slices.map((s,i)=>{
    const start=s.startSample!==undefined?number(s.startSample,'startSample'):number(s.start,'start')*audio.rate;
    const next=raw.slices[i+1];
    const end=s.endSample!==undefined?number(s.endSample,'endSample'):s.end!==undefined?number(s.end,'end')*audio.rate:next?(next.startSample!==undefined?number(next.startSample,'startSample'):number(next.start,'start')*audio.rate):audio.frames;
    const time=s.time===undefined?start/audio.rate:number(s.time,'time');
    if(start<0||end>audio.frames+0.5||start>=end||time<0||time>=loopDuration)throw Error('Slice '+(i+1)+' has out-of-range boundaries or timing.');
    const result={start:Math.round(start),end:Math.round(end),time:Math.round(time*44100),label:String(s.label??i+1).slice(0,48)};
    if(result.start>=result.end||result.end>audio.frames||result.time>=length)throw Error('Slice '+(i+1)+' is too short or outside the WAV after sample rounding.');
    return result;
  });
  for(let i=1;i<slices.length;i++)if(slices[i].start<=slices[i-1].start||slices[i].time<slices[i-1].time)throw Error('Source starts must increase; trigger times must be in playback order.');
  // Validate cyclic overlaps too: the PCM audition engine has eight independent one-shot voices.
  const events=[];
  for(const s of slices) {
    const duration=(s.end-s.start)/audio.rate, t=s.time/44100;
    if(duration>loopDuration*8)throw Error('A slice tail would exceed the eight-voice loop capacity.');
    for(let cycle=-8;cycle<=1;cycle++){events.push([t+cycle*loopDuration,1]);events.push([t+cycle*loopDuration+duration,-1]);}
  }
  events.sort((a,b)=>a[0]-b[0]||a[1]-b[1]);let active=0;
  for(const [time,change]of events){active+=change;if(time>=0&&time<loopDuration&&active>8)throw Error('More than eight slice tails overlap; this preview cannot preserve all of them.');}
  return {slices,length,loopDuration,bpm:raw.bpm===undefined?null:number(raw.bpm,'bpm'),warnings};
}
export function exportMarkers(audio,markers) {
  return {version:1,sampleRate:audio.rate,loopDuration:markers.length/44100,...(markers.bpm?{bpm:markers.bpm}:{}),slices:markers.slices.map(s=>({label:s.label,startSample:s.start,endSample:s.end,time:s.time/44100}))};
}
