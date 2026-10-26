/* SPDX-License-Identifier: GPL-3.0-only */
#define MIDI_VARIATION_NO_MAIN
#include "midi_variation_test.c"
static void mf_samples(unsigned n){
    seq_sink_t sink=SINK;sink.midi=0;sink.midi_try=0;sink.midi_pair_try=0;sink.midi_batch_try=0;
    seq_advance_arrangement(&seq,0,&sink);
    while(n){unsigned k=seq_until_event_arrangement(&seq,n);seq_advance_arrangement(&seq,k,&sink);n-=k;}
}
static void mf_setup(int format,int lane,int len){
    cv_setup(format,lane);memset(&proj.arr,0,sizeof proj.arr);
    proj.variant.arrangement.flags&=(uint8_t)~ARR_ENABLED;
    for(int p=0;p<5;p++)seq.ppat[p]=1;
    seq.chain_a=seq.chain_b=seq.song_on=0;seq.mute=seq.vmute=0;seq.bpm=120;seq.send_clock=0;seq.midi_ready=0;
    proj.set.bpm_x10=1200;proj.set.clk_out=0;proj.set.autosave_off=1;
    for(int b=0;b<2;b++){
        bpart_t *bp=&proj.pat[1].bass[b];bp->len=(uint8_t)len;bp->rate=RATE_16;bp->dir=DIR_FWD;bp->transpose=24;bp->gen.mutate_bars=0;
        for(int st=0;st<64;st++)bp->step[st]=(bstep_t){(uint8_t)(60+(st%3==1?4:st%3==2?7:0)),(uint8_t)(G_NOTE|((st&1)?BS_ACCENT:0))};
        bp->step[0].flags|=BS_SLIDE;if(len>3){bp->step[2].flags=G_TIE;bp->step[3].flags=G_REST;}
        bpart_t *dst=&proj.pat[6].bass[b];dst->gen.mutate_bars=4;dst->gen.density=83;
        for(int st=0;st<64;st++)dst->step[st]=(bstep_t){40,(uint8_t)(G_NOTE|((st&1)?BS_ACCENT:0))};
    }
    for(int k=0;k<2;k++){
        proj.pat[1].drum[k].len=proj.pat[6].drum[k].len=(uint8_t)len;
        proj.pat[1].drum[k].rate=proj.pat[6].drum[k].rate=RATE_16;
        for(int v=0;v<11;v++)for(int w=0;w<2;w++)proj.pat[1].drum[k].hit[v][w]=0x92929292u;
    }
    if(format)for(int l=0;l<25;l++)poly_set(proj.pat,(uint8_t *)&proj.sound,1,l,len,RATE_16);
    int part=lane<22?lane/11:lane-20,voice=lane<22?lane%11:0;
    int m=motion_alloc(proj.arr.lane,1,part,T_PROB,(part<<4)|voice,0);CHECK(m>=0);
    for(int st=0;st<64;st++)proj.arr.lane[m].val[st]=st==1?0:100;
    m=motion_alloc(proj.arr.lane,6,part,T_PROB,(part<<4)|voice,0);CHECK(m>=0);
    for(int st=0;st<64;st++)proj.arr.lane[m].val[st]=0;
    settle();
}
static void mf_variation(int lane){
    pattern_variation_t v={(uint8_t)lane,1,0,3,42,0,0,0,0,0,0};
    if(lane>=22){const bpart_t *bp=&proj.pat[1].bass[lane-22];v.notes=oracle_scale(&bp->gen);v.low=(uint8_t)(12*(bp->gen.base_oct+1));v.high=(uint8_t)(v.low+12*bp->gen.oct_range-1);}
    CHECK(ui_variation_config(&v)==VR_OK);
}
static void mf_token(int src,int request){CHECK(!midi_parameter(src,1231,request&127));CHECK(!midi_parameter(src,1232,request>>7));}
static int mf_arm(int src,int commit){return midi_parameter(src,1233,commit?127:109);}
static int mf_execute(int src,int commit){CHECK(!midi_parameter(src,commit?1222:1221,0));return midi_parameter(src,commit?1222:1221,127);}
static void mf_commit_stage(int src,int dst,uint32_t token,int request){
    exact_value(src,1220,(unsigned)dst);for(int byte=0;byte<4;byte++)exact_value(src,1237+byte,(token>>(8*byte))&255u);mf_token(src,request);
}
static void mf_query(int src,int page,int index,int available){
    project_t old=proj;seq_t oldseq=seq;undo_t history=undo;unsigned saved=store_writes;cc_source_t source=cc_sources[src];
    uint8_t a[]={125,70,76,51,1,77,(uint8_t)src,(uint8_t)page,(uint8_t)index};memcpy(incoming,a,sizeof a);incoming_n=sizeof a;wire_n=0;ed_service();
    CHECK(wire_n>=17 && wire[9]==index && wire[10]==(available?0:3));CHECK(get7(wire_n-6,5)==st_crc32(wire+1,wire_n-7));
    CHECK(!memcmp(&old,&proj,sizeof old)&&!memcmp(&oldseq,&seq,sizeof seq)&&!memcmp(&history,&undo,sizeof undo)&&!memcmp(&source,&cc_sources[src],sizeof source)&&saved==store_writes);
    char name[120];snprintf(name,sizeof name,"build/midi-freeze/query-%d-%d-%d%s.bin",src,page,index,available?"":"-unavailable");FILE *f=fopen(name,"wb");CHECK(f);CHECK(fwrite(wire,1,wire_n,f)==wire_n);fclose(f);
}
static void mf_expected(project_t *p,int lane,const pattern_freeze_t *f){
    int part=lane<22?lane/11:lane-20,voice=lane<22?lane%11:0;
    if(lane>=22){bpart_t *b=&p->pat[6].bass[lane-22];
        for(int st=0;st<f->bass.len;st++){b->step[st].note=f->bass.step[st].note;b->step[st].flags=(f->bass.step[st].flags&~BS_ACCENT)|(b->step[st].flags&BS_ACCENT);}
        b->len=f->bass.len;b->rate=f->bass.rate;b->dir=DIR_FWD;b->transpose=24;b->gen.mutate_bars=0;
    }else for(int st=0;st<f->bass.len;st++){uint32_t bit=1u<<(st&31);uint32_t *w=&p->pat[6].drum[part].hit[voice][st>>5];if(bstep_gate(&f->bass.step[st])==G_NOTE)*w|=bit;else *w&=~bit;}
    if(p->variant.arrangement.flags&POLY_FLAG)poly_set(p->pat,(uint8_t *)&p->sound,6,lane,f->bass.len,f->bass.rate);
    for(int m=0;m<NLANE;m++){lane_t *l=&p->arr.lane[m];if(l->used && l->pat==6 && l->part==part && l->t==T_PROB && l->v==(part<<4|voice))motion_clear(p->arr.lane,m);}
}
static void mf_capture(int src){
    seq_start(&seq);mf_samples(100000);mf_token(src,1);CHECK(!mf_arm(src,0));CHECK(!mf_execute(src,0));CHECK(cc_sources[src].generation.state==CG_PENDING);
    mf_samples(90000);CHECK(seq.freeze->state==FREEZE_HELD);cc_generation_service();CHECK(cc_sources[src].generation.state==CG_APPLIED);CHECK(freeze_capture_token());
    CHECK(!mf_execute(src,0));CHECK(cc_sources[src].generation.state==CG_APPLIED);seq_stop(&seq);mf_samples(1);
}
static void mf_fixture(int lane){
    mf_setup(2,lane,16);CHECK(project_save()==0);char name[128];snprintf(name,sizeof name,"build/midi-freeze/before-%d.json",lane);fixture_store(name);
    /* Independent golden path: existing native variation/Freeze/Commit adapters. */
    mf_variation(lane);seq_start(&seq);mf_samples(100000);CHECK(ui_freeze_request()==VR_OK);mf_samples(90000);CHECK(seq.freeze->state==FREEZE_HELD);seq_stop(&seq);mf_samples(1);
    snprintf(name,sizeof name,"build/midi-freeze/capture-%d.json",lane);FILE *f=fopen(name,"wb");CHECK(f);
    fprintf(f,"{\"capture_id\":%u,\"steps\":[",freeze_capture_token());for(int st=0;st<16;st++){bstep_t *s=&seq.freeze->bass.step[st];fprintf(f,st?",[%u,%u]":"[%u,%u]",s->note,s->flags);}fprintf(f,"]}\n");fclose(f);
    CHECK(ui_freeze_commit(6)==VR_OK);render();CHECK(project_save()==0);snprintf(name,sizeof name,"build/midi-freeze/after-%d.json",lane);fixture_store(name);
}
int main(void){
    for(int format=0;format<3;format++)for(int lane=0;lane<24;lane++)for(int src=0;src<3;src++){
        mf_setup(format,lane,16);cv_select(src,lane);mf_variation(lane);project_t base=proj;undo_t history=undo;unsigned saved=store_writes;
        mf_capture(src);CHECK(!memcmp(&proj,&base,sizeof base)&&!memcmp(&history,&undo,sizeof undo)&&store_writes==saved);
        pattern_freeze_t frozen=*seq.freeze;uint32_t token=freeze_capture_token();mf_query(src,5,0,1);
        for(int st=0;st<16;st++)mf_query(src,6,st,1);mf_query(src,6,16,0);
        mf_commit_stage(src,6,token,2);CHECK(!mf_arm(src,1));project_t expected=base;mf_expected(&expected,lane,&frozen);
        CHECK(!mf_execute(src,1));render();CHECK(cc_sources[src].generation.state==CG_APPLIED);CHECK(!memcmp(&proj,&expected,sizeof proj));CHECK(!memcmp(&frozen,seq.freeze,sizeof frozen));CHECK(store_writes==saved);
        undo_t applied=undo;CHECK(!mf_execute(src,1));CHECK(!memcmp(&applied,&undo,sizeof undo));
        CHECK(ui_cc_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&base,sizeof proj));CHECK(ui_cc_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&expected,sizeof proj));
        mf_token(src,3);CHECK(!mf_arm(src,1));CHECK(!mf_execute(src,1));render();CHECK(cc_sources[src].generation.state==CG_NO_CHANGE);
        CHECK(project_save()==0);project_t kept=proj;boot();render();CHECK(!memcmp(&proj,&kept,sizeof proj));CHECK(seq.freeze->state==FREEZE_RECORD && seq.variation.lane==255);
    }
    /* Refuse unsupported capture directions/first ties and shared-kit truncation. */
    for(int issue=0;issue<3;issue++){
        mf_setup(0,issue==2?0:22,16);int lane=issue==2?0:22;cv_select(0,lane);mf_variation(lane);
        if(issue==2){
            mf_capture(0);proj.pat[6].drum[0].len=8;mf_commit_stage(0,6,freeze_capture_token(),2);
            project_t old=proj;CHECK(mf_arm(0,1)<0 && cc_sources[0].generation.reason==CG_CAPTURE_UNSUPPORTED);CHECK(!memcmp(&old,&proj,sizeof proj));
        }else{
            if(issue==0)proj.pat[1].bass[0].dir=DIR_REV;else proj.pat[1].bass[0].step[0].flags=G_TIE;
            mf_token(0,1);CHECK(!mf_arm(0,0));project_t old=proj;CHECK(mf_execute(0,0)<0 && cc_sources[0].generation.reason==CG_CAPTURE_UNSUPPORTED);CHECK(!memcmp(&old,&proj,sizeof proj));
        }
    }
    /* Every active captured step,including64 and allthree source routes. */
    for(int src=0;src<3;src++){
        mf_setup(2,23,64);cv_select(src,23);mf_variation(23);seq_start(&seq);mf_samples(200000);mf_token(src,1);CHECK(!mf_arm(src,0));CHECK(!mf_execute(src,0));mf_samples(510000);CHECK(seq.freeze->state==FREEZE_HELD);cc_generation_service();seq_stop(&seq);mf_samples(1);mf_query(src,5,0,1);
        for(int st=0;st<64;st++)mf_query(src,6,st,1);
    }
    for(int issue=0;issue<15;issue++){
        mf_setup(2,22,16);cv_select(0,22);mf_variation(22);mf_capture(0);mf_commit_stage(0,6,freeze_capture_token(),2);project_t old=proj;undo_t history=undo;
        if(issue==0){mf_commit_stage(0,1,freeze_capture_token(),2);CHECK(mf_arm(0,1)<0);}
        if(issue==1){exact_value(0,1237,(freeze_capture_token()&255u)^1u);CHECK(mf_arm(0,1)<0);}
        if(issue==2){seq.freeze->state=FREEZE_RECORD;CHECK(mf_arm(0,1)<0);}
        if(issue>=3){CHECK(!mf_arm(0,1));}
        if(issue==3){seq.freeze->bass.step[0].note^=1;}
        if(issue==4){seq.variation.seed^=1;}
        if(issue==5){seq.transport_epoch++;}
        if(issue==6){proj.pat[0].swing^=1;old=proj;}
        if(issue==7){CHECK(!midi_parameter(0,1234,127));}
        if(issue==8){CHECK(!cc_set_mode(1));}
        if(issue==9){cc_reset_source(0);}
        if(issue==10){seq.playing=1;}
        if(issue==11){rejected_save=1;}
        if(issue==12){ui.rec=1;}
        if(issue==13){exact_value(0,1220,7);}
        if(issue==14){seq.req_start=1;}
        (void)midi_parameter(0,1222,0);CHECK(midi_parameter(0,1222,127)<0);CHECK(!memcmp(&old,&proj,sizeof proj)&&!memcmp(&history,&undo,sizeof undo));
        seq.playing=seq.req_start=ui.rec=rejected_save=0;CHECK(!cc_set_mode(0));
    }
    for(int issue=0;issue<5;issue++){
        mf_setup(2,22,16);cv_select(0,22);mf_variation(22);seq_start(&seq);mf_samples(1000);mf_token(0,1);CHECK(!mf_arm(0,0));CHECK(!mf_execute(0,0));
        project_t old=proj;undo_t history=undo;
        if(issue==0){CHECK(!midi_parameter(0,1234,127));CHECK(seq.freeze->state==FREEZE_RECORD);}
        if(issue==1){cc_reset_source(0);CHECK(seq.freeze->state==FREEZE_RECORD);}
        if(issue==2){CHECK(!cc_set_mode(2));CHECK(seq.freeze->state==FREEZE_RECORD);}
        if(issue==3){seq_stop(&seq);mf_samples(1);cc_generation_service();CHECK(cc_sources[0].generation.state==CG_REJECTED);}
        if(issue==4){cv_select(1,22);mf_token(1,1);CHECK(mf_arm(1,0)<0 && cc_sources[1].generation.reason==CG_OWNER);CHECK(!midi_parameter(0,1234,127));}
        CHECK(!memcmp(&old,&proj,sizeof proj)&&!memcmp(&history,&undo,sizeof undo));seq_stop(&seq);mf_samples(1);CHECK(!cc_set_mode(0));
    }
    for(int lane=0;lane<24;lane++)mf_fixture(lane);
    printf("FREEZE MIDI PASS: %d checkpoints;24lanes/3formats/3sources;pending/completion,64steps,capture identity,explicit destination,Undo/Redo/save/reboot,Cancel/stale/owner/mode/refusals;24 independent native fixture sets.\n",checks);return 0;
}
