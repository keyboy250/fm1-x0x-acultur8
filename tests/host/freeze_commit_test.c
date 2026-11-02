/* SPDX-License-Identifier: GPL-3.0-only */
#define VARIATION303_NO_MAIN
#include "variation303_test.c"

static int capture_audit(void *ctx,int b,int note,int accent,int slide)
{
    if(!s_bon_checked(ctx,b,note,accent,slide))return 0;
    CHECK(nevents<2048);events[nevents++]=(event_t){elapsed,seq.t[TRK_BASS0+b].variation_cycle,b,seq.t[TRK_BASS0+b].pos,note,accent,slide};
    return 1;
}
static void capture_samples(unsigned n)
{
    seq_sink_t sink=SINK;sink.bass_trigger=capture_audit;sink.midi=audit_midi;sink.midi_try=0;sink.midi_pair_try=0;sink.midi_batch_try=0;
    int feature=seq.lane_sound || (seq.arrangement.flags&ARR_ENABLED);
    if(feature)seq_advance_arrangement(&seq,0,&sink);else seq_advance(&seq,0,&sink);
    while(n){unsigned k=feature?seq_until_event_arrangement(&seq,n):seq_until_event(&seq,n);elapsed+=k;n-=k;
        if(feature)seq_advance_arrangement(&seq,k,&sink);else seq_advance(&seq,k,&sink);}
}
#define run_samples capture_samples

static void capture_bass(int format,int b,int mode)
{
    setup(format,b,mode,0,DIR_FWD,1);seq.variation.anchor=0;arm();
    /* Lane chance forces a known rejected NOTE and TIE, captured as rests. */
    int l=motion_alloc(proj.arr.lane,0,2+b,T_PROB,(2+b)<<4,0);CHECK(l>=0);
    for(int i=0;i<NSTEPS;i++)proj.arr.lane[l].val[i]=i==1?0:100;
    settle();before=proj;int h=undo.cur;unsigned writes=store_writes;uint32_t fp=project_fingerprint();
    seq_start(&seq);run_samples(90000);CHECK(seq.playing);
    CHECK(ui_freeze_request()==VR_OK);CHECK(seq.freeze->state==FREEZE_REQUEST);
    run_samples(90000);CHECK(seq.freeze->state==FREEZE_HELD);
    pattern_freeze_t frozen=*seq.freeze;CHECK(frozen.count==16);CHECK(frozen.bass.transpose==24);
    CHECK(bstep_gate(&frozen.bass.step[1])==G_REST);CHECK(bstep_gate(&frozen.bass.step[2])==G_REST);
    nevents=0;run_samples(88200*3);
    for(unsigned i=0;i<nevents;i++)if(events[i].bass==b){
        int p=events[i].step;CHECK(bstep_gate(&frozen.bass.step[p])!=G_REST);
        if(bstep_gate(&frozen.bass.step[p])==G_NOTE)CHECK(events[i].note==frozen.bass.step[p].note);
        CHECK(p!=1 && p!=2);
    }
    CHECK(!memcmp(&frozen,seq.freeze,sizeof frozen));CHECK(!memcmp(&before,&proj,sizeof proj));
    CHECK(h==undo.cur && writes==store_writes && fp==project_fingerprint());
    CHECK(ui_freeze_commit(6)==VR_BUSY);
    seq_stop(&seq);run_samples(1);CHECK(ons==offs);
    /* Preserve destination accents, timbre/generator controls, other parts and tail. */
    bpart_t *dst=&proj.pat[6].bass[b];dst->gen.accent=83;dst->gen.mutate_bars=4;
    for(int i=0;i<NSTEPS;i++)dst->step[i]=(bstep_t){40,(uint8_t)(G_NOTE|((i&1)?BS_ACCENT:0))};
    proj.pat[6].swing=70;sm_set(proj.pat[6].drum[0].accent,2);sm_set(proj.pat[6].drum[1].accent,5);
    int dl=motion_alloc(proj.arr.lane,6,2+b,T_PROB,(2+b)<<4,0);CHECK(dl>=0);
    for(int i=0;i<NSTEPS;i++)proj.arr.lane[dl].val[i]=0;
    settle();before=proj;
    CHECK(ui_freeze_commit(0)==VR_INVALID);CHECK(!memcmp(&before,&proj,sizeof proj));
    prob[2+b][0]=0;CHECK(ui_freeze_commit(6)==VR_OK);CHECK(prob[2+b][0]==100);after=proj;
    project_t expected=before;bpart_t *e=&expected.pat[6].bass[b];
    for(int i=0;i<16;i++){
        e->step[i].note=frozen.bass.step[i].note;
        e->step[i].flags=(frozen.bass.step[i].flags&~BS_ACCENT)|(e->step[i].flags&BS_ACCENT);
    }
    e->len=16;e->rate=frozen.bass.rate;e->dir=DIR_FWD;e->transpose=24;e->gen.mutate_bars=0;
    if(format)poly_set(expected.pat,(uint8_t *)&expected.sound,6,22+b,16,frozen.bass.rate);
    motion_clear(expected.arr.lane,dl);CHECK(!memcmp(&expected,&proj,sizeof proj));
    CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    /* Audition the stored destination normally, without the frozen overlay.
     * Its removed chance lane must not leave a cached rejection behind. */
    pattern_variation_t off=seq.variation;off.lane=255;CHECK(ui_variation_config(&off)==VR_OK);
    seq_cue_part(&seq,2+b,6);render();nevents=0;seq_start(&seq);run_samples(88200*2-1);seq_stop(&seq);run_samples(1);
    unsigned committed_notes=0;
    for(unsigned i=0;i<nevents;i++)if(events[i].bass==b){
        int st=events[i].step;CHECK(bstep_gate(&frozen.bass.step[st])==G_NOTE);
        CHECK(events[i].note==frozen.bass.step[st].note);committed_notes++;
    }
    CHECK(committed_notes>4);CHECK(!memcmp(&proj,&after,sizeof proj));CHECK(ons==offs);
    CHECK(project_save()==0);CHECK(!memcmp(&proj.pat,&after.pat,sizeof proj.pat));after=proj;boot();render();CHECK(!memcmp(&proj,&after,sizeof proj));
    CHECK(seq.variation.lane==255 && seq.freeze->state==FREEZE_RECORD);
}
static void capture_drums(void)
{
    for(int format=0;format<3;format++)for(int lane=0;lane<22;lane++){
        setup(format,0,GEN_SCALE,0,DIR_FWD,0);
        seq.variation=(pattern_variation_t){(uint8_t)lane,0,0,8,37,0,0,1,0,0,0};
        int k=lane/11,v=lane%11;
        int l=motion_alloc(proj.arr.lane,0,k,T_PROB,(k<<4)|v,0);CHECK(l>=0);
        for(int i=0;i<NSTEPS;i++)proj.arr.lane[l].val[i]=(i&1)?0:100;
        before=proj;seq_start(&seq);run_samples(90000);CHECK(ui_freeze_request()==VR_OK);
        run_samples(90000);CHECK(seq.freeze->state==FREEZE_HELD);
        pattern_freeze_t f=*seq.freeze;for(int i=1;i<16;i+=2)CHECK(bstep_gate(&f.bass.step[i])==G_REST);
        run_samples(20000);seq_stop(&seq);run_samples(1);CHECK(!memcmp(&proj,&before,sizeof proj));
        CHECK(ui_freeze_commit(31)==VR_OK);
        for(int i=0;i<16;i++)CHECK(sm_get(proj.pat[31].drum[k].hit[v],i)==(bstep_gate(&f.bass.step[i])==G_NOTE));
        CHECK(!memcmp(&proj.pat[0],&before.pat[0],sizeof(pattern_t)));
    }
}
static void freeze_boundaries(void)
{
    setup(2,0,GEN_SCALE,0,DIR_FWD,0);CHECK(ui_freeze_request()==VR_INVALID);arm();
    CHECK(ui_freeze_request()==VR_BUSY);CHECK(ui_freeze_commit(6)==VR_INVALID);
    seq_start(&seq);run_samples(1000);seq_stop(&seq);run_samples(1);CHECK(ui_freeze_request()==VR_BUSY);
    for(int d=1;d<NDIRS;d++){proj.pat[0].bass[0].dir=(uint8_t)d;CHECK(ui_freeze_request()==VR_UNSUPPORTED);}
    proj.pat[0].bass[0].dir=DIR_FWD;
    proj.pat[0].bass[0].step[0].flags=G_TIE;CHECK(ui_freeze_request()==VR_UNSUPPORTED);
    proj.pat[0].bass[0].step[0].flags=G_NOTE;
    /* Freeze one complete stopped pass; cancel and stale confirmation never write. */
    seq_start(&seq);run_samples(85000);seq_stop(&seq);run_samples(1);CHECK(seq.freeze->count==16);
    CHECK(ui_freeze_request()==VR_OK);CHECK(seq.freeze->state==FREEZE_HELD);
    seq.freeze->destination=6;*page_of(V_GEN)=5;build_pages();CHECK(capture_page());
    before=proj;button_tap(B_OCTDN);CHECK(ui.overlay==O_ASK);button_tap(B_HOME);CHECK(!memcmp(&proj,&before,sizeof proj));
    button_tap(B_OCTDN);proj.pat[1].swing++;button_tap(B_SEL);CHECK(strstr(ui.msg,"CHANGED"));
    before=proj;save_hold=1;CHECK(ui_freeze_commit(6)==VR_BUSY);save_hold=0;CHECK(!memcmp(&proj,&before,sizeof proj));
    /* Owner stays A/P1 while inspection changes part or pattern. */
    ui.part=PART_303B;CHECK(seq.variation.lane==22 && seq.freeze->state==FREEZE_HELD);
    seq_cue_part(&seq,PART_303A,1);render();CHECK(!seq_freeze_held(&seq,22));
    seq_cue_part(&seq,PART_303A,0);render();CHECK(seq_freeze_held(&seq,22));
    for(int bad=0;bad<12;bad++){
        int expected=VR_BUSY,dst=6;
        if(bad==0)seq.playing=1;
        if(bad==1)seq.req_start=1;
        if(bad==2)seq.req_stop=1;
        if(bad==3)seq.req_cont=1;
        if(bad==4)ui.rec=1;
        if(bad==5)ui.keys=1;
        if(bad==6)external_edit=1;
        if(bad==7)save_wait=1;
        if(bad==8){rejected_save=1;expected=VR_PROTECTED;}
        if(bad==9){dst=-1;expected=VR_INVALID;}
        if(bad==10){dst=32;expected=VR_INVALID;}
        if(bad==11)fail_dma_sync=1;
        remember();CHECK(ui_freeze_commit(dst)==expected);unchanged(bad!=11);
        seq.playing=seq.req_start=seq.req_stop=seq.req_cont=ui.rec=ui.keys=external_edit=save_wait=rejected_save=fail_dma_sync=0;
    }
    test_dma_timeouts=0;
    for(unsigned i=0;i<sizeof undo_shadow;i++)undo.shadow[i]=(uint8_t)~undo.region[i];
    remember();CHECK(ui_freeze_commit(6)==VR_FULL);unchanged(1);undo_reset(&undo);
    start_before_reserve=1;remember();CHECK(ui_freeze_commit(6)==VR_BUSY);unchanged(0);seq.req_start=0;
    start_during_stage=1;CHECK(ui_freeze_commit(6)==VR_OK);CHECK(!seq.playing && seq.req_start);
    render();CHECK(seq.playing);seq_stop(&seq);render();start_during_stage=0;
    CHECK(project_save()==0);render();int loaded=project_load();
    if(loaded)fprintf(stderr,"freeze load rc%d queue%u/%u hold%d stage%d play%d epoch%u\n",loaded,engine_queue_read(),engine_queue_written(),seq.transport_hold,seq.transport_stage,seq.playing,seq.transport_epoch);
    CHECK(loaded==0);CHECK(seq.freeze->state==FREEZE_RECORD && !seq.freeze->count);
}
static void short_long_external(void)
{
    const int lengths[]={1,4,7,32,64};
    for(int b=0;b<2;b++)for(unsigned ix=0;ix<5;ix++)for(int ext=0;ext<2;ext++){
        setup(2,b,GEN_CHORD,0,DIR_FWD,0);int len=lengths[ix];
        poly_set(proj.pat,(uint8_t *)&proj.sound,0,22+b,len,RATE_16);arm();
        before=proj;int history=undo.cur;seq.ext=(uint8_t)ext;seq.ext_bpm=120;
        seq_start(&seq);run_samples(1);CHECK(ui_freeze_request()==VR_OK);
        for(int n=0;n<4096 && seq.freeze->state!=FREEZE_HELD;n++){
            if(ext)seq_ext_clock(&seq);run_samples(ext?919:256);
        }
        CHECK(seq.freeze->state==FREEZE_HELD);CHECK(seq.freeze->count==len);
        pattern_freeze_t f=*seq.freeze;nevents=0;
        for(int n=0;n<500;n++){if(ext)seq_ext_clock(&seq);run_samples(ext?919:256);}
        CHECK(!memcmp(&f,seq.freeze,sizeof f));
        for(unsigned i=0;i<nevents;i++)if(events[i].bass==b)CHECK(events[i].note==f.bass.step[events[i].step].note);
        seq_stop(&seq);run_samples(1);CHECK(ons==offs);
        CHECK(!memcmp(&before,&proj,sizeof proj));CHECK(undo.cur==history);
        pattern_variation_t off=seq.variation;off.lane=255;CHECK(ui_variation_config(&off)==VR_OK);
        CHECK(seq.freeze->state==FREEZE_RECORD && !seq.freeze->count);
    }
}
static void wasm_fixture(int b)
{
    setup(2,b,GEN_SCALE,0,DIR_FWD,1);seq.variation.anchor=0;
    int l=motion_alloc(proj.arr.lane,0,2+b,T_PROB,(2+b)<<4,0);CHECK(l>=0);
    for(int i=0;i<NSTEPS;i++)proj.arr.lane[l].val[i]=i==1?0:100;
    settle();CHECK(project_save()==0);char path[128];
    snprintf(path,sizeof path,"build/freeze-commit/baseline-%d.json",b);fixture_store(path);
    arm();seq_start(&seq);run_samples(100000);CHECK(ui_freeze_request()==VR_OK);run_samples(90000);
    CHECK(seq.freeze->state==FREEZE_HELD);seq_stop(&seq);run_samples(1);
    snprintf(path,sizeof path,"build/freeze-commit/frozen-%d.json",b);FILE *f=fopen(path,"w");CHECK(f!=0);
    fprintf(f,"{\"notes\":[");int n=0;
    for(int i=0;i<16;i++){bstep_t *st=&seq.freeze->bass.step[i];if(bstep_gate(st)==G_NOTE)fprintf(f,"%s[%d,%d]",n++?",":"",st->note,(st->flags&BS_ACCENT)?127:90);}
    fprintf(f,"]}\n");fclose(f);
    *page_of(V_GEN)=5;build_pages();ui.msg_until=0;ui_frame();
    snprintf(path,sizeof path,"build/freeze-commit/native-capture-%d.png",b);png_write(path);
    CHECK(ui_freeze_commit(6)==VR_OK);CHECK(project_save()==0);
    snprintf(path,sizeof path,"build/freeze-commit/committed-%d.json",b);fixture_store(path);
}
int main(void)
{
    for(int f=0;f<3;f++)for(int b=0;b<2;b++)for(int mode=0;mode<4;mode++)capture_bass(f,b,mode);
    capture_drums();freeze_boundaries();short_long_external();wasm_fixture(0);wasm_fixture(1);
    printf("FREEZE COMMIT PASS: %d checkpoints; A/B and22 drum lanes,3formats,4 constraints, accepted probability gates, source/sound/AC/swing preservation, Undo/Redo/save/reboot and refusals.\n",checks);return 0;
}
