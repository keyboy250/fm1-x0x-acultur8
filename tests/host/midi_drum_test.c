/* SPDX-License-Identifier: GPL-3.0-only */
#define MIDI_303_NO_MAIN
#include "midi_303_test.c"
static void drum_select(int source,int kit,int voice){
    CHECK(!cc_parameter(1010,1,source));CHECK(!cc_parameter(1011,kit,source));
    CHECK(!cc_parameter(1068,kit,source));CHECK(!cc_parameter(1012,voice,source));
    CHECK(!cc_parameter(1069,kit*11+voice,source));CHECK(!cc_parameter(1071,1,source));
}
static void drum_recipe(int source,int density,int positions,int anchor,int seed,int length,int token){
    exact_value(source,1206,density);exact_value(source,1207,positions);exact_value(source,1208,anchor);
    exact_value(source,1209,seed);exact_value(source,1223,length);
    CHECK(!midi_parameter(source,1231,token&127));CHECK(!midi_parameter(source,1232,token>>7));
}
static void drum_trigger(int source){CHECK(!midi_parameter(source,1233,54));CHECK(!midi_parameter(source,1210,0));CHECK(!midi_parameter(source,1210,127));}
static void drum_wire_fixtures(void){
    for(int kit=0;kit<2;kit++)for(int voice=0;voice<11;voice++){
        mode_setup(2);CHECK(project_save()==0);char file[128];
        snprintf(file,sizeof file,"build/midi-drum/before-%d-%d.json",kit,voice);fixture_store(file);
        /* Goldens use the existing explicit native UI adapter, not the CC adapter. */
        CHECK(ui_generate_drum(1,kit,voice,50,0,1,256)==VR_OK);render();CHECK(project_save()==0);
        snprintf(file,sizeof file,"build/midi-drum/after-%d-%d.json",kit,voice);fixture_store(file);
    }
}
#ifndef MIDI_DRUM_NO_MAIN
int main(void){
    for(int format=0;format<3;format++)for(int kit=0;kit<2;kit++)for(int voice=0;voice<11;voice++)for(int src=0;src<3;src++){
        mode_setup(format);for(int s=0;s<3;s++)cc_reset_source(s);ui.part=PART_303B;
        drum_select(src,kit,voice);project_t old=proj;undo_t history=undo;
        drum_recipe(src,50,0,1,255,0,1);CHECK(!memcmp(&old,&proj,sizeof proj)&&!memcmp(&history,&undo,sizeof undo));
        uint32_t words[24];CHECK(cc_generation_snapshot(src,3,0,words)==12&&words[8]==255&&words[9]==256&&words[11]==1);
        CHECK(cc_generation_snapshot(src,2,0,words)==-1);drum_trigger(src);CHECK(!seq.transport_hold);
        CHECK(cc_sources[src].generation.state==CG_APPLIED||cc_sources[src].generation.state==CG_NO_CHANGE);
        CHECK(ui.part==PART_303B);project_t expected=old;
        memcpy(expected.pat[1].drum[kit].hit[voice],proj.pat[1].drum[kit].hit[voice],8);
        CHECK(!memcmp(&expected,&proj,sizeof proj));int len=ui_drum_length(&proj.pat[1],kit,voice);
        for(int i=0;i<64;i++)if(i>=len)CHECK(sm_get(proj.pat[1].drum[kit].hit[voice],i)==sm_get(old.pat[1].drum[kit].hit[voice],i));else if(i%4==0)CHECK(sm_get(proj.pat[1].drum[kit].hit[voice],i));
        project_t generated=proj;history=undo;
        CHECK(!midi_parameter(src,1210,0));CHECK(!midi_parameter(src,1210,127));CHECK(!memcmp(&generated,&proj,sizeof proj)&&!memcmp(&history,&undo,sizeof undo));
        if(memcmp(&old,&proj,sizeof proj)){CHECK(ui_cc_undo(0)==VR_OK);render();CHECK(!memcmp(&old,&proj,sizeof proj));CHECK(ui_cc_undo(1)==VR_OK);render();CHECK(!memcmp(&generated,&proj,sizeof proj));}
        query_unchanged(src,3);query_unchanged(src,7);
    }
    /* Every property encoded by the wire recipe must survive staging exactly. */
    mode_setup(2);cc_reset_source(0);drum_select(0,0,0);
    for(int d=0;d<=100;d++)for(int pos=0;pos<4;pos++)for(int an=0;an<6;an++){
        drum_recipe(0,d,pos,an,255,64,1);uint32_t w[24];CHECK(cc_generation_snapshot(0,3,0,w)==12);
        CHECK(w[5]==d&&w[6]==pos&&w[7]==an&&w[8]==255&&w[9]==256&&w[10]==64);
    }
    /* Invalid/incomplete/wrong-family targets, stale state and cancellation. */
    for(int issue=0;issue<10;issue++){
        mode_setup(2);cc_reset_source(0);drum_select(0,0,0);drum_recipe(0,50,0,0,1,0,1);project_t old=proj;
        if(issue==0){CHECK(!cc_parameter(1069,22,0));CHECK(!cc_parameter(1071,1,0));}
        if(issue==1)cc_sources[0].generation.seed_mask=0;
        if(issue==2)seq.req_start=1;
        if(issue==3)ui.rec=1;
        if(issue==4)rejected_save=1;
        int armed=midi_parameter(0,1233,54);
        if(issue<5)CHECK(armed<0);
        else{CHECK(!armed);if(issue==5)proj.pat[0].swing^=1;if(issue==6)seq.transport_epoch++;
            if(issue==7)CHECK(!midi_parameter(0,1234,127));if(issue==8)cc_reset_source(0);
            if(issue==9){exact_value(0,1207,1);exact_value(0,1208,1);} /* recipe change disarms */
            old=proj;CHECK(midi_parameter(0,1210,127)<0);
        }
        CHECK(!memcmp(&old,&proj,sizeof proj));seq.req_start=0;ui.rec=0;rejected_save=0;
    }
    /* KEEP/no-op, length, family switch and independent sources. */
    mode_setup(2);cc_reset_source(0);drum_select(0,0,0);memset(proj.pat[1].drum[0].hit[0],0,8);settle();
    drum_recipe(0,0,0,0,0,0,1);drum_trigger(0);CHECK(cc_sources[0].generation.state==CG_NO_CHANGE);
    drum_recipe(0,100,0,0,0,32,2);drum_trigger(0);CHECK(ui_drum_length(&proj.pat[1],0,0)==32);
    selected(0,0);recipe(0,4,0,0,3);CHECK(!cc_sources[0].generation.drum);CHECK(cc_generation_snapshot(0,3,0,(uint32_t[24]){0})<0);
    drum_select(1,1,10);drum_recipe(1,25,0,0,7,0,1);CHECK(cc_sources[1].generation.drum&&!cc_sources[0].generation.drum);
    /* Diagnostic modes are native-only, session-only and do not block queries. */
    mode_setup(2);cc_reset_source(0);drum_select(0,0,0);drum_recipe(0,50,0,0,0,0,1);CHECK(!midi_parameter(0,1233,54));
    project_t old=proj;CHECK(!cc_set_mode(1));CHECK(cc_mode()==1&&!cc_sources[0].ready&&!cc_sources[0].generation.seed_mask);
    CHECK(!memcmp(&old,&proj,sizeof proj));CHECK(midi_parameter(0,1010,127)==CC_UNSUPPORTED);CHECK(midi_parameter(0,1210,127)==CC_UNSUPPORTED);
    CHECK(!midi_parameter(0,0,64));render();CHECK(!cc_set_mode(2));old=proj;
    CHECK(midi_parameter(0,0,127)==CC_UNSUPPORTED);CHECK(midi_parameter(0,1120,127)==CC_UNSUPPORTED);
    CHECK(!memcmp(&old,&proj,sizeof proj));query_unchanged(0,7);CHECK(cc_set_mode(3)==CC_REJECTED&&cc_mode()==2);
    CHECK(!cc_sources[0].ready);boot();render();CHECK(cc_mode()==0);
    open_global();int found=0;for(int i=0;i<ui.list_n;i++)if(P_KIND(list_rows[i])==R_CCMODE){found++;pref_set(list_rows[i],1);CHECK(pref_get(list_rows[i])==1);}CHECK(found==1);
    CHECK(!cc_set_mode(0));
    /* Clearing queued CC on mode change must retain Note-Off and clock. */
    mi_w=mi_r=mi_ready=0;mi_claimed=0xffffffffu;
    CHECK(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,11u|(176u<<8)|(99u<<24),0));
    CHECK(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,8u|(0x82u<<8)|(60u<<16),0));
    CHECK(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,15u|(248u<<8),1));
    CHECK(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,11u|(176u<<8)|(1u<<16)|(70u<<24),2));
    uint32_t off=min_q[1],clock=min_q[2];CHECK(!cc_set_mode(2));
    CHECK(!min_q[0]&&!min_q[3]&&min_q[1]==off&&min_q[2]==clock);
    CHECK(!cc_set_mode(0));mi_w=mi_r=mi_ready=0;drum_wire_fixtures();
    printf("MIDI DRUM PASS: %d checkpoints;22 drums/3 formats/3 sources,exact recipes,anchors,isolation,Undo/Redo,Cancel/stale/no-op,CC modes and read-only query.\n",checks);return 0;
}
#endif
