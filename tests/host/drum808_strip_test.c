/* SPDX-License-Identifier: GPL-3.0-only */
/* White-box coverage of the active-only aggregator plus public audio API. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../../firmware/src/dsp/drum808.c"
void baseline_drum808_init(drum808_t *d);
void baseline_drum808_trigger(drum808_t *d, int t, float v);
void baseline_drum808_render_st(drum808_t *d, float *l, float *r, float *v, float *y, int n);
void baseline_drum808_set(drum808_t *d, int t, int p, int v);
static int fails;
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c); fails++; } } while (0)
static drum808_t a,b;
static void zero(float x[4][256]) { memset(x,0,4*256*sizeof(float)); }
static void bypass_identity(void)
{
    float x[4][256], y[4][256]; int t,i,k;
    drum808_init(&a); baseline_drum808_init(&b);
    /* Every track, alternate switches, noncentral pan, sends, drive, velocities,
     * simultaneous ringing pairs and retriggers/chokes, variable event slices. */
    for (k=0;k<300;k++) {
        t=k%D8_NUM;
        for(i=0;i<drum808_nparams(t);i++) {
            int value=(k*17+i*23)%128;
            drum808_set(&a,t,i,value); baseline_drum808_set(&b,t,i,value);
        }
        drum808_trigger(&a,t,(float)(k%9+1)/9.0f);
        baseline_drum808_trigger(&b,t,(float)(k%9+1)/9.0f);
        zero(x); zero(y);
        { int n=(k*37)%256+1;
          drum808_render_st(&a,x[0],x[1],x[2],x[3],n);
          baseline_drum808_render_st(&b,y[0],y[1],y[2],y[3],n);
          CHECK(!memcmp(x,y,sizeof x)); }
    }
}
static sound_strip_config_t config(void)
{
    sound_strip_config_t c; sound_strip_config_default(&c);
    c.flags=SOUND_STRIP_ENABLED|SOUND_STRIP_HIGHPASS|SOUND_STRIP_LOWPASS|SOUND_STRIP_LIMITER;
    c.highpass_hz=180; c.lowpass_hz=2700; c.boost_db=24; c.ceiling_db=12;
    return c;
}
static void all_tracks(void)
{
    int t,k,i; float x[4][256]; sound_strip_config_t c=config();
    for(t=0;t<D8_NUM;t++) {
        drum808_init(&a); CHECK(!drum808_strip_set(&a,t,&c));
        drum808_trigger(&a,t,1.0f); drum808_strip_meter_clear(&a);
        for(k=0;k<50;k++) {
            zero(x); drum808_render_st(&a,x[0],x[1],x[2],x[3],256);
            for(i=0;i<256;i++) { CHECK(isfinite(x[0][i])); CHECK(fabsf(x[0][i])<=a.strip[t].ceiling_target+1e-6f); }
        }
        CHECK(drum808_strip_meter(&a,t)->input_peak>0);
        CHECK(drum808_strip_meter(&a,t)->output_peak>0);
        CHECK(drum808_strip_meter(&a,t)->gain_min<=1);
        { sound_strip_meter_t saved=*drum808_strip_meter(&a,t);
          zero(x); drum808_render(&a,x[0],x[2],x[3],1);
          CHECK(a.strip_meter[t].input_peak>=saved.input_peak);
          CHECK(a.strip_meter[t].output_peak>=saved.output_peak);
          CHECK(a.strip_meter[t].gain_min<=saved.gain_min); }
        drum808_strip_meter_clear(&a); CHECK(a.strip_meter[t].input_peak==0 && a.strip_meter[t].gain_min==1);
    }
}
static void response(void)
{
    int t,k,i; float x[4][256], y[4][256], warm[256]; sound_strip_config_t c=config();
    for(t=0;t<D8_NUM;t++) {
        sound_strip_t reference;
        drum808_init(&a); drum808_init(&b); sound_strip_init(&reference);
        drum808_strip_set(&a,t,&c); sound_strip_set(&reference,&c);
        memset(warm,0,sizeof warm); sound_strip_process(&reference,warm,warm,256,0);
        zero(x); drum808_render(&a,x[0],x[2],x[3],256);
        /* Complete-voice placement includes level/velocity and nonzero shaping. */
        { int snd=track_sound(&a,t); set_pot(&a,snd,D8P_DRIVE,80); set_pot(&b,snd,D8P_DRIVE,80);
          set_pot(&a,snd,D8P_LEVEL,85); set_pot(&b,snd,D8P_LEVEL,85); }
        drum808_trigger(&a,t,0.63f); drum808_trigger(&b,t,0.63f);
        for(k=0;k<12;k++) {
            zero(x); zero(y); drum808_render(&a,x[0],x[2],x[3],256); drum808_render(&b,y[0],y[2],y[3],256);
            sound_strip_process(&reference,y[0],y[0],256,0);
            for(i=0;i<256;i++) CHECK(fabsf(x[0][i]-y[0][i])<2e-6f);
        }
    }
}
static void pairs(void)
{
    int tr,k,i; float x[4][256],y[4][256], scratch[256]; sound_strip_config_t c=config();
    for(tr=D8_RS;tr<=D8_CP;tr++) {
        sound_strip_t reference;
        drum808_init(&a); drum808_init(&b); sound_strip_init(&reference);
        CHECK(!drum808_strip_set(&a,tr,&c)); sound_strip_set(&reference,&c);
        memset(scratch,0,sizeof scratch); sound_strip_process(&reference,scratch,scratch,256,0);
        zero(x); drum808_render(&a,x[0],x[2],x[3],256);
        /* Direct switch is deliberate: both physical voices retain their tails. */
        a.sw[tr]=b.sw[tr]=0; drum808_trigger(&a,tr,1); drum808_trigger(&b,tr,1);
        a.sw[tr]=b.sw[tr]=1; drum808_trigger(&a,tr,0.8f); drum808_trigger(&b,tr,0.8f);
        /* Active paired track follows currently selected sound send controls. */
        a.potv[track_sound(&a,tr)][D8P_REV]=0.37f;
        a.potv[track_sound(&a,tr)][D8P_DLY]=0.61f;
        for(k=0;k<40;k++) {
            zero(x); zero(y); drum808_render(&a,x[0],x[2],x[3],256); drum808_render(&b,y[0],y[2],y[3],256);
            sound_strip_process(&reference,y[0],y[0],256,0);
            for(i=0;i<256;i++) {
                CHECK(fabsf(x[0][i]-y[0][i])<2e-6f);
                CHECK(fabsf(x[2][i]-x[0][i]*0.37f)<1e-6f);
                CHECK(fabsf(x[3][i]-x[0][i]*0.61f)<1e-6f);
            }
        }
    }
    /* Cancellation must never divide output by raw sum; filters retain a tail
     * even when opposing completed lanes sum to exactly zero. */
    drum808_init(&a); { float in[32], sum[32]; int l;
      for(i=0;i<32;i++) { in[i]=(i&1)?1.0f:-1.0f; sum[i]=0; }
      for(l=L_RS;l<=L_CL;l++) { int s=a.lane[l].snd; a.shp[s].drive=0; a.potv[s][D8P_LEVEL]=1/k_trim[s]; a.lane[l].hit=(l==L_RS)?1:-1; }
      strip_lane_add(&a,&a.lane[L_RS],in,32,sum); strip_lane_add(&a,&a.lane[L_CL],in,32,sum);
      for(i=0;i<32;i++) CHECK(fabsf(sum[i])<1e-6f);
      sound_strip_set(&a.strip[D8_RS],&c); a.strip[D8_RS].mix=1;
      in[0]=1; for(i=1;i<32;i++) in[i]=0;
      sound_strip_process(&a.strip[D8_RS],in,in,32,0);
      sound_strip_process(&a.strip[D8_RS],sum,sum,32,0);
      CHECK(isfinite(sum[0]) && fabsf(sum[0])>1e-10f);
    }
}
static void idle_tail(void)
{
    float x[4][256]; int k; sound_strip_config_t c=config();
    drum808_init(&a); drum808_strip_set(&a,D8_BD,&c);
    /* No raw oscillator runs while strip-only ramps/tails drain. */
    { d8_bd_t saved=a.bd; for(k=0;k<4;k++) {zero(x);drum808_render(&a,x[0],x[2],x[3],256);} CHECK(!memcmp(&saved,&a.bd,sizeof saved)); }
    a.strip[D8_BD].hp_z[0]=a.strip[D8_BD].hp_z[1]=0.1f;
    CHECK(drum808_active(&a)); zero(x); drum808_render(&a,x[0],x[2],x[3],256); CHECK(x[0][0]!=0);
    for(k=0;k<2000 && drum808_active(&a);k++) { zero(x);drum808_render(&a,x[0],x[2],x[3],256); }
    CHECK(!drum808_active(&a));
    c.flags=0; drum808_strip_set(&a,D8_BD,&c);
    for(k=0;k<4;k++) { zero(x);drum808_render(&a,x[0],x[2],x[3],256); }
    CHECK(sound_strip_bypassed(&a.strip[D8_BD]));
    CHECK(drum808_strip_set(&a,-1,&c)==-1 && drum808_strip_meter(&a,D8_NUM)==0);
}
int main(void) { bypass_identity(); all_tracks(); response(); pairs(); idle_tail(); printf("808 strips: %d failures, state %zu B, meters %zu B\n",fails,sizeof(a.strip),sizeof(a.strip_meter)); return fails?1:0; }
