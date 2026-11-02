/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define PLAT_CONTROL_TEST
#define PLAT_STORE_TEST
static unsigned test_dma_timeouts,fail_dma_sync;
static void action_test_sync(void);
#define CV_STAGE_DMA_TIMEOUTS test_dma_timeouts
#define CV_STAGE_SYNC action_test_sync
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../firmware/src/app/engine.c"
static int locked,irq_start;
static void action_test_sync(void){lcd_sync();if(fail_dma_sync)test_dma_timeouts++;}
uint32_t plat_control_enter(void){assert(!locked);if(irq_start){irq_start=0;seq.req_start=1;}locked=1;return 37;}
void plat_control_leave(uint32_t saved){assert(locked&&saved==37);locked=0;}
static void render(void){int32_t audio[512];engine_render(audio,256);}
static void fresh(void){
    for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
    memset(stored_known,0,sizeof stored_known);project_discard_rejected();store_fail_after=-1;checkpoint_verified=0;
    mi_w=mi_r=mi_ready=0;mi_claimed=0xffffffffu;memset(&cc_stats,0,sizeof cc_stats);
    seq.transport_hold=seq.transport_stage=0;seq.req_start=seq.req_stop=seq.req_cont=seq.playing=0;
    test_dma_timeouts=fail_dma_sync=0;ui.rec=ui.btn=ui.keys=0;perf.test=-1;perf.phase=0;perf.cc_stop_epoch=UINT32_MAX;ui.overlay=0;save_wait=reset_checkpoint_hold=0;
    boot();render();for(int i=0;i<CC_SOURCES;i++)cc_reset_source(i);ui.dirty=0;
}
static void context(int source,int pattern,int part,int destination,int block){
    assert(!cc_parameter(1010,pattern,source));assert(!cc_parameter(1011,part,source));
    assert(!cc_parameter(1015,block&127,source));assert(!cc_parameter(1016,block>>7,source));
    if(destination>=0)assert(!cc_parameter(1132,destination,source));
    assert(!cc_parameter(1071,1,source));
}
static int action(int act,int value,int source){return cc_parameter((uint16_t)(984+act),(uint32_t)value,(uint8_t)source);}
static void arm(int act,int source){assert(!cc_parameter(1133,(uint32_t)act,(uint8_t)source));}
static void settled(void){undo_reset(&undo);ui.dirty=0;render();}
static project_t oldproj;static undo_t oldundo;static uint8_t oldshadow[sizeof undo_shadow];static seq_t oldseq;
static uint32_t oldcq[CQ],oldr,oldw,oldwrites,olddirty;static uint8_t oldowner;static uint16_t oldedges[CC_SOURCES];
static void remember(void){oldproj=proj;oldundo=undo;oldseq=seq;memcpy(oldshadow,undo_shadow,sizeof oldshadow);memcpy(oldcq,cq,sizeof oldcq);oldr=cq_r;oldw=cq_w;oldwrites=store_writes;olddirty=ui.dirty;oldowner=external_edit;for(int i=0;i<CC_SOURCES;i++)oldedges[i]=cc_sources[i].native_high;}
static void unchanged(int storage){assert(!memcmp(&oldproj,&proj,sizeof proj));assert(!memcmp(&oldundo,&undo,sizeof undo));assert(!memcmp(oldshadow,undo_shadow,sizeof oldshadow));assert(!memcmp(&oldseq,&seq,sizeof seq));assert(!memcmp(oldcq,cq,sizeof cq));assert(oldr==cq_r&&oldw==cq_w);assert(olddirty==ui.dirty&&oldowner==external_edit);if(storage)assert(oldwrites==store_writes);for(int i=0;i<CC_SOURCES;i++)assert(oldedges[i]==cc_sources[i].native_high);}
static void clear_copy(void){
    for(int part=0;part<5;part++){
        fresh();assert(engine_lane_mode(1));render();
        proj.pat[17].drum[0].hit[0][0]=3;proj.pat[17].drum[1].hit[2][1]=7;
        proj.pat[17].bass[0].step[63].flags=G_NOTE;proj.pat[17].bass[1].step[62].flags=G_NOTE;
        poly_break_set(proj.pat,(uint8_t *)&proj.sound,17,63,1,8);
        for(int lane=0;lane<25;lane++)poly_set(proj.pat,(uint8_t *)&proj.sound,17,lane,1+lane,7-lane%8);
        settled();context(0,17,part,0,0);arm(2,0);project_t before=proj;
        ui.part=(part+2)%5;for(int p=0;p<5;p++)seq.ppat[p]=31;
        assert(!action(2,127,0));render();assert(ui.part==(part+2)%5);assert(!memcmp(&proj.pat[31],&before.pat[31],sizeof(pattern_t)));
        for(int lane=0;lane<25;lane++){assert(poly_length(proj.pat,(uint8_t *)&proj.sound,17,lane)==1+lane);assert(poly_rate(proj.pat,(uint8_t *)&proj.sound,17,lane)==7-lane%8);}
        assert(ui_cc_undo(0)==VR_OK);render();assert(!memcmp(&proj,&before,sizeof proj));
    }
    fresh();assert(engine_lane_mode(1));render();
    for(int lane=0;lane<25;lane++)poly_set(proj.pat,(uint8_t *)&proj.sound,3,lane,64-lane,lane%8);
    for(int st=0;st<64;st++)poly_break_set(proj.pat,(uint8_t *)&proj.sound,3,st,st%2,st%9);
    proj.pat[3].drum[1].hit[10][1]=0x81234567;proj.pat[3].bass[1].step[63].flags=G_NOTE;
    int k=motion_alloc(proj.arr.lane,3,1,T_808,0,0);assert(k>=0);proj.arr.lane[k].val[63]=91;
    settled();context(1,3,0,25,0);arm(4,1);project_t before=proj;
    for(int p=0;p<5;p++)seq.ppat[p]=31;
    assert(!action(4,1,1));render();
    for(int lane=0;lane<25;lane++){assert(poly_length(proj.pat,(uint8_t *)&proj.sound,25,lane)==64-lane);assert(poly_rate(proj.pat,(uint8_t *)&proj.sound,25,lane)==lane%8);}
    for(int st=0;st<64;st++){assert(poly_break_gate(proj.pat,(uint8_t *)&proj.sound,25,st)==st%2);assert(poly_break_slice(proj.pat,(uint8_t *)&proj.sound,25,st)==st%9);}
    assert(motion_find(proj.arr.lane,25,1,T_808,0,0)>=0);assert(!memcmp(&proj.pat[31],&before.pat[31],sizeof(pattern_t)));
    remember();assert(!action(4,99,1));unchanged(1);
    assert(ui_cc_undo(0)==VR_OK);render();assert(!memcmp(&proj,&before,sizeof proj));
    context(1,3,0,25,0);assert(!action(4,0,1));arm(3,1);assert(!action(3,1,1));render();
    assert(!proj.pat[3].drum[1].hit[10][1]&&!bstep_gate(&proj.pat[3].bass[1].step[63])&&!poly_break_gate(proj.pat,(uint8_t *)&proj.sound,3,63));
    puts("Actions clear/copy: explicit pattern/part/destination, all25 timings/BREAK64, motion and one-step Undo PASS");
}
static void songs(void){
    for(int act=7;act<=9;act++){
        fresh();proj.arr.song.len=4;for(int i=0;i<4;i++){proj.arr.song.bar[i].pat[2]=i+4;proj.arr.song.bar[i].mute=(uint8_t)(32*i|i);proj.variant.arrangement.degree[i]=i+1;}
        settled();context(2,0,0,0,1);arm(act,2);project_t before=proj;ui.song_sel=3;
        assert(!action(act,127,2));render();
        if(act==7){assert(proj.arr.song.len==5);assert(!memcmp(&proj.arr.song.bar[2],&before.arr.song.bar[1],sizeof(song_bar_t)));assert(proj.variant.arrangement.degree[2]==2);}
        if(act==8){assert(proj.arr.song.len==3);assert(!memcmp(&proj.arr.song.bar[1],&before.arr.song.bar[2],sizeof(song_bar_t)));assert(proj.variant.arrangement.degree[1]==3);}
        if(act==9)assert(!proj.arr.song.len&&!proj.variant.arrangement.degree[3]);
        assert(ui_cc_undo(0)==VR_OK);render();assert(!memcmp(&proj,&before,sizeof proj));
    }
    puts("Actions song insert/delete/clear: explicit block, chord/repeat/mute rows shift together and Undo PASS");
}
static void refusals(void){
    const int acts[]={1,2,3,4,5,6,7,8,9,10,11};
    for(unsigned a=0;a<sizeof acts/sizeof acts[0];a++)for(int guard=0;guard<4;guard++){
        fresh();context(0,0,0,1,0);int act=acts[a];if(act!=1&&act!=6&&act!=10&&act!=11)arm(act,0);
        if(guard==0)ui_external_edit(1);
        if(guard==1)ui.rec=1;
        if(guard==2)ui.btn=1;
        if(guard==3)ui.keys=1;
        remember();assert(action(act,127,0)==CC_REJECTED);unchanged(1);assert(cc_source_field(0,31)==CA_OWNER);
    }
    for(int guard=0;guard<10;guard++){
        fresh();context(0,0,0,1,0);arm(2,0);
        if(guard==0)seq.playing=1;
        if(guard==1)seq.req_start=1;
        if(guard==2)seq.req_stop=1;
        if(guard==3)seq.req_cont=1;
        if(guard==4)seq.transport_hold=1;
        if(guard==5)seq.transport_stage=1;
        if(guard==6)rejected_save=1;
        if(guard==7){perf.test=0;perf.phase=0;}if(guard==8){assert(cq_put(C_STRIP,255,0,0));}if(guard==9)ui.overlay=O_ASK;
        remember();assert(action(2,127,0)==CC_REJECTED);unchanged(1);
    }
    fresh();context(0,0,0,1,0);arm(2,0);remember();assert(action(2,1,1)==CC_REJECTED);unchanged(1);
    assert(!cc_parameter(1010,1,0));assert(!cc_parameter(1071,1,0));remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    arm(2,0);ui_external_changed();remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    arm(2,0);assert(!cc_parameter(1134,1,0));remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    arm(2,0);cc_reset_source(0);context(0,0,0,1,0);remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    fresh();context(0,0,0,-1,0);remember();assert(cc_parameter(1133,4,0)==CC_REJECTED);unchanged(1);
    fresh();context(0,0,0,1,0);for(int k=0;k<NLANE;k++){int n=motion_alloc(proj.arr.lane,k%2?0:2,0,T_909,0,0);assert(n>=0);}settled();arm(4,0);remember();assert(action(4,1,0)==CC_REJECTED);unchanged(1);assert(cc_source_field(0,31)==CA_MOTION);
    fresh();context(0,0,0,1,0);arm(2,0);for(unsigned i=0;i<sizeof undo_shadow;i++)undo_shadow[i]^=1;remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);assert(cc_source_field(0,31)==CA_JOURNAL);
    for(int timed=0;timed<2;timed++){
        fresh();context(0,0,0,1,0);arm(2,0);if(timed)fail_dma_sync=1;else test_dma_timeouts=1;
        remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);assert(cc_source_field(0,31)==CA_SCRATCH);
    }
    puts("Actions refusal: all11 owner guards, transport/protection/CQ/perf, source/context/revision/cancel/reset, missing destination, motion/journal capacity no mutation PASS");
}
static void save_reset_views(void){
    for(int fail=0;fail<3;fail++){
        fresh();context(0,0,0,0,0);seq.bpm=147.0f;seq.accent_q7=20;memset(stored_known,0,sizeof stored_known);store_fail_after=fail;
        remember();assert(action(1,127,0)==CC_REJECTED);unchanged(0);assert(cc_source_field(0,31)==CA_IO);assert(!project_checkpoint_matches());
    }
    fresh();context(0,0,0,0,0);arm(5,0);remember();assert(action(5,1,0)==CC_REJECTED);unchanged(1);assert(cc_source_field(0,31)==CA_CHECKPOINT);
    proj.pat[0].drum[0].hit[0][0]=0x1111;ui_external_changed();assert(!action(1,1,0));render();assert(project_checkpoint_matches()&&!ui.dirty);
    project_t recovery=proj;uint32_t writes=store_writes;arm(5,0);assert(!action(5,1,0));render();assert(reset_checkpoint_hold&&store_writes==writes&&!proj.pat[0].drum[0].hit[0][0]);
    now_ms+=30000;autosave();assert(store_writes==writes);assert(!project_load());render();assert(!memcmp(&proj,&recovery,sizeof proj));assert(!reset_checkpoint_hold);
    context(0,0,0,0,0);assert(!action(5,0,0));arm(5,0);ui.dirty=1;remember();assert(action(5,1,0)==CC_REJECTED);unchanged(1);
    ui.dirty=0;proj.pat[1].swing=1;arm(5,0);remember();assert(action(5,1,0)==CC_REJECTED);unchanged(1);
    fresh();context(0,0,0,0,0);remember();assert(!action(6,1,0));assert(!memcmp(&oldproj,&proj,sizeof proj)&&!memcmp(&oldseq,&seq,sizeof seq));
    assert(!action(10,1,0)&&ui.view==V_PERF);assert(!action(11,1,0)&&perf_testing());assert(!memcmp(&oldproj,&proj,sizeof proj));
    fresh();assert(engine_lane_mode(1));render();context(0,0,0,0,0);remember();assert(action(11,1,0)==CC_REJECTED);unchanged(1);assert(cc_source_field(0,31)==CA_BENCHMARK);
    puts("Actions Save/reset/views: failedSave restores settings+journal+transport; verified recovery/no reset flash or autosave; dirty/stale checkpoint refusal; About/Perf/benchmark PASS");
}
static void protocol_jobs(void){
    for(int source=0;source<3;source++)for(int raw=1;raw<128;raw+=63){
        fresh();proj.pat[7].drum[0].hit[0][0]=1;settled();context(source,7,0,8,0);arm(2,source);
        uint32_t packet=11u|(0xbbu<<8)|(18u<<16)|((uint32_t)raw<<24);
        assert(!cc_message(packet,source));render();assert(!proj.pat[7].drum[0].hit[0][0]);remember();assert(!cc_message(packet,source));unchanged(1);
        assert(!action(2,0,source));arm(2,source);assert(!cc_parameter(1072,986&127,source));assert(!cc_parameter(1073,986>>7,source));
        assert(!cc_parameter(1074,127,source));assert(!cc_parameter(1075,0,source));assert(!cc_parameter(1130,0,source));assert(!cc_parameter(1077,1,source));render();
    }
    fresh();context(0,0,0,1,0);arm(2,0);remember();
    assert(action(2,128,0)==CC_REJECTED);assert(action(2,-1,0)==CC_REJECTED);assert(cc_parameter(1132,32,0)==CC_REJECTED);assert(cc_parameter(1133,12,0)==CC_REJECTED);unchanged(1);
    project_defaults();render();remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    fresh();context(0,0,0,1,0);arm(2,0);seq_start(&seq);render();assert(seq.playing);
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,11u|(0xbbu<<8)|(18u<<16)|(127u<<24),0));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(0xfcu<<8),0));
    for(int i=0;i<4;i++){plat_midi_service();render();}assert(!seq.playing&&mi_r==mi_w);
    fresh();context(0,0,0,1,0);arm(2,0);remember();irq_start=1;assert(action(2,1,0)==CC_REJECTED);oldseq.req_start=1;unchanged(1);
    /* Benchmark is one accepted bounded job. Source detach does not roll it
     * back; it completes/restores through the existing main-loop job. Stop
     * is still serviced and triggers its normal restoration path. */
    fresh();context(0,0,0,0,0);project_t before=proj;assert(!action(11,127,0));cc_reset_source(0);
    run_ms(16000);render();assert(!perf_testing()&&!seq.playing);assert(!memcmp(&proj,&before,sizeof proj));assert(seq.pat==proj.pat);
    fresh();context(0,0,0,0,0);before=proj;assert(!action(11,127,0));run_ms(1200);seq_stop(&seq);run_ms(500);render();assert(!perf_testing()&&!seq.playing);assert(!memcmp(&proj,&before,sizeof proj));
    puts("Actions wire/jobs: all3 sources rawpositive/exact edges, scalar holes, defaults invalidation, terminal refusal before Stop, reservation IRQ race, benchmark completion/Stop restore PASS");
}
static void journal_measure(void){
    static undo_t sample_undo;static uint8_t a[sizeof undo_shadow],b[sizeof undo_shadow];uint32_t rng=1977;
    for(unsigned trial=0;trial<128;trial++){
        uint32_t len=trial%4==0?sizeof a:trial%4==1?8192:trial%4==2?31:0;
        memset(&sample_undo,0,sizeof sample_undo);sample_undo.region=a;sample_undo.shadow=b;sample_undo.len=len;
        for(uint32_t i=0;i<len;i++){rng=rng*1664525u+1013904223u;a[i]=(uint8_t)rng;b[i]=(uint8_t)(a[i]^((rng>>24)%(trial%7+1)==0));}
        uint32_t measure=ca_undo_bytes(a,b,len);int result=undo_commit(&sample_undo);
        if(measure>UNDO_POOL)assert(result==-1);else if(result)assert(result==1&&sample_undo.used==measure);else assert(measure==8);
    }
    puts("Actions journal preflight: 128 dense/sparse/empty prospective entries exactly match real undo encoding and oversize refusal PASS");
}

static void external_transport(unsigned status)
{
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(status<<8),0));
    plat_midi_service();render();
}
static void confirmation_transport(void)
{
    /* Both transport-only cycles and audio-owned canonical edits invalidate a
     * confirmation, including when no UI frame observes playback. */
    for(int cont=0;cont<2;cont++){
        fresh();context(0,0,0,1,0);arm(2,0);uint32_t epoch=seq.transport_epoch;
        external_transport(cont?0xfb:0xfa);assert(seq.playing);external_transport(0xfc);
        assert(!seq.playing&&seq.transport_epoch>epoch);remember();
        assert(action(2,1,0)==CC_REJECTED);unchanged(1);assert(cc_source_field(0,31)==CA_CONFIRM);
    }
    fresh();context(0,0,0,1,0);arm(9,0);uint32_t revision=variant_current_revision();
    seq.song_on=seq.song_rec=1;external_transport(0xfa);assert(proj.arr.song.len==1);
    external_transport(0xfc);assert(variant_current_revision()==revision);remember();
    assert(action(9,1,0)==CC_REJECTED);unchanged(1);assert(cc_source_field(0,31)==CA_CONFIRM);
    fresh();proj.pat[0].bass[0].gen.mutate_bars=1;proj.pat[0].bass[0].gen.density=100;
    settled();context(0,0,2,1,0);arm(2,0);project_t before=proj;revision=variant_current_revision();
    external_transport(0xfa);for(int i=0;i<900;i++)render();external_transport(0xfc);
    assert(memcmp(&before.pat[0].bass[0],&proj.pat[0].bass[0],sizeof(bpart_t)));
    assert(variant_current_revision()==revision);remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    /* Canonical bytes returning by Undo still change the revision. A silent
     * direct edit without either publication is detected by the fingerprint. */
    fresh();context(0,0,0,1,0);arm(2,0);before=proj;proj.pat[0].swing=7;ui_external_changed();
    assert(ui_cc_undo(0)==VR_OK);render();assert(!memcmp(&before,&proj,sizeof proj));
    remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    fresh();context(0,0,0,1,0);arm(2,0);proj.pat[31].swing=3;
    remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    for(int pending=0;pending<3;pending++){
        fresh();context(0,0,0,1,0);if(pending==0)seq.req_start=1;else if(pending==1)seq.req_cont=1;else seq.req_stop=1;
        remember();assert(cc_parameter(1133,2,0)==CC_REJECTED);unchanged(1);
    }
    fresh();context(0,0,0,1,0);remember();irq_start=1;assert(cc_parameter(1133,2,0)==CC_REJECTED);oldseq.req_start=1;unchanged(1);
    /* Saturation is terminal for new confirmations/jobs until normal engine
     * initialization. No wrapping counter can resurrect an old witness. */
    fresh();context(0,0,0,1,0);seq.transport_epoch=UINT32_MAX-1;arm(2,0);
    external_transport(0xfc);assert(seq.transport_epoch==UINT32_MAX);external_transport(0xfc);assert(seq.transport_epoch==UINT32_MAX);
    remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);assert(cc_parameter(1133,2,0)==CC_REJECTED);unchanged(1);
    seq.stop_epoch=UINT32_MAX;remember();assert(action(11,1,0)==CC_REJECTED);unchanged(1);
    fresh();context(0,0,0,1,0);assert(!action(1,1,0));render();arm(2,0);assert(!project_load());render();remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    fresh();context(0,0,0,1,0);arm(2,0);boot();render();remember();assert(action(2,1,0)==CC_REJECTED);unchanged(1);
    puts("Actions arm witness: external Start/Continue+Stop, audio song record/auto-mutate, Undo return, silent edit, pending/IRQ transport, saturation and load/reinit PASS");
}
static void early_benchmark_stop(void)
{
    /* Ordered real ingress CC995 -> FC, before any render or UI frame. */
    fresh();context(0,0,0,0,0);project_t before=proj;
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,11u|(0xbbu<<8)|(27u<<16)|(127u<<24),0));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(0xfcu<<8),0));
    plat_midi_service();assert(perf_testing()&&perf.phase==0);render();assert(!seq.playing);
    perf_test_tick();assert(perf.phase==3&&!seq.req_start);render();perf_test_tick();
    assert(!perf_testing()&&!seq.playing&&seq.pat==proj.pat&&!memcmp(&before,&proj,sizeof proj));
    /* Main Stop request before audio consumes it must also cancel phase0. */
    fresh();context(0,0,0,0,0);before=proj;assert(!action(11,1,0));seq_stop(&seq);
    perf_test_tick();assert(perf.phase==3&&!seq.req_start);render();perf_test_tick();assert(!perf_testing()&&!memcmp(&before,&proj,sizeof proj));
    render();assert(!action(11,0,0));assert(!action(11,1,0));run_ms(1200);external_transport(0xfc);perf_test_tick();render();perf_test_tick();assert(!perf_testing()&&!memcmp(&before,&proj,sizeof proj));
    /* Reset/load retire a CC-owned pending job so no old phase starts later. */
    for(int load=0;load<2;load++){
        fresh();context(0,0,0,0,0);assert(!action(1,1,0));render();assert(!action(11,1,0));
        if(load)assert(!project_load());else project_defaults();
        render();before=proj;perf_test_tick();assert(!perf_testing()&&!seq.req_start&&!memcmp(&before,&proj,sizeof proj));
    }
    /* A legacy UI-created benchmark has no CC stop witness and retains its
     * prior lifecycle rather than being silently changed by this feature. */
    fresh();perf_test_start();assert(perf.cc_stop_epoch==UINT32_MAX);run_ms(16000);assert(!perf_testing());
    puts("Actions benchmark: ordered CC995/FC prestart, pending main Stop, midphase/retry, reset/load retirement and legacy lifecycle PASS");
}

int main(void){fresh();journal_measure();clear_copy();songs();refusals();save_reset_views();protocol_jobs();confirmation_transport();early_benchmark_stop();printf("Native actions candidate PASS; source-state bytes=%zu, physical/storage-crash atomicity not proven\n",sizeof cc_sources);return 0;}
