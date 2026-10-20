import {toDb} from './slice-core.mjs';
const colours={text:'#c5d6df',grid:'#344451',match:'#ff77ce',band:'#69cae7',floor:'#ffdc78'};
export function drawSpectrum(canvas,data,profile,rate){
  const g=canvas.getContext('2d'),w=canvas.width,h=canvas.height,L=62,R=w-24,T=20,B=h-42,max=rate/2;
  g.clearRect(0,0,w,h);if(!data)return;
  const x=hz=>L+Math.log(Math.max(20,hz)/20)/Math.log(max/20)*(R-L),y=db=>T+(0-Math.max(-96,Math.min(0,db)))/96*(B-T);
  if(profile.frequency){const bw=profile.frequency/Math.max(.1,profile.frequency/Math.max(20,profile.bandwidth)),lo=(Math.sqrt(bw*bw+4*profile.frequency**2)-bw)/2,hi=lo+bw;g.fillStyle='#69cae72b';g.fillRect(x(Math.max(20,lo)),T,x(Math.min(max,hi))-x(Math.max(20,lo)),B-T);g.strokeStyle=colours.band;g.setLineDash([5,5]);g.beginPath();g.moveTo(x(profile.frequency),T);g.lineTo(x(profile.frequency),B);g.stroke();g.setLineDash([]);}
  g.font='13px system-ui';g.textAlign='right';for(const db of [0,-24,-48,-72,-96]){g.fillStyle=colours.text;g.fillText(String(db),L-8,y(db)+4);g.strokeStyle=colours.grid;g.beginPath();g.moveTo(L,y(db));g.lineTo(R,y(db));g.stroke();}
  g.textAlign='center';for(const hz of (w<600?[20,100,400,2000,10000]:[20,50,100,200,400,1000,2000,5000,10000,20000]))if(hz<=max){g.fillStyle=colours.text;g.fillText(hz>=1000?hz/1000+'k':String(hz),x(hz),B+21);}
  g.fillText('Frequency (Hz) · original region · shaded detector band',w/2,h-3);g.textAlign='left';g.fillText('dBFS',4,14);
  g.strokeStyle='#b8ecdb';g.lineWidth=1.5;g.beginPath();let began=false;for(const bin of data.bins)if(bin.hz>=20){const xx=x(bin.hz),yy=y(bin.db);if(!began){g.moveTo(xx,yy);began=true;}else g.lineTo(xx,yy);}g.stroke();g.lineWidth=1;
}
export function drawEnergy(canvas,audio,data,range,matches){
  const g=canvas.getContext('2d'),w=canvas.width,h=canvas.height,L=62,R=w-24,T=24,B=h-38;g.clearRect(0,0,w,h);if(!audio||!data)return;
  const x=frame=>L+frame/audio.frames*(R-L),y=db=>T+(-Math.max(-96,Math.min(0,db)))/96*(B-T);
  g.fillStyle='#69cae718';g.fillRect(x(range.start),T,x(range.end)-x(range.start),B-T);
  g.font='13px system-ui';g.textAlign='right';for(const db of [0,-24,-48,-72,-96]){g.fillStyle=colours.text;g.fillText(String(db),L-8,y(db)+4);g.strokeStyle=colours.grid;g.beginPath();g.moveTo(L,y(db));g.lineTo(R,y(db));g.stroke();}
  g.textAlign='center';for(let i=0;i<=4;i++)g.fillText((audio.duration*i/4).toFixed(2)+'s',L+i*(R-L)/4,B+22);
  g.textAlign='left';g.fillText('Band RMS dBFS · full loop',L,15);
  g.strokeStyle=colours.band;g.beginPath();for(let px=0;px<=R-L;px++){let peak=0;const a=Math.floor(px/(R-L)*data.energy.length),b=Math.max(a+1,Math.ceil((px+1)/(R-L)*data.energy.length));for(let j=a;j<Math.min(b,data.energy.length);j++)peak=Math.max(peak,data.energy[j]);const yy=y(toDb(peak));if(!px)g.moveTo(L+px,yy);else g.lineTo(L+px,yy);}g.stroke();
  g.strokeStyle=colours.floor;g.setLineDash([8,5]);g.beginPath();g.moveTo(L,y(data.profile.minDb));g.lineTo(R,y(data.profile.minDb));g.stroke();g.setLineDash([]);g.fillStyle=colours.floor;g.textAlign='right';g.fillText('Minimum '+data.profile.minDb+' dBFS',R,y(data.profile.minDb)-5);
  if(matches){g.fillStyle='#8896a0';for(const i of data.belowFloor){g.beginPath();g.arc(x(i*data.hop),y(toDb(data.energy[i])),2,0,2*Math.PI);g.fill();}g.fillStyle=colours.match;for(const m of data.matches){const xx=x(m.frame),yy=y(m.db);g.beginPath();g.moveTo(xx,yy-8);g.lineTo(xx-5,yy-16);g.lineTo(xx+5,yy-16);g.closePath();g.fill();}}
}
export function drawMatches(canvas,audio,matches){const g=canvas.getContext('2d');g.fillStyle=colours.match;for(const m of matches){const x=m.frame/audio.frames*canvas.width;g.fillRect(x,0,2,canvas.height/2);g.beginPath();g.moveTo(x,14);g.lineTo(x-6,2);g.lineTo(x+6,2);g.closePath();g.fill();}}
