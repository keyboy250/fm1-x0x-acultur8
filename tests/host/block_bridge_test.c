/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define PLAT_CONTROL_TEST
#define PLAT_STORE_TEST
static unsigned test_dma_timeouts,fail_dma_sync;
static int start_during_stage,start_before_reserve;
static void bridge_test_sync(void);
#define CV_STAGE_DMA_TIMEOUTS test_dma_timeouts
#define CV_STAGE_SYNC bridge_test_sync
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../firmware/src/app/engine.c"
static int locked,checks;
#ifndef OPTIONAL_CONTROL_ENTER_HOOK
#define OPTIONAL_CONTROL_ENTER_HOOK() ((void)0)
#endif
uint32_t plat_control_enter(void){OPTIONAL_CONTROL_ENTER_HOOK();assert(!locked);if(start_before_reserve){start_before_reserve=0;seq.req_start=1;}locked=1;return 91;}
void plat_control_leave(uint32_t s){assert(locked&&s==91);locked=0;}
static void render(void){int32_t b[512];engine_render(b,256);}
static void bridge_test_sync(void){lcd_sync();if(fail_dma_sync)test_dma_timeouts++;if(start_during_stage){start_during_stage=0;seq_start(&seq);render();assert(!seq.playing&&seq.req_start&&seq.transport_hold);}}
#define CHECK(x) do{assert(x);checks++;}while(0)
static project_t before,after,original;
static undo_t oldundo;
static uint8_t oldshadow[sizeof undo_shadow];
static seq_t oldseq;
static unsigned oldwrites;
static uint8_t olddirty,oldselection;
static void fresh(void){
    for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
    memset(stored_known,0,sizeof stored_known);project_discard_rejected();store_fail_after=-1;checkpoint_verified=0;
    mi_w=mi_r=mi_ready=0;mi_claimed=0xffffffffu;memset(&cc_stats,0,sizeof cc_stats);
    seq.transport_hold=seq.transport_stage=0;seq.req_start=seq.req_stop=seq.req_cont=seq.playing=0;
    test_dma_timeouts=fail_dma_sync=0;ui.rec=ui.btn=ui.keys=0;perf.test=-1;perf.phase=0;perf.cc_stop_epoch=UINT32_MAX;
    ui.overlay=0;save_wait=reset_checkpoint_hold=0;start_during_stage=start_before_reserve=0;
    boot();render();for(int i=0;i<CC_SOURCES;i++)cc_reset_source(i);ui.dirty=0;
}
static void settle(void){render();undo_commit(&undo);ui.dirty=0;}
static void remember(void){before=proj;oldundo=undo;oldseq=seq;memcpy(oldshadow,undo_shadow,sizeof oldshadow);oldwrites=store_writes;olddirty=ui.dirty;oldselection=ui.song_sel;}
static void unchanged(int runtime){CHECK(!memcmp(&before,&proj,sizeof proj));CHECK(!memcmp(&oldundo,&undo,sizeof undo));CHECK(!memcmp(oldshadow,undo_shadow,sizeof oldshadow));CHECK(oldwrites==store_writes&&olddirty==ui.dirty&&oldselection==ui.song_sel);if(runtime)CHECK(!memcmp(&oldseq,&seq,sizeof seq));}
static void song_setup(void){proj.arr.song.len=3;for(int r=0;r<3;r++)for(int p=0;p<5;p++)proj.arr.song.bar[r].pat[p]=(uint8_t)(r+1);proj.variant.arrangement.flags|=ARR_ENABLED;engine_arrangement_refresh(&proj.variant.arrangement);settle();}
static void basic_and_history(void){
    fresh();song_setup();
    for(int part=0;part<5;part++)proj.arr.song.bar[0].pat[part]=(uint8_t)(part+1);
    proj.arr.song.bar[0].mute=(uint8_t)(3u<<5|2u);proj.variant.arrangement.degree[0]=4;
    proj.pat[1].drum[0].hit[0][0]=0x1111;proj.pat[2].drum[1].hit[7][1]=0x80000001;
    proj.pat[3].bass[0].step[2]=(bstep_t){52,G_NOTE|BS_ACCENT};
    proj.pat[4].bass[1].step[7]=(bstep_t){67,G_NOTE|BS_SLIDE};proj.pat[5].brk.steps=0x8421;
    int lane=motion_alloc(proj.arr.lane,2,1,T_808,0,0);CHECK(lane>=0);proj.arr.lane[lane].val[5]=83;
    seq.song_start=2;seq.arrangement.editing=0;settle();before=proj;original=proj;
    int n=undo.cur,bank=ui_duplicate_block(0);CHECK(bank==6);after=proj;
    CHECK(proj.arr.song.len==4&&ui.song_sel==1&&seq.arrangement.editing==1&&seq.song_start==3);
    CHECK(proj.arr.song.bar[1].mute==before.arr.song.bar[0].mute&&proj.variant.arrangement.degree[1]==4);
    for(int p=0;p<5;p++)CHECK(proj.arr.song.bar[1].pat[p]==bank);
    CHECK(!memcmp(&proj.pat[bank].drum[0],&before.pat[1].drum[0],sizeof(dpart_t)));
    CHECK(!memcmp(&proj.pat[bank].drum[1],&before.pat[2].drum[1],sizeof(dpart_t)));
    CHECK(!memcmp(&proj.pat[bank].bass[0],&before.pat[3].bass[0],sizeof(bpart_t)));
    CHECK(!memcmp(&proj.pat[bank].bass[1],&before.pat[4].bass[1],sizeof(bpart_t)));
    CHECK(!memcmp(&proj.pat[bank].brk,&before.pat[5].brk,sizeof(brkpart_t)));
    for(int k=0;k<NPAT;k++)if(k!=bank)CHECK(!memcmp(&proj.pat[k],&before.pat[k],sizeof(pattern_t)));
    int copy=motion_find(proj.arr.lane,bank,1,T_808,0,0);CHECK(copy>=0&&proj.arr.lane[copy].val[5]==83);
    CHECK(undo.cur==n+1);render();seq.playing=1;remember();CHECK(ui_external_undo(0)==VR_BUSY);unchanged(1);seq.playing=0;
    CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&original,sizeof proj));
}
static void independent_save(void){
    fresh();song_setup();proj.pat[1].drum[0].hit[0][0]=1;settle();before=proj;
    int bank=ui_duplicate_block(0);CHECK(bank>=0);after=proj;render();
    CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    proj.pat[bank].drum[0].hit[0][0]=0x8888;CHECK(proj.pat[1].drum[0].hit[0][0]==1);
    ui.song_sel=1;pref_set(PR(R_SREP,0,0,0),3);render();CHECK(SONG_REP(&proj.arr.song.bar[1])==4);
    bridge_native_action(ACT_BLOCK_NOTES);render();CHECK(ui.view==V_HOME);for(int p=0;p<5;p++)CHECK(seq.ppat[p]==bank);
    CHECK(project_save()==0);after=proj;CHECK(project_load()==0);render();CHECK(!memcmp(&proj,&after,sizeof proj));
}
static void expanded(void){
    fresh();CHECK(engine_lane_mode(1));render();CHECK(engine_rate_set(1,0,0,1));render();song_setup();
    for(int l=0;l<25;l++)poly_set(proj.pat,(uint8_t *)&proj.sound,1,l,64-l,l%21);
    for(int st=0;st<64;st++)poly_break_set(proj.pat,(uint8_t *)&proj.sound,1,st,st%2,st%9);
    proj.pat[1].drum[1].hit[10][1]=0xa3123;settle();before=proj;
    int bank=ui_duplicate_block(0);CHECK(bank>=4);CHECK(poly_valid(proj.pat,(const uint8_t *)&proj.sound));
    for(int l=0;l<25;l++){CHECK(poly_length(proj.pat,(const uint8_t *)&proj.sound,bank,l)==64-l);CHECK(poly_rate(proj.pat,(const uint8_t *)&proj.sound,bank,l)==l%21);}
    for(int st=0;st<64;st++){CHECK(poly_break_gate(proj.pat,(const uint8_t *)&proj.sound,bank,st)==st%2);CHECK(poly_break_slice(proj.pat,(const uint8_t *)&proj.sound,bank,st)==st%9);}
    for(int k=0;k<NPAT;k++)if(k!=bank)CHECK(!memcmp(&proj.pat[k],&before.pat[k],sizeof(pattern_t)));
    render();CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
}
static void boundaries(void){
    for(int pos=0;pos<191;pos++){
        fresh();proj.arr.song.len=191;
        for(int k=0;k<191;k++){for(int p=0;p<5;p++)proj.arr.song.bar[k].pat[p]=1;proj.arr.song.bar[k].mute=(uint8_t)((k%8)<<5|(k%32));proj.variant.arrangement.degree[k]=(uint8_t)(k%8);}
        settle();before=proj;CHECK(ui_duplicate_block(pos)==2);CHECK(proj.arr.song.len==192);
        for(int k=0;k<=pos;k++){CHECK(!memcmp(&proj.arr.song.bar[k],&before.arr.song.bar[k],sizeof(song_bar_t)));CHECK(proj.variant.arrangement.degree[k]==before.variant.arrangement.degree[k]);}
        for(int k=pos+2;k<192;k++){CHECK(!memcmp(&proj.arr.song.bar[k],&before.arr.song.bar[k-1],sizeof(song_bar_t)));CHECK(proj.variant.arrangement.degree[k]==before.variant.arrangement.degree[k-1]);}
    }
    fresh();song_setup();proj.arr.song.bar[130].pat[0]=11;proj.variant.arrangement.degree[130]=6;settle();CHECK(ui_duplicate_block(0)>=0);CHECK(proj.arr.song.bar[131].pat[0]==11&&proj.variant.arrangement.degree[131]==6);
}
static void rejections(void){
    for(int mode=0;mode<12;mode++){
        fresh();song_setup();int expected=VR_BUSY;
        if(mode==0){proj.arr.song.len=192;expected=VR_FULL;}
        if(mode==1){proj.arr.song.bar[191].mute=32;expected=VR_FULL;}
        if(mode==2){proj.variant.arrangement.degree[191]=1;expected=VR_FULL;}
        if(mode==3){for(int i=0;i<32;i++)proj.pat[i].swing=1;expected=VR_FULL;}
        if(mode==4){for(int i=0;i<NLANE;i++){proj.arr.lane[i].used=1;proj.arr.lane[i].pat=1;proj.arr.lane[i].part=0;}expected=VR_FULL;}
        if(mode==5)seq.playing=1;
        if(mode==6)seq.req_start=1;
        if(mode==7)seq.req_cont=1;
        if(mode==8)external_edit=1;
        if(mode==9)ui.rec=1;
        if(mode==10)fail_dma_sync=1;
        if(mode==11){for(int i=4;i<32;i++)proj.pat[i].bass[0].gen.density++;expected=VR_FULL;}
        undo_commit(&undo);remember();CHECK(ui_duplicate_block(0)==expected);unchanged(mode!=10);
    }
    fresh();song_setup();start_before_reserve=1;remember();CHECK(ui_duplicate_block(0)==VR_BUSY);unchanged(0);CHECK(seq.req_start);
    fresh();song_setup();seq.song_start=2;start_during_stage=1;CHECK(ui_duplicate_block(0)>=0);CHECK(seq.req_start&&!seq.playing);render();CHECK(seq.playing&&seq.song_pos==3);
}
static seq_t play;
static const seq_sink_t silent_sink={0};
static void play_advance(unsigned frames){while(frames){unsigned n=seq_until_event_arrangement(&play,frames);seq_advance_arrangement(&play,n,&silent_sink);frames-=n;}}
static void arrangement_playback(void){
    for(int ext=0;ext<2;ext++){
        fresh();song_setup();proj.arr.song.len=2;proj.pat[1].drum[0].len=5;settle();
        int bank=ui_duplicate_block(0);CHECK(bank>=0);render();ui.song_sel=1;pref_set(PR(R_SREP,0,0,0),3);render();
        seq_init(&play,proj.pat);play.song=&proj.arr.song;play.bpm=120;play.send_clock=0;seq_arrangement_apply(&play,&proj.variant.arrangement);seq_start(&play);
        if(ext)seq_ext_clock(&play);
        play_advance(1);CHECK(play.song_pos==0&&play.ppat[0]==1);
        for(int bar=1;bar<=6;bar++){
            if(ext)for(int tick=0;tick<96;tick++){seq_ext_clock(&play);play_advance(919);}
            else play_advance(88200);
            int expected=bar==6?0:bar>=5?2:1;
            CHECK(play.song_pos==expected);
            CHECK(play.ppat[0]==(expected==1?bank:expected==2?2:1));
        }
    }
}
static void history_and_protection(void){
    fresh();song_setup();proj.variant.arrangement.flags&=~ARR_ENABLED;engine_arrangement_refresh(&proj.variant.arrangement);settle();
    seq.song_start=2;ui.song_sel=2;before=proj;
    CHECK(ui_duplicate_block(2)>=0);render();CHECK(ui.song_sel==3);seq.song_start=3;
    seq.playing=1;remember();CHECK(ui_external_undo(0)==VR_BUSY);unchanged(1);seq.playing=0;
    CHECK(ui_external_undo(0)==VR_OK);render();CHECK(ui.song_sel==2&&seq.song_start==2&&seq.arrangement.editing==2);
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(ui.song_sel==3&&seq.song_start==2);
    fresh();song_setup();rejected_save=1;remember();CHECK(ui_duplicate_block(0)==VR_PROTECTED);unchanged(1);
    fresh();song_setup();
    for(int row=4;row<32;row++)proj.arr.song.bar[row].pat[0]=(uint8_t)row;
    settle();remember();CHECK(ui_duplicate_block(0)==VR_FULL);unchanged(1);
    fresh();song_setup();ui.song_sel=0;pref_set(PR(R_SBAR,0,0,0),2);CHECK(seq.arrangement.editing==2);
}
static void controls(void){
    fresh();song_setup();ui.song_sel=0;ui.view=V_SONG;build_pages();CHECK(pg.n==3);open_list_of_pages();int found=0;
    for(int i=0;i<ui.list_n;i++)if(P_KIND(list_rows[i])==R_ACT&&P_A(list_rows[i])==ACT_BLOCK_DUP){ui.list_sel=(uint8_t)i;found=1;break;}
    CHECK(found);list_enter();CHECK(ui.overlay==O_ASK);before=proj;button_tap(B_HOME);CHECK(!memcmp(&proj,&before,sizeof proj));
    list_enter();button_tap(B_SEL);render();CHECK(proj.arr.song.len==4&&ui.song_sel==1);
    ui.overlay=O_NONE;pref_set(PR(R_SLEN,0,0,0),3);pref_set(PR(R_BMODE,0,0,0),1);render();CHECK(proj.variant.arrangement.flags&ARR_ENABLED);
    pref_set(PR(R_SPAT,0,0,0),7);pref_set(PR(R_BDEGREE,0,0,0),4);render();CHECK(proj.arr.song.bar[ui.song_sel].pat[0]==7&&proj.variant.arrangement.degree[ui.song_sel]==4);
    ui.song_sel=2;bridge_native_action(ACT_BLOCK_QUEUE);render();CHECK(seq.song_start==2);
    char num[24],unit[12];pref_value(PR(R_SLEN,0,0,0),num,unit);CHECK(!strcmp(unit,"blk"));CHECK(!strcmp(pref_desc(PR(R_SBAR,0,0,0))->name,"BLOCK"));
    ui.song_sel=0;pref_set(PR(R_SREP,0,0,0),-1);render();CHECK(SONG_REP(&song()->bar[0])==1);
    pref_set(PR(R_SPAT,0,0,0),99);render();CHECK(song()->bar[0].pat[0]==31);
    CHECK(bridge_native_block(R_SPAT,2,8)==VR_INVALID);
    ui.view=V_HOME;ui_frame();CHECK(seq.song_on); /* home audition retains arrangement identity */
    seq.song_start=0;seq_start(&seq);render();CHECK(seq.playing&&seq.song_pos==0);
    seq.song_on=0;ui_frame();CHECK(seq.song_on);
    ui.song_sel=2;bridge_native_action(ACT_BLOCK_QUEUE);render();CHECK(seq.arrangement.queued==2);seq_stop(&seq);render();
    ui.song_sel=(uint8_t)song()->len;before=proj;bridge_native_action(ACT_BLOCK_DUP);CHECK(!memcmp(&before,&proj,sizeof proj)&&strstr(ui.msg,"SELECT AN EXISTING BLOCK"));
}
#ifndef BLOCK_BRIDGE_NO_MAIN
int main(void){
    basic_and_history();independent_save();expanded();boundaries();rejections();arrangement_playback();history_and_protection();controls();
    printf("BLOCK BRIDGE: %d checkpoints PASS (real project/UI/staging/Undo/storage)\n",checks);return 0;
}
#endif
