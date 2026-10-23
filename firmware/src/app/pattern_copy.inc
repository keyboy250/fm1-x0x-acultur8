/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-only stopped transaction. The source is a stored slot, never ppat[].
 * Scope 0 copies all parts; the remaining IDs select one drum or bass part.
 * The destination's shared accent and swing always remain authoritative. */
static POLY_COLD int pc_motion_scope(const lane_t *l,int pattern,int scope)
{
    int kit,voice;
    if(!l->used || l->pat!=pattern)return 0;
    if(!scope)return 1;
    kit=pc_scope_part(scope);voice=pc_scope_voice(scope);
    if(kit>=PART_303A)return l->part==kit;
    return l->part==kit && ((l->t==kit && l->v==voice) ||
           (l->t==T_PROB && l->v==(kit<<4|voice)));
}

static POLY_COLD int pc_stage(project_t *p,int src,int dst,int scope)
{
    int k,n,part;
    if(!scope){
        uint32_t accent[NKIT][NSW];
        uint8_t swing=p->pat[dst].swing;
        for(part=0;part<NKIT;part++)for(k=0;k<NSW;k++)accent[part][k]=p->pat[dst].drum[part].accent[k];
        for(part=0;part<NPARTS;part++)edit_copy_notes(p,dst,src,part);
        for(part=0;part<NKIT;part++)for(k=0;k<NSW;k++)p->pat[dst].drum[part].accent[k]=accent[part][k];
        p->pat[dst].swing=swing;
    }else if(scope>=PCOPY_303A){
        /* Bass articulation (note accent/slide/tie), direction, transpose and
         * pattern-owned timing travel with this bass part, not the kit AC rows. */
        edit_copy_notes(p,dst,src,pc_scope_part(scope));
    }else{
        int kit=pc_scope_part(scope),voice=pc_scope_voice(scope),lane=kit*NDRUM+voice;
        for(k=0;k<NSW;k++)p->pat[dst].drum[kit].hit[voice][k]=p->pat[src].drum[kit].hit[voice][k];
        if(p->variant.arrangement.flags&POLY_FLAG)
            poly_set(p->pat,(uint8_t *)&p->sound,dst,lane,
                poly_length(p->pat,(const uint8_t *)&p->sound,src,lane),
                poly_rate(p->pat,(const uint8_t *)&p->sound,src,lane));
        /* Legacy kit length/rate is shared: preserve it for a single-row paste. */
    }
    /* Reclaim the entire selected destination scope first, in the candidate.
     * A capacity failure never publishes partially replaced notes or motion. */
    for(k=0;k<NLANE;k++)if(pc_motion_scope(&p->arr.lane[k],dst,scope))motion_clear(p->arr.lane,k);
    for(k=0;k<NLANE;k++){
        const lane_t *l=&p->arr.lane[k];
        if(!pc_motion_scope(l,src,scope))continue;
        n=motion_alloc(p->arr.lane,dst,l->part,l->t,l->v,l->i);
        if(n<0)return VR_FULL;
        for(part=0;part<NSTEPS;part++)p->arr.lane[n].val[part]=l->val[part];
    }
    return VR_OK;
}

POLY_COLD int ui_pattern_copy(int src,int dst,int scope)
{
    project_t *candidate;
    int rc;
    if(src<0 || src>=NPAT || dst<0 || dst>=NPAT || scope<0 || scope>=PCOPY_COUNT)return VR_INVALID;
    if(project_write_protected())return VR_PROTECTED;
    if(external_edit || ui.rec || ui.keys || perf_testing() || save_wait || ui_saves_held() ||
       seq.playing || seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage ||
       seq.transport_hold || engine_queue_read()!=engine_queue_written())return VR_BUSY;
    if(src==dst)return VR_OK;
    candidate=(project_t *)ui_project_stage(sizeof(project_t));
    if(!candidate)return VR_BUSY;
    /* A MIDI event arriving while staging must not make preflight stale. */
    if(engine_queue_read()!=engine_queue_written()){
        ui_project_stage_finish(0);return VR_BUSY;
    }
    *candidate=proj;
    rc=pc_stage(candidate,src,dst,scope);
    if(rc==VR_OK && (!variant_valid(&candidate->variant) || !arrangement_song_valid(&candidate->arr.song) ||
       ((candidate->variant.arrangement.flags&POLY_FLAG) && !poly_valid(candidate->pat,(const uint8_t *)&candidate->sound))))rc=VR_INVALID;
    if(rc==VR_OK && (ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL ||
       ca_undo_bytes((const uint8_t *)candidate+UNDO_OFF,(const uint8_t *)&proj+UNDO_OFF,sizeof undo_shadow)>UNDO_POOL))rc=VR_FULL;
    if(rc!=VR_OK){ui_project_stage_finish(0);return rc;}
    undo_commit(&undo);
    proj=*candidate;
    /* Stopped-only history marker, deliberately NOT a block insertion marker. */
    if(undo_commit(&undo)>0)ud_put16(undo.pool+undo.at[undo.cur-1]+6,4096u);
    engine_motion_reset();engine_cc_refresh(2);engine_brk_loops();
    ui.outline_ok=0;mark_dirty();ui.act_t=plat_ms();
    ui_project_stage_finish(0);
    return VR_OK;
}

static POLY_COLD void copy_native_action(int confirm,int arg)
{
    int src,dst,scope,rc,k,p,refs=0;
    char a[28],b[28];
    if(!confirm){
        src=copy_config[0];dst=copy_config[1];scope=copy_config[2];
        if(src==dst){say("SAME SLOT: NO CHANGE",0);return;}
        for(k=0;k<song()->len;k++){
            for(p=0;p<NPARTS;p++)if((!scope || p==pc_scope_part(scope)) && song()->bar[k].pat[p]==dst)break;
            if(p<NPARTS)refs++;
        }
        put_s(put_i(put_s(put_i(put_s(a,"P"),src+1)," -> P"),dst+1)," KEEP ACC/SW");
        put_s(put_i(put_s(put_s(b,COPY_SCOPE_N[scope]),"; "),refs)," BLOCKS");
        copy_fingerprint=project_fingerprint();copy_epoch=seq.transport_epoch;
        ask(ACT_SCOPED_PASTE,src|(dst<<5)|(scope<<10),a,b);
        return;
    }
    src=arg&31;dst=(arg>>5)&31;scope=(arg>>10)&31;
    if(copy_fingerprint!=project_fingerprint() || copy_epoch!=seq.transport_epoch){say("CHANGED: CONFIRM AGAIN",0);return;}
    rc=ui_pattern_copy(src,dst,scope);
    if(rc==VR_OK)say("PASTED: ACC/SW KEPT",0);
    else say(rc==VR_FULL?"NO ROOM: NOTHING PASTED":rc==VR_PROTECTED?"PROJECT PROTECTED":rc==VR_INVALID?"INVALID COPY SCOPE":"STOP / WAIT TO PASTE",0);
}
