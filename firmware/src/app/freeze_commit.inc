/* SPDX-License-Identifier: GPL-3.0-only */
/* Capture accepted gates and final pitches at audio-owned step boundaries.
 * Main publishes only a one-byte request while running. No ISR allocation. */
static int capture_page(void)
{return ui.view==V_GEN && ((is_drum() && cur_page()==3) || (is_303() && cur_page()==5));}

static POLY_COLD int ui_freeze_request(void)
{
    pattern_freeze_t *f=seq.freeze;int lane=seq.variation.lane;
    if(!f || lane>=24 || !seq_freeze_matches(&seq,lane))return VR_INVALID;
    if(lane>=22 && (proj.pat[seq.variation.pattern].bass[lane-22].dir!=DIR_FWD ||
       bstep_gate(&proj.pat[seq.variation.pattern].bass[lane-22].step[0])==G_TIE))return VR_UNSUPPORTED;
    if(external_edit || ui.rec || ui.keys || perf_testing() || save_wait || ui_saves_held())return VR_BUSY;
    uint32_t saved=plat_control_enter();
    if(!seq_freeze_matches(&seq,lane)){plat_control_leave(saved);return VR_BUSY;}
    if(seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage || seq.transport_hold ||
       engine_queue_read()!=engine_queue_written()){plat_control_leave(saved);return VR_BUSY;}
    if(f->state!=FREEZE_HELD){
        if(seq.playing)f->state=FREEZE_REQUEST;
        else if(f->count && f->count==f->bass.len)f->state=FREEZE_HELD;
        else {plat_control_leave(saved);return VR_BUSY;}
    }
    plat_control_leave(saved);return VR_OK;
}

/* Content identity,not a monotonic session ID. Only a complete HELD buffer is
 * readable; audio never writes a HELD pass. Zero is reserved for unavailable. */
static POLY_COLD uint32_t freeze_capture_token(void)
{
    pattern_freeze_t *f=seq.freeze;int lane=seq.variation.lane;
    if(!f || lane>=24 || f->state!=FREEZE_HELD || !f->bass.len || f->bass.len>NSTEPS || f->count!=f->bass.len || !seq_freeze_matches(&seq,lane))return 0;
    uint32_t h=2166136261u;const uint8_t *v=(const uint8_t *)&seq.variation;
    for(unsigned i=0;i<sizeof seq.variation;i++)h=(h^v[i])*16777619u;
    h=(h^f->bass.len)*16777619u;h=(h^f->bass.rate)*16777619u;
    for(unsigned i=0;i<f->bass.len;i++){h=(h^f->bass.step[i].note)*16777619u;h=(h^f->bass.step[i].flags)*16777619u;}
    return h?h:1;
}
static uint32_t freeze_commit_fingerprint(void)
{return variation_runtime_fingerprint()^freeze_capture_token();}

/* Destination slot only, same selected part. Chance/condition results are baked
 * into gates; remove only that destination probability lane, not sound motion. */
static POLY_COLD int ui_freeze_commit_checked(int dst,const cc_action_guard_t *guard)
{
    pattern_freeze_t *f=seq.freeze;int lane=seq.variation.lane,src=seq.variation.pattern;
    if(!f || f->state!=FREEZE_HELD || lane>=24 || dst<0 || dst>=NPAT || dst==src)return VR_INVALID;
    if(project_write_protected())return VR_PROTECTED;
    if(external_edit || ui.rec || ui.keys || perf_testing() || save_wait || ui_saves_held() ||
       seq.playing || seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage || seq.transport_hold ||
       engine_queue_read()!=engine_queue_written())return VR_BUSY;
    project_t *candidate=(project_t *)ui_project_stage(sizeof(project_t));
    if(!candidate)return VR_BUSY;
    if(engine_queue_read()!=engine_queue_written()){ui_project_stage_finish(0);return VR_BUSY;}
    if(guard && (guard->revision!=variant_current_revision() || guard->transport_epoch!=seq.transport_epoch || guard->fingerprint!=freeze_commit_fingerprint())){ui_project_stage_finish(0);return VR_INVALID;}
    *candidate=proj;
    int part=lane<22?lane/11:lane-20,voice=lane<22?lane%11:0;
    if(lane>=22){
        bpart_t *bp=&candidate->pat[dst].bass[lane-22];
        for(int i=0;i<f->bass.len;i++){
            bp->step[i].note=f->bass.step[i].note;
            bp->step[i].flags=(f->bass.step[i].flags&~BS_ACCENT)|(bp->step[i].flags&BS_ACCENT);
        }
        bp->len=f->bass.len;bp->rate=f->bass.rate;bp->dir=DIR_FWD;bp->transpose=24;
        /* A stored frozen line must not resume autonomous note rewriting. */
        bp->gen.mutate_bars=0;
    }else{
        uint32_t *row=candidate->pat[dst].drum[part].hit[voice];
        for(int i=0;i<f->bass.len;i++){
            uint32_t mask=1u<<(i&31);
            if(bstep_gate(&f->bass.step[i])==G_NOTE)row[i>>5]|=mask;else row[i>>5]&=~mask;
        }
    }
    if(candidate->variant.arrangement.flags&POLY_FLAG)
        poly_set(candidate->pat,(uint8_t *)&candidate->sound,dst,lane,f->bass.len,f->bass.rate);
    for(int i=0;i<NLANE;i++){
        lane_t *l=&candidate->arr.lane[i];
        if(l->used && l->pat==dst && l->part==part && l->t==T_PROB && l->v==(part<<4|voice))motion_clear(candidate->arr.lane,i);
    }
    if(!variant_valid(&candidate->variant) || !arrangement_song_valid(&candidate->arr.song) ||
       ((candidate->variant.arrangement.flags&POLY_FLAG) && !poly_valid(candidate->pat,(uint8_t *)&candidate->sound))){ui_project_stage_finish(0);return VR_INVALID;}
    if(ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL ||
       ca_undo_bytes((uint8_t *)candidate+UNDO_OFF,(uint8_t *)&proj+UNDO_OFF,sizeof undo_shadow)>UNDO_POOL){ui_project_stage_finish(0);return VR_FULL;}
    undo_commit(&undo);proj=*candidate;
    if(undo_commit(&undo)>0)ud_put16(undo.pool+undo.at[undo.cur-1]+6,4096u);
    engine_probability_clear(lane);engine_motion_reset();engine_cc_refresh(2);engine_brk_loops();
    ui.outline_ok=0;mark_dirty();ui.act_t=plat_ms();ui_project_stage_finish(0);return VR_OK;
}

static POLY_COLD int ui_freeze_commit(int dst)
{return ui_freeze_commit_checked(dst,0);}

static POLY_COLD void capture_native_action(int commit,int confirm,int arg)
{
    pattern_freeze_t *f=seq.freeze;int rc;
    if(!commit){
        rc=ui_freeze_request();
        say(rc==VR_OK?(f->state==FREEZE_HELD?"FROZEN: BASE UNCHANGED":"FREEZE AT END OF PASS"):rc==VR_UNSUPPORTED?"NEEDS FWD / NO FIRST TIE":rc==VR_INVALID?"ARM VARIATION FIRST":"PLAY A FULL PASS / WAIT",0);return;
    }
    if(!f || f->state!=FREEZE_HELD){say("FREEZE A FULL PASS FIRST",0);return;}
    if(!confirm){
        if(seq.playing){say("STOP TO STORE FROZEN",0);return;}
        if(f->destination==seq.variation.pattern){say("CHOOSE A DIFFERENT SLOT",0);return;}
        char a[28],b[28];int lane=seq.variation.lane;
        put_s(put_i(put_s(a,"STORE FROZEN TO P"),f->destination+1),"?");
        put_s(put_s(b,lane>=22?PART_N[lane-20]:COPY_SCOPE_N[lane+1])," KEEP ACC/SW");
        edit_fingerprint=variation_fingerprint();edit_epoch=seq.transport_epoch;
        ask(ACT_FREEZE_COMMIT,f->destination,a,b);return;
    }
    if(edit_fingerprint!=variation_fingerprint() || edit_epoch!=seq.transport_epoch){say("CHANGED: CONFIRM AGAIN",0);return;}
    rc=ui_freeze_commit(arg);
    say(rc==VR_OK?"STORED: SAVE TO KEEP":rc==VR_PROTECTED?"PROJECT PROTECTED":rc==VR_FULL?"UNDO FULL: NOTHING STORED":rc==VR_INVALID?"INVALID DESTINATION":"STOP / WAIT TO STORE",0);
}
