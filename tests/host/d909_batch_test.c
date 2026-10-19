/* SPDX-License-Identifier: GPL-3.0-only */
/* Differential test against immutable installed 99cce17 renderer. */
#include "dsp/drum909.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
void ref_drum909_init(drum909_t *);
void ref_drum909_trigger(drum909_t *,int,float);
void ref_drum909_set(drum909_t *,int,int,int);
int ref_drum909_strip_set(drum909_t *,int,const sound_strip_config_t *);
void ref_drum909_strip_meter_clear(drum909_t *);
void ref_drum909_render_st(drum909_t *,float *,float *,float *,float *,int);
void ref_drum909_render(drum909_t *,float *,float *,float *,int);
int ref_drum909_active(const drum909_t *);
#ifdef BATCH_TRACK_CALLS
static int phase;
static unsigned long strip_calls[2], strip_frames[2];
void batch_real_process(sound_strip_t *,float *,float *,uint32_t,sound_strip_meter_t *);
void sound_strip_process(sound_strip_t *s,float *l,float *r,uint32_t n,sound_strip_meter_t *m)
{
 strip_calls[phase]++;strip_frames[phase]+=n;batch_real_process(s,l,r,n,m);
}
#define PHASE(n) (phase=(n))
#else
#define PHASE(n) ((void)0)
#endif
static uint32_t random_state=0x909ba7c1u;
static uint32_t rnd(void){random_state^=random_state<<13;random_state^=random_state>>17;return random_state^=random_state<<5;}
static void exact(const void *a,const void *b,size_t n,const char *what,int trial,int step){
 if(memcmp(a,b,n)){fprintf(stderr,"FAIL %s trial%d step%d\n",what,trial,step);assert(0);}
}
static void state_equal(drum909_t *a,drum909_t *b,int trial,int step){
 drum909_t copy=*a;
 /* Each translation unit owns equivalent read-only decoded sample tables. */
 for(int v=0;v<4;v++)copy.smp[v].buf=b->smp[v].buf;
 exact(&copy,b,sizeof copy,"full synthesis/strip/meter/noise state",trial,step);
}
static double bench(int reference,int use_scratch,int stripped,int n){
 drum909_t d; if(reference)ref_drum909_init(&d);else drum909_init(&d);
 sound_strip_config_t c={146,480,24,10,24,15};
 if(stripped)for(int v=0;v<11;v++){
  if(reference)assert(!ref_drum909_strip_set(&d,v,&c));else assert(!drum909_strip_set(&d,v,&c));
 }
 float x[4][256],scratch[256];clock_t begin=clock();
 for(int block=0;block<4096;block++){
  if(block%128==0)for(int v=0;v<11;v++){
   if(reference)ref_drum909_trigger(&d,v,1.0f);else drum909_trigger(&d,v,1.0f);
  }
  memset(x,0,sizeof x);
  if(reference){ref_drum909_strip_meter_clear(&d);ref_drum909_render_st(&d,x[0],x[1],x[2],x[3],n);}
  else {drum909_strip_meter_clear(&d);drum909_render_st_scratch(&d,x[0],x[1],x[2],x[3],n,use_scratch?scratch:0);}
 }
 volatile uint32_t keep=d.noise;(void)keep;return (double)(clock()-begin)/CLOCKS_PER_SEC;
}
int main(void){
 unsigned long frames=0,blocks=0;const int sizes[]={0,1,2,7,13,31,32,64,127,128,219,220,255,256,257};
 for(int trial=0;trial<48;trial++){
  drum909_t a,b;memset(&a,0,sizeof a);memset(&b,0,sizeof b);ref_drum909_init(&a);drum909_init(&b);
  for(int step=0;step<512;step++){
   int n=sizes[rnd()%15],k=n>256?256:n;
   if(step%17==0)for(int v=0;v<11;v++){
    sound_strip_config_t c={20+rnd()%19981,20+rnd()%19981,rnd()%25,rnd()%49,10+rnd()%241,(trial+v+step/17)%16};
    assert(!ref_drum909_strip_set(&a,v,&c));assert(!drum909_strip_set(&b,v,&c));
   }
   if(step%7==0 && step<360)for(int v=0;v<11;v++){
    if((rnd()&3)==0 || step==0){float velocity=(float)(rnd()%128)/127.0f;ref_drum909_trigger(&a,v,velocity);drum909_trigger(&b,v,velocity);}
   }
   if(step%5==0 && step<300){
    int v=rnd()%11,i=rnd()%drum909_nparams(v);int value=rnd()%(drum909_param(v,i)->max+1);
    ref_drum909_set(&a,v,i,value);drum909_set(&b,v,i,value);
   }
   if(step%3==0){ref_drum909_strip_meter_clear(&a);drum909_strip_meter_clear(&b);}
   float x[4][264],y[4][264],scratch[264],initial[264];
   for(int j=0;j<4;j++)for(int i=0;i<264;i++)x[j][i]=y[j][i]=(float)((i+j)%19-9)*0.00001f;
   for(int i=0;i<264;i++)scratch[i]=initial[i]=(float)(i+1)*0.123f;
   PHASE(0);
   if(!(step%23)) {
    ref_drum909_render(&a,x[0]+4,x[2]+4,x[3]+4,n);
    PHASE(1);drum909_render(&b,y[0]+4,y[2]+4,y[3]+4,n);
   } else {
    ref_drum909_render_st(&a,x[0]+4,x[1]+4,x[2]+4,x[3]+4,n);
    PHASE(1);drum909_render_st_scratch(&b,y[0]+4,y[1]+4,y[2]+4,y[3]+4,n,step%19?scratch+4:0);
   }
   exact(x,y,sizeof x,"audio outputs and output canaries",trial,step);
   exact(scratch,initial,4*sizeof(float),"scratch prefix",trial,step);
   exact(scratch+4+k,initial+4+k,(260-k)*sizeof(float),"scratch suffix",trial,step);
   if(!(step%19) || !(step%23))exact(scratch,initial,sizeof scratch,"NULL scratch not touched",trial,step);
   state_equal(&a,&b,trial,step);assert(ref_drum909_active(&a)==drum909_active(&b));frames+=k;blocks++;
  }
 }
 printf("PASS pinned909 batch equivalence:48 trials %lu blocks %lu frames; full state/meters,11drums,all flags,pan/sends,choke,live edits,tail/idle,0..257 blocks,scratch/output canaries,NULL fallback\n",blocks,frames);
#ifdef BATCH_TRACK_CALLS
 assert(strip_frames[0]==strip_frames[1]);assert(strip_calls[1]<strip_calls[0]);
 printf("PROCESSOR COUNTS reference%lu candidate%lu frames%lu/%lu\n",strip_calls[0],strip_calls[1],strip_frames[0],strip_frames[1]);
 return 0;
#endif
 for(int stripped=0;stripped<2;stripped++)for(int run=0;run<5;run++){
  double a,b;if(run&1){b=bench(0,1,stripped,256);a=bench(1,0,stripped,256);}else{a=bench(1,0,stripped,256);b=bench(0,1,stripped,256);}
  printf("HOST909 %s run%d reference%.6fs candidate%.6fs ratio%.4f\n",stripped?"all-strips":"bypass",run,a,b,b/a);
 }
 return 0;
}
