/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual application + engines; delayed core2 job validates publication and join ownership. */
#define _POSIX_C_SOURCE 200809L
#define plat_cpu2_run host_cpu2_run
#define plat_cpu2_wait host_cpu2_wait
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#undef plat_cpu2_run
#undef plat_cpu2_wait
#include <assert.h>
static void (*pending_job)(uint32_t);
static uint32_t pending_arg, dispatches, joins;
static int dual, watched=-1;
static float job_peak;
int plat_cpu2_run(void (*fn)(uint32_t),uint32_t arg)
{
    assert(!pending_job); if(!dual)return 0;
    pending_job=fn;pending_arg=arg;dispatches++;return 1;
}
int plat_cpu2_wait(void)
{
    assert(pending_job);pending_job(pending_arg);pending_job=0;joins++;
    if(watched>=0) {const sound_strip_meter_t *m=engine_strip_meter(watched);if(m->output_peak>job_peak)job_peak=m->output_peak;}
    return 0;
}
static int32_t audio[512], active_capture[100*512];
static void render(void) {engine_render(audio,256);assert(!pending_job);}
static void fresh(void)
{
    int i;for(i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
    memset(stored_known,0,sizeof stored_known);project_discard_rejected();boot();
    engine_guard_enable(0);engine_usb_full(1);render();
}
static int command(int strip,int index,int value)
{
    variant_command_t c={1,VC_SET_STRIP,11,(uint8_t)strip,0,(uint8_t)index,0,value};variant_reply_t r;
    return variant_command(&c,&r);
}
static void trigger(int strip,int down)
{
    if(strip<22) {if(down)engine_drum(strip/11,strip%11,0.8f);}
    else if(strip<24) {if(down)engine_bass_on(strip-22,48,0,0);else engine_bass_off(strip-22);}
    else engine_brk_live(0,down);
}
static double capture(int strip,int active,int use_dual)
{
    double energy=0;float peak=0;int block,i;
    dual=use_dual;watched=strip;job_peak=0;fresh();
    /* Every config exists already; coalesce control bytes before any secondary job. */
    assert(command(strip,VP_BOOST,24)==VR_QUEUED);
    assert(command(strip,VP_CEILING,48)==VR_QUEUED);
    assert(command(strip,VP_FLAGS,active?9:0)==VR_QUEUED);
    for(block=0;block<8;block++)render();
    assert(engine_strip_config(strip)->boost_db==24 && engine_strip_config(strip)->ceiling_db==48);
    trigger(strip,1);
    for(block=0;block<100;block++) {
        if(block==20)trigger(strip,0);
        render();
        if(active && use_dual) assert(!memcmp(active_capture+block*512,audio,sizeof audio));
        else if(active) memcpy(active_capture+block*512,audio,sizeof audio);
        for(i=0;i<512;i++) {double x=(double)audio[i]/8388608.0;energy+=x*x;}
        if(active) {
            const sound_strip_meter_t *m=engine_strip_meter(strip);
            assert(m && m->input_peak>=0 && m->output_peak<=0.003982f && m->gain_min>=0 && m->gain_min<=1);
            if(m->input_peak>peak)peak=m->input_peak;
            for(i=0;i<25;i++)if(i!=strip)assert(engine_strip_meter(i)->input_peak==0 && engine_strip_meter(i)->output_peak==0);
        }
    }
    if(active)assert(peak>1e-5f);
    return energy;
}
static void paths(void)
{
    int strip;variant_snapshot_t s;
    for(strip=0;strip<25;strip++) {
        double dry=capture(strip,0,0),limited=capture(strip,1,0),dual_energy=capture(strip,1,1);
        assert(dry>1e-8 && limited<dry*0.5);
        assert(limited==dual_energy); /* identical DSP/mix order after delayed join */
    }
    assert(dispatches==joins && dispatches>0);
    assert(variant_snapshot(VS_INFO,0,0,&s)==VR_OK && s.value[7]==0x01ffffffu);
    for(strip=0;strip<25;strip++)assert(variant_snapshot(VS_METERS,strip,0,&s)==VR_OK && s.count==3);
    assert(!engine_strip_meter(-1) && !engine_strip_meter(25));
    puts("all25 engine: isolated audible limiting, other-sound isolation, full mask/meters and delayed-core2 bitwise PCM PASS");
}
static void queue_controls(void)
{
    variant_settings_t before;uint32_t written,accepted=0;
    dual=1;watched=-1;fresh();before=proj.variant;
    assert(command(25,VP_FLAGS,1)==VR_INVALID && command(24,VP_FLAGS,16)==VR_INVALID);
    assert(command(24,VP_COUNT,1)==VR_INVALID);
    assert(!memcmp(&before,&proj.variant,sizeof before));
    while(engine_strip_set(24,VP_BOOST,(int)(accepted%25u))) accepted++;
    assert(accepted==1024u);before=proj.variant;written=engine_queue_written();
    assert(command(22,VP_FLAGS,1)==VR_BUSY && command(11,VP_BOOST,9)==VR_BUSY);
    assert(!memcmp(&before,&proj.variant,sizeof before) && written==engine_queue_written());
    render();assert(engine_queue_read()==written && engine_strip_config(24)->boost_db==(accepted-1u)%25u);
    for(int i=0;i<25;i++)assert(command(i,VP_FLAGS,1)==VR_QUEUED);
    render();for(int i=0;i<25;i++)assert(engine_strip_config(i)->flags==1);
    puts("all25 engine: invalid/capacity rejection preserves desired configs; coalesced final controls applied before core2 dispatch PASS");
}

static void split_meter(void)
{
    const sound_strip_meter_t *m;
    dual=1;watched=11;fresh();
    assert(command(11,VP_BOOST,12)==VR_QUEUED && command(11,VP_FLAGS,1)==VR_QUEUED);
    render();render();
    seq.playing=1;seq.send_clock=1;seq.clk_to_next=64u<<8;
    for(int i=0;i<NTRACKS;i++)seq.t[i].to_next=0xffffffffu;
    trigger(11,1);job_peak=0;uint32_t before=dispatches;render();
    assert(dispatches-before>=2);m=engine_strip_meter(11);
    assert(m->output_peak>0 && m->output_peak==job_peak); /* complete-block aggregate, not final event stretch */
    puts("all25 engine: meter peak retained across event stretches until outer-block clear PASS");
}
static void tail_policy(void)
{
    sound_strip_t state;sound_strip_config_t c;float mono[256];sound_strip_meter_t m;
    sound_strip_init(&state);sound_strip_config_default(&c);c.flags=15;c.highpass_hz=20;c.lowpass_hz=20000;c.ceiling_db=48;
    assert(!sound_strip_active(&state));assert(!sound_strip_set(&state,&c));
    for(int i=0;i<256;i++)mono[i]=4;
    sound_strip_process(&state,mono,mono,256,&m);assert(sound_strip_active(&state));
    int block;
    for(block=0;block<4000 && sound_strip_active(&state);block++) {
        memset(mono,0,sizeof mono);sound_strip_process(&state,mono,mono,256,&m);
    }
    assert(block<4000 && state.gain==1.0f && !sound_strip_active(&state));
    c.flags=0;assert(!sound_strip_set(&state,&c));
    for(block=0;block<8 && !sound_strip_bypassed(&state);block++) {memset(mono,0,sizeof mono);sound_strip_process(&state,mono,mono,256,&m);}
    assert(sound_strip_bypassed(&state) && !sound_strip_active(&state));
    puts("all25 common: finite enabled silent filter/limiter tails and exact settled disable PASS");
}
int main(void){tail_policy();paths();queue_controls();split_meter();return 0;}
