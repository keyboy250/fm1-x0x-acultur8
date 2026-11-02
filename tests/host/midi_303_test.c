/* SPDX-License-Identifier: GPL-3.0-only */
#define RANDOM_OPTIONS_NO_MAIN
#include "random_options_test.c"

static int midi_parameter(int source,int number,uint32_t value)
{
    /* Production ingress/CC7 scaling; inverse mapping only for reachable values.
     * Wider canonical integers travel through the real exact framing path. */
    int cc=number%88,channel=number/88;
    if(cc>=32)cc+=32;
    return cc_message(11u|((uint32_t)source<<4)|((uint32_t)(176+channel)<<8)|((uint32_t)cc<<16)|(value<<24),(uint8_t)source);
}
static void exact_value(int source,int number,unsigned value)
{
    CHECK(!cc_parameter(1072,(unsigned)number&127,(uint8_t)source));
    CHECK(!cc_parameter(1073,(unsigned)number>>7,(uint8_t)source));
    CHECK(!cc_parameter(1074,value&127,(uint8_t)source));
    CHECK(!cc_parameter(1075,(value>>7)&127,(uint8_t)source));
    CHECK(!cc_parameter(1130,(value>>14)&127,(uint8_t)source));
    CHECK(!midi_parameter(source,1077,127));
}
static void selected(int source,int bass)
{
    CHECK(!cc_parameter(1010,1,(uint8_t)source));CHECK(!cc_parameter(1011,2+bass,(uint8_t)source));
    CHECK(!cc_parameter(1013,bass,(uint8_t)source));CHECK(!cc_parameter(1069,22+bass,(uint8_t)source));
    CHECK(!cc_parameter(1071,1,(uint8_t)source));
}
static void recipe(int source,int mode,int length,int step,unsigned token)
{
    exact_value(source,1203,(unsigned)mode);exact_value(source,1204,0);
    exact_value(source,1223,(unsigned)length);exact_value(source,1224,(unsigned)step);
    for(unsigned byte=0;byte<4;byte++)exact_value(source,1227+(int)byte,(0x12345678u>>(byte*8))&255u);
    CHECK(!midi_parameter(source,1231,token&127));CHECK(!midi_parameter(source,1232,token>>7));
}
static void trigger(int source)
{
    CHECK(!midi_parameter(source,1233,36)); /* round36*7/127=2 */
    CHECK(!midi_parameter(source,1205,0));CHECK(!midi_parameter(source,1205,127));
}
static void query_unchanged(unsigned source,unsigned page)
{
    project_t before=proj;seq_t oldseq=seq;undo_t oldundo=undo;uint8_t oldui[sizeof ui];memcpy(oldui,&ui,sizeof ui);
    unsigned saved=store_writes,w=engine_queue_written(),r=engine_queue_read();
    uint8_t frame[]={125,70,76,51,1,77,(uint8_t)source,(uint8_t)page,0};
    memcpy(incoming,frame,sizeof frame);incoming_n=sizeof frame;wire_n=0;ed_service();
    CHECK(wire_n>=17&&wire[10]==0&&wire[5]==1&&wire[6]==77&&wire[7]==source&&wire[8]==page);
    CHECK(get7(wire_n-6,5)==st_crc32(wire+1,wire_n-7));
    CHECK(!memcmp(&before,&proj,sizeof proj)&&!memcmp(&oldseq,&seq,sizeof seq)&&!memcmp(&oldundo,&undo,sizeof undo));
    CHECK(!memcmp(oldui,&ui,sizeof ui)&&store_writes==saved&&w==engine_queue_written()&&r==engine_queue_read());
    char path[128];snprintf(path,sizeof path,"build/midi-303/query-%u-%u.bin",source,page);
    FILE *f=fopen(path,"wb");CHECK(f);CHECK(fwrite(wire,1,wire_n,f)==wire_n);fclose(f);
}
static void midi_fixtures(void)
{
    for(int bass=0;bass<2;bass++){
        char path[128];option_setup(2,bass);cc_reset_source(2);CHECK(project_save()==0);
        snprintf(path,sizeof path,"build/midi-303/before-%d.json",bass);fixture_store(path);
        selected(2,bass);recipe(2,4,32,0,1);trigger(2);CHECK(cc_sources[2].generation.state==CG_APPLIED);
        CHECK(project_save()==0);snprintf(path,sizeof path,"build/midi-303/whole-%d.json",bass);fixture_store(path);
        recipe(2,4,64,7,2);trigger(2);CHECK(cc_sources[2].generation.state==CG_APPLIED);
        CHECK(project_save()==0);snprintf(path,sizeof path,"build/midi-303/single-%d.json",bass);fixture_store(path);
        for(unsigned page=0;page<=4;page++)if(page!=3)query_unchanged(2,page);
    }
}
#ifndef MIDI_303_NO_MAIN
int main(void)
{
    for(int format=0;format<3;format++)for(int bass=0;bass<2;bass++)for(int src=0;src<3;src++)for(int mode=0;mode<5;mode++){
        option_setup(format,bass);for(int i=0;i<3;i++)cc_reset_source((uint8_t)i);
        selected(src,bass);recipe(src,mode,32,0,1);project_t before=proj;uint8_t panel[sizeof ui];memcpy(panel,&ui,sizeof ui);
        trigger(src);CHECK(cc_sources[src].generation.state==CG_APPLIED);CHECK(!seq.transport_hold);
        const bpart_t *bp=&proj.pat[1].bass[bass];CHECK(bp->len==32&&bp->gen.seed==0x12345678u);
        const uint16_t mask=wanted_mask(bass,mode);
        for(int i=0;i<32;i++){CHECK(mask&(1u<<(bp->step[i].note%12)));CHECK((bp->step[i].flags&BS_ACCENT)==(before.pat[1].bass[bass].step[i].flags&BS_ACCENT));}
        CHECK(!memcmp(&before.sound,&proj.sound,sizeof proj.sound));CHECK(!memcmp(&before.pat[0],&proj.pat[0],sizeof(pattern_t)));
        CHECK(ui.part==((const uint8_t *)panel)[1]);
        project_t generated=proj;undo_t journal=undo;
        CHECK(!midi_parameter(src,1205,127));CHECK(!memcmp(&generated,&proj,sizeof proj)&&!memcmp(&journal,&undo,sizeof undo));
        CHECK(!midi_parameter(src,1205,0));CHECK(!midi_parameter(src,1205,127));CHECK(!memcmp(&generated,&proj,sizeof proj)&&!memcmp(&journal,&undo,sizeof undo));
        for(unsigned page=0;page<=4;page++)if(page!=3)query_unchanged((unsigned)src,page);
        CHECK(ui_cc_undo(0)==VR_OK);render();CHECK(!memcmp(&before,&proj,sizeof proj));CHECK(ui_cc_undo(1)==VR_OK);render();CHECK(!memcmp(&generated,&proj,sizeof proj));
        /* New token,single NOTE: exactly its pitch byte changes. */
        recipe(src,mode,64,7,2);before=proj;trigger(src);CHECK(cc_sources[src].generation.state==CG_APPLIED||cc_sources[src].generation.state==CG_NO_CHANGE);
        generated=proj;generated.pat[1].bass[bass].step[6].note=before.pat[1].bass[bass].step[6].note;
        CHECK(!memcmp(&generated,&before,sizeof proj));CHECK(proj.pat[1].bass[bass].len==32);
    }
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,1);
    project_t before=proj;CHECK(!midi_parameter(0,1233,36));CHECK(!midi_parameter(0,1234,127));
    CHECK(midi_parameter(0,1205,127)==CC_REJECTED);CHECK(!memcmp(&before,&proj,sizeof proj));
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,1);CHECK(!midi_parameter(0,1233,36));
    proj.pat[1].bass[1].step[3].note^=1;before=proj;CHECK(midi_parameter(0,1205,127)==CC_REJECTED);CHECK(cc_sources[0].generation.reason==CG_STALE);CHECK(!memcmp(&before,&proj,sizeof proj));
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,1);CHECK(!midi_parameter(0,1233,36));
    seq.req_start=1;before=proj;CHECK(midi_parameter(0,1205,127)==CC_REJECTED);CHECK(!memcmp(&before,&proj,sizeof proj));seq.req_start=0;
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,1);cc_reset_source(0);before=proj;
    CHECK(midi_parameter(0,1205,127)==CC_REJECTED);CHECK(!memcmp(&before,&proj,sizeof proj));
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,1);CHECK(!midi_parameter(0,1233,36));
    CHECK(cc_parameter(1203,5,0)==CC_REJECTED);before=proj;CHECK(midi_parameter(0,1205,127)==CC_REJECTED);CHECK(!memcmp(&before,&proj,sizeof proj));
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,1);CHECK(!midi_parameter(0,1233,36));
    CHECK(cc_parameter(1233,1,0)==CC_UNSUPPORTED);before=proj;CHECK(midi_parameter(0,1205,127)==CC_REJECTED);CHECK(!memcmp(&before,&proj,sizeof proj));
    option_setup(2,0);for(int i=0;i<3;i++){cc_reset_source((uint8_t)i);selected(i,i&1);recipe(i,4,32,0,1);}
    cc_generation_t other1=cc_sources[1].generation,other2=cc_sources[2].generation;trigger(0);
    CHECK(!memcmp(&other1,&cc_sources[1].generation,sizeof other1)&&!memcmp(&other2,&cc_sources[2].generation,sizeof other2));
    ui_init();CHECK(!cc_sources[0].generation.seed&&!cc_sources[1].generation.seed&&!cc_sources[2].generation.seed);
    /* A one-tone intersection in a single octave has no legal alternate. */
    option_setup(2,0);cc_reset_source(0);selected(0,0);
    proj.pat[1].bass[0].gen.scale=3;proj.pat[1].bass[0].gen.root=0;proj.pat[1].bass[0].gen.base_oct=0;proj.pat[1].bass[0].gen.oct_range=1;
    proj.pat[1].bass[0].step[0]=(bstep_t){19,G_NOTE};settle();recipe(0,3,0,1,1);
    before=proj;undo_t saved_undo=undo;trigger(0);CHECK(cc_sources[0].generation.state==CG_NO_CHANGE);
    CHECK(!memcmp(&before,&proj,sizeof proj)&&!memcmp(&saved_undo,&undo,sizeof undo));
    for(int gate=0;gate<=2;gate+=2){
        option_setup(2,0);cc_reset_source(0);selected(0,0);proj.pat[1].bass[0].step[0].flags=(uint8_t)gate;settle();recipe(0,4,0,1,1);
        before=proj;CHECK(!midi_parameter(0,1233,36));CHECK(!midi_parameter(0,1205,0));CHECK(midi_parameter(0,1205,127)==CC_REJECTED);
        CHECK(!memcmp(&before,&proj,sizeof proj)&&cc_sources[0].generation.state==CG_REJECTED);
    }
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,1);
    for(int byte=0;byte<4;byte++)exact_value(0,1227+byte,0);
    CHECK(midi_parameter(0,1233,36)==CC_REJECTED&&cc_sources[0].generation.reason==CG_VALUE);
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,1);before=proj;
    uint32_t saved_w=cq_w;cq_w=cq_r+CQ;CHECK(midi_parameter(0,1233,36)==CC_REJECTED);CHECK(cc_sources[0].generation.reason==CG_QUEUE);cq_w=saved_w;
    CHECK(!memcmp(&before,&proj,sizeof proj));
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,0,1,3);trigger(0);
    CHECK(!midi_parameter(0,1231,1));CHECK(midi_parameter(0,1233,36)==CC_REJECTED);
    CHECK(cc_sources[0].generation.last_request==3&&cc_sources[0].generation.result_request==1);
    uint32_t receipt[24];CHECK(cc_generation_snapshot(0,4,0,receipt)>0&&receipt[6]==1&&receipt[7]==1&&receipt[8]==CG_REJECTED);
    option_setup(2,0);cc_reset_source(0);selected(0,0);recipe(0,4,32,0,16383);
    exact_value(0,1204,191);for(int byte=0;byte<4;byte++)exact_value(0,1227+byte,255);
    trigger(0);CHECK(cc_sources[0].generation.state==CG_APPLIED&&proj.pat[1].bass[0].gen.seed==UINT32_MAX);
    CHECK(cc_generation_snapshot(0,2,0,receipt)>0&&receipt[6]==191&&receipt[9]==UINT32_MAX);
    for(unsigned page=0;page<=4;page++)if(page!=3)query_unchanged(0,page);
    midi_fixtures();
    printf("MIDI303 PASS: %d checkpoints; both303/3formats/3sources/5constraints, CC7/exact, query CRC/no mutation, dedup, one-note isolation, Undo/Redo, Cancel/stale/transport/reset.\n",checks);
    return 0;
}
#endif
