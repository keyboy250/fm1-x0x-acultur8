#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../firmware/src/dsp/drum808.c"
void *shape_shape_ref_new(void);void shape_shape_ref_hit(void*,int,float);void shape_shape_ref_render(void*,float*,float*,float*,float*,int);void shape_ref_kick(void*,int,int);
static float a[4][256],b[4][256];
static float total(float *x,int n){float e=0;for(int i=0;i<n;i++)e+=x[i]*x[i];return e;}
int main(void){
 drum808_t d;void *old=shape_shape_ref_new();
 for(int wave=0;wave<4;wave++){
  drum808_init(&d);drum808_hybrid_set(&d,HK_ENABLE,1);shape_ref_kick(old,HK_ENABLE,1);drum808_hybrid_set(&d,HK_SOURCE,wave);shape_ref_kick(old,HK_SOURCE,wave);
  for(int block=0;block<300;block++){int n=1+(block*31)%256;memset(a,0,sizeof a);memset(b,0,sizeof b);if(block%23==0){drum808_trigger(&d,0,.75f);shape_shape_ref_hit(old,0,.75f);}drum808_render_st(&d,a[0],a[1],a[2],a[3],n);shape_shape_ref_render(old,b[0],b[1],b[2],b[3],n);for(int k=0;k<4;k++)if(memcmp(a[k],b[k],n*sizeof(float))){for(int i=0;i<n;i++)if(a[k][i]!=b[k][i]){fprintf(stderr,"parity wave%d block%d bus%d sample%d new%.9g old%.9g\n",wave,block,k,i,a[k][i],b[k][i]);break;}abort();}}
  free(old);old=shape_shape_ref_new();
 }
 free(old);hybrid_kick_t h={0};hybrid_kick_defaults(&h);h.decay_scale=1;h.cfg[HK_SA]=20;h.cfg[HK_SH]=0;h.cfg[HK_SD]=15;h.elapsed=0;assert(fabsf(hk_envelope(&h,HK_SA))<1e-5f);h.elapsed=441;assert(fabsf(hk_envelope(&h,HK_SA)-.5f)<.001f);h.elapsed=44100;assert(hk_envelope(&h,HK_SA)==0);
 h.elapsed=8000;float neutral=hk_envelope(&h,HK_SA);h.cfg[HK_SC]=127;assert(hk_envelope(&h,HK_SA)<neutral);h.cfg[HK_SC]=0;assert(hk_envelope(&h,HK_SA)>neutral);
 float energies[3]={0};for(int mode=0;mode<3;mode++){drum808_init(&d);drum808_hybrid_set(&d,HK_ENABLE,1);drum808_hybrid_set(&d,HK_CLICK,30);drum808_hybrid_set(&d,HK_CRUNCH,mode==1?100:0);drum808_hybrid_set(&d,HK_SMOOTH,mode==2?100:0);hybrid_kick_trigger(&d.hybrid,1);for(int i=0;i<1000;i++){d.hybrid.elapsed=i;float x=hk_click(&d.hybrid);assert(isfinite(x));if(i>=530)assert(x==0);energies[mode]+=x*x;}}
 assert(energies[0]>0&&energies[1]!=energies[0]&&energies[2]<energies[0]);
 for(int w=0;w<4;w++)for(int curve=0;curve<=127;curve+=127){drum808_init(&d);drum808_hybrid_set(&d,HK_ENABLE,1);drum808_hybrid_set(&d,HK_SOURCE,w);drum808_hybrid_set(&d,HK_WENV,1);drum808_hybrid_set(&d,HK_WD,127);drum808_hybrid_set(&d,HK_WH,127);drum808_hybrid_set(&d,HK_WC,curve);drum808_hybrid_set(&d,HK_SENV,1);drum808_hybrid_set(&d,HK_CLICK,60);drum808_hybrid_set(&d,HK_CRUNCH,100);drum808_hybrid_set(&d,HK_SMOOTH,100);drum808_trigger(&d,0,1);assert(d.hybrid.left>150000);for(int j=0;j<1600;j++){memset(a,0,sizeof a);drum808_render_st(&d,a[0],a[1],a[2],a[3],256);for(int k=0;k<4;k++)for(int i=0;i<256;i++)assert(isfinite(a[k][i]));}assert(!d.hybrid.active);}
 puts("KICK_SHAPE_PASS:0.1.34 hybrid bypass PCM parity1200blocks, envelope breakpoints/curves,click texture/duration and finite extended-tail extremes");return 0;
}
