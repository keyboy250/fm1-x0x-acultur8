/* SPDX-License-Identifier: GPL-3.0-only */
#define BLOCK_BRIDGE_NO_MAIN
#include "block_bridge_test.c"

/* Independent explicit expected menu mapping (not the implementation helpers). */
static const int expected_part[25]={-1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,3};
static const int expected_voice[25]={-1,0,1,2,3,4,5,6,7,8,9,10,0,1,2,3,4,5,6,7,8,9,10,0,1};
static int expected_lane(int scope){int p=expected_part[scope];return p<2?p*11+expected_voice[scope]:20+p;}
static int expected_motion(const lane_t *l,int pat,int scope){int p=expected_part[scope],v=expected_voice[scope];return l->used&&l->pat==pat&&(!scope||(l->part==p&&(p>=2||(l->t==p&&l->v==v)||(l->t==T_PROB&&l->v==(p<<4|v)))));}

static void mode_setup(int mode)
{
    fresh();
    if(mode){CHECK(engine_lane_mode(1));render();}
    if(mode==2){CHECK(engine_rate_set(1,0,0,1));render();}
    for(int pat=1;pat<=2;pat++){
        pattern_t *p=&proj.pat[pat];
        p->swing=(uint8_t)(pat*15);
        for(int kit=0;kit<2;kit++){
            for(int w=0;w<2;w++)p->drum[kit].accent[w]=pat==1?0xaaaaaaaau:0x55555555u;
            for(int v=0;v<11;v++)for(int w=0;w<2;w++)p->drum[kit].hit[v][w]=(uint32_t)(pat*12345+kit*77+v*31+w);
        }
        for(int b=0;b<2;b++){
            p->bass[b].len=(uint8_t)(pat==1?64-b:23+b);
            p->bass[b].rate=(uint8_t)(pat+b);
            p->bass[b].dir=(uint8_t)(pat+b);
            p->bass[b].transpose=(uint8_t)(24+pat+b);
            p->bass[b].gen.seed=(uint32_t)(pat*999+b);
            for(int st=0;st<64;st++)p->bass[b].step[st]=(bstep_t){(uint8_t)(30+(pat*7+b*13+st)%60),(uint8_t)((st+pat)%3|((st%2)?BS_SLIDE:BS_ACCENT))};
        }
        if(mode)for(int lane=0;lane<25;lane++)poly_set(proj.pat,(uint8_t *)&proj.sound,pat,lane,pat==1?64-lane:16+lane,(lane+pat)%(mode==2?21:8));
        else for(int kit=0;kit<2;kit++){p->drum[kit].len=(uint8_t)(16*pat);p->drum[kit].rate=(uint8_t)pat;}
        if(mode)for(int st=0;st<64;st++)poly_break_set(proj.pat,(uint8_t *)&proj.sound,pat,st,(st+pat)%2,(st+pat)%9);
    }
    /* Both selected and nonselected voice automation plus shared kit/FX. */
    for(int pat=1;pat<=2;pat++)for(int kit=0;kit<2;kit++)for(int v=0;v<11;v++){
        int n=motion_alloc(proj.arr.lane,pat,kit,kit,v,0);CHECK(n>=0);proj.arr.lane[n].val[40]=(uint8_t)(pat*10+v);
        n=motion_alloc(proj.arr.lane,pat,kit,T_PROB,kit<<4|v,0);CHECK(n>=0);proj.arr.lane[n].val[40]=(uint8_t)(pat*20+v);
    }
    for(int pat=1;pat<=2;pat++)for(int b=0;b<2;b++){
        int n=motion_alloc(proj.arr.lane,pat,2+b,T_303,b,0);CHECK(n>=0);proj.arr.lane[n].val[63]=(uint8_t)(pat*15+b);
        n=motion_alloc(proj.arr.lane,pat,2+b,T_PROB,(2+b)<<4,0);CHECK(n>=0);proj.arr.lane[n].val[63]=(uint8_t)(pat*25+b);
        n=motion_alloc(proj.arr.lane,pat,2+b,T_FX,0,b);CHECK(n>=0);proj.arr.lane[n].val[63]=(uint8_t)(pat*12+b);
    }
    int n=motion_alloc(proj.arr.lane,2,0,T_909,DR_KIT,0);CHECK(n>=0);proj.arr.lane[n].val[0]=55;
    n=motion_alloc(proj.arr.lane,2,0,T_FX,0,0);CHECK(n>=0);proj.arr.lane[n].val[0]=44;
    /* Different active selection must not change the explicit P2 source. */
    for(int p=0;p<5;p++)seq.ppat[p]=3;
    settle();
}
static void scoped(int mode,int scope)
{
    mode_setup(mode);before=proj;int history=undo.cur;
    CHECK(ui_pattern_copy(1,2,scope)==VR_OK);after=proj;
    CHECK(undo.cur==history+1);
    CHECK(!memcmp(&proj.set,&before.set,sizeof proj.set));
    CHECK(!memcmp(&proj.variant,&before.variant,sizeof proj.variant));
    CHECK(!memcmp(&proj.arr.song,&before.arr.song,sizeof proj.arr.song));
    CHECK(proj.pat[2].swing==before.pat[2].swing);
    for(int kit=0;kit<2;kit++)CHECK(!memcmp(proj.pat[2].drum[kit].accent,before.pat[2].drum[kit].accent,sizeof proj.pat[2].drum[kit].accent));
    for(int pat=0;pat<32;pat++)if(pat!=2)CHECK(!memcmp(&proj.pat[pat],&before.pat[pat],sizeof(pattern_t)));
    for(unsigned i=0;i<sizeof(sound_t);i++)if(!mode || !poly_cell_owned(i))CHECK(((uint8_t *)&proj.sound)[i]==((uint8_t *)&before.sound)[i]);
    int kit=expected_part[scope],voice=expected_voice[scope];
    for(int k=0;k<2;k++)for(int v=0;v<11;v++){
        int selected=!scope||(kit==k&&voice==v);
        CHECK(!memcmp(proj.pat[2].drum[k].hit[v],before.pat[selected?1:2].drum[k].hit[v],sizeof proj.pat[2].drum[k].hit[v]));
    }
    for(int l=0;l<25;l++)if(mode){
        int selected=!scope||l==expected_lane(scope);
        CHECK(poly_length(proj.pat,(uint8_t *)&proj.sound,2,l)==poly_length(before.pat,(uint8_t *)&before.sound,selected?1:2,l));
        CHECK(poly_rate(proj.pat,(uint8_t *)&proj.sound,2,l)==poly_rate(before.pat,(uint8_t *)&before.sound,selected?1:2,l));
    }
    if(scope){
        for(int b=0;b<2;b++)CHECK(!memcmp(&proj.pat[2].bass[b],&before.pat[kit==2+b?1:2].bass[b],sizeof(bpart_t)));
        if(mode)for(int st=0;st<64;st++){
            CHECK(poly_break_gate(proj.pat,(uint8_t *)&proj.sound,2,st)==poly_break_gate(before.pat,(uint8_t *)&before.sound,2,st));
            CHECK(poly_break_slice(proj.pat,(uint8_t *)&proj.sound,2,st)==poly_break_slice(before.pat,(uint8_t *)&before.sound,2,st));
        }else CHECK(!memcmp(&proj.pat[2].brk,&before.pat[2].brk,sizeof(brkpart_t)));
        for(int k=0;k<NLANE;k++)if(!expected_motion(&before.arr.lane[k],2,scope))CHECK(!memcmp(&proj.arr.lane[k],&before.arr.lane[k],sizeof(lane_t)) || !before.arr.lane[k].used);
        for(int k=0;k<2;k++)if(!mode){CHECK(proj.pat[2].drum[k].len==before.pat[2].drum[k].len);CHECK(proj.pat[2].drum[k].rate==before.pat[2].drum[k].rate);}
    }else{
        CHECK(!memcmp(proj.pat[2].bass,before.pat[1].bass,sizeof proj.pat[2].bass));
        if(mode)for(int st=0;st<64;st++){
            CHECK(poly_break_gate(proj.pat,(uint8_t *)&proj.sound,2,st)==poly_break_gate(before.pat,(uint8_t *)&before.sound,1,st));
            CHECK(poly_break_slice(proj.pat,(uint8_t *)&proj.sound,2,st)==poly_break_slice(before.pat,(uint8_t *)&before.sound,1,st));
        }
    }
    for(int k=0;k<NLANE;k++)if(expected_motion(&before.arr.lane[k],1,scope)){
        lane_t *l=&before.arr.lane[k];int n=motion_find(proj.arr.lane,2,l->part,l->t,l->v,l->i);
        CHECK(n>=0);CHECK(!memcmp(proj.arr.lane[n].val,l->val,64));
    }
    render();CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    CHECK(project_save()==0);after=proj;CHECK(project_load()==0);CHECK(!memcmp(&proj,&after,sizeof proj));
    boot();render();CHECK(!memcmp(&proj,&after,sizeof proj));
    proj.pat[2].drum[0].hit[0][0]^=1;CHECK(!memcmp(&proj.pat[1],&before.pat[1],sizeof(pattern_t)));
}
static void failed(void)
{
    for(int m=0;m<13;m++){
        mode_setup(2);int expected=VR_BUSY,src=1,dst=2,scope=1;
        if(m==0)seq.playing=1;
        if(m==1)seq.req_start=1;
        if(m==2)seq.req_stop=1;
        if(m==3)seq.req_cont=1;
        if(m==4)ui.rec=1;
        if(m==5)ui.keys=1;
        if(m==6)external_edit=1;
        if(m==7)save_wait=1;
        if(m==8){rejected_save=1;expected=VR_PROTECTED;}
        if(m==9)fail_dma_sync=1;
        if(m==10){src=-1;expected=VR_INVALID;}
        if(m==11){scope=25;expected=VR_INVALID;}
        if(m==12){dst=32;expected=VR_INVALID;}
        remember();CHECK(ui_pattern_copy(src,dst,scope)==expected);unchanged(m!=9);
    }
    mode_setup(0);remember();CHECK(ui_pattern_copy(1,1,0)==VR_OK);unchanged(1);
    mode_setup(2);start_before_reserve=1;remember();CHECK(ui_pattern_copy(1,2,1)==VR_BUSY);unchanged(0);
    mode_setup(2);start_during_stage=1;CHECK(ui_pattern_copy(1,2,1)==VR_OK);CHECK(!seq.playing&&seq.req_start);render();CHECK(seq.playing);seq_stop(&seq);render();
    /* Entire pool occupied: fail atomically if scope frees insufficient slots. */
    mode_setup(2);
    for(int i=0;i<NLANE;i++){lane_t *l=&proj.arr.lane[i];memset(l,0,sizeof *l);l->used=1;l->pat=3;l->part=0;l->t=T_909;l->v=2;}
    proj.arr.lane[0].pat=1;proj.arr.lane[0].v=0;settle();remember();
    CHECK(ui_pattern_copy(1,2,1)==VR_FULL);unchanged(1);
    /* Reclaiming one matching destination lane makes the same full pool fit. */
    proj.arr.lane[1].pat=2;proj.arr.lane[1].v=0;settle();CHECK(ui_pattern_copy(1,2,1)==VR_OK);
}
static void history_capacity(void)
{
    mode_setup(0);CHECK(ui_pattern_copy(1,2,1)==VR_OK);render();
    seq.playing=1;remember();CHECK(ui_external_undo(0)==VR_BUSY);unchanged(1);seq.playing=0;
    /* A pending gesture that cannot fit must not be silently discarded. */
    mode_setup(2);for(unsigned i=0;i<sizeof undo_shadow;i++)undo.shadow[i]=(uint8_t)~undo.region[i];remember();
    CHECK(ui_pattern_copy(1,2,0)==VR_FULL);unchanged(1);
    /* All 160 source motion lanes need more than one history entry can store. */
    mode_setup(2);
    for(int k=0;k<NLANE;k++){
        lane_t *l=&proj.arr.lane[k];memset(l,0,sizeof *l);l->used=1;l->pat=1;
        l->part=(uint8_t)(k/40);l->t=(uint8_t)(k/40);l->v=(uint8_t)((k/8)%5);l->i=(uint8_t)(k%8);
        for(int st=0;st<64;st++)l->val[st]=(uint8_t)((st+k)%128);
    }
    settle();remember();CHECK(ui_pattern_copy(1,2,0)==VR_FULL);unchanged(1);
}
static void menu(void)
{
    mode_setup(2);ui.view=V_GLO;open_global();int found=0;
    CHECK(P_KIND(list_rows[0])==R_ACT&&P_A(list_rows[0])==ACT_COPY_SEQUENCES);
    CHECK(P_KIND(list_rows[2])==R_CCMODE&&P_KIND(list_rows[3])==R_CLKOUT);
    int count=0;for(int k=0;k<ui.list_n;k++){if(!list_skip(k))count++;CHECK(P_KIND(list_rows[k])!=R_COPYCFG);}CHECK(count==22);
    before=proj;list_enter();CHECK(copy_sequences_open());CHECK(ui.list_sel==1&&ui.list_n==6);
    button_tap(B_HOME);CHECK(!copy_sequences_open()&&ui.view==V_GLO&&ui.overlay==O_LIST&&ui.list_sel==0);CHECK(!memcmp(&before,&proj,sizeof proj));
    list_enter();CHECK(copy_sequences_open());
    for(int k=0;k<ui.list_n;k++)if(P_KIND(list_rows[k])==R_ACT&&P_A(list_rows[k])==ACT_SCOPED_PASTE){ui.list_sel=(uint8_t)k;found=1;break;}
    CHECK(found);
    pref_set(PR(R_COPYCFG,0,0,0),1);pref_set(PR(R_COPYCFG,1,0,0),2);pref_set(PR(R_COPYCFG,2,0,0),1);
    char n[24],u[12];pref_value(PR(R_COPYCFG,0,0,0),n,u);CHECK(!strcmp(n,"P2"));
    list_enter();CHECK(ui.overlay==O_ASK);CHECK(strstr(ui.ask_q[0],"KEEP ACC/SW"));remember();button_tap(B_HOME);unchanged(1);
    open_global();list_enter();for(int k=0;k<ui.list_n;k++)if(P_KIND(list_rows[k])==R_ACT&&P_A(list_rows[k])==ACT_SCOPED_PASTE)ui.list_sel=(uint8_t)k;
    list_enter();proj.pat[1].drum[0].hit[0][0]^=1;remember();button_tap(B_SEL);unchanged(1);CHECK(strstr(ui.msg,"CHANGED"));
    copy_native_action(0,0);button_tap(B_SEL);CHECK(strstr(ui.msg,"PASTED"));
    CHECK(proj.pat[2].drum[0].hit[0][0]==proj.pat[1].drum[0].hit[0][0]);
}
static void fixture_store(const char *path)
{
    FILE *f=fopen(path,"wb");CHECK(f!=0);fputs("{",f);int first=1;
    for(int i=0;i<OBJ_NOBJ;i++)if(store_len[i]>=0){fprintf(f,"%s\"%d\":[",first?"":",",i);first=0;for(int j=0;j<store_len[i];j++)fprintf(f,"%s%u",j?",":"",store[i][j]);fputc(']',f);}
    fputs("}",f);fclose(f);
}
static void midi_copy(void)
{
    static const unsigned gm[2][11]={{36,38,41,45,50,37,39,42,46,49,51},{36,38,41,45,50,37,39,56,49,46,42}};
    for(int scope=0;scope<25;scope++){
        fresh();
        for(int pat=0;pat<NPAT;pat++)for(int part=0;part<NPARTS;part++)edit_clear_part(&proj,&proj.pat[pat],part);
        int part=scope?expected_part[scope]:0,voice=scope?expected_voice[scope]:0;
        unsigned expected_note;
        if(part<2){
            proj.pat[1].drum[part].hit[voice][0]=1;
            proj.pat[1].drum[part].accent[0]=0;proj.pat[2].drum[part].accent[0]=1;
            expected_note=gm[part][voice];
        }else{
            bpart_t *bass=&proj.pat[1].bass[part-2];bass->transpose=24;bass->dir=DIR_FWD;
            bass->step[0]=(bstep_t){(uint8_t)(60+part),G_NOTE|BS_ACCENT};
            expected_note=(unsigned)(60+part);
        }
        proj.set.midi_ch[part]=7;
        proj.variant.arrangement.flags=(uint8_t)((proj.variant.arrangement.flags&~ARR_ENABLED)|ARR_NOTES_OUT);
        project_apply_io();settle();CHECK(ui_pattern_copy(1,2,scope)==VR_OK);render();
        for(int p=0;p<NPARTS;p++)seq_cue_part(&seq,p,2);
        render();midi_log=tmpfile();CHECK(midi_log!=0);
        seq_start(&seq);for(int i=0;i<90;i++)render();seq_stop(&seq);render();
        rewind(midi_log);char line[128];unsigned when,status,note,velocity;int on=0,off=0;
        while(fgets(line,sizeof line,midi_log))if(sscanf(line,"%u ms: %x %x %x",&when,&status,&note,&velocity)==4){
            if((status&0xf0u)==0x90u && velocity){CHECK(status==0x96);CHECK(note==expected_note);CHECK(velocity==127);on++;}
            if((status&0xf0u)==0x80u)off++;
        }
        fclose(midi_log);midi_log=0;CHECK(on>0);CHECK(off==on);
    }
}
static void upper_bank(void)
{
    for(int scope=0;scope<25;scope++){
        mode_setup(2);int part=scope?expected_part[scope]:0,voice=scope?expected_voice[scope]:0;
        if(part<2)proj.pat[31].drum[part].hit[voice][1]=0x80000000u;
        else proj.pat[31].bass[part-2].step[63]=(bstep_t){70,G_NOTE|BS_SLIDE|BS_ACCENT};
        int lane=scope?expected_lane(scope):0;
        poly_set(proj.pat,(uint8_t *)&proj.sound,31,lane,64,20);
        settle();before=proj;CHECK(ui_pattern_copy(31,0,scope)==VR_OK);
        if(part<2)CHECK(proj.pat[0].drum[part].hit[voice][1]==0x80000000u);
        else CHECK(!memcmp(&proj.pat[0].bass[part-2],&before.pat[31].bass[part-2],sizeof(bpart_t)));
        CHECK(poly_length(proj.pat,(uint8_t *)&proj.sound,0,lane)==64);
        CHECK(poly_rate(proj.pat,(uint8_t *)&proj.sound,0,lane)==20);
        CHECK(!memcmp(&proj.pat[31],&before.pat[31],sizeof(pattern_t)));
        CHECK(proj.pat[0].swing==before.pat[0].swing);
        for(int k=0;k<2;k++)CHECK(!memcmp(proj.pat[0].drum[k].accent,before.pat[0].drum[k].accent,sizeof proj.pat[0].drum[k].accent));
    }
}
static void all_scope_menu_and_failures(void)
{
    CHECK(PCOPY_COUNT==25);CHECK(COPY_P[2].max==24);
    for(int scope=0;scope<25;scope++){
        mode_setup(2);remember();CHECK(ui_pattern_copy(1,1,scope)==VR_OK);unchanged(1);
        seq.playing=1;remember();CHECK(ui_pattern_copy(1,2,scope)==VR_BUSY);unchanged(1);
        seq.playing=0;settle();before=proj;
        if(scope){CHECK(pc_scope_part(scope)==expected_part[scope]);CHECK(pc_scope_voice(scope)==expected_voice[scope]);}
        /* Upper-bank source/destination and every scope must survive packing.
         * Each block refers to P32 in exactly the part under test. */
        proj.arr.song.len=1;
        for(int part=0;part<5;part++)proj.arr.song.bar[0].pat[part]=0;
        proj.arr.song.bar[0].pat[scope?expected_part[scope]:0]=31;
        settle();before=proj;
        copy_config[0]=1;copy_config[1]=31;copy_config[2]=(uint8_t)scope;copy_config[3]=(uint8_t)scope;
        copy_native_action(0,0);CHECK(ui.overlay==O_ASK);
        CHECK(strstr(ui.ask_q[0],"P2 -> P32 KEEP ACC/SW"));
        CHECK(strstr(ui.ask_q[1],COPY_SCOPE_N[scope]));CHECK(strstr(ui.ask_q[1],"1 BLOCKS"));
        CHECK(strlen(ui.ask_q[1])<28);
        button_tap(B_SEL);CHECK(strstr(ui.msg,"PASTED"));after=proj;
        CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
        CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
        /* A full motion pool without matching destination lanes rejects every
         * scope atomically, including the two whole-bass scopes. */
        mode_setup(2);
        int part=scope?expected_part[scope]:0,voice=scope?expected_voice[scope]:0;
        for(int k=0;k<NLANE;k++){lane_t *l=&proj.arr.lane[k];memset(l,0,sizeof *l);l->used=1;l->pat=3;l->part=(uint8_t)part;l->t=(uint8_t)(part<2?part:T_303);l->v=(uint8_t)voice;}
        proj.arr.lane[0].pat=1;settle();remember();CHECK(ui_pattern_copy(1,2,scope)==VR_FULL);unchanged(1);
    }
}
static void fixtures(void)
{
    char path[96];
    for(int scope=0;scope<25;scope++){
        mode_setup(2);CHECK(project_save()==0);
        snprintf(path,sizeof path,"build/copy-review/before-%d.json",scope);fixture_store(path);
        CHECK(ui_pattern_copy(1,2,scope)==VR_OK);CHECK(project_save()==0);
        snprintf(path,sizeof path,"build/copy-review/after-%d.json",scope);fixture_store(path);
    }
    mode_setup(2);ui.view=V_GLO;open_global();list_enter();CHECK(copy_sequences_open());int moves=0;
    for(int k=1;k<ui.list_n;k++)if(!list_skip(k)){
        if(P_KIND(list_rows[k])==R_COPYCFG && P_A(list_rows[k])==0)break;
        moves++;
    }
    FILE *f=fopen("build/copy-review/menu-moves.json","wb");CHECK(f!=0);fprintf(f,"{\"sourceMoves\":%d}",moves);fclose(f);
    pref_set(PR(R_COPYCFG,0,0,0),1);pref_set(PR(R_COPYCFG,1,0,0),2);pref_set(PR(R_COPYCFG,2,0,0),1);
    for(int k=0;k<ui.list_n;k++)if(P_KIND(list_rows[k])==R_ACT&&P_A(list_rows[k])==ACT_SCOPED_PASTE)ui.list_sel=(uint8_t)k;
    ui.msg_until=0;ui_frame();png_write("build/copy-review/menu.png");list_enter();ui_frame();png_write("build/copy-review/confirm.png");
}
#ifndef PATTERN_COPY_NO_MAIN
int main(void)
{
    for(int mode=0;mode<3;mode++)for(int scope=0;scope<25;scope++)scoped(mode,scope);
    failed();history_capacity();menu();midi_copy();upper_bank();all_scope_menu_and_failures();fixtures();
    printf("PATTERN COPY: %d checkpoints PASS (75 mode/scope cases, canonical UI/staging/history/storage)\n",checks);
    return 0;
}

#endif
