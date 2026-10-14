/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef D8_ORACLE_REFERENCE
#include D8_ORACLE_SOURCE
#else
#include "../../firmware/src/dsp/drum808.c"
#endif
static uint32_t float_bits(float x){uint32_t n;memcpy(&n,&x,4);return n;}
static uint64_t state_hash(const void *p,size_t n){const uint8_t *b=p;uint64_t h=14695981039346656037ull;while(n--)h=(h^*b++)*1099511628211ull;return h;}
static void render_case(drum808_t *d,int track,FILE *pcm){
 float l[64]={0},r[64]={0},rev[64]={0},dly[64]={0};
 if(track<D8_NUM)drum808_trigger(d,track,0.91f);else for(int t=0;t<D8_NUM;t++)drum808_trigger(d,t,0.91f);
 drum808_render_st(d,l,r,rev,dly,64);
 assert(fwrite(l,sizeof l,1,pcm)==1&&fwrite(r,sizeof r,1,pcm)==1&&fwrite(rev,sizeof rev,1,pcm)==1&&fwrite(dly,sizeof dly,1,pcm)==1);
}

static int sound_owner(drum808_t *d,int sound){
 for(int t=0;t<D8_NUM;t++)for(int sw=0;sw<2;sw++){
  d->sw[t]=(uint8_t)sw;if(track_sound(d,t)==sound)return t;d->sw[t]=0;
 }
 assert(0);return -1;
}
static void long_tail_cases(const char *path){
 char name[512];assert(snprintf(name,sizeof name,"%s.tails",path)<(int)sizeof name);
 FILE *f=fopen(name,"wb");assert(f);int values[]={0,64,127};
 for(int active=0;active<2;active++)for(int sound=0;sound<D8S_NUM;sound++)for(int v=0;v<3;v++){
  drum808_t d;drum808_init(&d);int track=sound_owner(&d,sound);set_pot(&d,sound,D8P_DECAY,values[v]);
  if(active){sound_strip_config_t cfg;sound_strip_config_default(&cfg);cfg.flags=SOUND_STRIP_ENABLED|SOUND_STRIP_LIMITER;cfg.boost_db=12;cfg.ceiling_db=6;
   for(int t=0;t<D8_NUM;t++)assert(!drum808_strip_set(&d,t,&cfg));
   float l[256]={0},r[256]={0},rev[256]={0},dly[256]={0};drum808_render_st(&d,l,r,rev,dly,256);
  }
  drum808_trigger(&d,track,0.91f);
  for(int block=0;block<128;block++){
   float l[256]={0},r[256]={0},rev[256]={0},dly[256]={0};drum808_strip_meter_clear(&d);drum808_render_st(&d,l,r,rev,dly,256);
   assert(fwrite(l,sizeof l,1,f)==1&&fwrite(r,sizeof r,1,f)==1&&fwrite(rev,sizeof rev,1,f)==1&&fwrite(dly,sizeof dly,1,f)==1);
  }
  printf("long-tail %d %d %d %016llx\n",active,sound,values[v],(unsigned long long)state_hash(&d,sizeof d));
 }
 fclose(f);
}

int main(int argc,char **argv){
 assert(argc==2);FILE *pcm=fopen(argv[1],"wb");assert(pcm);drum808_t d;int cases=0;
 printf("runtime-size %zu\n",sizeof d);
 for(int sound=0;sound<D8S_NUM;sound++)for(int slot=0;slot<D8P_NUM;slot++){
#ifndef D8_ORACLE_REFERENCE
  const d8_range_t *r=&k_ranges[k_spec[sound][slot].range];unsigned def=k_spec[sound][slot].def;
#else
  const d8_spec_t *r=&k_spec[sound][slot];unsigned def=r->def;
#endif
  assert(r->exp<=1&&def<=127);printf("range %d %d %08x %08x %u %u\n",sound,slot,float_bits(r->min),float_bits(r->max),r->exp,def);
  for(int value=0;value<128;value++){
   drum808_init(&d);assert(d.pot[sound][slot]==def);int track=-1;
   for(int t=0;t<D8_NUM&&track<0;t++)for(int sw=0;sw<2;sw++){
    d.sw[t]=(uint8_t)sw;if(track_sound(&d,t)==sound){track=t;break;}d.sw[t]=0;
   }
   assert(track>=0);set_pot(&d,sound,slot,value);
   printf("pot %d %d %d %08x %016llx\n",sound,slot,value,float_bits(d.potv[sound][slot]),(unsigned long long)state_hash(&d,sizeof d));
   render_case(&d,track,pcm);cases++;
  }
 }
 for(int bank=0;bank<2;bank++)for(int track=0;track<=D8_KIT;track++){
  assert(!drum808_param(track,-1)&&!drum808_param(track,drum808_nparams(track)));
  for(int index=0;index<drum808_nparams(track);index++){
   const x0x_param_t *p=drum808_param(track,index);printf("descriptor %d %d %s %u %u",track,index,p->name,p->max,p->def);
   if(p->names){for(int n=0;n<=p->max;n++)printf(" [%s]",p->names[n]);}else printf(" null-labels");putchar('\n');
   for(int value=-1;value<=(int)p->max+1;value++){
    drum808_init(&d);for(int t=0;t<D8_NUM;t++)d.sw[t]=(uint8_t)bank;
    drum808_set(&d,track,index,value);assert(drum808_get(&d,track,index)==(value<0?0:value>p->max?p->max:value));
    printf("public %d %d %d %d %016llx\n",bank,track,index,value,(unsigned long long)state_hash(&d,sizeof d));
    render_case(&d,track,pcm);cases++;
   }
  }
 }
 assert(!drum808_param(-1,0)&&!drum808_param(D8_KIT+1,0));fclose(pcm);long_tail_cases(argv[1]);printf("cases %d\n",cases);return 0;
}
