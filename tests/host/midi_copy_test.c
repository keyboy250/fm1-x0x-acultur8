/* SPDX-License-Identifier: GPL-3.0-only */
#define MIDI_VARIATION_NO_MAIN
#include "midi_variation_test.c"
static int routes;
static unsigned cp_raw(unsigned value,unsigned max){return (value*127+max/2)/max;}
static void cp_recipe(int src,int dst,int from,int to,unsigned id){
    const int value[]={1,dst,from,to},max[]={31,31,24,24};
    for(int i=0;i<4;i++)CHECK(!midi_parameter(src,1241+i,cp_raw(value[i],max[i])));
    CHECK(!midi_parameter(src,1231,id&127));CHECK(!midi_parameter(src,1232,id>>7));
}
static int cp_preview(int src){return midi_parameter(src,1245,127);}
static int cp_execute(int src){int rc=midi_parameter(src,1246,0);return rc<0?rc:midi_parameter(src,1246,127);}
static void cp_setup(int format){mode_setup(format);for(int s=0;s<3;s++)cc_reset_source(s);CHECK(!cc_set_mode(0));}
static void cp_case(int format,int src,int from,int to,int same){
    cp_setup(format);int dst=same?1:2;project_t base=proj;undo_t journal=undo;
    CHECK(ui_pattern_copy_to(1,dst,from,to)==VR_OK);project_t golden=proj;
    /* UI engine oracle is independently covered by pattern/cross-copy suites. */
    cp_setup(format);CHECK(!memcmp(&base,&proj,sizeof proj));journal=undo;
    uint8_t panel=ui.part;unsigned writes=store_writes;
    cp_recipe(src,dst,from,to,1);CHECK(!cp_preview(src));
    CHECK(cc_sources[src].generation.state==CG_ACCEPTED);
    CHECK(!memcmp(&base,&proj,sizeof proj)&&!memcmp(&journal,&undo,sizeof undo));
    CHECK(ui.part==panel&&store_writes==writes);
    uint32_t words[24];CHECK(cc_generation_snapshot(src,10,0,words)==16);CHECK(words[10]==1&&words[9]==15);
    CHECK(!cp_execute(src));CHECK(!memcmp(&golden,&proj,sizeof proj));
    int changed=memcmp(&base,&golden,sizeof proj)!=0;
    CHECK(cc_sources[src].generation.state==(changed?CG_APPLIED:CG_NO_CHANGE));
    CHECK(ui.part==panel&&store_writes==writes&&!seq.playing);
    journal=undo;CHECK(!cp_execute(src));CHECK(!memcmp(&journal,&undo,sizeof undo)&&!memcmp(&golden,&proj,sizeof proj));
    if(changed){CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&base,&proj,sizeof proj));CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&golden,&proj,sizeof proj));}
    if(format==2&&src==2&&!same){
        char file[128];CHECK(project_save()==0);snprintf(file,sizeof file,"build/midi-copy/after-%d-%d.json",from,to);fixture_store(file);
        cp_setup(format);CHECK(project_save()==0);snprintf(file,sizeof file,"build/midi-copy/before-%d-%d.json",from,to);fixture_store(file);
        cp_recipe(src,dst,from,to,1);CHECK(!cp_preview(src));query_unchanged(src,10);query_unchanged(src,4);
    }
    routes++;
}
static void cp_refusals(void){
    for(int src=0;src<3;src++){
        cp_setup(2);cp_recipe(src,2,2,4,1);CHECK(!cp_preview(src));query_unchanged(src,10);query_unchanged(src,4);query_unchanged(src,7);
        for(int mode=1;mode<=2;mode++){CHECK(!cc_set_mode(mode));project_t base=proj;CHECK(midi_parameter(src,1241,127)<0);CHECK(midi_parameter(src,1245,127)<0);CHECK(midi_parameter(src,1246,127)<0);query_unchanged(src,0);query_unchanged(src,4);query_unchanged(src,7);uint32_t words[24];CHECK(cc_generation_snapshot(src,10,0,words)<0);CHECK(!memcmp(&base,&proj,sizeof proj));}CHECK(!cc_set_mode(0));cp_recipe(src,2,2,4,1);CHECK(!cp_preview(src));query_unchanged(src,10);
    }
    for(int fault=0;fault<16;fault++){
        cp_setup(2);cp_recipe(0,2,2,4,1);project_t base=proj;undo_t journal=undo;
        if(fault<10)CHECK(!cp_preview(0));
        if(fault==0)CHECK(!midi_parameter(0,1234,127));
        if(fault==1){proj.pat[0].swing^=1;base=proj;}
        if(fault==2)seq.transport_epoch++;
        if(fault==3)CHECK(!midi_parameter(0,1244,cp_raw(6,24)));
        if(fault==4)cc_reset_source(0);
        if(fault==5)CHECK(!cc_set_mode(1));
        if(fault==6)seq.playing=1;
        if(fault==7)seq.req_start=1;
        if(fault==8)external_edit=1;
        if(fault==9)save_hold=1;
        if(fault==10)CHECK(!midi_parameter(0,1244,cp_raw(23,24)));
        if(fault==11){cc_reset_source(0);CHECK(!midi_parameter(0,1241,cp_raw(1,31)));}
        if(fault==12)ui.rec=1;
        if(fault==13)rejected_save=1;
        if(fault==14)start_during_stage=1;
        if(fault==15)save_wait=1;
        if(fault>=10)CHECK(cp_preview(0)<0);
        CHECK(cp_execute(0)<0);CHECK(!memcmp(&base,&proj,sizeof proj)&&!memcmp(&journal,&undo,sizeof undo));
        save_hold=save_wait=external_edit=seq.playing=seq.req_start=ui.rec=rejected_save=0;CHECK(!cc_set_mode(0));
    }
    /* Sources isolate staging but execution invalidates another source's preview. */
    cp_setup(2);cp_recipe(0,2,2,4,1);CHECK(!cp_preview(0));cp_recipe(1,2,2,6,1);CHECK(!cp_preview(1));CHECK(!cp_execute(1));project_t applied=proj;CHECK(cp_execute(0)<0);CHECK(!memcmp(&applied,&proj,sizeof proj));
    CHECK(!midi_parameter(1,1231,0));CHECK(cp_preview(1)<0);CHECK(cc_sources[1].generation.reason==CG_STALE);
    cp_setup(2);cp_recipe(0,2,2,4,1);CHECK(!cp_preview(0));CHECK(midi_parameter(0,1233,109)<0);CHECK(!cp_active(&cc_sources[0].generation));
}
#ifndef MIDI_COPY_NO_MAIN
int main(void){
    for(int format=0;format<3;format++)for(int src=0;src<3;src++)for(int same=0;same<2;same++)for(int from=0;from<25;from++)for(int to=0;to<25;to++)
        if((from==0&&to==0)||(from>0&&from<23&&to>0&&to<23)||(from>=23&&to>=23))cp_case(format,src,from,to,same);
    cp_refusals();
    printf("COPY MIDI PASS: %d checkpoints; %d compatible routes across3 formats/3 sources/same-slot and cross-slot;preview/no-write,guard/dedup/cancel/modes,UI golden,Undo/Redo;489 saved wire-plan fixtures.\n",checks,routes);return 0;
}

#endif
