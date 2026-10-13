/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual 909 renderer + actual shared strip; optional independent pre-change oracle. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "dsp/drum909.h"
#include "dsp/fastmath.h"

#ifdef KICK_LEGACY_ORACLE
void legacy_drum909_init(drum909_t *);
void legacy_drum909_trigger(drum909_t *, int, float);
void legacy_drum909_render(drum909_t *, float *, float *, float *, int);
void legacy_drum909_render_st(drum909_t *, float *, float *, float *, float *, int);
void legacy_drum909_set(drum909_t *, int, int, int);
#else
#define legacy_drum909_init drum909_init
#define legacy_drum909_trigger drum909_trigger
#define legacy_drum909_render drum909_render
#define legacy_drum909_render_st drum909_render_st
#define legacy_drum909_set drum909_set
#endif

#define N 256
static int fails;
#define CHECK(x) do { if (!(x)) { if (fails < 20) fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); ++fails; } } while (0)
static void zero(float *a) { memset(a, 0, N * sizeof(*a)); }
static int near(float a, float b) { return fabsf(a-b) <= 2e-6f; }

static void bypass(void)
{
    drum909_t a, b;
    sound_strip_config_t config;
    float l[N], r[N], rev[N], dly[N], bl[N], br[N], brev[N], bdly[N];
    drum909_init(&a); legacy_drum909_init(&b);
    sound_strip_config_default(&config);
    config.boost_db = 24; config.highpass_hz = 999; config.lowpass_hz = 300;
    config.flags = SOUND_STRIP_HIGHPASS | SOUND_STRIP_LOWPASS | SOUND_STRIP_LIMITER;
    CHECK(drum909_bd_strip_set(&a, &config) == 0);
    for (int v=0; v<DR_NUM; ++v) {
        drum909_set(&a,v,drum909_nparams(v)-1,11 + v*9);
        legacy_drum909_set(&b,v,drum909_nparams(v)-1,11 + v*9);
        a.send_rev[v] = b.send_rev[v] = 0.31f;
        a.send_dly[v] = b.send_dly[v] = 0.72f;
    }
    for (int block=0; block<600; ++block) {
        if (block%7 == 0) {
            int v=(block/7)%DR_NUM;
            drum909_trigger(&a,v,0.9f); legacy_drum909_trigger(&b,v,0.9f);
        }
        zero(l);zero(r);zero(rev);zero(dly);zero(bl);zero(br);zero(brev);zero(bdly);
        drum909_render_st(&a,l,r,rev,dly,N);
        legacy_drum909_render_st(&b,bl,br,brev,bdly,N);
        CHECK(memcmp(l,bl,sizeof(l)) == 0 && memcmp(r,br,sizeof(r)) == 0);
        CHECK(memcmp(rev,brev,sizeof(rev)) == 0 && memcmp(dly,bdly,sizeof(dly)) == 0);
        CHECK(drum909_bd_strip_meter(&a)->input_peak == 0.0f && drum909_bd_strip_meter(&a)->output_peak == 0.0f);
        CHECK(drum909_bd_strip_meter(&a)->gain_min == 1.0f);
    }
    puts("PASS settled bypass: 600 mixed/retriggered stereo blocks bitwise");
}

static void integrated(void)
{
    drum909_t actual, raw;
    sound_strip_t external;
    sound_strip_config_t c;
    float l[N],r[N],rev[N],dly[N],input[N],unused[N];
    drum909_init(&actual); legacy_drum909_init(&raw); sound_strip_init(&external);
    sound_strip_config_default(&c);
    c.flags=SOUND_STRIP_FLAGS; c.boost_db=24; c.ceiling_db=9;
    c.highpass_hz=80; c.lowpass_hz=1500;
    CHECK(drum909_bd_strip_set(&actual,&c)==0); CHECK(sound_strip_set(&external,&c)==0);
    drum909_set(&actual,DR_BD,1,127); legacy_drum909_set(&raw,DR_BD,1,127);
    drum909_set(&actual,DR_BD,3,127); legacy_drum909_set(&raw,DR_BD,3,127);
    actual.accent=raw.accent=4.0f;
    actual.send_rev[DR_BD]=0.3f; actual.send_dly[DR_BD]=0.7f;
    drum909_set(&actual,DR_BD,10,23); drum909_pan_settle(&actual);
    int limited=0, changed=0;
    for (int block=0; block<180; ++block) {
        if (block%3==0 && block<90) {
            drum909_trigger(&actual,DR_BD,1.0f); legacy_drum909_trigger(&raw,DR_BD,1.0f);
        }
        if (block==35) { c.ceiling_db=24; c.highpass_hz=200; c.lowpass_hz=4000; }
        if (block==55) c.flags=SOUND_STRIP_ENABLED|SOUND_STRIP_LIMITER;
        if (block==70) c.flags=SOUND_STRIP_FLAGS;
        if (block==90) { actual.bt[DR_BD].mute=raw.bt[DR_BD].mute=1; }
        if (block==100) c.flags=0;
        if (block==110) { c.flags=SOUND_STRIP_FLAGS; c.ceiling_db=12; }
        if (block==120) { drum909_trigger(&actual,DR_BD,0.8f); legacy_drum909_trigger(&raw,DR_BD,0.8f); }
        if (block==35 || block==55 || block==70 || block==100 || block==110) {
            CHECK(drum909_bd_strip_set(&actual,&c)==0); CHECK(sound_strip_set(&external,&c)==0);
        }
        zero(l);zero(r);zero(rev);zero(dly);zero(input);zero(unused);
        int was_active=drum909_active(&actual) && !sound_strip_bypassed(&actual.bd_strip);
        legacy_drum909_render(&raw,input,unused,unused,N);
        sound_strip_meter_t m={0.0f,0.0f,1.0f};
        if(was_active) sound_strip_process(&external,input,input,N,&m);
        drum909_render_st(&actual,l,r,rev,dly,N);
        const sound_strip_meter_t *a=drum909_bd_strip_meter(&actual);
        CHECK(near(a->input_peak,m.input_peak) && near(a->output_peak,m.output_peak));
        CHECK(near(a->gain_min,m.gain_min));
        if(a->gain_min<0.99f)limited=1;
        if(block==90) CHECK(drum909_active(&actual));
        if(block>100 && block<110) CHECK(sound_strip_bypassed(&actual.bd_strip));
        for(int i=0;i<N;++i) {
            CHECK(near(l[i],input[i]*actual.pan_l[DR_BD]));
            CHECK(near(r[i],input[i]*actual.pan_r[DR_BD]));
            CHECK(near(rev[i],input[i]*0.3f) && near(dly[i],input[i]*0.7f));
            if(c.flags & SOUND_STRIP_ENABLED) CHECK(fabsf(input[i])<=fm_db2lin(-(float)c.ceiling_db)+1e-6f);
            if (block==90 && fabsf(input[i])>1e-6f)changed=1;
        }
    }
    CHECK(limited && changed);
    c.highpass_hz=0;
    sound_strip_t before=actual.bd_strip;
    CHECK(drum909_bd_strip_set(&actual,&c)==-1);
    CHECK(memcmp(&before,&actual.bd_strip,sizeof(before))==0);
    puts("PASS summed body/click/retriggers/tails, live ceilings/stages, pan/sends, meter and invalid config");
}

static void other_voices(void)
{
    for(int v=DR_SD;v<DR_NUM;++v) {
        drum909_t a,b;
        sound_strip_config_t c;
        float x[N],y[N],s[N],t[N];
        drum909_init(&a);legacy_drum909_init(&b);sound_strip_config_default(&c);
        c.flags=SOUND_STRIP_FLAGS;c.boost_db=24;c.ceiling_db=48;
        CHECK(drum909_bd_strip_set(&a,&c)==0);
        drum909_trigger(&a,v,1.0f);legacy_drum909_trigger(&b,v,1.0f);
        for(int k=0;k<100;++k) {
            zero(x);zero(y);zero(s);zero(t);
            drum909_render(&a,x,s,t,N);legacy_drum909_render(&b,y,s,t,N);
            CHECK(memcmp(x,y,sizeof(x))==0);
        }
    }
    puts("PASS all ten other voices retain bitwise legacy output with BD strip enabled");
}

static void response(void)
{
    double energy[3]={0.0,0.0,0.0};
    for(int mode=0;mode<3;++mode) {
        drum909_t d;
        sound_strip_config_t c;
        float x[N],s[N],t[N];
        drum909_init(&d);sound_strip_config_default(&c);
        c.flags=SOUND_STRIP_ENABLED;
        if(mode==1) { c.flags|=SOUND_STRIP_HIGHPASS; c.highpass_hz=1000; }
        if(mode==2) { c.flags|=SOUND_STRIP_LOWPASS; c.lowpass_hz=20; }
        CHECK(drum909_bd_strip_set(&d,&c)==0);drum909_trigger(&d,DR_BD,1.0f);
        for(int b=0;b<32;++b) {
            zero(x);zero(s);zero(t);drum909_render(&d,x,s,t,N);
            if(b>=16)for(int i=0;i<N;++i)energy[mode]+=(double)x[i]*(double)x[i];
        }
    }
    CHECK(energy[0]>0.001 && energy[1]<energy[0]*0.05 && energy[2]<energy[0]*0.3);
    printf("PASS actual kick filter response: HP1000 energy ratio %.4f; LP20 %.4f\n",energy[1]/energy[0],energy[2]/energy[0]);
    drum909_t d;
    sound_strip_config_t c;
    float x[N],s[N],t[N];
    drum909_init(&d);sound_strip_config_default(&c);c.flags=SOUND_STRIP_ENABLED;
    CHECK(drum909_bd_strip_set(&d,&c)==0);
    zero(x);zero(s);zero(t);drum909_render(&d,x,s,t,N);
    CHECK(!drum909_active(&d));
    c.flags|=SOUND_STRIP_LOWPASS;c.lowpass_hz=500;
    CHECK(drum909_bd_strip_set(&d,&c)==0);CHECK(drum909_active(&d));
    for(int b=0;b<3;++b) { zero(x);zero(s);zero(t);drum909_render(&d,x,s,t,N); }
    CHECK(!drum909_active(&d) && d.bd_strip.mix==1.0f);
    puts("PASS silent stage change completes both fade legs before next trigger");
}

static double bench(int mode)
{
    drum909_t d;
    float x[N],s[N],t[N];
    sound_strip_config_t c;
    if(mode==0)legacy_drum909_init(&d);else drum909_init(&d);
    sound_strip_config_default(&c);c.flags=SOUND_STRIP_FLAGS;c.boost_db=24;
    if(mode==2)drum909_bd_strip_set(&d,&c);
    clock_t start=clock();
    for(int b=0;b<8192;++b) {
        if(b%16==0) { if(mode==0)legacy_drum909_trigger(&d,DR_BD,1.0f);else drum909_trigger(&d,DR_BD,1.0f); }
        zero(x);zero(s);zero(t);
        if(mode==0)legacy_drum909_render(&d,x,s,t,N);else drum909_render(&d,x,s,t,N);
    }
    return (double)(clock()-start)/(double)CLOCKS_PER_SEC;
}

static void guard_tail(void)
{
    drum909_t d;
    sound_strip_config_t c;
    float x[N],s[N],t[N];
    drum909_init(&d);sound_strip_config_default(&c);
    c.flags=SOUND_STRIP_ENABLED|SOUND_STRIP_LIMITER;c.ceiling_db=48;c.boost_db=24;
    CHECK(drum909_bd_strip_set(&d,&c)==0);drum909_trigger(&d,DR_BD,1.0f);
    zero(x);zero(s);zero(t);drum909_render(&d,x,s,t,32);
    CHECK(d.bd_strip.guard_gain<1.0f && d.bd_strip.mix>0.0f);
    c.flags=0;CHECK(drum909_bd_strip_set(&d,&c)==0);
    zero(x);zero(s);zero(t);drum909_render(&d,x,s,t,32);
    CHECK(d.bd_strip.mix==0.0f && (d.bd_strip.flags>>16)>0);
    d.bt[DR_BD].mute=0;
    CHECK(drum909_active(&d) && !sound_strip_bypassed(&d.bd_strip));
    zero(x);zero(s);zero(t);drum909_render(&d,x,s,t,N);
    CHECK(sound_strip_bypassed(&d.bd_strip) && !drum909_active(&d));
    zero(x);zero(s);zero(t);drum909_render(&d,x,s,t,N);
    CHECK(drum909_bd_strip_meter(&d)->input_peak==0.0f && drum909_bd_strip_meter(&d)->output_peak==0.0f);
    CHECK(drum909_bd_strip_meter(&d)->gain_min==1.0f);
    puts("PASS final-guard release drains after interrupted fade reaches dry and BD becomes silent");
}

int main(void)
{
    bypass();integrated();other_voices();response();guard_tail();
    for(int run=0;run<3;++run) {
        double times[3];
        for(int i=0;i<3;++i) { int mode=(i+run)%3; times[mode]=bench(mode); }
        printf("Host CPU run%d, 2097152 frames: legacy=%.6fs bypass=%.6fs enabled=%.6fs; bypass/legacy=%.3f enabled/legacy=%.3f\n",run+1,times[0],times[1],times[2],times[1]/times[0],times[2]/times[0]);
    }
    printf("Memory: drum909=%zu B; BD strip=%zu B + meter=%zu B. Physical-device timing pending.\n",sizeof(drum909_t),sizeof(sound_strip_t),sizeof(sound_strip_meter_t));
#ifndef KICK_LEGACY_ORACLE
    puts("NOTE: no pre-change oracle supplied; bypass comparison is against current disabled renderer.");
#endif
    return fails?1:0;
}
