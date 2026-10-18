/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#ifdef D9_ORACLE_REFERENCE
#include D9_REFERENCE_C
#else
#include "../../firmware/src/dsp/drum909.c"
#endif
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
static uint32_t bits(float x){uint32_t b;memcpy(&b,&x,4);return b;}
static uint64_t hashbytes(const void *p,size_t n){const uint8_t *b=p;uint64_t h=14695981039346656037ull;while(n--)h=(h^*b++)*1099511628211ull;return h;}
/* Hash logical PCM, independent of byte-coded versus decoded mu-law storage. */
static uint64_t sample_hash(const void *buf,unsigned len){
 uint64_t h=14695981039346656037ull;
 for(unsigned i=0;i<len;i++){
#ifdef D9_ORACLE_REFERENCE
  int16_t sample=((const int16_t *)buf)[i];
#else
  int16_t sample=d9_sample_read(buf,i);
#endif
  const uint8_t *b=(const uint8_t *)&sample;
  for(unsigned n=0;n<sizeof sample;n++)h=(h^b[n])*1099511628211ull;
 }
 return h;
}

static void long_tail_cases(const char *path){
 char name[512];assert(snprintf(name,sizeof name,"%s.tails",path)<(int)sizeof name);FILE *f=fopen(name,"wb");assert(f);
 const int values[]={0,64,127};int cases=0;
 for(int active=0;active<2;active++)for(int voice=0;voice<DR_NUM;voice++)for(int value=0;value<3;value++){
  drum909_t d,normalized;memset(&d,0,sizeof d);drum909_init(&d);
  for(int i=0;i<drum909_nparams(voice);i++)if(d9_specs[voice][i].field==F_DECAY)drum909_set(&d,voice,i,values[value]);
  if(active){sound_strip_config_t cfg;sound_strip_config_default(&cfg);cfg.flags=SOUND_STRIP_ENABLED|SOUND_STRIP_LIMITER;cfg.boost_db=12;cfg.ceiling_db=6;
   for(int v=0;v<DR_NUM;v++)assert(!drum909_strip_set(&d,v,&cfg));
   float l[256]={0},r[256]={0},rev[256]={0},dly[256]={0};drum909_render_st(&d,l,r,rev,dly,256);
  }
  drum909_trigger(&d,voice,0.91f);
  for(int block=0;block<128;block++){
   float l[256]={0},r[256]={0},rev[256]={0},dly[256]={0};drum909_strip_meter_clear(&d);drum909_render_st(&d,l,r,rev,dly,256);
   assert(fwrite(l,sizeof l,1,f)==1&&fwrite(r,sizeof r,1,f)==1&&fwrite(rev,sizeof rev,1,f)==1&&fwrite(dly,sizeof dly,1,f)==1);
  }
  normalized=d;for(int n=0;n<4;n++)normalized.smp[n].buf=0;
  printf("long-tail %d %d %d %016llx\n",active,voice,values[value],(unsigned long long)hashbytes(&normalized,sizeof normalized));cases++;
 }
 fclose(f);printf("tail cases %d\n",cases);
}

int main(int argc,char **argv){
 assert(argc==2);FILE *pcm=fopen(argv[1],"wb");assert(pcm);
 int cases=0;drum909_t d,normalized;float l[64],r[64],rev[64],dly[64];
 for(int t=0;t<NTARGETS;t++)for(int v=0;v<NVOICES_MAX;v++)for(int i=0;i<engine_nparams(t,v);i++){
  const x0x_param_t *p=engine_param(t,v,i);assert(p);
  printf("descriptor %d %d %d %s %u %u",t,v,i,p->name,p->max,p->def);
  if(p->names){printf(" labels");for(int n=0;n<=p->max;n++)printf(" [%s]",p->names[n]);}else printf(" null-labels");putchar('\n');
 }
 memset(&d,0,sizeof d);drum909_init(&d);
 for(int n=0;n<4;n++)printf("sample %d %u %016llx\n",n,d.smp[n].len,(unsigned long long)sample_hash(d.smp[n].buf,d.smp[n].len));
 for(int v=0;v<=DR_KIT;v++){
  assert(!drum909_param(v,-1)&&!drum909_param(v,drum909_nparams(v)));
  for(int i=0;i<drum909_nparams(v);i++){
   const d9_pspec_t *s=&d9_specs[v][i];
   unsigned field=s->field,curve=s->curve,table=s->exp;
#ifndef D9_ORACLE_REFERENCE
   const d9_range_t *range=&d9_ranges[s->range];float lo=range->lo,hi=range->hi;
#else
   float lo=s->lo,hi=s->hi;
#endif
   assert(field<=F_PAN&&curve<=CV_SW);if(curve==CV_EXP)assert(table<X0X_EXP_COUNT);else assert(table==0);
   printf("spec %d %d %u %u %u %08x %08x\n",v,i,field,curve,table,bits(lo),bits(hi));
   for(int value=-1;value<=(int)s->ui.max+1;value++){
    memset(&d,0,sizeof d);drum909_init(&d);drum909_set(&d,v,i,value);
    assert(drum909_get(&d,v,i)==(value<0?0:value>s->ui.max?s->ui.max:value));
    normalized=d;for(int n=0;n<4;n++)normalized.smp[n].buf=0;
    printf("state %d %d %d %016llx\n",v,i,value,(unsigned long long)hashbytes(&normalized,sizeof normalized));
    memset(l,0,sizeof l);memset(r,0,sizeof r);memset(rev,0,sizeof rev);memset(dly,0,sizeof dly);
    if(v<DR_NUM)drum909_trigger(&d,v,0.91f);else for(int n=0;n<DR_NUM;n++)drum909_trigger(&d,n,0.91f);
    drum909_render_st(&d,l,r,rev,dly,64);
    assert(fwrite(l,sizeof l,1,pcm)==1&&fwrite(r,sizeof r,1,pcm)==1&&fwrite(rev,sizeof rev,1,pcm)==1&&fwrite(dly,sizeof dly,1,pcm)==1);cases++;
   }
  }
 }
 assert(!drum909_param(-1,0)&&!drum909_param(DR_KIT+1,0));fclose(pcm);long_tail_cases(argv[1]);printf("parameter cases %d\n",cases);return 0;
}
