/* SPDX-License-Identifier: GPL-3.0-only */
/* Hardware-first monophonic PHASE adapter. The original bass303_t allocation
 * is used exclusively by either engine. No extra RAM, sample buffers or heap.
 * started==2 is the audio-owner PHASE tag; legacy303 uses only0/1. Its12 pots
 * remain intact. Returning to303 reconstructs derived state from those pots.
 * Oscillator core: Leo Kuroshita/Hugelton Instruments (MIT, notice retained). */
#pragma once
#include "bass303.h"
#include "fastmath.h"
#include "phase303b_sine.h"
static inline int32_t phase_sine_i(uint32_t ph)
{
 unsigned i=ph>>22;int32_t a=phase_sine[i],b=phase_sine[(i+1u)&1023u];
 return a+(((b-a)*(int32_t)((ph>>7)&32767u))>>15);
}
static inline int32_t pd_cos(uint32_t ph16)
{return -phase_sine_i(((ph16&65535u)<<16)+0x40000000u);}
#include "phase303b_wave.inc"
static int phase_active(const bass303_t *b){return b->started==2;}
static void slot_all_off(bass303_t *b)
{if(!phase_active(b)){bass303_all_off(b);return;}b->gate=0;b->amp_y=0.0f;b->env_y=0.0f;b->dcb_x1=b->dcb_y1=0.0f;b->idle=1;}
static void phase_select(bass303_t *b,unsigned config)
{
 unsigned on=config&128u;uint8_t pots[BASS303_NPARAMS];int i,lite=b->lite;
 if(!on&&!phase_active(b))return;
 if(on&&phase_active(b)&&(unsigned)b->r_b[0]==config)return;
 if(!on){for(i=0;i<BASS303_NPARAMS;i++)pots[i]=b->pot[i];bass303_init(b);for(i=0;i<BASS303_NPARAMS;i++)bass303_set(b,i,pots[i]);bass303_set_lite(b,lite);return;}
 b->started=2;b->wave=(int)(config&7u);b->env_scaler=(float)((config>>8)&127u)/127.0f;
 b->env_offset=(float)((config>>3)&15u)/15.0f;b->r_b[0]=(float)config;
 b->gate=0;b->idle=1;b->amp_y=b->env_y=0.0f;b->quiet=0;b->dcb_x1=b->dcb_y1=0.0f;
 b->decay_c=0.9999244171f; /*300ms at44.1kHz*/
 b->ampd_pw[0]=0.9998740316f; /*180ms release*/
 b->ampn_pw[0]=0.0075300848f; /*3ms attack*/
}
static void slot_set(bass303_t *b,int i,int v)
{if(phase_active(b)){if(i>=0&&i<BASS303_NPARAMS)b->pot[i]=(uint8_t)v;}else bass303_set(b,i,v);}
static void slot_note_on(bass303_t *b,int note,int accent,int slide)
{
 if(!phase_active(b)){bass303_note_on(b,note,accent,slide);return;}
 if(note<0||note>127)return;
 b->slew_y=440.0f*fm_exp2f((float)(note-69)/12.0f);
 if(!slide||b->idle){b->osc_freq=b->slew_y;b->quiet=0;b->env_y=1.0f;}
 b->accent_gain=accent?0.55f:0.40f;b->gate=1;b->idle=0;
}
static void slot_note_off(bass303_t *b)
{if(phase_active(b))b->gate=0;else bass303_note_off(b);}
static void slot_render(bass303_t *b,float *out,int n)
{
 int i;pd_t bend={0};uint32_t ph,inc;float amp,env;
 if(!phase_active(b)){bass303_render(b,out,n);return;}
 if(b->idle){for(i=0;i<n;i++)out[i]=0.0f;return;}
 ph=(uint32_t)b->quiet;amp=b->amp_y;env=b->env_y;
 for(i=0;i<n;i++){
  float depth=b->env_scaler+env*b->env_offset;
  if(depth>1.0f)depth=1.0f;
  /* Coefficients once per32 samples, matching the donor's control-block shape. */
  if(!(i&31))pd_setup(&bend,(unsigned)b->wave,(unsigned)(depth*56000.0f));
  b->osc_freq+=(b->slew_y-b->osc_freq)*0.001f;
  inc=(uint32_t)(b->osc_freq*(4294967296.0f/BASS303_SR));
  if(b->gate)amp+=(b->accent_gain-amp)*b->ampn_pw[0];else amp*=b->ampd_pw[0];
  env*=b->decay_c;
  {float x=(float)pd_wave(&bend,ph>>16)*(amp/32768.0f);float y=x-b->dcb_x1+0.9986f*b->dcb_y1;b->dcb_x1=x;b->dcb_y1=y;out[i]=y;}ph+=inc;
 }
 b->quiet=(int32_t)ph;b->amp_y=amp;b->env_y=env;
 if(!b->gate&&amp<0.000001f&&fm_fabsf(b->dcb_y1)<0.000001f){b->idle=1;b->amp_y=0.0f;}else b->idle=0;
}
