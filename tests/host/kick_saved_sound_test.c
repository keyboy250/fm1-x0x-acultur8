#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../firmware/src/dsp/drum808.h"
#include "../preset-oracle/captured_sine.h"
void *p35_new(void);void p35_cfg(void*,int,int);void p35_base(void*,int,int);void p35_strip(void*,const sound_strip_config_t*);void p35_hit(void*,float);void p35_render(void*,float*,float*,float*,float*,int);
static void configure(drum808_t *current,void *old){
 drum808_init(current);
 for(int i=0;i<39;i++){drum808_hybrid_set(current,i,captured_cfg[i]);p35_cfg(old,i,captured_cfg[i]);}
 for(int i=0;i<10;i++){drum808_set(current,0,i,captured_base[i]);p35_base(old,i,captured_base[i]);}
 drum808_strip_set(current,0,&captured_strip);p35_strip(old,&captured_strip);
}
int main(void){
 drum808_t current;void *old=p35_new();configure(&current,old);
 unsigned frames=0,next=0,isolated_hits=0;
 /* Preserve frozen full-preset PCM for nonoverlapping hits. The original
  *53-block cadence overlaps this preset and deliberately exercises the repair. */
 for(int block=0;block<1600;block++){
  float a[4][256]={{0}},b[4][256]={{0}};int n=1+(block*59)%256;
  if(frames>=next){drum808_trigger(&current,0,.75f);p35_hit(old,.75f);next=frames+66150;isolated_hits++;}
  drum808_render_st(&current,a[0],a[1],a[2],a[3],n);p35_render(old,b[0],b[1],b[2],b[3],n);
  for(int k=0;k<4;k++)assert(!memcmp(a[k],b[k],n*sizeof(float)));frames+=(unsigned)n;
 }
 assert(isolated_hits>=3);free(old);old=p35_new();configure(&current,old);
 unsigned repaired=0,differences=0;
 /* Retain the original dense cadence and verify continuity plus finite output,
  * rather than requiring the discontinuity under repair to remain unchanged. */
 for(int block=0;block<1000;block++){
  float a[4][256]={{0}},b[4][256]={{0}};int n=1+(block*59)%256;
  if(block%53==0){
   float prior=current.hybrid.retrigger_last;drum808_trigger(&current,0,.75f);p35_hit(old,.75f);
   int smoothing=current.hybrid.retrigger_left!=0;
   drum808_render_st(&current,a[0],a[1],a[2],a[3],1);p35_render(old,b[0],b[1],b[2],b[3],1);
   if(smoothing){assert(fabsf(current.hybrid.retrigger_last-prior)<1e-5f);repaired++;}
   for(int k=0;k<4;k++)if(a[k][0]!=b[k][0])differences++;
   memset(a,0,sizeof a);memset(b,0,sizeof b);n--;
  }
  if(n){drum808_render_st(&current,a[0],a[1],a[2],a[3],n);p35_render(old,b[0],b[1],b[2],b[3],n);}
  for(int k=0;k<4;k++)for(int i=0;i<n;i++){assert(isfinite(a[k][i]));if(a[k][i]!=b[k][i])differences++;}
 }
 assert(repaired>0&&differences>0);
 for(int i=0;i<39;i++)assert(current.hybrid.cfg[i]==captured_cfg[i]);
 free(old);printf("CAPTURED_KICK_PASS: isolated full-preset PCM parity1600variableblocks/%uhits; dense1000block continuity on%u repaired hits; intentional overlapping sound changes\n",isolated_hits,repaired);return 0;
}
