/* SPDX-License-Identifier: GPL-3.0-only */
/* Reuse the real host app, synthetic storage, staged IRQ probes and oracles. */
#define BLOCK_BRIDGE_NO_MAIN
#include "block_bridge_test.c"

static void deletion_expected(int index)
{
    after=before;
    for(int k=index;k<191;k++){
        after.arr.song.bar[k]=before.arr.song.bar[k+1];
        after.variant.arrangement.degree[k]=before.variant.arrangement.degree[k+1];
    }
    memset(&after.arr.song.bar[191],0,sizeof(song_bar_t));
    after.variant.arrangement.degree[191]=0;
    after.arr.song.len--;
}
static void deletion_preserved(void)
{
    CHECK(!memcmp(proj.pat,before.pat,sizeof proj.pat));
    CHECK(!memcmp(&proj.sound,&before.sound,sizeof proj.sound));
    CHECK(!memcmp(proj.variant.strip,before.variant.strip,sizeof proj.variant.strip));
    CHECK(!memcmp(proj.arr.lane,before.arr.lane,sizeof proj.arr.lane));
}
static void deletion_positions(void)
{
    for(int index=0;index<192;index++){
        fresh();proj.arr.song.len=192;proj.variant.arrangement.flags|=ARR_ENABLED;
        for(int k=0;k<192;k++){
            for(int p=0;p<5;p++)proj.arr.song.bar[k].pat[p]=(uint8_t)((k+p)%32);
            proj.arr.song.bar[k].mute=(uint8_t)((k%8)<<5|k%32);
            proj.variant.arrangement.degree[k]=(uint8_t)(k%8);
        }
        for(int p=0;p<32;p++)proj.pat[p].drum[0].hit[0][0]=1u<<(p%32);
        for(int k=0;k<NLANE;k++){
            lane_t *l=&proj.arr.lane[k];l->used=l->gen=1;
            l->pat=(uint8_t)(k%32);l->part=l->t=(uint8_t)(k/32);l->v=l->i=0;
            for(int v=0;v<64;v++)l->val[v]=(uint8_t)v;
        }
        engine_arrangement_refresh(&proj.variant.arrangement);settle();
        seq.song_start=(uint8_t)index;seq.song_pos=(uint16_t)index;ui.song_sel=(uint8_t)index;
        before=proj;deletion_expected(index);int n=undo.cur;
        CHECK(ui_delete_block(index)==VR_OK);CHECK(!memcmp(&proj,&after,sizeof proj));deletion_preserved();
        CHECK(undo.cur==n+1);CHECK(ui.song_sel==(index<191?index:190));render();
        CHECK(seq.song_start==ui.song_sel&&seq.song_pos==ui.song_sel);
        CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
        CHECK(ui.song_sel==index&&seq.song_start==index&&seq.song_pos==index);
        CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    }
    fresh();song_setup();proj.arr.song.bar[150].pat[0]=17;proj.variant.arrangement.degree[150]=6;
    settle();before=proj;deletion_expected(1);CHECK(ui_delete_block(1)==VR_OK);
    CHECK(!memcmp(&proj,&after,sizeof proj));CHECK(proj.arr.song.bar[149].pat[0]==17);
}
static void deletion_rejections(void)
{
    for(int mode=0;mode<12;mode++){
        fresh();song_setup();int index=1,want=VR_BUSY;
        if(mode==0){index=-1;want=VR_INVALID;}
        if(mode==1){index=3;want=VR_INVALID;}
        if(mode==2)seq.playing=1;
        if(mode==3)seq.req_start=1;
        if(mode==4)seq.req_cont=1;
        if(mode==5)external_edit=1;
        if(mode==6)ui.rec=1;
        if(mode==7)ui.keys=1;
        if(mode==8){rejected_save=1;want=VR_PROTECTED;}
        if(mode==9)fail_dma_sync=1;
        if(mode==10)ui_hold_saves(1);
        if(mode==11)engine_set(T_909,0,0,64);
        remember();CHECK(ui_delete_block(index)==want);unchanged(mode!=9);
        if(mode==10)ui_hold_saves(0);
    }
    fresh();song_setup();
    for(int p=0;p<32;p++){
        for(int kit=0;kit<2;kit++)for(int v=0;v<11;v++)for(int w=0;w<2;w++)proj.pat[p].drum[kit].hit[v][w]=0xffffffffu;
        for(int bass=0;bass<2;bass++)for(int step=0;step<64;step++)proj.pat[p].bass[bass].step[step]=(bstep_t){100,G_NOTE};
    }
    for(int k=0;k<NLANE;k++)memset(proj.arr.lane[k].val,63,64);
    CHECK(ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL);
    remember();CHECK(ui_delete_block(1)==VR_FULL);unchanged(0);
}
static void deletion_transport_empty(void)
{
    fresh();song_setup();seq.song_start=2;start_during_stage=1;
    CHECK(ui_delete_block(0)==VR_OK);CHECK(seq.req_start&&!seq.playing);render();
    CHECK(seq.playing&&seq.song_pos==1);seq_stop(&seq);render();
    fresh();song_setup();proj.arr.song.len=1;settle();before=proj;start_during_stage=1;
    CHECK(ui_delete_block(0)==VR_OK);CHECK(!song()->len&&seq.req_start);render();
    CHECK(!seq.playing&&!seq.req_start&&!seq.req_cont);deletion_preserved();
    seq_start(&seq);seq_continue(&seq);render();CHECK(!seq.playing&&!seq.req_start&&!seq.req_cont);
    ui.view=V_SONG;button_tap(B_PLAY);render();CHECK(!seq.playing&&strstr(ui.msg,"ARRANGEMENT IS EMPTY"));
    CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    seq_start(&seq);render();CHECK(seq.playing);seq_stop(&seq);render();
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!song()->len);
    CHECK(project_save()==0);after=proj;CHECK(project_load()==0);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    seq_start(&seq);render();CHECK(!seq.playing);
}
static void deletion_history(void)
{
    for(int start=0;start<3;start++){
        fresh();song_setup();seq.song_start=(uint8_t)start;seq.song_pos=(uint16_t)start;
        CHECK(ui_delete_block(1)==VR_OK);render();
        CHECK(ui_external_undo(0)==VR_OK);render();CHECK(seq.song_start==start&&seq.song_pos==start);
    }
    fresh();song_setup();proj.variant.arrangement.flags&=~ARR_ENABLED;engine_arrangement_refresh(&proj.variant.arrangement);settle();
    CHECK(ui_delete_block(1)==VR_OK);render();seq.playing=1;remember();
    CHECK(ui_external_undo(0)==VR_BUSY);unchanged(1);seq.playing=0;
    CHECK(ui_external_undo(0)==VR_OK);render();CHECK(song()->len==3);
    fresh();song_setup();proj.arr.song.len=192;
    for(int k=0;k<192;k++){proj.arr.song.bar[k].pat[0]=(uint8_t)(k%32);proj.variant.arrangement.degree[k]=(uint8_t)(k%8);}
    settle();before=proj;
    for(int k=0;k<192;k++){CHECK(ui_delete_block(0)==VR_OK);render();}
    CHECK(!song()->len);int available=undo.cur;CHECK(available>0&&available<=32);
    for(int k=0;k<available;k++){CHECK(ui_external_undo(0)==VR_OK);render();}
    CHECK(song()->len==available);deletion_preserved();
    for(int k=0;k<available;k++)CHECK(!memcmp(&song()->bar[k],&before.arr.song.bar[192-available+k],sizeof(song_bar_t)));
    for(int k=0;k<available;k++){CHECK(ui_external_undo(1)==VR_OK);render();}
    CHECK(!song()->len);deletion_preserved();
}
static void deletion_controls(void)
{
    fresh();song_setup();original=proj;ui.view=V_SONG;ui.song_sel=1;build_pages();open_list_of_pages();int found=0;
    for(int k=0;k<ui.list_n;k++)if(P_KIND(list_rows[k])==R_ACT&&P_A(list_rows[k])==ACT_BLOCK_DEL){ui.list_sel=(uint8_t)k;found=1;break;}
    CHECK(found);list_enter();CHECK(ui.overlay==O_ASK&&ui.ask_act==ACT_BLOCK_DEL);
    remember();button_tap(B_HOME);unchanged(1);
    list_enter();button_tap(B_SEL);render();CHECK(song()->len==2&&ui.song_sel==1);
    CHECK(song()->bar[1].pat[0]==3);
    /* Actual SONG menu dispatch restores exactly the same project as the shortcut. */
    open_list_of_pages();found=0;
    for(int k=0;k<ui.list_n;k++)if(P_KIND(list_rows[k])==R_ACT&&P_A(list_rows[k])==ACT_UNDO){ui.list_sel=(uint8_t)k;found=1;break;}
    CHECK(found);CHECK(!strcmp(ACT_P[ACT_UNDO].name,"UNDO"));
    seq.playing=1;remember();list_enter();unchanged(1);seq.playing=0;
    list_enter();render();CHECK(song()->len==3);CHECK(!memcmp(&proj,&original,sizeof proj));
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(song()->len==2);
    ui.view=V_SONG;open_list_of_pages();
    for(int k=0;k<ui.list_n;k++)if(P_KIND(list_rows[k])==R_ACT&&P_A(list_rows[k])==ACT_UNDO){ui.list_sel=(uint8_t)k;break;}
    rejected_save=1;remember();list_enter();unchanged(1);rejected_save=0;
    list_enter();render();CHECK(song()->len==3);
}
int main(void)
{
    deletion_positions();deletion_rejections();deletion_transport_empty();deletion_history();deletion_controls();
    printf("BLOCK DELETE: %d checkpoints PASS (patterns/sounds/motion intact, Undo/Redo, transport, all192 positions)\n",checks);
    return 0;
}
