/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../firmware/src/dsp/drum808.c"
void *oracle_new(void);void oracle_hit(void*,int,float);void oracle_set(void*,int,int,int);void oracle_render(void*,float*,float*,float*,float*,int);
static float a[4][256],b[4][256];
static void render(drum808_t *d,int n){memset(a,0,sizeof a);drum808_render_st(d,a[0],a[1],a[2],a[3],n);for(int j=0;j<4;j++)for(int i=0;i<n;i++)assert(isfinite(a[j][i]));}
static float energy(void){float e=0;for(int i=0;i<256;i++)e+=a[0][i]*a[0][i];return e;}
int main(void){
 drum808_t d;drum808_init(&d);void *old=oracle_new();
 for(int k=0;k<500;k++){
  int t=k%11,idx=k%drum808_nparams(t),value=(k*19)%128,n=17+k%240;
  drum808_set(&d,t,idx,value);oracle_set(old,t,idx,value);
  if(k%3==0){float vel=(float)(k%127+1)/127;drum808_trigger(&d,t,vel);oracle_hit(old,t,vel);}
  memset(b,0,sizeof b);oracle_render(old,b[0],b[1],b[2],b[3],n);render(&d,n);
  for(int j=0;j<4;j++)assert(memcmp(a[j],b[j],(size_t)n*sizeof(float))==0);
 }
 for(int source=0;source<4;source++){
  drum808_init(&d);drum808_hybrid_set(&d,HK_ENABLE,1);drum808_hybrid_set(&d,HK_SOURCE,source);drum808_hybrid_set(&d,HK_BLEND,100);drum808_trigger(&d,0,1);
  float total=0;for(int k=0;k<300;k++){render(&d,256);total+=energy();}assert(total>.01f);assert(!d.hybrid.active);
 }
 /* Exact original waveform when enabled with blend0 and jitter0. */
 drum808_init(&d);drum808_t original=d;drum808_hybrid_set(&d,HK_ENABLE,1);drum808_hybrid_set(&d,HK_BLEND,0);drum808_hybrid_set(&d,HK_CHANCE,0);
 drum808_trigger(&d,0,1);drum808_trigger(&original,0,1);
 for(int k=0;k<100;k++){memset(b,0,sizeof b);drum808_render_st(&original,b[0],b[1],b[2],b[3],256);render(&d,256);for(int j=0;j<4;j++)assert(!memcmp(a[j],b[j],sizeof a[j]));}
 /* Repeated native-synth-only bypass must not acquire a residual correction. */
 for(int k=0;k<100;k++){if(k%7==0){drum808_trigger(&d,0,1);drum808_trigger(&original,0,1);}memset(b,0,sizeof b);drum808_render_st(&original,b[0],b[1],b[2],b[3],256);render(&d,256);for(int j=0;j<4;j++)assert(!memcmp(a[j],b[j],sizeof a[j]));}
 /* Uniform bounded probability; default tier remapping is disabled. */
 drum808_init(&d);int varied=0;d.hybrid.cfg[HK_CHANCE]=10;d.hybrid.cfg[HK_RANGE]=10;
 for(int i=0;i<20000;i++){int v=(int)(hybrid_kick_velocity(&d.hybrid,110.0f/127)*127+.5f);assert(v>=100&&v<=120);varied+=v!=110;}assert(varied>1400&&varied<2400);
 d.hybrid.cfg[HK_CHANCE]=0;assert(hybrid_kick_velocity(&d.hybrid,110.0f/127)==110.0f/127);
 /* Real ADPCM one-shot path, bounded playback and envelope extremes. */
 static const uint8_t sample[4410]={0x77,0x77,0x77,0x77,0x88,0x88};
 for(int mode=0;mode<2;mode++){
  drum808_init(&d);drum808_hybrid_sample(&d,sample,8820,22050);d.hybrid.cfg[HK_ENABLE]=1;d.hybrid.cfg[HK_SOURCE]=4;d.hybrid.cfg[HK_BLEND]=100;
  d.hybrid.cfg[HK_ATTACK]=100;d.hybrid.cfg[HK_PUNCH]=mode?127:0;d.hybrid.cfg[HK_VPUNCH]=127;d.hybrid.cfg[HK_FILTER]=1;
  drum808_trigger(&d,0,mode?1.0f:.1f);for(int i=0;i<600;i++)render(&d,256);assert(!d.hybrid.active);
 }
 free(old);printf("HYBRID_KICK_PASS:500 variable-block full-kit oracle parity,4 wavetables,blend0,jitter bounds and custom one-shot;host size %zu bytes\n",sizeof(hybrid_kick_t));return 0;
}
