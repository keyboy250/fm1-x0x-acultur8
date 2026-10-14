/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define PLAT_CONTROL_TEST
#define arrangement_valid cc_test_arrangement_valid
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#undef arrangement_valid
#include <assert.h>
#include "../../firmware/src/app/engine.c"
static int locked,irq_start,irq_song,irq_fill,irq_collision,validation_calls;
int arrangement_valid(const arrangement_settings_t *a);
int cc_test_arrangement_valid(const arrangement_settings_t *a){assert(!locked);validation_calls++;return arrangement_valid(a);}
uint32_t plat_control_enter(void){assert(!locked);
    if(irq_song){irq_song=0;seq_start(&seq);seq_advance(&seq,0,&SINK);}
    /* Defensive future-capacity/channel changes, injected before lock entry.
     * These are controlled test interleavings, not an extra production producer. */
    if(irq_fill){irq_fill=0;while(engine_queue_space())assert(cq_put(C_STRIP,255,0,0));}
    if(irq_collision){seq.ch[0]=(uint8_t)(irq_collision-1);irq_collision=0;}
    if(irq_start){irq_start=0;seq_start(&seq);seq_advance_poly(&seq,0,&SINK);}locked=1;return 29;}
void plat_control_leave(uint32_t saved){assert(locked && saved==29);locked=0;}
static void render(void){int32_t block[512];engine_render(block,256);}
static void fresh(void){for(int i=0;i<OBJ_NOBJ;i++)store_len[i]=-1;memset(stored_known,0,sizeof stored_known);project_discard_rejected();mi_w=mi_r=mi_ready=0;mi_claimed=0xffffffffu;boot();render();for(int i=0;i<CC_SOURCES;i++)cc_reset_source(i);}
static void context(int source,int p,int lane,int step){assert(!cc_parameter(1010,p,source));assert(!cc_parameter(1069,lane,source));assert(!cc_parameter(1014,step,source));assert(!cc_parameter(1071,1,source));}
static int length(int p,int lane){return poly_length(proj.pat,(uint8_t *)&proj.sound,p,lane);}
static int rate(int p,int lane){return poly_rate(proj.pat,(uint8_t *)&proj.sound,p,lane);}
static void capacity(void){if(engine_queue_space()<4)render();}
static uint32_t control(int n,int v){int c=n%88;return 11u|((uint32_t)(0xb0+n/88)<<8)|((uint32_t)(c<32?c:c+32)<<16)|((uint32_t)v<<24);}
static void protection_order(void){
    project_t before;uint32_t q;
    fresh();context(0,0,0,0);before=proj;seq_start(&seq);
    assert(cc_parameter(1131,1,0)==CC_REJECTED && !memcmp(&proj,&before,sizeof proj));render();
    /* Invalid initialization ahead of real Stop must not pin the ingress. */
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,control(1131,127),0));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(0xfcu<<8),0));
    plat_midi_service();render();assert(!seq.playing && mi_r==mi_w && !(proj.variant.arrangement.flags&POLY_FLAG));
    before=proj;irq_start=1;assert(cc_parameter(1131,1,0)==CC_REJECTED && !memcmp(&proj,&before,sizeof proj));seq_stop(&seq);render();
    assert(!cc_parameter(1131,1,0));render();assert(poly_valid(proj.pat,(uint8_t *)&proj.sound));
    before=proj;q=engine_queue_written();while(engine_queue_space())assert(cq_put(C_STRIP,255,0,0));
    q=engine_queue_written();assert(cc_parameter(1128,3,0)==CC_BUSY && q==engine_queue_written() && !memcmp(&before,&proj,sizeof proj));
    /* A busy timing packet stays ordered ahead of Stop, then retries after CQ drain. */
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,control(1128,4),0));assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(0xfcu<<8),0));
    plat_midi_service();assert(mi_ready!=mi_w);render();plat_midi_service();render();assert(mi_r==mi_w && length(0,0)==3);
    before=proj;rejected_save=1;assert(cc_parameter(1129,7,0)==CC_REJECTED && !memcmp(&before,&proj,sizeof proj));rejected_save=0;
    assert(cc_parameter(1128,0,0)==CC_REJECTED);assert(cc_parameter(1128,65,0)==CC_REJECTED);assert(cc_parameter(1129,8,0)==CC_REJECTED);assert(cc_parameter(1131,2,0)==CC_REJECTED);
    /* No free motion slot is a terminal semantic refusal, not audio backpressure. */
    for(int i=0;i<NLANE;i++){proj.arr.lane[i].used=1;proj.arr.lane[i].pat=1;proj.arr.lane[i].part=0;proj.arr.lane[i].t=T_PROB;}
    before=proj;seq_start(&seq);render();
    assert(cc_parameter(1040,100,0)==CC_REJECTED && !memcmp(&before,&proj,sizeof proj));
    assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,control(1040,127),0));assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(0xfcu<<8),0));plat_midi_service();render();assert(!seq.playing && mi_r==mi_w);
    puts("lane CC: semantic init/pending/IRQ race rejection, CQ retry/order/Stop and protected bounds PASS");
}
static void rejected_arrangement(int id,int value,int expected){
    project_t before=proj;undo_t history=undo;uint8_t shadow[sizeof undo_shadow];variant_snapshot_t view;
    memcpy(shadow,undo_shadow,sizeof shadow);assert(variant_snapshot(0,0,0,&view)==VR_OK);uint32_t revision=view.revision,writes=store_writes;
    assert(cc_parameter((uint16_t)id,(uint32_t)value,0)==expected);
    assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&history,&undo,sizeof undo) && !memcmp(shadow,undo_shadow,sizeof shadow));
    assert(variant_snapshot(0,0,0,&view)==VR_OK && view.revision==revision && writes==store_writes);
}
static void prospective_publication(void){
    fresh();context(0,0,0,0);
    assert(!cc_parameter(1056,1,0));render();int before=validation_calls;
    rejected_arrangement(1054,HARMONY_CHROMATIC,CC_REJECTED);
    rejected_arrangement(1062,90,CC_REJECTED);rejected_arrangement(1066,90,CC_REJECTED);
    rejected_arrangement(1064,seq.ch[0],CC_REJECTED);assert(validation_calls>=before+4);
    /* The lock-entry channel recheck sees a collision absent in the prospective
     * main-owned config. No failed control is copied into canonical or journal. */
    irq_collision=9;rejected_arrangement(1064,8,CC_REJECTED);seq.ch[0]=0;
    irq_fill=1;rejected_arrangement(1053,4,CC_BUSY);render();
    assert(proj.variant.arrangement.key==0 && proj.variant.arrangement.degree[0]==1);
    /* Accepted controls publish exactly their byte, retaining every other
     * valid field including desired packed mode and NotesOut flags. */
    assert(!cc_parameter(1061,1,0));assert(!cc_parameter(1062,25,0));assert(!cc_parameter(1063,90,0));
    assert(!cc_parameter(1064,8,0));assert(!cc_parameter(1065,61,0));assert(!cc_parameter(1066,49,0));assert(!cc_parameter(1067,85,0));render();
    assert(proj.variant.arrangement.key==0 && proj.variant.arrangement.degree[0]==1 && arrangement_valid(&proj.variant.arrangement));
    /* Real audio Start/legacy song recording occurs after prospective validation
     * and before lock entry. Mask/pattern writes preserve those newest bytes. */
    for(int field=6;field<=8;field++){
        fresh();context(0,0,0,0);seq.song_on=seq.song_rec=1;seq.song_start=0;seq.mute=seq_part_mask(PART_303A);
        for(int p=0;p<NPARTS;p++)seq.pcue[p]=(uint8_t)(p+1);
        int calls=validation_calls;irq_song=1;
        assert(!cc_parameter((uint16_t)(1051+field),field==6?3:field==7?31:1,0));
        assert(seq.playing && proj.arr.song.len==1 && validation_calls>calls);
        assert(proj.arr.song.bar[0].mute==(uint8_t)(4|(field==6?64:field==8?1:0)));
        for(int p=0;p<NPARTS;p++)assert(proj.arr.song.bar[0].pat[p]==(uint8_t)(field==7&&p==0?31:p+1));
        seq_stop(&seq);render();
    }
    puts("CC prospective validation: outside IRQ, rejected project/journal/revision/storage preservation, fresh collision/CQ checks and real audio song-byte interleavings PASS");
}
static void exhaustive(void){
    static uint8_t expected[32][25][2];project_t before;
    fresh();assert(!cc_parameter(1131,1,0));render();seq_start(&seq);render();assert(seq.playing);
    /* Exhaustive setter domains, including every pattern/lane/rate/length. */
    for(int p=0;p<32;p++)for(int l=0;l<25;l++){
        context(0,p,l,0);
        for(int r=0;r<8;r++)for(int n=1;n<=64;n++){
            capacity();assert(!cc_parameter(1128,n,0));assert(!cc_parameter(1129,r,0));assert(length(p,l)==n && rate(p,l)==r);
        }
    }
    render();
    /* A nonuniform independent oracle catches distant lane/pattern aliases. */
    for(int p=0;p<32;p++)for(int l=0;l<25;l++){
        expected[p][l][0]=1+(p*17+l*7)%64;expected[p][l][1]=(p*3+l*5)%8;
        context(0,p,l,0);capacity();assert(!cc_parameter(1128,expected[p][l][0],0));assert(!cc_parameter(1129,expected[p][l][1],0));
    }
    uint32_t seed=0x21baf019u;
    for(int i=0;i<200;i++){
        seed=seed*1664525u+1013904223u;int p=(seed>>8)%32,l=(seed>>16)%25,field=seed&1;
        int value=field?(seed>>24)%8:1+(seed>>24)%64;expected[p][l][field]=value;
        context(i%3,p,l,0);capacity();assert(!cc_parameter(1128+field,value,i%3));
        for(int pp=0;pp<32;pp++)for(int ll=0;ll<25;ll++)assert(length(pp,ll)==expected[pp][ll][0] && rate(pp,ll)==expected[pp][ll][1]);
    }
    render();seq_stop(&seq);render();before=proj;assert(project_save()==0 && get32(store[OBJ_PAT0])==5);assert(project_load()==0);render();assert(!memcmp(&before.sound,&proj.sound,sizeof proj.sound) && !memcmp(before.pat,proj.pat,sizeof proj.pat));
    puts("lane CC: all32x25x8x64 exact domains, distinct matrix distant independence, save/reopen PASS");
}
static void breaks_and_undo(void){
    fresh();assert(!cc_parameter(1131,1,0));render();
    for(int p=0;p<32;p++)for(int step=0;step<64;step++){
        context(0,p,24,step);capacity();assert(!cc_parameter(1029,(p+step)&1,0));assert(!cc_parameter(1030,(p*3+step)%9,0));
    }
    for(int p=0;p<32;p++)for(int step=0;step<64;step++)assert(poly_break_gate(proj.pat,(uint8_t *)&proj.sound,p,step)==((p+step)&1) && poly_break_slice(proj.pat,(uint8_t *)&proj.sound,p,step)==(p*3+step)%9);
    render();context(0,31,24,63);assert(!cc_parameter(1030,8,0));assert(poly_break_gate(proj.pat,(uint8_t *)&proj.sound,31,63)==0);render();
    assert(project_save()==0 && project_load()==0);render();assert(poly_break_slice(proj.pat,(uint8_t *)&proj.sound,31,63)==8);
    /* Interleaved physical-UI and CC calls share canonical bytes and one journal.
     * The UI target is deliberately different from the explicit source context. */
    context(0,0,1,0);context(1,0,2,0);ui.part=PART_909;seq.ppat[PART_909]=0;ui.sel[0]=1;
    int a=length(0,1),b=length(0,2);ui_external_edit(1);
    pref_set(PR(R_DLEN,0,0,0),6);assert(length(0,1)==7);
    assert(!cc_parameter(1128,11,0));assert(length(0,1)==11);
    assert(!cc_parameter(1128,13,1));assert(length(0,2)==13);
    pref_set(PR(R_DLEN,0,0,0),8);assert(length(0,1)==9 && length(0,2)==13);ui_external_edit(0);render();
    assert(ui_external_undo(0)==VR_OK);render();assert(length(0,1)==a && length(0,2)==b);
    assert(ui_external_undo(1)==VR_OK);render();assert(length(0,1)==9 && length(0,2)==13);
    puts("lane CC: all32 BREAK64 gate/slice preservation, reopen and simultaneous UI/CC last-write/shared undo PASS");
}
static void all_lane_periods(void){
    static const int ticks[8]={6,4,3,8,24,12,16,2};
    fresh();assert(!cc_parameter(1131,1,0));render();
    for(int l=0;l<25;l++){context(0,0,l,0);assert(!cc_parameter(1128,3+l%17,0));assert(!cc_parameter(1129,l%8,0));}
    render();seq.send_clock=0;seq.ext=1;seq.ext_period=235200;seq_start(&seq);seq_ext_clock(&seq);render();
    for(int t=1;t<=48;t++){
        seq_ext_clock(&seq);render();
        for(int l=0;l<25;l++)assert(seq_lane_position(&seq,l)==(t/ticks[l%8])%(3+l%17));
    }
    assert(seq.arrangement.tick==48);seq_stop(&seq);render();
    puts("lane CC: independent actual engine positions all25 mixed eight rates/odd lengths; common bar clock PASS");
}
static void live_phase(void){
    fresh();assert(!cc_parameter(1131,1,0));render();context(0,0,1,0);
    assert(!cc_parameter(1128,64,0));assert(!cc_parameter(1129,0,0));render();seq.send_clock=0;seq.ext=1;seq.ext_period=235200;
    seq_start(&seq);seq_ext_clock(&seq);render();assert(seq_lane_position(&seq,1)==0);
    for(int i=0;i<2;i++){seq_ext_clock(&seq);render();}
    assert(!cc_parameter(1128,7,0));assert(!cc_parameter(1129,4,0));render();assert(seq_lane_position(&seq,1)==0);
    for(int i=0;i<3;i++){seq_ext_clock(&seq);render();assert(seq_lane_position(&seq,1)==0);}
    seq_ext_clock(&seq);render();assert(seq_lane_position(&seq,1)==1);
    for(int i=0;i<23;i++){seq_ext_clock(&seq);render();assert(seq_lane_position(&seq,1)==1);}seq_ext_clock(&seq);render();assert(seq_lane_position(&seq,1)==2);
    /* Off/On keeps prepared rows, changing only at the common musical bar. */
    assert(!cc_parameter(1131,0,0));render();assert(seq.lane_enabled);seq.arrangement.clock_started=1;seq.arrangement.tick=95;seq_ext_clock(&seq);render();assert(!seq.lane_enabled && length(0,1)==7 && rate(0,1)==4);
    assert(!cc_parameter(1128,11,0));assert(!cc_parameter(1129,7,0));render();assert(length(0,1)==11 && rate(0,1)==7);
    assert(!cc_parameter(1131,1,0));seq.arrangement.tick=95;seq_ext_clock(&seq);render();assert(seq.lane_enabled);
    /* Legacy direct addresses keep bass/kit selection and cannot overwrite slab. */
    context(0,0,1,0);assert(!cc_parameter(1012,1,0));assert(!cc_parameter(1071,1,0));assert(!cc_parameter(901,12,0));assert(!cc_parameter(902,6,0));render();assert(length(0,1)==13 && rate(0,1)==6 && poly_valid(proj.pat,(uint8_t *)&proj.sound));
    seq_stop(&seq);render();puts("lane CC: live next-local-step latched phase, initialized Off edits, next96-tick mode and legacy address packed safety PASS");
}
int main(void){prospective_publication();protection_order();exhaustive();breaks_and_undo();all_lane_periods();live_phase();puts("individual lane + live CC integration native PASS");return 0;}
