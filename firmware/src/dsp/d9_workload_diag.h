/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "sound_strip.h"
#define D9W_MAGIC 0x44395701u
#define D9W_VOICES 11u
#define D9W_COLUMNS 8u
#define D9W_COMPLETE 1u
#define D9W_INVALID 2u
/* Rows: synth,strip,tail,handled,potential-filterzero,mixed frames;
 * packed synth(low16)/strip(high16) calls;OR of entry state flags. */
typedef struct {
 volatile uint32_t frames,calls,live[D9W_VOICES][D9W_COLUMNS];
 volatile uint32_t header[12],retained[D9W_VOICES][D9W_COLUMNS];
} d9_workload_state_t;
typedef char d9_workload_size_760[(sizeof(d9_workload_state_t)==760)?1:-1];
extern d9_workload_state_t d9_workload_state;
static inline int d9w_same(float a,float b){union{float f;uint32_t u;}x,y;x.f=a;y.f=b;return x.u==y.u;}
static inline uint32_t d9w_state(const sound_strip_t *s){
 uint32_t desired=(s->flags>>8)&15u;
 uint32_t f=desired|((s->flags&14u)<<4);
 if(s->mix==1.0f)f|=1u<<8;
 if(d9w_same(s->hp_z[0],0.0f)&&d9w_same(s->hp_z[1],0.0f))f|=1u<<9;
 if(d9w_same(s->lp_z[0],0.0f)&&d9w_same(s->lp_z[1],0.0f))f|=1u<<10;
 if(s->flags>>16)f|=1u<<11;
 if(s->guard_gain!=1.0f)f|=1u<<12;
 if(d9w_same(s->hp,s->hp_target)&&d9w_same(s->lp,s->lp_target)&&d9w_same(s->boost,s->boost_target)&&d9w_same(s->ceiling,s->ceiling_target))f|=1u<<13;
 if(s->gain<1.0f)f|=1u<<14;
 return f;
}
/* Read-only predicate for the unpromoted57 extension,not an audio shortcut. */
static inline int d9w_filterzero_eligible(const sound_strip_t *s,uint32_t f){
 uint32_t desired=f&15u;
 return (desired&1u) && ((f>>4)&14u)==(desired&14u) && (f&(1u<<8)) && (f&(1u<<13)) &&
  !(f&((1u<<11)|(1u<<12))) && s->gain>=0.0f && s->gain<=1.0f && s->release>=0.0f && s->release<=1.0f &&
  s->boost>=1.0f && s->boost<=16.0f && s->ceiling>0.0f && s->ceiling<=1.0f &&
  (!(desired&2u)||((f&(1u<<9))&&s->hp>0.0f&&s->hp<=1.0f)) &&
  (!(desired&4u)||((f&(1u<<10))&&s->lp>0.0f&&s->lp<=1.0f));
}
static inline void d9w_batch(unsigned n){d9_workload_state.frames+=n;d9_workload_state.calls++;}
static inline void d9w_note(unsigned v,unsigned n,unsigned m,int bypass,int handled,int potential,uint32_t flags){
 volatile uint32_t *r=d9_workload_state.live[v];
 r[0]+=m;r[1]+=bypass?0u:n;r[2]+=(!bypass&&!m)?n:0u;r[3]+=handled?n:0u;
 r[4]+=potential?n:0u;r[5]+=bypass?m:n;r[6]+=(m?1u:0u)+(bypass?0u:65536u);r[7]|=flags;
}
/* Audio core alone writes. Called after render/DMA check inside base seqlock. */
static inline void d9_workload_finish(int capture,const volatile uint32_t *base,uint32_t frames){
 uint32_t native=d9_workload_state.frames,calls=d9_workload_state.calls;
 if(capture){
  volatile uint32_t *h=d9_workload_state.header;
  h[0]=D9W_MAGIC;h[1]=base[1];h[2]=base[2];h[3]=base[3];h[4]=base[4];h[5]=base[17];
  h[6]=frames;h[7]=native;h[8]=calls;h[9]=(frames==512u&&native==frames&&calls)?D9W_COMPLETE:0u;
  if(native>frames)h[9]|=D9W_INVALID;
  h[10]=D9W_VOICES;h[11]=D9W_COLUMNS;
 }
 for(unsigned v=0;v<D9W_VOICES;v++)for(unsigned i=0;i<D9W_COLUMNS;i++){
  if(capture)d9_workload_state.retained[v][i]=d9_workload_state.live[v][i];
  d9_workload_state.live[v][i]=0;
 }
 d9_workload_state.frames=d9_workload_state.calls=0;
}
int d9_workload_snapshot(unsigned index,uint32_t *words);
