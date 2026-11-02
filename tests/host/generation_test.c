/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_QUERY_MAIN parent_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
static void gen_setup(int mode)
{
    save_hold=0;mode_setup(mode);ui.song_sel=0;
    proj.arr.song.len=1;memset(&proj.arr.song.bar[0],0,sizeof(song_bar_t));
    arrangement_settings_t *a=&proj.variant.arrangement;
    a->key=0;a->mode=HARMONY_IONIAN;a->parent=HARMONY_NO_PARENT;a->degree[0]=5;
    settle();
}
static uint16_t oracle_scale(const tb3po_cfg_t *g)
{
    const int pcs[6][8]={{7,0,2,3,5,7,8,10},{7,0,1,3,5,7,8,10},{7,0,2,3,5,7,8,11},{5,0,3,5,7,10,0,0},{7,0,2,3,5,7,9,10},{7,0,2,4,5,7,9,11}};
    uint16_t m=0;for(int i=1;i<=pcs[g->scale][0];i++)m|=(uint16_t)(1u<<((g->root+pcs[g->scale][i])%12));return m;
}
static void pure(void)
{
    bpart_t p,old,repeat;
    pattern_t initialized;pattern_init(&initialized,123,456);p=initialized.bass[0];
    for(int scale=0;scale<6;scale++)for(int root=0;root<12;root++){p.gen.scale=(uint8_t)scale;p.gen.root=(uint8_t)root;CHECK(tb3po_scale_mask(&p.gen)==oracle_scale(&p.gen));}
    p.gen.density=100;p.gen.accent=40;p.gen.slide=25;p.len=64;
    for(int mask=1;mask<4096;mask++){
        p.gen.seed=(uint32_t)mask;p.gen.root=9;p.gen.scale=0;old=p;
        CHECK(tb3po_generate_mask(&p,(uint16_t)mask,0,127));repeat=old;CHECK(tb3po_generate_mask(&repeat,(uint16_t)mask,0,127));CHECK(!memcmp(&p,&repeat,sizeof p));
        CHECK(!memcmp(&p.gen,&old.gen,sizeof p.gen));CHECK(p.len==old.len&&p.dir==old.dir&&p.transpose==old.transpose&&p.rate==old.rate);
        for(int i=0;i<64;i++){CHECK(mask&(1u<<(p.step[i].note%12)));CHECK(bstep_gate(&p.step[i])==G_NOTE);}
    }
    for(int note=0;note<128;note++){
        old=p;CHECK(tb3po_generate_mask(&p,(uint16_t)(1u<<(note%12)),note,note));for(int i=0;i<64;i++)CHECK(p.step[i].note==note);
        old=p;CHECK(!tb3po_generate_mask(&p,(uint16_t)(1u<<((note+1)%12)),note,note));CHECK(!memcmp(&p,&old,sizeof p));
    }
    for(int bad=0;bad<7;bad++){
        old=p;int lo=0,hi=127;uint16_t mask=0xfff;
        if(bad==0)mask=0;if(bad==1)mask=0x8000;if(bad==2)lo=-1;if(bad==3)hi=128;if(bad==4)lo=128;
        if(bad==5)p.len=0;if(bad==6)p.gen.density=101;repeat=p;CHECK(!tb3po_generate_mask(&p,mask,lo,hi));CHECK(!memcmp(&p,&repeat,sizeof p));p=old;
    }
    p.gen.density=0;CHECK(tb3po_generate_mask(&p,0xfff,24,47));for(int i=0;i<64;i++)CHECK(bstep_gate(&p.step[i])==G_REST);
}
static void transaction(int format,int bass,int mode,uint32_t seed)
{
    gen_setup(format);if(format==2&&seed==12345){proj.pat[1].bass[bass].gen.seed=seed;settle();}before=proj;int old_history=undo.cur;
    uint16_t expected=mode==GEN_SCALE?oracle_scale(&proj.pat[1].bass[bass].gen):mode==GEN_KEY?0xab5u:mode==GEN_FREE?0xfffu:(1u<<7|1u<<11|1u<<2);
    if(mode==GEN_BOTH)expected&=oracle_scale(&proj.pat[1].bass[bass].gen);
    CHECK(ui_generate_constrained(1,bass,mode,0,seed)==VR_OK);after=proj;
    project_t *want=&original;*want=before;want->pat[1].bass[bass]=after.pat[1].bass[bass];CHECK(!memcmp(&proj,want,sizeof proj));
    bpart_t *b=&proj.pat[1].bass[bass];CHECK(b->gen.seed==seed);bpart_t expected_part=before.pat[1].bass[bass];memcpy(expected_part.step,b->step,sizeof b->step);expected_part.gen.seed=seed;CHECK(!memcmp(&expected_part,b,sizeof *b));
    int low=12*(b->gen.base_oct+1),high=low+12*b->gen.oct_range-1;
    for(int i=0;i<64;i++){CHECK(b->step[i].note>=low&&b->step[i].note<=high);CHECK(expected&(1u<<(b->step[i].note%12)));if(i>=b->len)CHECK(bstep_gate(&b->step[i])==G_REST);if(b->step[i].flags&BS_SLIDE)CHECK(bstep_gate(&b->step[(i+1)%b->len])!=G_REST);}
    CHECK(undo.cur==old_history+1);CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    if(format==2 && seed==12345){
        char path[120];CHECK(project_save()==0);after=proj;
        snprintf(path,sizeof path,"build/generation/after-%d-%d.json",bass,mode);fixture_store(path);
        CHECK(ui_external_undo(0)==VR_OK);render();CHECK(project_save()==0);before=proj;
        snprintf(path,sizeof path,"build/generation/before-%d-%d.json",bass,mode);fixture_store(path);
        snprintf(path,sizeof path,"build/generation/before-%d-%d.bin",bass,mode);snapshot(path);
        CHECK(ui_generate_constrained(1,bass,mode,0,seed)==VR_OK);render();
        snprintf(path,sizeof path,"build/generation/after-%d-%d.bin",bass,mode);snapshot(path);
        CHECK(ui_external_undo(0)==VR_OK);render();
        snprintf(path,sizeof path,"build/generation/undo-%d-%d.bin",bass,mode);snapshot(path);
        CHECK(!memcmp(&proj,&before,sizeof proj));CHECK(ui_external_undo(1)==VR_OK);render();CHECK(project_save()==0);after=proj;
        CHECK(project_load()==0);CHECK(!memcmp(&proj,&after,sizeof proj));boot();render();CHECK(!memcmp(&proj,&after,sizeof proj));
    }
}
static void refused(void)
{
    for(int flag=0;flag<15;flag++){
        gen_setup(2);int rc=VR_BUSY;
        if(flag==0)seq.playing=1;if(flag==1)seq.req_start=1;if(flag==2)seq.req_stop=1;if(flag==3)seq.req_cont=1;
        if(flag==4)seq.transport_stage=1;if(flag==5)seq.transport_hold=1;if(flag==6)ui.rec=1;if(flag==7)ui.keys=1;
        if(flag==8)external_edit=1;if(flag==9)save_wait=1;if(flag==10)save_hold=1;if(flag==11){rejected_save=1;rc=VR_PROTECTED;}
        if(flag==12){perf.test=0;perf.phase=1;}if(flag==13)fail_dma_sync=1;if(flag==14)start_before_reserve=1;
        remember();CHECK(ui_generate_constrained(1,0,GEN_CHORD,0,12345)==rc);unchanged(flag!=14);
    }
    gen_setup(2);proj.pat[1].bass[0].gen.mutate_bars=4;settle();remember();CHECK(ui_generate_constrained(1,0,GEN_SCALE,0,1)==VR_UNSUPPORTED);unchanged(1);
    gen_setup(2);proj.variant.arrangement.degree[0]=0;settle();remember();CHECK(ui_generate_constrained(1,0,GEN_CHORD,0,1)==VR_MISSING);unchanged(1);
    gen_setup(2);proj.variant.arrangement.key=2;proj.variant.arrangement.degree[0]=1;proj.pat[1].bass[0].gen.root=0;proj.pat[1].bass[0].gen.scale=3;settle();remember();CHECK(ui_generate_constrained(1,0,GEN_BOTH,0,1)==VR_MISSING);unchanged(1);
    gen_setup(2);remember();CHECK(ui_generate_constrained(1,0,GEN_CHORD,1,1)==VR_MISSING);unchanged(1);
    gen_setup(2);for(unsigned i=0;i<sizeof undo_shadow;i++)undo.shadow[i]=(uint8_t)~undo.region[i];remember();CHECK(ui_generate_constrained(1,0,GEN_FREE,0,1)==VR_FULL);unchanged(1);
    int bad[][3]={{-1,0,0},{32,0,0},{0,-1,0},{0,2,0},{0,0,-1},{0,0,5}};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){gen_setup(2);remember();CHECK(ui_generate_constrained(bad[i][0],bad[i][1],bad[i][2],0,1)==VR_INVALID);unchanged(1);}
    gen_setup(2);remember();CHECK(ui_generate_constrained(1,0,GEN_FREE,0,0)==VR_INVALID);unchanged(1);
}
static void gen_menu(void)
{
    gen_setup(2);ui.part=PART_303A;seq.ppat[PART_303A]=1;set_view(V_GEN);*page_of(V_GEN)=2;build_pages();CHECK(pg.n==6);
    pref_set(PR(R_NEWGEN,0,0,0),GEN_CHORD);CHECK(ui.gen_mode[0]==GEN_CHORD);CHECK(!ui.gen_mode[1]);
    before=proj;generate_native_action(0,0);CHECK(ui.overlay==O_ASK);CHECK(strstr(ui.ask_q[0],"303A P2"));CHECK(!strcmp(ui.ask_q[1],"BLOCK CHORD B1"));
    button_tap(B_HOME);CHECK(!memcmp(&proj,&before,sizeof proj));
    generate_native_action(0,0);proj.pat[2].swing^=1;after=proj;button_tap(B_SEL);CHECK(!memcmp(&proj,&after,sizeof proj));CHECK(strstr(ui.msg,"CHANGED"));
    gen_setup(2);ui.part=PART_303A;seq.ppat[PART_303A]=1;set_view(V_GEN);*page_of(V_GEN)=2;build_pages();pref_set(PR(R_NEWGEN,0,0,0),GEN_CHORD);
    button_tap(B_OCTDN);CHECK(ui.overlay==O_ASK);button_tap(B_SEL);CHECK(strstr(ui.msg,"GENERATED"));
    for(int i=0;i<64;i++)CHECK((1u<<7|1u<<11|1u<<2)&(1u<<(proj.pat[1].bass[0].step[i].note%12)));
    after=proj;button_tap(B_OCTDN);button_tap(B_SEL);CHECK(!memcmp(&proj,&after,sizeof proj));
    ui.msg_until=0;ui_frame();png_write("build/generation/page.png");button_tap(B_OCTUP);ui_frame();png_write("build/generation/confirmation.png");button_tap(B_HOME);
    int m0=ui.gen_mode[0];ui_init();CHECK(!ui.gen_mode[0]&&!ui.gen_mode[1]);CHECK(m0==GEN_CHORD);
}
static void context(void)
{
    gen_setup(2);unsigned base_masks[]={0xab5u,0x6adu,0x5abu,0xad5u,0x6b5u,0x5adu,0x56bu,0x295u,0x4a9u,0xfffu};
    for(int key=0;key<12;key++)for(int m=0;m<10;m++){
        proj.variant.arrangement.key=(uint8_t)key;proj.variant.arrangement.mode=(uint8_t)m;
        unsigned expected=((base_masks[m]<<key)|(base_masks[m]>>(12-key)))&0xfffu;
        CHECK(generation_mask(&proj,GEN_KEY,0)==expected);
    }
    const int offsets[7][7]={{0,2,4,5,7,9,11},{0,2,3,5,7,9,10},{0,1,3,5,7,8,10},{0,2,4,6,7,9,11},{0,2,4,5,7,9,10},{0,2,3,5,7,8,10},{0,1,3,5,6,8,10}};
    for(int key=0;key<12;key++)for(int m=0;m<7;m++)for(int d=1;d<=7;d++){
        proj.variant.arrangement.key=(uint8_t)key;proj.variant.arrangement.mode=(uint8_t)m;proj.variant.arrangement.degree[0]=(uint8_t)d;
        unsigned expected=0;for(int tone=0;tone<3;tone++)expected|=1u<<((key+offsets[m][(d-1+2*tone)%7])%12);
        CHECK(generation_mask(&proj,GEN_CHORD,0)==expected);
    }
    gen_setup(2);proj.arr.song.len=NSONG;proj.variant.arrangement.degree[NSONG-1]=7;settle();
    ui.part=PART_303B;seq.ppat[PART_303B]=31;ui.song_sel=NSONG-1;ui.gen_mode[1]=GEN_BOTH;before=proj;
    generate_native_action(0,0);CHECK(ui.overlay==O_ASK);CHECK((ui.ask_arg&31)==31);CHECK(((ui.ask_arg>>5)&1)==1);CHECK(((ui.ask_arg>>9)&255)==NSONG-1);
    button_tap(B_SEL);CHECK(strstr(ui.msg,"GENERATED"));CHECK(!memcmp(&proj.pat[1],&before.pat[1],sizeof(pattern_t)));
    CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    generate_native_action(0,0);seq.transport_epoch++;after=proj;button_tap(B_SEL);CHECK(strstr(ui.msg,"CHANGED"));CHECK(!memcmp(&proj,&after,sizeof proj));
    gen_setup(2);engine_set(T_909,0,0,50);remember();CHECK(ui_generate_constrained(1,0,GEN_FREE,0,1)==VR_BUSY);unchanged(1);
    gen_setup(2);start_during_stage=1;CHECK(ui_generate_constrained(1,0,GEN_FREE,0,1)==VR_OK);CHECK(!seq.playing&&seq.req_start);render();CHECK(seq.playing);seq_stop(&seq);render();
}
#ifndef GENERATION_NO_MAIN
int main(void)
{
    pure();context();for(int f=0;f<3;f++)for(int b=0;b<2;b++)for(int m=0;m<GEN_MODES;m++)for(int seed=12345;seed<12357;seed++)transaction(f,b,m,(uint32_t)seed);
    refused();gen_menu();printf("CONSTRAINED GENERATION PASS: %d checkpoints; all4095 pitch sets,128 single-note ranges,360 format/part/mode/seed transactions, Undo/Redo/storage/query snapshots/refusals/menu.\n",checks);return 0;
}

#endif
