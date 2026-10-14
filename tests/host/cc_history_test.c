/* SPDX-License-Identifier: GPL-3.0-only */
/* Real project/journal/CC ingress; no storage/device/browser substitutes. */
#define _POSIX_C_SOURCE 200809L
#define PLAT_CONTROL_TEST
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../firmware/src/app/engine.c"
static int locked,irq_start;
uint32_t plat_control_enter(void){assert(!locked);if(irq_start){irq_start=0;seq.req_start=1;}locked=1;return 37;}
void plat_control_leave(uint32_t saved){assert(locked&&saved==37);locked=0;}
static void render(void){int32_t audio[512];engine_render(audio,256);}
static void fresh(void){
    for(int i=0;i<OBJ_NOBJ;i++)store_len[i]=-1;
    memset(stored_known,0,sizeof stored_known);project_discard_rejected();
    mi_w=mi_r=mi_ready=0;mi_claimed=0xffffffffu;memset(&cc_stats,0,sizeof cc_stats);
    /* Recreate a process boot after deliberately injected persistent states. */
    seq.transport_hold=seq.transport_stage=0;seq.req_start=seq.req_stop=seq.req_cont=seq.playing=0;
    ui.rec=ui.btn=ui.keys=0;perf.test=-1;perf.phase=0;
    boot();render();for(int i=0;i<CC_SOURCES;i++)cc_reset_source(i);ui.dirty=0;
}
static int pot(void){return proj.sound.v[T_909][0][0];}
static void edit(int value){ui_external_edit(1);ui_external_sound(T_909,0,0,value);render();ui_external_edit(0);}
static uint32_t history_packet(int redo,int raw){return 11u|(0xbcu<<8)|((uint32_t)(93+redo)<<16)|((uint32_t)raw<<24);}
static int history(int redo,int raw,int source){return cc_message(history_packet(redo,raw),(uint8_t)source);}
static project_t before_project;static undo_t before_undo;static uint8_t before_shadow[sizeof undo_shadow];
static uint32_t before_q[CQ],before_read,before_written,before_store;static uint8_t before_owner,before_dirty,before_edges[CC_SOURCES];
static void remember(void){before_project=proj;before_undo=undo;memcpy(before_shadow,undo_shadow,sizeof before_shadow);memcpy(before_q,cq,sizeof cq);
    before_read=engine_queue_read();before_written=engine_queue_written();before_store=store_writes;before_owner=external_edit;before_dirty=ui.dirty;
    for(int i=0;i<CC_SOURCES;i++)before_edges[i]=cc_sources[i].action_high;
}
static void unchanged(void){assert(!memcmp(&before_project,&proj,sizeof proj));assert(!memcmp(&before_undo,&undo,sizeof undo));assert(!memcmp(before_shadow,undo_shadow,sizeof undo_shadow));
    assert(!memcmp(before_q,cq,sizeof cq));assert(before_read==engine_queue_read()&&before_written==engine_queue_written());assert(before_store==store_writes&&before_owner==external_edit&&before_dirty==ui.dirty);
    for(int i=0;i<CC_SOURCES;i++)assert(before_edges[i]==cc_sources[i].action_high);
}
static void edges(void){
    for(int source=0;source<CC_SOURCES;source++){
        fresh();int original=pot();edit(12);edit(40);ui.part=PART_BRK;seq.ppat[PART_BRK]=31;
        assert(history(0,63,source)==0&&pot()==40);assert(history(0,64,source)==0&&pot()==12);render();
        remember();assert(history(0,127,source)==0);unchanged();
        assert(history(0,0,source)==0);assert(history(0,127,source)==0&&pot()==original);render();
        assert(history(1,127,source)==0&&pot()==12);render();remember();assert(history(1,127,source)==0);unchanged();
        assert(history(1,0,source)==0);assert(history(1,127,source)==0&&pot()==40);render();
        assert(seq.transport_hold==0);assert(store_writes==before_store);
        remember();assert(cc_parameter(1117,2,(uint8_t)source)==CC_REJECTED);assert(cc_parameter(1118,0xffffffffu,(uint8_t)source)==CC_REJECTED);unchanged();
    }
    fresh();edit(12);edit(40);assert(history(0,127,0)==0&&pot()==12);render();
    assert(history(0,127,1)==0&&pot()!=12);render();assert(history(1,127,2)==0&&pot()==12);render();
    cc_reset_source(2);assert(history(1,127,2)==0&&pot()==40);render();
    puts("CC history: both raw boolean endpoints, repeated edges, independent sources/reset and exact bounds PASS");
}
static void rejected_guards(void){
    for(int guard=0;guard<12;guard++){
        fresh();edit(12);edit(40);assert(history(0,127,0)==0&&pot()==12);render();assert(history(0,0,0)==0);
        if(guard==0){ui_external_edit(1);ui_external_sound(T_909,0,0,44);render();}
        if(guard==1)ui.rec=1;
        if(guard==2)ui.btn=1;
        if(guard==3)ui.keys=1;
        if(guard==4)seq.playing=1;
        if(guard==5)seq.req_start=1;
        if(guard==6)seq.req_stop=1;
        if(guard==7)seq.req_cont=1;
        if(guard==8)seq.transport_stage=1;
        if(guard==9)seq.transport_hold=1;
        if(guard==10){perf.test=0;perf.phase=0;}
        if(guard==11)rejected_save=1;
        remember();assert(history(0,127,0)==CC_REJECTED);unchanged();assert(history(1,127,0)==CC_REJECTED);unchanged();
        if(guard==0){assert(external_edit);ui_external_edit(0);assert(!external_edit);}
    }
    fresh();edit(20);while(engine_queue_space()>=768)assert(cq_put(C_STRIP,255,0,0));
    remember();assert(history(0,127,0)==CC_REJECTED);unchanged();render();assert(history(0,127,0)==0&&pot()!=20);render();
    /* A Start request arriving at the reservation's lock cannot race Undo. */
    fresh();edit(20);remember();irq_start=1;assert(history(0,127,0)==CC_REJECTED);unchanged();assert(seq.req_start&&seq.transport_hold==0);
    /* Release never changes the project and can rearm under protection. */
    fresh();edit(20);assert(history(0,127,0)==0);render();rejected_save=1;assert(history(0,0,0)==0);assert(!(cc_sources[0].action_high&(1u<<5)));
    puts("CC history: owner/record/physical input/transport/stage/perf/protection/CQ/IRQ refusal preserves project, full journal, shadow, queue, owner and storage PASS");
}
static void ordering_and_exact(void){
    fresh();edit(20);seq_start(&seq);render();assert(seq.playing);
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,history_packet(0,127),0));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(0xfcu<<8),0));
    for(int i=0;i<4;i++){plat_midi_service();render();}assert(!seq.playing&&mi_r==mi_w&&pot()==20);assert(cc_stats.rejected==1);
    assert(history(0,127,0)==0&&pot()!=20);render();
    /* Exact canonical 0/1 values redispatch once, without borrowing UI focus. */
    fresh();edit(20);assert(!cc_parameter(1071,1,0));assert(!cc_parameter(1072,1117&127,0));assert(!cc_parameter(1073,1117>>7,0));
    assert(!cc_parameter(1074,1,0));assert(!cc_parameter(1075,0,0));assert(cc_parameter(1077,1,0)==CC_REJECTED&&pot()==20);
    assert(!cc_parameter(1130,0,0));assert(!cc_parameter(1077,1,0)&&pot()!=20);render();
    assert(cc_sources[0].exact_parts==0);remember();assert(cc_parameter(1077,1,0)==CC_REJECTED);unchanged();
    /* Global settings stay outside this journal: explicitly verify the limit. */
    fresh();edit(20);assert(!cc_parameter(907,100,0));render();assert(proj.set.bpm_x10==1200);assert(!history(0,127,0));render();assert(pot()!=20&&proj.set.bpm_x10==1200);
    /* Initialized lane state uses its existing nested stopped reservation. */
    fresh();ui_external_edit(1);assert(!cc_parameter(1131,1,0));render();ui_external_edit(0);assert(proj.variant.arrangement.flags&POLY_FLAG);
    assert(!history(0,127,0));render();assert(!(proj.variant.arrangement.flags&POLY_FLAG)&&seq.transport_hold==0);
    assert(!history(1,127,0));render();assert((proj.variant.arrangement.flags&POLY_FLAG)&&seq.transport_hold==0);
    puts("CC history: rejected history cannot pin realtimeStop, exact framing, lane Undo/Redo and explicit global-settings history gap PASS");
}
int main(void){edges();rejected_guards();ordering_and_exact();return 0;}
