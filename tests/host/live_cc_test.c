/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
static uint16_t number(int channel,int controller){return (uint16_t)((channel-1)*88+(controller<32?controller:controller-32));}
static uint32_t packet(int channel,int controller,int value){return 11u|((uint32_t)(175+channel)<<8)|((uint32_t)controller<<16)|((uint32_t)value<<24);}
static void fresh(void){
    for(int i=0;i<OBJ_NOBJ;i++)store_len[i]=-1;
    memset(stored_known,0,sizeof stored_known);project_discard_rejected();
    mi_w=mi_r=mi_ready=0;mi_claimed=0xffffffffu;memset(&cc_stats,0,sizeof cc_stats);for(int s=0;s<CC_SOURCES;s++)cc_reset_source(s);
    boot();int32_t block[512];engine_render(block,256);ui.dirty=0;
}
static void render(void){int32_t block[512];engine_render(block,256);for(int i=0;i<512;i++)assert(block[i]>=-8388608 && block[i]<=8388607);}
static void context(int source,int pattern,int bass){
    /* Stable draft slots1010(pattern),1013(bass),1071(context commit). */
    assert(cc_parameter(1010,(uint32_t)pattern,(uint8_t)source)==0);
    assert(cc_parameter(1013,(uint32_t)bass,(uint8_t)source)==0);
    assert(cc_parameter(1071,1,(uint8_t)source)==0);
}
int main(void){
    fresh();seq_start(&seq);render();assert(seq.playing);
    assert(cc_message(packet(1,0,127),0)==0);assert(proj.sound.v[T_909][0][0]==127);assert(ui.dirty);render();
    assert(engine_queue_read()==engine_queue_written());
    /* Every native descriptor row, including KIT/FX/master/mixer, reaches the
     * authoritative canonical pot and existing audio CQ while playing. */
    for(unsigned i=0;i<CC_BINDINGS_COUNT;i++)if((cc_binding_words[i]&31u)==CCK_SOUND){
        uint32_t word=cc_binding_words[i];int t=(word>>5)&31,v=(word>>10)&15,j=(word>>14)&31;
        const x0x_param_t *d=engine_param(t,v,j);assert(d);
        assert(cc_parameter(cc_numbers[i],d->max,0)==0);assert(proj.sound.v[t][v][j]==d->max);render();
    }
    /* Fixed sound map does not move when note channels change. */
    seq.ch[0]=7;assert(cc_message(packet(1,0,0),1)==0);assert(proj.sound.v[T_909][0][0]==0);
    /* All25 strips are indexed directly; stage flags preserve other bits. */
    for(int s=0;s<25;s++){
        assert(cc_parameter((uint16_t)(576+s*12),20000,0)==0);
        assert(cc_parameter((uint16_t)(576+s*12+2),24,0)==0);
        assert(cc_parameter((uint16_t)(576+s*12+6),1,0)==0);
        assert(proj.variant.strip[s].highpass_hz==20000 && proj.variant.strip[s].boost_db==24 && (proj.variant.strip[s].flags&1));render();
    }
    /* Unknown/unsupported rows never become implemented by inventory existence. */
    context(0,0,0);
    assert(cc_parameter(1128,16,0)==CC_UNSUPPORTED);assert(cc_parameter(1129,7,0)==CC_UNSUPPORTED);
    assert(cc_message(packet(1,120,127),0)==CC_UNSUPPORTED);
    /* Explicit contexts never borrow the open panel or another input source. */
    context(0,3,0);context(1,4,1);ui.part=PART_BRK;seq.ppat[PART_BRK]=7;
    assert(cc_parameter(920,99,0)==0);assert(cc_parameter(920,12,1)==0);
    assert(proj.pat[3].bass[0].gen.density==99 && proj.pat[4].bass[1].gen.density==12);
    assert(cc_parameter(903,0,0)==0);assert(proj.pat[3].bass[0].len==1);
    assert(cc_parameter(904,3,1)==0);assert(proj.pat[4].bass[1].rate==3);
    assert(cc_parameter(923,2,0)==0);assert(proj.pat[3].bass[0].gen.oct_range==3);
    assert(cc_parameter(1048,1,0)==0);assert(proj.pat[3].bass[0].step[0].note<=127);render();
    /* Seed/motion/full-condition integers retain their exact canonical values. */
    int lk=motion_alloc(proj.arr.lane,3,PART_909,T_PROB,0,0);assert(lk==0);
    for(int condition=101;condition<=135;condition++){assert(cc_parameter(1031,(uint32_t)condition,0)==0);assert(proj.arr.lane[0].val[0]==condition);render();}
    assert(cc_parameter(1031,255,0)==0);assert(proj.arr.lane[0].val[0]==255);
    assert(cc_parameter(1031,136,0)==CC_REJECTED);
    /* Ordinary CC exact transaction accepts three 7-bit chunks (21 bits).
     * Browser BPM setter remains unsupported, not falsely claimed as live. */
    assert(cc_parameter(1072,1107&127,0)==0);assert(cc_parameter(1073,1107>>7,0)==0);
    assert(cc_parameter(1074,300000&127,0)==0);assert(cc_parameter(1075,(300000>>7)&127,0)==0);assert(cc_parameter(1130,300000>>14,0)==0);
    assert(cc_parameter(1077,1,0)==CC_UNSUPPORTED);assert(cc_sources[0].exact==300000);
    /* Source reset drops incomplete transactions instead of rebinding UI focus. */
    cc_reset_source(0);assert(cc_parameter(1077,1,0)==CC_REJECTED);
    /* Protected storage cannot suppress the safety Stop control. */
    rejected_save=1;assert(cc_parameter(0,33,0)==CC_REJECTED);
    assert(cc_parameter(1120,1,0)==0);render();assert(!seq.playing);rejected_save=0;
    /* Reset removes unreviewed old controls from this source only; queued
     * Note-Off/Stop stay ordered and another controller's context survives. */
    fresh();assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet(1,0,77),0));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet(1,0,88),1));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,8u|(0x82u<<8)|(60u<<16),0));
    cc_reset_source(0);plat_midi_service();assert(proj.sound.v[T_909][0][0]==88);
    uint32_t left;assert(plat_midi_in(&left)&&((left>>8)&255u)==0x82);assert(!plat_midi_in(&left));
    /* IRQ intake does not touch canonical state; adjacent latest wins safely. */
    fresh();assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet(1,0,40),0));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet(1,0,70),0));assert(mi_w==1 && cc_ingress_coalesced);
    uint32_t read;assert(!plat_midi_in(&read));assert(proj.sound.v[T_909][0][0]!=70);
    plat_midi_service();assert(proj.sound.v[T_909][0][0]==70);assert(!plat_midi_in(&read));
    /* A claimed control cannot be overwritten by an interrupt while main applies it. */
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet(1,0,10),0));mi_claimed=mi_w-1;
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet(1,0,20),0));assert(mi_w==3);mi_claimed=0xffffffffu;
    /* Accepted CC-before-note ordering survives main handoff; audio never calls CC. */
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,9u|(0x92u<<8)|(60u<<16)|(100u<<24),0));
    assert(!plat_midi_in(&read));plat_midi_service();assert(proj.sound.v[T_909][0][0]==20);
    assert(plat_midi_in(&read)&&((read>>8)&255)==0x92);assert(!plat_midi_in(&read));
    /* Capacity reserve admits realtimeStop after CC saturation; bounded service settles. */
    fresh();for(unsigned i=0;i<MQ-16;i++)assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet(1,32+(i%32),i%128),0));
    assert(!cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet(1,32,1),0));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(0xfcu<<8),0));
    for(int i=0;i<9;i++){plat_midi_service();render();}assert(mi_w==mi_r);
    puts("Live CC native: main-only intake, fixed channels, all25 strips, coalescing/claim guard, ordered handoff, reserved Stop capacity and explicit lane gaps PASS");
    return 0;
}
