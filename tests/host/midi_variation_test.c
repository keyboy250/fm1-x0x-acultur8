/* SPDX-License-Identifier: GPL-3.0-only */
#define MIDI_DRUM_NO_MAIN
#include "midi_drum_test.c"
static void cv_select(int source,int lane){if(lane<22)drum_select(source,lane/11,lane%11);else selected(source,lane-22);}
static void cv_recipe(int src,int every,int amount,int lock,int seed,int mode,int shape,int range,int constraint,int block,int token){
    const int values[]={every,amount,lock,seed,mode,shape,range};
    for(int i=0;i<7;i++)exact_value(src,1211+i,values[i]);
    exact_value(src,1235,constraint);exact_value(src,1236,block);
    CHECK(!midi_parameter(src,1231,token&127));CHECK(!midi_parameter(src,1232,token>>7));
}
static int cv_arm(int src,int off){return midi_parameter(src,1233,off?91:73);}
static int cv_execute(int src,int off){CHECK(!midi_parameter(src,off?1219:1218,0));return midi_parameter(src,off?1219:1218,127);}
static void cv_setup(int format,int lane){if(lane<22)mode_setup(format);else option_setup(format,lane-22);for(int i=0;i<3;i++)cc_reset_source(i);}
static int16_t trace[8][16][8];static uint8_t trace_n[8][16];static int trace_lane;
static const uint8_t cv_gm[2][11]={{36,38,41,45,50,37,39,42,46,49,51},{36,38,41,45,50,37,39,56,49,46,42}};
/* MIDI attacks have their own legato/retrigger semantics. Compare MIDI to MIDI,
 * rather than treating every internal 303 trigger as an external Note On. */
static void cv_midi(void *ctx,uint8_t status,uint8_t note,uint8_t velocity){
    (void)ctx;int part=trace_lane<22?trace_lane/11:trace_lane-20;
    if(status!=(0x90|seq.ch[part]) || !velocity ||
       (trace_lane<22 && note!=cv_gm[trace_lane/11][trace_lane%11]))return;
    unsigned cycle=seq_lane_cycles(&seq,trace_lane);int step=seq_lane_position(&seq,trace_lane);
    if(cycle<8 && step>=0 && step<16){unsigned n=trace_n[cycle][step]++;CHECK(n<8);trace[cycle][step][n]=(int16_t)(trace_lane<22?velocity:note);}
}
static void cv_fixture(int lane){
    cv_setup(2,lane);for(int p=0;p<5;p++)seq.ppat[p]=1;
    for(int b=0;b<2;b++)proj.pat[1].bass[b].gen.mutate_bars=0;
    for(int l=0;l<25;l++)poly_set(proj.pat,(uint8_t *)&proj.sound,1,l,16,RATE_16);
    for(int kit=0;kit<2;kit++){proj.pat[1].drum[kit].len=16;for(int voice=0;voice<11;voice++)proj.pat[1].drum[kit].hit[voice][0]=0x8106u;}
    for(int b=0;b<2;b++){proj.pat[1].bass[b].len=16;proj.pat[1].bass[b].rate=RATE_16;proj.pat[1].bass[b].dir=DIR_FWD;}
    proj.variant.arrangement.flags&=(uint8_t)~ARR_ENABLED;proj.variant.arrangement.flags|=ARR_NOTES_OUT;proj.set.notes_out=1;proj.set.bpm_x10=1200;proj.set.clk_out=0;seq.send_clock=0;seq.send_notes=1;seq.bpm=120;settle();
    CHECK(project_save()==0);char name[128];snprintf(name,sizeof name,"build/midi-variation/before-%d.json",lane);fixture_store(name);
    /* Construct canonical recipe independently of MIDI staging. */
    pattern_variation_t v={(uint8_t)lane,1,0,3,42,3,0,0,0,0,0};
    if(lane>=22){const bpart_t *bp=&proj.pat[1].bass[lane-22];v.notes=oracle_scale(&bp->gen);v.low=(uint8_t)(12*(bp->gen.base_oct+1));v.high=(uint8_t)(v.low+12*bp->gen.oct_range-1);}
    CHECK(ui_variation_config(&v)==VR_OK);project_t base=proj;undo_t journal=undo;memset(trace,0,sizeof trace);memset(trace_n,0,sizeof trace_n);trace_lane=lane;
    seq_sink_t sink=SINK;sink.midi=cv_midi;sink.midi_try=0;sink.midi_pair_try=0;sink.midi_batch_try=0;
    seq_midi_prepare(&seq,1,&sink);
    seq_start(&seq);unsigned samples=88200u*8u-1u;
    while(samples){unsigned n=samples>256?256:samples;seq_advance_arrangement(&seq,0,&sink);while(n){unsigned k=seq_until_event_arrangement(&seq,n);seq_advance_arrangement(&seq,k,&sink);samples-=k;n-=k;}}
    seq_stop(&seq);CHECK(!memcmp(&base,&proj,sizeof proj)&&!memcmp(&journal,&undo,sizeof undo));
    snprintf(name,sizeof name,"build/midi-variation/trace-%d.json",lane);FILE *f=fopen(name,"wb");CHECK(f);
    fprintf(f,"{\"lane\":%d,\"channel\":%d,\"note\":%d,\"trace\":[",lane,seq.ch[lane<22?lane/11:lane-20],lane<22?cv_gm[lane/11][lane%11]:0);
    for(int cy=0;cy<8;cy++){fprintf(f,cy?",[":"[");for(int st=0;st<16;st++){fprintf(f,st?",[":"[");for(int n=0;n<trace_n[cy][st];n++)fprintf(f,n?",%d":"%d",trace[cy][st][n]);fprintf(f,"]");}fprintf(f,"]");}fprintf(f,"]}\n");fclose(f);
}
#ifndef MIDI_VARIATION_NO_MAIN
int main(void){
    for(int format=0;format<3;format++)for(int lane=0;lane<24;lane++)for(int src=0;src<3;src++)for(int mode=0;mode<(lane<22?1:4);mode++){
        cv_setup(format,lane);cv_select(src,lane);project_t base=proj;undo_t history=undo;uint8_t panel=ui.part;
        cv_recipe(src,2,8,3,255,2,1,3,mode,0,1);CHECK(!memcmp(&base,&proj,sizeof proj));
        CHECK(!cv_arm(src,0));CHECK(!cv_execute(src,0));CHECK(cc_sources[src].generation.state==CG_APPLIED);CHECK(!seq.transport_hold);
        pattern_variation_t expected={(uint8_t)lane,1,2,8,255,3,2,7,0,0,0};
        if(lane>=22){expected.notes=wanted_mask(lane-22,mode);expected.low=48;expected.high=71;}
        CHECK(!memcmp(&seq.variation,&expected,sizeof expected));CHECK(!memcmp(&base,&proj,sizeof proj)&&!memcmp(&history,&undo,sizeof undo)&&panel==ui.part);
        CHECK(!cv_execute(src,0));CHECK(cc_sources[src].generation.state==CG_APPLIED);
        query_unchanged(src,8);query_unchanged(src,9);
        CHECK(!midi_parameter(src,1231,2));CHECK(!cv_arm(src,1));CHECK(!cv_execute(src,1));CHECK(seq.variation.lane==255);
        CHECK(!memcmp(&base,&proj,sizeof proj)&&!memcmp(&history,&undo,sizeof undo));
        CHECK(!midi_parameter(src,1231,3));CHECK(!cv_arm(src,1));CHECK(!cv_execute(src,1));CHECK(cc_sources[src].generation.state==CG_NO_CHANGE);
    }
    for(int issue=0;issue<10;issue++){
        cv_setup(2,22);cv_select(0,22);cv_recipe(0,0,3,3,0,0,0,0,0,0,1);project_t base=proj;pattern_variation_t live=seq.variation;
        if(issue==0){CHECK(!cv_arm(0,0));CHECK(!midi_parameter(0,1234,127));}
        if(issue==1){CHECK(!cv_arm(0,0));seq.variation.seed^=1;live=seq.variation;}
        if(issue==2){CHECK(!cv_arm(0,0));seq.transport_epoch++;}
        if(issue==3){CHECK(!cv_arm(0,0));proj.pat[0].swing^=1;base=proj;}
        if(issue==4){CHECK(!cv_arm(0,0));cc_reset_source(0);}
        if(issue==5){CHECK(!cv_arm(0,0));exact_value(0,1212,1);}
        if(issue==6){ui.rec=1;CHECK(cv_arm(0,0)<0);ui.rec=0;}
        if(issue==7){seq.req_start=1;CHECK(cv_arm(0,0)<0);seq.req_start=0;}
        if(issue==8){rejected_save=1;CHECK(cv_arm(0,0)<0);rejected_save=0;}
        if(issue==9){CHECK(!cv_arm(0,0));CHECK(!cc_set_mode(1));}
        int rc=midi_parameter(0,1218,127);CHECK(rc<0);CHECK(!memcmp(&base,&proj,sizeof proj)&&!memcmp(&live,&seq.variation,sizeof live));CHECK(!cc_set_mode(0));
    }
    /* A later source changing the live runtime recipe makes the first arm stale. */
    cv_setup(2,0);cv_select(0,0);cv_recipe(0,0,3,3,42,0,0,0,0,0,1);CHECK(!cv_arm(0,0));
    cv_select(1,1);cv_recipe(1,1,2,0,7,1,1,1,0,0,1);CHECK(!cv_arm(1,0));CHECK(!cv_execute(1,0));
    pattern_variation_t live=seq.variation;CHECK(cv_execute(0,0)<0&&cc_sources[0].generation.reason==CG_STALE);CHECK(!memcmp(&live,&seq.variation,sizeof live));
    CHECK(!midi_parameter(0,1231,2));CHECK(cv_arm(0,1)<0&&cc_sources[0].generation.reason==CG_CONTEXT);
    /* Switching back to a generation family preserves original control semantics. */
    selected(0,0);recipe(0,4,0,0,3);CHECK(!(cc_sources[0].generation.flags&CG_VAR)&&!cc_sources[0].generation.drum);
    drum_select(2,1,0);drum_recipe(2,50,0,0,0,0,1);CHECK(!(cc_sources[2].generation.flags&CG_VAR)&&cc_sources[2].generation.drum);
    for(int lane=0;lane<24;lane++){cv_setup(2,lane);CHECK(project_save()==0);char name[128];snprintf(name,sizeof name,"build/midi-variation/state-before-%d.json",lane);fixture_store(name);}
    const int targets[]={0,11,21,22,23};for(unsigned i=0;i<5;i++)cv_fixture(targets[i]);
    printf("VARIATION MIDI PASS: %d checkpoints;24 lanes/3 formats/3 sources/303 constraints;runtime-only Arm/OFF,no-op,dedup,Cancel/stale/source/owner/mode guards;5 actual8-cycle trace oracles.\n",checks);return 0;
}
#endif
