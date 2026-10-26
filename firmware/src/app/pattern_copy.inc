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

/* Cross-part copies keep destination timbre and its automation. Only musical
 * probability/condition lanes have the same meaning across different sounds. */
static int pc_probability(const lane_t *l,int scope)
{return l->t==T_PROB && l->v==(pc_scope_part(scope)<<4|(pc_scope_part(scope)<NKIT?pc_scope_voice(scope):0));}

static POLY_COLD int pc_stage(project_t *p,const project_t *source,int src,int dst,int scope,int to)
{
    int k,n,part,cross=scope!=to;
    if(!scope){
        uint32_t accent[NKIT][NSW];
        uint8_t swing=p->pat[dst].swing;
        for(part=0;part<NKIT;part++)for(k=0;k<NSW;k++)accent[part][k]=p->pat[dst].drum[part].accent[k];
        for(part=0;part<NPARTS;part++)edit_copy_notes(p,dst,src,part);
        for(part=0;part<NKIT;part++)for(k=0;k<NSW;k++)p->pat[dst].drum[part].accent[k]=accent[part][k];
        p->pat[dst].swing=swing;
    }else if(scope>=PCOPY_303A){
        /* Cross-part copies retain destination note accents as well as kit AC.
         * Slides/ties, direction, transpose and timing travel with the notes. */
        bpart_t *d=&p->pat[dst].bass[pc_scope_part(to)-NKIT];
        const bpart_t *s=&source->pat[src].bass[pc_scope_part(scope)-NKIT];
        if(cross){
            for(k=0;k<NSTEPS;k++){
                d->step[k].note=s->step[k].note;
                d->step[k].flags=(s->step[k].flags&~BS_ACCENT)|(d->step[k].flags&BS_ACCENT);
            }
            d->len=s->len;d->rate=s->rate;d->dir=s->dir;d->transpose=s->transpose;
            uint8_t accent=d->gen.accent;
            d->gen=s->gen;d->gen.accent=accent;
        }else *d=*s;
    }else{
        int kit=pc_scope_part(to),voice=pc_scope_voice(to),lane=kit*NDRUM+voice;
        int sk=pc_scope_part(scope),sv=pc_scope_voice(scope),sl=sk*NDRUM+sv;
        for(k=0;k<NSW;k++)p->pat[dst].drum[kit].hit[voice][k]=source->pat[src].drum[sk].hit[sv][k];
        if(p->variant.arrangement.flags&POLY_FLAG)
            poly_set(p->pat,(uint8_t *)&p->sound,dst,lane,
                poly_length(source->pat,(const uint8_t *)&source->sound,src,sl),
                poly_rate(source->pat,(const uint8_t *)&source->sound,src,sl));
        /* Legacy kit length/rate is shared: preserve it for a single-row paste. */
    }
    /* Reclaim the entire selected destination scope first, in the candidate.
     * A capacity failure never publishes partially replaced notes or motion. */
    for(k=0;k<NLANE;k++)if(pc_motion_scope(&p->arr.lane[k],dst,to) && (!cross || pc_probability(&p->arr.lane[k],to)))motion_clear(p->arr.lane,k);
    for(k=0;k<NLANE;k++){
        const lane_t *l=&source->arr.lane[k];
        if(!pc_motion_scope(l,src,scope) || (cross && !pc_probability(l,scope)))continue;
        n=motion_alloc(p->arr.lane,dst,cross?pc_scope_part(to):l->part,l->t,
                       cross?(pc_scope_part(to)<<4|(pc_scope_part(to)<NKIT?pc_scope_voice(to):0)):l->v,l->i);
        if(n<0)return VR_FULL;
        for(part=0;part<NSTEPS;part++)p->arr.lane[n].val[part]=l->val[part];
    }
    return VR_OK;
}

static POLY_COLD int ui_pattern_copy_checked(int src,int dst,int scope,int to,const cc_action_guard_t *guard,int apply)
{
    project_t *candidate;
    int rc;
    if(src<0 || src>=NPAT || dst<0 || dst>=NPAT || scope<0 || scope>=PCOPY_COUNT || to<0 || to>=PCOPY_COUNT || pc_group(scope)!=pc_group(to))return VR_INVALID;
    if(project_write_protected())return VR_PROTECTED;
    if(external_edit || ui.rec || ui.keys || perf_testing() || save_wait || ui_saves_held() ||
       seq.playing || seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage ||
       seq.transport_hold || engine_queue_read()!=engine_queue_written())return VR_BUSY;
    if(guard && (guard->revision!=variant_current_revision() || guard->transport_epoch!=seq.transport_epoch || guard->fingerprint!=project_fingerprint()))return VR_INVALID;
    if(src==dst && scope==to)return VR_OK;
    candidate=(project_t *)ui_project_stage(sizeof(project_t));
    if(!candidate)return VR_BUSY;
    /* A MIDI event arriving while staging must not make preflight stale. */
    if(engine_queue_read()!=engine_queue_written()){
        ui_project_stage_finish(0);return VR_BUSY;
    }
    if(guard && (guard->revision!=variant_current_revision() || guard->transport_epoch!=seq.transport_epoch || guard->fingerprint!=project_fingerprint())){ui_project_stage_finish(0);return VR_INVALID;}
    *candidate=proj;
    rc=pc_stage(candidate,&proj,src,dst,scope,to);
    if(rc==VR_OK && (!variant_valid(&candidate->variant) || !arrangement_song_valid(&candidate->arr.song) ||
       ((candidate->variant.arrangement.flags&POLY_FLAG) && !poly_valid(candidate->pat,(const uint8_t *)&candidate->sound))))rc=VR_INVALID;
    if(rc==VR_OK && (ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL ||
       ca_undo_bytes((const uint8_t *)candidate+UNDO_OFF,(const uint8_t *)&proj+UNDO_OFF,sizeof undo_shadow)>UNDO_POOL))rc=VR_FULL;
    if(rc!=VR_OK){ui_project_stage_finish(0);return rc;}
    if(!apply){ui_project_stage_finish(0);return VR_OK;}
    undo_commit(&undo);
    proj=*candidate;
    /* Stopped-only history marker, deliberately NOT a block insertion marker. */
    if(undo_commit(&undo)>0)ud_put16(undo.pool+undo.at[undo.cur-1]+6,4096u);
    engine_motion_reset();engine_cc_refresh(2);engine_brk_loops();
    ui.outline_ok=0;mark_dirty();ui.act_t=plat_ms();
    ui_project_stage_finish(0);
    return VR_OK;
}

POLY_COLD int ui_pattern_copy_to(int src,int dst,int scope,int to)
{return ui_pattern_copy_checked(src,dst,scope,to,0,1);}

POLY_COLD int ui_pattern_copy(int src,int dst,int scope)
{return ui_pattern_copy_to(src,dst,scope,scope);}

static POLY_COLD void copy_native_action(int confirm,int arg)
{
    int src,dst,scope,to,rc,k,p,refs=0;
    char a[28],b[28];
    if(!confirm){
        src=copy_config[0];dst=copy_config[1];scope=copy_config[2];to=copy_config[3];
        if(src==dst && scope==to){say("SAME SLOT: NO CHANGE",0);return;}
        for(k=0;k<song()->len;k++){
            for(p=0;p<NPARTS;p++)if((!to || p==pc_scope_part(to)) && song()->bar[k].pat[p]==dst)break;
            if(p<NPARTS)refs++;
        }
        put_s(put_i(put_s(put_i(put_s(a,"P"),src+1)," -> P"),dst+1)," KEEP ACC/SW");
        if(scope==to)put_s(put_i(put_s(put_s(b,COPY_SCOPE_N[scope]),"; "),refs)," BLOCKS");
        else put_s(put_i(put_s(put_s(put_s(put_s(b,COPY_SHORT_N[scope])," > "),COPY_SHORT_N[to]),";"),refs),"B");
        edit_fingerprint=project_fingerprint();edit_epoch=seq.transport_epoch;
        ask(ACT_SCOPED_PASTE,src|(dst<<5)|(scope<<10)|(to<<15),a,b);
        return;
    }
    src=arg&31;dst=(arg>>5)&31;scope=(arg>>10)&31;to=(arg>>15)&31;
    if(edit_fingerprint!=project_fingerprint() || edit_epoch!=seq.transport_epoch){say("CHANGED: CONFIRM AGAIN",0);return;}
    rc=ui_pattern_copy_to(src,dst,scope,to);
    if(rc==VR_OK)say("PASTED: ACC/SW KEPT",0);
    else say(rc==VR_FULL?"NO ROOM: NOTHING PASTED":rc==VR_PROTECTED?"PROJECT PROTECTED":rc==VR_INVALID?"INVALID COPY SCOPE":"STOP / WAIT TO PASTE",0);
}
