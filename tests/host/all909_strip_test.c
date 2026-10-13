/* SPDX-License-Identifier: GPL-3.0-only */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dsp/drum909.h"
#include "dsp/fastmath.h"
size_t all909_reference_size(void);
void legacy_drum909_init(void *);
void legacy_drum909_set(void *,int,int,int);
void legacy_drum909_trigger(void *,int,float);
void legacy_drum909_render(void *,float *,float *,float *,int);
void legacy_drum909_render_st(void *,float *,float *,float *,float *,int);
void all909_reference_silence(void *,int);
static int fails;
#define CHECK(x) do {if(!(x)){if(fails++<15)fprintf(stderr,"FAIL line%d %s\n",__LINE__,#x);}}while(0)
static int closef(float x,float y) {return fabsf(x-y)<2e-6f;}
static void silence(drum909_t *d,int v)
{
    if(v<=DR_SD)d->bt[v].mute=0;
    else if(v<=DR_HT)d->tom[v-DR_LT].mute=0;
    else if(v==DR_RS)d->rim.mute=0;
    else if(v==DR_CP)d->clap.mute=0;
    else d->smp[v-DR_CH].mute=0;
}
static void bypass(void *ref)
{
    drum909_t d;
    float a[4][256],b[4][256];
    const int sizes[]={1,255,256,13,32,128};
    drum909_init(&d);legacy_drum909_init(ref);
    CHECK(sizeof(d)-all909_reference_size()==800);
    sound_strip_config_t cfg;sound_strip_config_default(&cfg);
    cfg.flags=SOUND_STRIP_HIGHPASS|SOUND_STRIP_LOWPASS|SOUND_STRIP_LIMITER;cfg.boost_db=24;
    for(int v=0;v<DR_NUM;++v) {
        CHECK(drum909_strip_set(&d,v,&cfg)==0);
        int pan=drum909_nparams(v)-1;
        for(int i=pan-2;i<=pan;++i) {
            int val=i==pan?13+v*9:89;
            drum909_set(&d,v,i,val);legacy_drum909_set(ref,v,i,val);
        }
    }
    for(int block=0;block<1200;++block) {
        if(block%7==0)for(int v=0;v<DR_NUM;++v) {
            drum909_trigger(&d,v,0.8f);legacy_drum909_trigger(ref,v,0.8f);
        }
        memset(a,0,sizeof(a));memset(b,0,sizeof(b));
        int n=sizes[block%6];drum909_strip_meter_clear(&d);
        drum909_render_st(&d,a[0],a[1],a[2],a[3],n);
        legacy_drum909_render_st(ref,b[0],b[1],b[2],b[3],n);
        CHECK(!memcmp(a,b,sizeof(a)));
        for(int v=0;v<DR_NUM;++v) {
            const sound_strip_meter_t *m=drum909_strip_meter(&d,v);
            CHECK(m->input_peak==0 && m->output_peak==0 && m->gain_min==1);
        }
    }
    puts("PASS all11 default/settled bypass bitwise: mixed voices, variable blocks, pan/sends");
}
static void voice(void *ref,int v)
{
    drum909_t d;
    sound_strip_t oracle;
    sound_strip_config_t c;
    float out[4][256],raw[256],unused[256];
    drum909_init(&d);legacy_drum909_init(ref);sound_strip_init(&oracle);
    sound_strip_config_default(&c);c.flags=SOUND_STRIP_FLAGS;c.boost_db=24;c.ceiling_db=18;
    c.highpass_hz=500;c.lowpass_hz=1500;
    CHECK(drum909_strip_set(&d,v,&c)==0);CHECK(sound_strip_set(&oracle,&c)==0);
    int pan=drum909_nparams(v)-1;
    drum909_set(&d,v,pan,20);drum909_pan_settle(&d);
    d.send_rev[v]=0.31f;d.send_dly[v]=0.73f;
    int audible=0,limited=0,changed=0;
    for(int block=0;block<120;++block) {
        if(block<64 && block%11==0) {drum909_trigger(&d,v,1);legacy_drum909_trigger(ref,v,1);}
        if(block==30)c.ceiling_db=30;
        if(block==45)c.flags=SOUND_STRIP_ENABLED|SOUND_STRIP_LIMITER;
        if(block==58)c.flags=SOUND_STRIP_FLAGS;
        if(block==65) {silence(&d,v);all909_reference_silence(ref,v);}
        if(block==75)c.flags=0;
        if(block==85)c.flags=SOUND_STRIP_FLAGS;
        if(block==92) {drum909_trigger(&d,v,0.7f);legacy_drum909_trigger(ref,v,0.7f);}
        if(block==30||block==45||block==58||block==75||block==85) {
            CHECK(drum909_strip_set(&d,v,&c)==0);CHECK(sound_strip_set(&oracle,&c)==0);
        }
        memset(out,0,sizeof(out));memset(raw,0,sizeof(raw));memset(unused,0,sizeof(unused));
        int processing=drum909_active(&d) && !sound_strip_bypassed(v?&d.strip_extra[v-1]:&d.bd_strip);
        legacy_drum909_render(ref,raw,unused,unused,256);
        sound_strip_meter_t expected={0,0,1};
        float before[256];memcpy(before,raw,sizeof(raw));
        if(processing)sound_strip_process(&oracle,raw,raw,256,&expected);
        drum909_strip_meter_clear(&d);drum909_render_st(&d,out[0],out[1],out[2],out[3],256);
        const sound_strip_meter_t *m=drum909_strip_meter(&d,v);
        CHECK(closef(m->input_peak,expected.input_peak));CHECK(closef(m->output_peak,expected.output_peak));
        CHECK(closef(m->gain_min,expected.gain_min));
        if(m->gain_min<0.99f)limited=1;
        for(int i=0;i<256;++i) {
            CHECK(closef(out[0][i],raw[i]*d.pan_l[v]));CHECK(closef(out[1][i],raw[i]*d.pan_r[v]));
            CHECK(closef(out[2][i],raw[i]*0.31f));CHECK(closef(out[3][i],raw[i]*0.73f));
            if(fabsf(before[i])>1e-5f)audible=1;
            if(fabsf(raw[i]-before[i])>1e-4f)changed=1;
            if(c.flags&SOUND_STRIP_ENABLED)CHECK(fabsf(raw[i])<=fm_db2lin(-(float)c.ceiling_db)+1e-6f);
        }
        for(int other=0;other<DR_NUM;++other)if(other!=v) {
            const sound_strip_meter_t *m0=drum909_strip_meter(&d,other);
            CHECK(m0->input_peak==0 && m0->output_peak==0 && m0->gain_min==1);
        }
    }
    CHECK(audible && limited && changed);
    printf("PASS voice%d summed source->strip->pan/sends, response/ceiling, edits/retrigger/tails/meter\n",v);
}
static void accumulation(void *ref)
{
    drum909_t d;sound_strip_t oracle;sound_strip_config_t c;
    drum909_init(&d);legacy_drum909_init(ref);sound_strip_init(&oracle);
    sound_strip_config_default(&c);c.flags=SOUND_STRIP_FLAGS;c.boost_db=24;c.ceiling_db=12;
    drum909_strip_set(&d,DR_SD,&c);sound_strip_set(&oracle,&c);
    drum909_trigger(&d,DR_SD,1);legacy_drum909_trigger(ref,DR_SD,1);
    drum909_strip_meter_clear(&d);
    sound_strip_meter_t full={0,0,1},first={0,0,1};
    for(int chunk=0;chunk<8;++chunk) {
        float x[32]={0},y[32]={0},z[32]={0},raw[32]={0};
        legacy_drum909_render(ref,raw,y,z,32);
        sound_strip_meter_t meter;sound_strip_process(&oracle,raw,raw,32,&meter);
        if(!chunk)first=meter;
        sound_strip_meter_merge(&full,&meter);
        drum909_render(&d,x,y,z,32);
        const sound_strip_meter_t *m=drum909_strip_meter(&d,DR_SD);
        CHECK(closef(m->input_peak,full.input_peak));CHECK(closef(m->output_peak,full.output_peak));
        CHECK(closef(m->gain_min,full.gain_min));
        CHECK(m->input_peak>=first.input_peak && m->gain_min<=first.gain_min);
    }
    drum909_strip_meter_clear(&d);
    const sound_strip_meter_t *m=drum909_strip_meter(&d,DR_SD);
    CHECK(m->input_peak==0 && m->output_peak==0 && m->gain_min==1);
    CHECK(drum909_strip_set(&d,-1,&c)==-1 && drum909_strip_set(&d,DR_NUM,&c)==-1);
    CHECK(!drum909_strip_meter(&d,-1) && !drum909_strip_meter(&d,DR_NUM));
    c.highpass_hz=0;
    sound_strip_t saved=d.strip_extra[DR_SD-1];
    CHECK(drum909_strip_set(&d,DR_SD,&c)==-1 && !memcmp(&saved,&d.strip_extra[DR_SD-1],sizeof(saved)));
    puts("PASS meter max-peaks/min-gain accumulate eight event chunks until explicit outer clear; invalid config unchanged");
}
static void mixed_enabled(void)
{
    drum909_t d;drum909_init(&d);
    sound_strip_config_t c;sound_strip_config_default(&c);
    c.flags=SOUND_STRIP_FLAGS;c.boost_db=24;
    for(int v=0;v<DR_NUM;++v) {
        c.ceiling_db=(uint8_t)(9+v);c.highpass_hz=(uint16_t)(50+31*v);c.lowpass_hz=(uint16_t)(1000+100*v);
        CHECK(drum909_strip_set(&d,v,&c)==0);
    }
    int seen[DR_NUM]={0},reduced[DR_NUM]={0};
    for(int block=0;block<180;++block) {
        if(block%8==0) {
            for(int v=0;v<DR_NUM;++v)if(v!=DR_CH && v!=DR_OH)drum909_trigger(&d,v,1);
            drum909_trigger(&d,DR_OH,1);
        }
        if(block%8==4)drum909_trigger(&d,DR_CH,1);
        float l[256]={0},r[256]={0},rev[256]={0},dly[256]={0};
        drum909_strip_meter_clear(&d);
        drum909_render_st(&d,l,r,rev,dly,256);
        for(int v=0;v<DR_NUM;++v) {
            const sound_strip_meter_t *m=drum909_strip_meter(&d,v);
            CHECK(m->input_peak<=64.0f && m->gain_min>=0 && m->gain_min<=1);
            CHECK(m->output_peak<=fm_db2lin(-(float)(9+v))+1e-6f);
            if(m->input_peak>1e-5f)seen[v]=1;
            if(m->gain_min<0.99f)reduced[v]=1;
        }
        for(int i=0;i<256;++i)CHECK(isfinite(l[i]) && isfinite(r[i]));
    }
    for(int v=0;v<DR_NUM;++v)CHECK(seen[v] && reduced[v]);
    puts("PASS all11 simultaneous voices retain independent ceilings/meters through retriggers and CH/OH choke");
}
static double benchmark(void *ref,int mode)
{
    drum909_t d;drum909_init(&d);legacy_drum909_init(ref);
    sound_strip_config_t c;sound_strip_config_default(&c);c.flags=SOUND_STRIP_FLAGS;c.boost_db=24;
    if(mode==2)for(int v=0;v<DR_NUM;++v)drum909_strip_set(&d,v,&c);
    clock_t start=clock();
    for(int block=0;block<4096;++block) {
        if(block%128==0)for(int v=0;v<DR_NUM;++v) {
            if(mode==0)legacy_drum909_trigger(ref,v,1);else drum909_trigger(&d,v,1);
        }
        float x[4][256]={{0}};
        if(mode==0)legacy_drum909_render_st(ref,x[0],x[1],x[2],x[3],256);
        else {drum909_strip_meter_clear(&d);drum909_render_st(&d,x[0],x[1],x[2],x[3],256);}
    }
    return (double)(clock()-start)/(double)CLOCKS_PER_SEC;
}
int main(void)
{
    void *ref=malloc(all909_reference_size());if(!ref)return 2;
    bypass(ref);for(int v=0;v<DR_NUM;++v)voice(ref,v);accumulation(ref);mixed_enabled();
    printf("all909 strips: %d failures, state=%zu B; added=%zu B\n",fails,sizeof(drum909_t),sizeof(drum909_t)-all909_reference_size());
    const char *skip=getenv("ALL909_SKIP_BENCH");
    if(!skip || strcmp(skip,"1"))for(int run=0;run<3;++run) {
        double t[3];for(int i=0;i<3;++i){int mode=(run+i)%3;t[mode]=benchmark(ref,mode);}
        printf("Host all909 1048576frames run%d accepted=%.6fs bypass=%.6fs enabled=%.6fs; bypass/accepted=%.3f enabled/accepted=%.3f\n",run+1,t[0],t[1],t[2],t[1]/t[0],t[2]/t[0]);
    }
    free(ref);return fails?1:0;
}
