/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_QUERY_MAIN parent_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
static int allowed(int pos,int i){return !pos || (pos==1 && i%2) || (pos==2 && !(i%2)) || (pos==3 && i%4==2);}
static int anchor_pos(int anchor,int i){return anchor==1||anchor==4?i%4==0:i%16==4||i%16==12;}
static void drum_pure(void)
{
    for(int len=1;len<=64;len++)for(int pos=0;pos<4;pos++)for(int an=0;an<6;an++)for(int d=0;d<=100;d+=25)for(int seed=1;seed<=8;seed++){
        uint32_t row[2]={0xaaa5a55au,0xdeadbeefu},old[2],again[2];memcpy(old,row,sizeof row);memcpy(again,row,sizeof row);
        int valid=1,anchors=0;
        for(int i=0;i<len;i++){
            int ap=anchor_pos(an,i),prior=(old[i/32]>>(i%32))&1u;
            int forced=((an==1||an==2)&&ap)||(an==3&&prior),locked=(an==4||an==5)&&ap;
            anchors+=ap;if(!allowed(pos,i)&&(forced||(locked&&prior)))valid=0;
        }
        if(an!=0&&an!=3&&!anchors)valid=0;
        int rc=pattern_generate_drum(row,len,d,pos,an,(uint32_t)seed);CHECK(rc==valid);
        CHECK(pattern_generate_drum(again,len,d,pos,an,(uint32_t)seed)==rc);CHECK(!memcmp(row,again,sizeof row));
        if(!valid){CHECK(!memcmp(old,row,sizeof row));continue;}
        memcpy(again,row,sizeof row);CHECK(pattern_generate_drum(again,len,d,pos,an,(uint32_t)seed));CHECK(!memcmp(again,row,sizeof row));
        for(int i=0;i<64;i++){
            int on=(row[i/32]>>(i%32))&1u,prior=(old[i/32]>>(i%32))&1u,ap=anchor_pos(an,i);
            if(i>=len){CHECK(on==prior);continue;}
            if(on)CHECK(allowed(pos,i));
            if((an==1||an==2)&&ap)CHECK(on);
            else if((an==4||an==5)&&ap)CHECK(on==prior);
            else if(an==3&&prior)CHECK(on);
            else if(d==0)CHECK(!on);
            else if(d==100)CHECK(on==allowed(pos,i));
        }
    }
    uint32_t example[2]={0,0};
    CHECK(pattern_generate_drum(example,64,100,DR_POS_EVEN,DR_AN_NONE,1));CHECK(example[0]==0xaaaaaaaau&&example[1]==0xaaaaaaaau);
    CHECK(pattern_generate_drum(example,64,100,DR_POS_ODD,DR_AN_NONE,1));CHECK(example[0]==0x55555555u&&example[1]==0x55555555u);
    int bad[][5]={{0,50,0,0,1},{65,50,0,0,1},{16,-1,0,0,1},{16,101,0,0,1},{16,50,-1,0,1},{16,50,4,0,1},{16,50,0,-1,1},{16,50,0,6,1},{16,50,0,0,0}};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){uint32_t row[2]={7,9};CHECK(!pattern_generate_drum(row,bad[i][0],bad[i][1],bad[i][2],bad[i][3],(uint32_t)bad[i][4]));CHECK(row[0]==7&&row[1]==9);}
}
static void drum_transaction(int format,int kit,int voice,int seed)
{
    mode_setup(format);before=proj;int history=undo.cur,len=ui_drum_length(&proj.pat[1],kit,voice);
    CHECK(ui_generate_drum(1,kit,voice,50,DR_POS_ALL,DR_AN_BEAT,(uint32_t)seed)==VR_OK);after=proj;
    original=before;memcpy(original.pat[1].drum[kit].hit[voice],after.pat[1].drum[kit].hit[voice],sizeof original.pat[1].drum[kit].hit[voice]);CHECK(!memcmp(&proj,&original,sizeof proj));
    for(int i=0;i<64;i++){if(i>=len)CHECK(sm_get(after.pat[1].drum[kit].hit[voice],i)==sm_get(before.pat[1].drum[kit].hit[voice],i));else if(i%4==0)CHECK(sm_get(after.pat[1].drum[kit].hit[voice],i));}
    CHECK(undo.cur==history+1);CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    if(format==2&&seed==1){
        char path[128];CHECK(project_save()==0);after=proj;
        snprintf(path,sizeof path,"build/drum-generation/after-%d-%d.json",kit,voice);fixture_store(path);
        CHECK(ui_external_undo(0)==VR_OK);render();CHECK(project_save()==0);before=proj;
        snprintf(path,sizeof path,"build/drum-generation/before-%d-%d.json",kit,voice);fixture_store(path);
        snprintf(path,sizeof path,"build/drum-generation/before-%d-%d.bin",kit,voice);snapshot(path);
        CHECK(ui_generate_drum(1,kit,voice,50,DR_POS_ALL,DR_AN_BEAT,1)==VR_OK);render();
        snprintf(path,sizeof path,"build/drum-generation/after-%d-%d.bin",kit,voice);snapshot(path);
        CHECK(ui_external_undo(0)==VR_OK);render();snprintf(path,sizeof path,"build/drum-generation/undo-%d-%d.bin",kit,voice);snapshot(path);CHECK(!memcmp(&proj,&before,sizeof proj));
        CHECK(ui_external_undo(1)==VR_OK);render();CHECK(project_save()==0);after=proj;CHECK(project_load()==0);CHECK(!memcmp(&proj,&after,sizeof proj));boot();render();CHECK(!memcmp(&proj,&after,sizeof proj));
    }
}
static void drum_refused(void)
{
    for(int flag=0;flag<15;flag++){
        save_hold=0;mode_setup(2);int rc=VR_BUSY;
        if(flag==0)seq.playing=1;if(flag==1)seq.req_start=1;if(flag==2)seq.req_stop=1;if(flag==3)seq.req_cont=1;
        if(flag==4)seq.transport_stage=1;if(flag==5)seq.transport_hold=1;if(flag==6)ui.rec=1;if(flag==7)ui.keys=1;
        if(flag==8)external_edit=1;if(flag==9)save_wait=1;if(flag==10)save_hold=1;if(flag==11){rejected_save=1;rc=VR_PROTECTED;}
        if(flag==12){perf.test=0;perf.phase=1;}if(flag==13)fail_dma_sync=1;if(flag==14)start_before_reserve=1;
        remember();CHECK(ui_generate_drum(1,0,0,50,0,0,1)==rc);unchanged(flag!=14);
    }
    save_hold=0;mode_setup(2);remember();CHECK(ui_generate_drum(1,0,0,50,DR_POS_EVEN,DR_AN_BEAT,1)==VR_INVALID);unchanged(1);
    mode_setup(2);for(unsigned i=0;i<sizeof undo_shadow;i++)undo.shadow[i]=(uint8_t)~undo.region[i];remember();CHECK(ui_generate_drum(1,0,0,50,0,0,1)==VR_FULL);unchanged(1);
    for(int field=0;field<7;field++){mode_setup(2);int a[7]={1,0,0,50,0,0,1};a[field]=-1;remember();CHECK(ui_generate_drum(a[0],a[1],a[2],a[3],a[4],a[5],(uint32_t)a[6])==(field==6?VR_OK:VR_INVALID));if(field!=6)unchanged(1);}
    mode_setup(2);memset(proj.pat[1].drum[0].hit[0],0,8);settle();remember();CHECK(ui_generate_drum(1,0,0,0,0,0,1)==VR_OK);unchanged(1);
    mode_setup(2);engine_set(T_909,0,0,50);remember();CHECK(ui_generate_drum(1,0,0,50,0,0,1)==VR_BUSY);unchanged(1);
    mode_setup(2);start_during_stage=1;CHECK(ui_generate_drum(1,0,0,50,0,0,1)==VR_OK);CHECK(!seq.playing&&seq.req_start);render();CHECK(seq.playing);seq_stop(&seq);render();
}
static void drum_menu(void)
{
    mode_setup(2);ui.part=PART_808;ui.sel[1]=10;seq.ppat[1]=31;set_view(V_PART);button_tap(B_ARP);CHECK(ui.view==V_GEN&&pg.n==5);
    before=proj;for(int i=0;i<4;i++)pref_set(PR(R_DRUMGEN,0,0,i),i==0?50:i==3?255:0);CHECK(!memcmp(&proj,&before,sizeof proj));
    button_tap(B_OCTDN);CHECK(ui.overlay==O_ASK);CHECK(strstr(ui.ask_q[0],"808"));CHECK(strstr(ui.ask_q[0],"P32"));CHECK(strstr(ui.ask_q[1],"SEED 256"));button_tap(B_HOME);CHECK(!memcmp(&proj,&before,sizeof proj));CHECK(ui.drum_gen[3]==255);
    button_tap(B_OCTUP);CHECK(ui.overlay==O_ASK);int packed=ui.ask_arg;CHECK(((packed>>22)&255)!=255);button_tap(B_HOME);CHECK(ui.drum_gen[3]==255);
    button_tap(B_OCTDN);proj.pat[2].swing^=1;after=proj;button_tap(B_SEL);CHECK(strstr(ui.msg,"CHANGED"));CHECK(!memcmp(&proj,&after,sizeof proj));
    button_tap(B_OCTDN);seq.transport_epoch++;after=proj;button_tap(B_SEL);CHECK(strstr(ui.msg,"CHANGED"));CHECK(!memcmp(&proj,&after,sizeof proj));
    button_tap(B_OCTDN);before=proj;ui.part=PART_909;ui.sel[0]=3;seq.ppat[0]=1;ui.drum_gen[0]=0;button_tap(B_SEL);CHECK(strstr(ui.msg,"GENERATED"));original=before;memcpy(original.pat[31].drum[1].hit[10],proj.pat[31].drum[1].hit[10],8);CHECK(!memcmp(&proj,&original,sizeof proj));
    ui_init();CHECK(ui.drum_gen[0]==50&&!ui.drum_gen[1]&&!ui.drum_gen[2]&&!ui.drum_gen[3]);
    mode_setup(2);ui.part=PART_909;seq.ppat[0]=1;ui.sel[0]=1;set_view(V_GEN);pref_set(PR(R_DRUMGEN,0,0,2),DR_AN_BEAT);ui.msg_until=0;ui_frame();png_write("build/drum-generation/page.png");button_tap(B_OCTDN);ui_frame();png_write("build/drum-generation/confirmation.png");button_tap(B_SEL);after=proj;int history=undo.cur;button_tap(B_OCTDN);button_tap(B_SEL);CHECK(!memcmp(&proj,&after,sizeof proj));CHECK(undo.cur==history);CHECK(strstr(ui.msg,"SAME RHYTHM"));
    button_tap(B_OCTUP);int fresh_raw=(ui.ask_arg>>22)&255;button_tap(B_SEL);CHECK(ui.drum_gen[3]==fresh_raw);CHECK(strstr(ui.msg,"GENERATED")||strstr(ui.msg,"SAME RHYTHM"));
    ui.sel[0]=11;before=proj;button_tap(B_OCTDN);CHECK(ui.overlay!=O_ASK);CHECK(strstr(ui.msg,"TRACK"));CHECK(!memcmp(&proj,&before,sizeof proj));
}
int main(void)
{
    drum_pure();for(int f=0;f<3;f++)for(int k=0;k<2;k++)for(int v=0;v<11;v++)for(int seed=1;seed<=3;seed++)drum_transaction(f,k,v,seed);
    drum_refused();drum_menu();printf("DRUM GENERATION PASS: %d checkpoints; 61440 length/constraint/seed cases, 198 format/lane/seed transactions; Undo/Redo, storage/query snapshots, refusals, races and menu.\n",checks);return 0;
}
