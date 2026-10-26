/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_QUERY_MAIN parent_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
static void cross_setup(int mode){save_hold=0;mode_setup(mode);}
static int prob_voice(int scope){int p=expected_part[scope];return p<<4|(p<2?expected_voice[scope]:0);}
static void route(int mode,int from,int to,int same)
{
    cross_setup(mode);int src=1,dst=same?1:2,dp=expected_part[to],sp=expected_part[from],dv=expected_voice[to],sv=expected_voice[from];
    if(from>=23){
        for(int st=0;st<64;st++){
            proj.pat[src].bass[sp-2].step[st].flags=(proj.pat[src].bass[sp-2].step[st].flags&~BS_ACCENT)|((st&1)?BS_ACCENT:0);
            proj.pat[dst].bass[dp-2].step[st].flags=(proj.pat[dst].bass[dp-2].step[st].flags&~BS_ACCENT)|((st&1)?0:BS_ACCENT);
        }
        proj.pat[src].bass[sp-2].gen.accent=13;proj.pat[dst].bass[dp-2].gen.accent=87;
    }
    /* Exercise condition high bits, an unset cell, and the last step. */
    int q=motion_find(proj.arr.lane,src,sp,T_PROB,prob_voice(from),0);CHECK(q>=0);
    proj.arr.lane[q].val[0]=135;proj.arr.lane[q].val[63]=MOT_NONE;settle();before=proj;
    int history=undo.cur;int rc=ui_pattern_copy_to(src,dst,from,to);if(rc!=VR_OK)fprintf(stderr,"route failed mode%d from%d to%d same%d rc%d\n",mode,from,to,same,rc);CHECK(rc==VR_OK);after=proj;CHECK(undo.cur==history+1);
    CHECK(!memcmp(&proj.set,&before.set,sizeof proj.set));CHECK(!memcmp(&proj.variant,&before.variant,sizeof proj.variant));
    CHECK(!memcmp(&proj.arr.song,&before.arr.song,sizeof proj.arr.song));
    for(int pat=0;pat<32;pat++)if(pat!=dst)CHECK(!memcmp(&proj.pat[pat],&before.pat[pat],sizeof(pattern_t)));
    CHECK(proj.pat[dst].swing==before.pat[dst].swing);
    for(int kit=0;kit<2;kit++){
        CHECK(!memcmp(proj.pat[dst].drum[kit].accent,before.pat[dst].drum[kit].accent,sizeof proj.pat[dst].drum[kit].accent));
        for(int v=0;v<11;v++){
            const void *want=kit==dp&&v==dv?before.pat[src].drum[sp].hit[sv]:before.pat[dst].drum[kit].hit[v];
            CHECK(!memcmp(proj.pat[dst].drum[kit].hit[v],want,sizeof proj.pat[dst].drum[kit].hit[v]));
        }
        if(!mode){CHECK(proj.pat[dst].drum[kit].len==before.pat[dst].drum[kit].len);CHECK(proj.pat[dst].drum[kit].rate==before.pat[dst].drum[kit].rate);}
    }
    for(int b=0;b<2;b++){
        bpart_t expected=before.pat[dst].bass[b];
        if(dp==b+2){
            expected=before.pat[src].bass[sp-2];
            for(int st=0;st<64;st++)expected.step[st].flags=(expected.step[st].flags&~BS_ACCENT)|(before.pat[dst].bass[b].step[st].flags&BS_ACCENT);
            expected.gen.accent=before.pat[dst].bass[b].gen.accent;
        }
        CHECK(!memcmp(&proj.pat[dst].bass[b],&expected,sizeof expected));
    }
    if(mode)for(int lane=0;lane<25;lane++){
        int selected=lane==expected_lane(to),sl=selected?expected_lane(from):lane,pat=selected?src:dst;
        CHECK(poly_length(proj.pat,(uint8_t *)&proj.sound,dst,lane)==poly_length(before.pat,(uint8_t *)&before.sound,pat,sl));
        CHECK(poly_rate(proj.pat,(uint8_t *)&proj.sound,dst,lane)==poly_rate(before.pat,(uint8_t *)&before.sound,pat,sl));
    }
    for(unsigned i=0;i<sizeof(sound_t);i++)if(!mode || !poly_cell_owned(i))CHECK(((uint8_t *)&proj.sound)[i]==((uint8_t *)&before.sound)[i]);
    if(mode)for(int st=0;st<64;st++){
        CHECK(poly_break_gate(proj.pat,(uint8_t *)&proj.sound,dst,st)==poly_break_gate(before.pat,(uint8_t *)&before.sound,dst,st));
        CHECK(poly_break_slice(proj.pat,(uint8_t *)&proj.sound,dst,st)==poly_break_slice(before.pat,(uint8_t *)&before.sound,dst,st));
    }else CHECK(!memcmp(&proj.pat[dst].brk,&before.pat[dst].brk,sizeof(brkpart_t)));
    for(int k=0;k<NLANE;k++){
        lane_t *l=&before.arr.lane[k];
        if(l->used && !(l->pat==dst&&l->part==dp&&l->t==T_PROB&&l->v==prob_voice(to)))CHECK(!memcmp(l,&proj.arr.lane[k],sizeof *l));
    }
    int target=motion_find(proj.arr.lane,dst,dp,T_PROB,prob_voice(to),0);CHECK(target>=0);CHECK(!memcmp(proj.arr.lane[target].val,before.arr.lane[q].val,64));
    render();CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    if(mode==2&&!same&&((from==2&&(to==4||to==6))||(from==12&&to==1)||(from==23&&to==24)||(from==24&&to==23))){
        char path[128];CHECK(project_save()==0);after=proj;
        snprintf(path,sizeof path,"build/cross-copy/after-%d-%d.json",from,to);fixture_store(path);
        snapshot("build/cross-copy/query-after.bin");CHECK(!memcmp(&proj,&after,sizeof proj));
        CHECK(ui_external_undo(0)==VR_OK);render();CHECK(project_save()==0);
        /* Save captures legacy default IO settings outside the Undo region. */
        before=proj;
        snprintf(path,sizeof path,"build/cross-copy/before-%d-%d.json",from,to);fixture_store(path);
        snprintf(path,sizeof path,"build/cross-copy/before-%d-%d.bin",from,to);snapshot(path);
        CHECK(ui_pattern_copy_to(src,dst,from,to)==VR_OK);render();
        snprintf(path,sizeof path,"build/cross-copy/after-%d-%d.bin",from,to);snapshot(path);
        CHECK(ui_external_undo(0)==VR_OK);render();
        snprintf(path,sizeof path,"build/cross-copy/undo-%d-%d.bin",from,to);snapshot(path);
        CHECK(!memcmp(&proj,&before,sizeof proj));
        CHECK(ui_external_undo(1)==VR_OK);render();CHECK(project_save()==0);after=proj;
        CHECK(project_load()==0);CHECK(!memcmp(&proj,&after,sizeof proj));boot();render();CHECK(!memcmp(&proj,&after,sizeof proj));
    }
}
static void cross_refusals(void)
{
    int bad[][2]={{0,1},{1,0},{2,23},{24,11},{25,1},{1,25},{-1,3},{23,-1}};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){cross_setup(2);remember();CHECK(ui_pattern_copy_to(1,2,bad[i][0],bad[i][1])==VR_INVALID);unchanged(1);}
    for(int flag=0;flag<13;flag++){
        cross_setup(2);int rc=VR_BUSY;
        if(flag==0)seq.playing=1;if(flag==1)seq.req_start=1;if(flag==2)seq.req_stop=1;if(flag==3)seq.req_cont=1;
        if(flag==4)seq.transport_stage=1;if(flag==5)seq.transport_hold=1;if(flag==6)ui.rec=1;if(flag==7)ui.keys=1;
        if(flag==8)external_edit=1;if(flag==9)save_wait=1;if(flag==10)save_hold=1;
        if(flag==11){rejected_save=1;rc=VR_PROTECTED;}if(flag==12){perf.test=0;perf.phase=1;}
        remember();CHECK(ui_pattern_copy_to(1,2,2,6)==rc);unchanged(1);
    }
    cross_setup(2);for(int k=0;k<NLANE;k++){lane_t*l=&proj.arr.lane[k];memset(l,0,sizeof *l);l->used=1;l->pat=3;l->part=0;l->t=T_909;}
    proj.arr.lane[0].pat=1;proj.arr.lane[0].t=T_PROB;proj.arr.lane[0].v=1;
    settle();remember();CHECK(ui_pattern_copy_to(1,2,2,6)==VR_FULL);unchanged(1);
    cross_setup(2);for(unsigned i=0;i<sizeof undo_shadow;i++)undo.shadow[i]=(uint8_t)~undo.region[i];remember();CHECK(ui_pattern_copy_to(1,2,23,24)==VR_FULL);unchanged(1);
    cross_setup(2);fail_dma_sync=1;remember();CHECK(ui_pattern_copy_to(1,2,2,6)==VR_BUSY);unchanged(1);
    cross_setup(2);start_before_reserve=1;remember();CHECK(ui_pattern_copy_to(1,2,23,24)==VR_BUSY);unchanged(0);
    cross_setup(2);start_during_stage=1;CHECK(ui_pattern_copy_to(1,2,2,6)==VR_OK);CHECK(!seq.playing&&seq.req_start);render();CHECK(seq.playing);seq_stop(&seq);render();
    /* Source without probability clears target probability, retains all tone motion. */
    cross_setup(2);int n=motion_find(proj.arr.lane,1,0,T_PROB,1,0);CHECK(n>=0);motion_clear(proj.arr.lane,n);settle();
    int tone=motion_find(proj.arr.lane,2,1,T_808,0,0);lane_t saved=proj.arr.lane[tone];
    CHECK(ui_pattern_copy_to(1,2,2,12)==VR_OK);CHECK(motion_find(proj.arr.lane,2,1,T_PROB,16,0)<0);CHECK(!memcmp(&saved,&proj.arr.lane[tone],sizeof saved));
}
static void cross_menu(void)
{
    cross_setup(2);ui.view=V_GLO;open_global();list_enter();CHECK(copy_sequences_open()&&ui.list_n==6);
    CHECK(pref_desc(PR(R_COPYCFG,3,0,0))->max==0);
    pref_set(PR(R_COPYCFG,2,0,0),2);CHECK(copy_config[3]==2);CHECK(pref_desc(PR(R_COPYCFG,3,0,0))->max==21);
    pref_set(PR(R_COPYCFG,3,0,0),16);CHECK(copy_config[3]==17);
    pref_set(PR(R_COPYCFG,2,0,0),23);CHECK(copy_config[3]==23);CHECK(pref_desc(PR(R_COPYCFG,3,0,0))->max==1);
    pref_set(PR(R_COPYCFG,3,0,0),1);CHECK(copy_config[3]==24);
    char val[24],unit[12];pref_value(PR(R_COPYCFG,3,0,0),val,unit);CHECK(!strcmp(val,"303 B"));
    /* Twenty-bit confirmation route cannot fit the old uint16 field. */
    pref_set(PR(R_COPYCFG,0,0,0),31);pref_set(PR(R_COPYCFG,1,0,0),0);before=proj;copy_native_action(0,0);
    CHECK(ui.overlay==O_ASK);CHECK((ui.ask_arg>>15)==24);CHECK(strstr(ui.ask_q[1],"303 A > 303 B"));CHECK(strlen(ui.ask_q[1])<28);
    button_tap(B_HOME);CHECK(!memcmp(&proj,&before,sizeof proj));
    copy_native_action(0,0);button_tap(B_SEL);CHECK(strstr(ui.msg,"PASTED"));
    for(int st=0;st<64;st++){
        CHECK(proj.pat[0].bass[1].step[st].note==before.pat[31].bass[0].step[st].note);
        CHECK((proj.pat[0].bass[1].step[st].flags&BS_ACCENT)==(before.pat[0].bass[1].step[st].flags&BS_ACCENT));
    }
    CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    /* Same-pattern copy to a different lane is not a no-op. */
    pref_set(PR(R_COPYCFG,0,0,0),1);pref_set(PR(R_COPYCFG,1,0,0),1);pref_set(PR(R_COPYCFG,2,0,0),2);pref_set(PR(R_COPYCFG,3,0,0),5);
    copy_native_action(0,0);CHECK(ui.overlay==O_ASK);button_tap(B_SEL);CHECK(strstr(ui.msg,"PASTED"));
    CHECK(!memcmp(proj.pat[1].drum[0].hit[5],proj.pat[1].drum[0].hit[1],8));
}
int main(void)
{
    parent_query_main();
    for(int mode=0;mode<3;mode++)for(int same=0;same<2;same++)for(int from=1;from<25;from++)for(int to=1;to<25;to++){
        if(from==to || ((from<23)!=(to<23)))continue;
        route(mode,from,to,same);
    }
    cross_refusals();cross_menu();
    printf("CROSS PART COPY PASS: %d checkpoints; all462 drum routes and2 bass routes x3 formats x2 pattern relations; source/sound/AC/swing isolation, probability conditions, Undo/Redo, storage, command49 and canonical menu.\n",checks);
    return 0;
}
