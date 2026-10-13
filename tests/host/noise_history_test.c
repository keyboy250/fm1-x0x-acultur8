/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dsp/drum909.h"
size_t noise_reference_size(void);
void legacy_drum909_init(void *);
void legacy_drum909_set(void *,int,int,int);
void legacy_drum909_trigger(void *,int,float);
void legacy_drum909_render_st(void *,float *,float *,float *,float *,int);
int legacy_drum909_bd_strip_set(void *,const sound_strip_config_t *);
const sound_strip_meter_t *legacy_drum909_bd_strip_meter(const void *);
static int failures;
static void check(int ok,const char *what,int block) { if(!ok) { if(failures++<10)fprintf(stderr,"FAIL %s block%d\n",what,block); } }
static double bench(int reference,void *old)
{
    drum909_t d;
    float l[256],r[256],rev[256],dly[256];
    if(reference)legacy_drum909_init(old);else drum909_init(&d);
    clock_t start=clock();
    for(int b=0;b<32768;++b) {
        if(b%128==0)for(int v=0;v<DR_NUM;++v) {
            if(reference)legacy_drum909_trigger(old,v,1.0f);else drum909_trigger(&d,v,1.0f);
        }
        memset(l,0,sizeof(l));memset(r,0,sizeof(r));memset(rev,0,sizeof(rev));memset(dly,0,sizeof(dly));
        if(reference)legacy_drum909_render_st(old,l,r,rev,dly,256);else drum909_render_st(&d,l,r,rev,dly,256);
    }
    return (double)(clock()-start)/(double)CLOCKS_PER_SEC;
}
int main(void)
{
    void *old=malloc(noise_reference_size());
    if(!old)return 2;
    const int sizes[]={1,255,256,17,128,2,254,64};
    for(int scenario=0;scenario<DR_NUM+3;++scenario) {
        drum909_t d;
        float a[4][256],b[4][256];
        drum909_init(&d);legacy_drum909_init(old);
        sound_strip_config_t config;
        sound_strip_config_default(&config);
        if(scenario==DR_NUM+1) { config.flags=SOUND_STRIP_FLAGS;config.boost_db=24;config.ceiling_db=12; }
        drum909_bd_strip_set(&d,&config);legacy_drum909_bd_strip_set(old,&config);
        for(int v=0;v<DR_NUM;++v) {
            int pan=drum909_nparams(v)-1;
            drum909_set(&d,v,pan,13+v*9);legacy_drum909_set(old,v,pan,13+v*9);
            /* Send indices immediately precede Pan on every 909 voice. */
            drum909_set(&d,v,pan-2,67);legacy_drum909_set(old,v,pan-2,67);
            drum909_set(&d,v,pan-1,103);legacy_drum909_set(old,v,pan-1,103);
        }
        for(int block=0;block<2400;++block) {
            int n=scenario==DR_NUM+2?256:sizes[block%8];
            if(block%37==0 || block%37==1 || block%37==7) {
                for(int v=0;v<DR_NUM;++v)if(scenario>=DR_NUM || scenario==v) {
                    drum909_trigger(&d,v,0.9f);legacy_drum909_trigger(old,v,0.9f);
                }
            }
            if(scenario==DR_NUM+1 && block%200==100) {
                config.flags=config.flags?0:SOUND_STRIP_FLAGS;
                drum909_bd_strip_set(&d,&config);legacy_drum909_bd_strip_set(old,&config);
            }
            memset(a,0,sizeof(a));memset(b,0,sizeof(b));
            drum909_render_st(&d,a[0],a[1],a[2],a[3],n);
            legacy_drum909_render_st(old,b[0],b[1],b[2],b[3],n);
            check(memcmp(a,b,sizeof(a))==0,"audio/pan/sends",block);
            check(memcmp(drum909_bd_strip_meter(&d),legacy_drum909_bd_strip_meter(old),sizeof(sound_strip_meter_t))==0,"meter",block);
        }
    }
    check(noise_reference_size()-sizeof(drum909_t)==1024,"state reduction",0);
    printf("noise history: %d failures; original=%zu compact=%zu savings=%zu B; history=%d current block=256\n",failures,noise_reference_size(),sizeof(drum909_t),noise_reference_size()-sizeof(drum909_t),DR_NZ_HIST);
    const char *skip_bench=getenv("NOISE_SKIP_BENCH");
    for(int run=0;run<((skip_bench && !strcmp(skip_bench,"1"))?0:3);++run) {
        double before,after;
        if(run%2) { after=bench(0,old);before=bench(1,old); }
        else { before=bench(1,old);after=bench(0,old); }
        printf("Host mixed-909 8388608frames run%d original=%.6fs compact=%.6fs ratio=%.4f\n",run+1,before,after,after/before);
    }
    free(old);return failures?1:0;
}
