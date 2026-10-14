/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-only explicit-context action service. Included after legacy actions so
 * panel behavior is not borrowed or temporarily retargeted. */
static POLY_COLD uint32_t ca_undo_bytes(const uint8_t *a,const uint8_t *b,uint32_t len)
{
    uint32_t i=0,bytes=8;
    while(i<len){
        uint32_t n=ud_run(a,b,len,&i);
        if(!n)break;
        bytes+=4+((n+1)&~1u);i+=n;
    }
    return bytes;
}
static POLY_COLD void ca_clear_lanes(project_t *candidate,int pattern,int part)
{
    int k;for(k=0;k<NLANE;k++)if(candidate->arr.lane[k].used&&candidate->arr.lane[k].pat==pattern&&candidate->arr.lane[k].part==part)motion_clear(candidate->arr.lane,k);
}
static POLY_COLD int ca_copy_part(project_t *candidate,int dst,int src,int part)
{
    edit_copy_notes(candidate,dst,src,part);
    ca_clear_lanes(candidate,dst,part);
    return edit_copy_motion(candidate,dst,src,part);
}



POLY_COLD int ui_cc_action_ready(int act)
{
    if(act<ACT_SAVE||act>ACT_PERF_TEST)return CA_INVALID;
    if(external_edit||ui.rec||ui.btn||ui.keys||perf_testing()||ui.overlay==O_ASK||save_wait)return CA_OWNER;
    if(act!=ACT_ABOUT&&act!=ACT_PERF&&project_write_protected())return CA_PROTECTED;
    if(seq.transport_hold||seq.transport_stage)return CA_STOP;
    if(act==ACT_ABOUT||act==ACT_PERF)return CA_OK;
    if(seq.playing||seq.req_start||seq.req_stop||seq.req_cont||seq.transport_hold||seq.transport_stage)return CA_STOP;
    if(engine_queue_read()!=engine_queue_written())return CA_QUEUE;
    if(act==ACT_PERF_TEST&&(proj.variant.arrangement.flags&POLY_FLAG))return CA_BENCHMARK;
    if(act==ACT_PERF_TEST && seq.stop_epoch==UINT32_MAX)return CA_STOP;
    return CA_OK;
}
static POLY_COLD int ca_action_apply(int act,int pattern,int part,int destination,int block)
{
    int reason=CA_OK,i;project_t *candidate;
    if(pattern<0||pattern>=NPAT||part<0||part>=NPARTS||destination<0||destination>=NPAT||block<0||block>=NSONG)return CA_CONTEXT;
    if(act==ACT_ABOUT){char t[28];put_i(put_s(t,"X0X " X0X_VERSION " MEM "),project_mem_pct());say(t,0);return CA_OK;}
    if(act==ACT_PERF){set_view(V_PERF);return CA_OK;}
    if(act==ACT_SAVE){
        settings_t saved=proj.set;int rc;
        if(ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL)return CA_JOURNAL;
        if(!engine_transport_reserve())return CA_STOP;
        rc=project_save();engine_transport_release();
        if(rc){proj.set=saved;return rc==-7?CA_SCRATCH:rc==-8?CA_PROTECTED:rc==-9?CA_STOP:CA_IO;}
        undo_commit(&undo);ui_external_saved();return CA_OK;
    }
    if(act==ACT_PERF_TEST){
        if(!engine_transport_reserve())return CA_STOP;
        ui.overlay=O_NONE;perf_test_start();perf.cc_stop_epoch=seq.stop_epoch;engine_transport_release();return CA_OK;
    }
    if(act==ACT_RESET && (ui.dirty||!project_checkpoint_matches()))return CA_CHECKPOINT;
    candidate=(project_t *)ui_project_stage(sizeof(project_t));
    if(!candidate)return CA_SCRATCH;
    /* Transport cannot restart; CQ was empty and only this main producer may
     * publish. No failure path below changes canonical, journal, or storage. */
    *candidate=proj;
    switch(act){
    case ACT_CLEAR_PART:edit_clear_part(candidate,&candidate->pat[pattern],part);ca_clear_lanes(candidate,pattern,part);break;
    case ACT_CLEAR_PAT:
        for(i=0;i<NPARTS;i++){edit_clear_part(candidate,&candidate->pat[pattern],i);ca_clear_lanes(candidate,pattern,i);}break;
    case ACT_COPY:
        if(pattern==destination){reason=CA_CONTEXT;break;}
        /* Whole destination lanes are released in the candidate first: no
         * part-order false shortage and no partial canonical copy. */
        for(i=0;i<NPARTS;i++)ca_clear_lanes(candidate,destination,i);
        for(i=0;i<NPARTS;i++)if(ca_copy_part(candidate,destination,pattern,i)){reason=CA_MOTION;break;}
        break;
    case ACT_SONG_INS:
    case ACT_SONG_DEL:{
        song_t *s=&candidate->arr.song;uint8_t *degree=candidate->variant.arrangement.degree;
        if(block>=s->len||(act==ACT_SONG_INS&&s->len>=NSONG)){reason=CA_CONTEXT;break;}
        edit_song_shift(s,degree,block,act==ACT_SONG_INS);
        break;}
    case ACT_SONG_CLR:memset(&candidate->arr.song,0,sizeof candidate->arr.song);memset(candidate->variant.arrangement.degree,0,sizeof candidate->variant.arrangement.degree);break;
    case ACT_RESET:project_defaults_into(candidate);break;
    default:reason=CA_INVALID;break;
    }
    if(!reason && (!variant_valid(&candidate->variant)||!arrangement_song_valid(&candidate->arr.song)||
       ((candidate->variant.arrangement.flags&POLY_FLAG)&&!poly_valid(candidate->pat,(const uint8_t *)&candidate->sound))))reason=CA_INVALID;
    if(!reason && act!=ACT_RESET && (ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL||
       ca_undo_bytes((const uint8_t *)candidate+UNDO_OFF,(const uint8_t *)&proj+UNDO_OFF,sizeof undo_shadow)>UNDO_POOL))reason=CA_JOURNAL;
    if(reason){ui_project_stage_finish(0);return reason;}
    if(act!=ACT_RESET)undo_commit(&undo);
    proj=*candidate;
    if(act==ACT_RESET){
        /* Recovery is the verified pre-reset save. Never overwrite it through
         * autosave, and do not imply global settings are in the undo journal. */
        cc_actions_reset();undo_reset(&undo);reset_checkpoint_hold=1;
        engine_notes_clear();engine_apply_sound(&proj.sound);variant_apply();
        seq.bpm=(float)proj.set.bpm_x10/10.0f;seq.accent_q7=proj.set.accent_q7;
        project_apply_io();palette_set(proj.set.palette);
    }else undo_commit(&undo);
    engine_motion_reset();engine_cc_refresh(2);engine_brk_loops();
    ui.outline_ok=0;ui_draw_invalidate();mark_dirty();ui.act_t=plat_ms();
    ui_project_stage_finish(0);return CA_OK;
}

/* Main-only fingerprinting is bounded by sizeof(project_t). The reservation
 * blocks audio transport/mutation while IRQ input may still publish requests.
 * No project scan, CC-source write or UI-job write runs in IRQ/audio context. */
POLY_COLD int ui_cc_action_arm(int act,cc_action_guard_t *guard)
{
    int reason=ui_cc_action_ready(act);
    if(reason)return reason;
    if(!engine_transport_reserve())return CA_STOP;
    if(seq.transport_epoch==UINT32_MAX)reason=CA_CONFIRM;
    else {guard->revision=variant_current_revision();guard->transport_epoch=seq.transport_epoch;guard->fingerprint=project_fingerprint();}
    engine_transport_release();return reason;
}
POLY_COLD int ui_cc_action(int act,int pattern,int part,int destination,int block,const cc_action_guard_t *guard)
{
    int reason=ui_cc_action_ready(act);
    int destructive=act==2||act==3||act==4||act==5||act==7||act==8||act==9;
    if(reason)return reason;
    if(!destructive)return ca_action_apply(act,pattern,part,destination,block);
    if(!guard)return CA_CONFIRM;
    if(!engine_transport_reserve())return CA_STOP;
    if(seq.transport_epoch==UINT32_MAX || guard->transport_epoch!=seq.transport_epoch ||
       guard->revision!=variant_current_revision() || guard->fingerprint!=project_fingerprint())reason=CA_CONFIRM;
    else reason=ca_action_apply(act,pattern,part,destination,block);
    engine_transport_release();return reason;
}

/* Project reinitialization is main-only. Retire only a CC-owned job before a
 * load/default swap; an old phase must not later restore over the new project. */
void ui_cc_job_reset(void)
{
    if(perf.cc_stop_epoch==UINT32_MAX)return;
    if(perf.phase!=0 && perf.phase!=4)perf_restore();
    perf.test=PT_IDLE;perf.phase=4;perf.cc_stop_epoch=UINT32_MAX;
}
