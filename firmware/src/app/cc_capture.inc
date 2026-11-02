/* SPDX-License-Identifier: GPL-3.0-only */
/* Fourth recipe family reuses the16-byte source state. Main-only receipts;
 * the existing audio-owned Freeze FSM remains the sole capture publisher. */
static int cf_context(const cc_source_t *s)
{return cg_context(s) && seq.variation.lane==s->applied.strip && seq.variation.pattern==s->applied.pattern && seq_freeze_matches(&seq,s->applied.strip);}
static void cf_cancel_pending(cc_source_t *s)
{
    if((s->generation.flags&CG_CAPTURE) && !s->generation.drum && s->generation.state==CG_PENDING){
        uint32_t saved=plat_control_enter();
        if(seq.freeze && seq.variation.lane==s->applied.strip && seq.variation.pattern==s->applied.pattern && seq.freeze->state==FREEZE_REQUEST)seq.freeze->state=FREEZE_RECORD;
        plat_control_leave(saved);
        s->generation.state=CG_CANCELLED;s->generation.reason=CG_CANCEL;
        s->generation.flags&=(uint8_t)~CG_ARM;
    }
}
static void cf_family(cc_source_t *s)
{
    cc_generation_t *g=&s->generation;
    if(!(g->flags&CG_CAPTURE) || g->drum){
        cg_changed(s);g->flags=(uint8_t)((g->flags&~CG_VAR)|CG_CAPTURE);
        g->seed=0;g->seed_mask=0;g->drum=0;g->mode=0;g->block=0;g->rsv2=0;
    }
}
static int cf_ready(const cc_source_t *s,int commit)
{
    if(!cf_context(s))return CG_CONTEXT;
    if(project_write_protected())return CG_PROTECTED;
    if(external_edit || ui.rec || ui.btn || ui.keys || ui.overlay==O_ASK || perf_testing() || save_wait || ui_saves_held())return CG_OWNER;
    if(seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage || seq.transport_hold || (commit && seq.playing))return CG_STOP;
    if(engine_queue_read()!=engine_queue_written())return CG_QUEUE;
    if(commit){
        if(!s->generation.rsv2 || s->generation.block>=NPAT || s->generation.block==s->applied.pattern)return CG_DESTINATION;
        if(!freeze_capture_token())return CG_INCOMPLETE;
        if(s->generation.seed_mask!=15 || s->generation.seed!=freeze_capture_token())return CG_STALE;
        if(s->applied.strip<22 && !(proj.variant.arrangement.flags&POLY_FLAG) &&
           (proj.pat[s->generation.block].drum[s->applied.kit].len!=seq.freeze->bass.len ||
            proj.pat[s->generation.block].drum[s->applied.kit].rate!=seq.freeze->bass.rate))return CG_CAPTURE_UNSUPPORTED;
    }
    return CG_OK;
}
static POLY_COLD int cf_apply(cc_source_t *s,int field,uint32_t value)
{
    cc_generation_t *g=&s->generation;
    if(field==29 || (field>=32 && field<36)){
        cf_cancel_pending(s);cf_family(s);
        if(field==29){if(!g->rsv2 || g->block!=value){cg_changed(s);g->block=(uint8_t)value;g->rsv2=1;}}
        else{unsigned shift=(unsigned)(field-32)*8;uint32_t next=(g->seed&~(255u<<shift))|(value<<shift);
            if(next!=g->seed || !(g->seed_mask&(1u<<(field-32)))){cg_changed(s);g->seed=next;g->seed_mask|=(uint8_t)(1u<<(field-32));}}
        return CC_APPLIED;
    }
    if(field==10 && (value==6 || value==7)){cf_family(s);if(g->mode!=(value==7)){cf_cancel_pending(s);cg_changed(s);g->mode=(uint8_t)(value==7);}}
    if(!(g->flags&CG_CAPTURE) || g->drum)return cg_refuse(s,CG_CONTEXT);
    if(field==8 || field==9){
        unsigned shift=field==8?0:7;uint16_t request=(uint16_t)((g->request&~(127u<<shift))|(value<<shift));
        if(g->request!=request){cf_cancel_pending(s);g->flags&=(uint8_t)~CG_ARM;g->request=request;if(cg_generation!=UINT32_MAX)cg_generation++;}
        g->flags|=(uint8_t)(field==8?CG_TOKEN_LO:CG_TOKEN_HI);return CC_APPLIED;
    }
    if(field==11 || (field==10&&!value)){
        if(value || field==10){cf_cancel_pending(s);if((g->flags&CG_ARM)||!(g->flags&CG_LAST)){g->state=CG_CANCELLED;g->reason=CG_CANCEL;}
            g->flags&=(uint8_t)~CG_ARM;if(cg_generation!=UINT32_MAX)cg_generation++;}
        return CC_APPLIED;
    }
    if(field==10){
        if(value!=(uint32_t)(g->mode?7:6))return cg_refuse(s,CG_VALUE);
        if((g->flags&(CG_TOKEN_LO|CG_TOKEN_HI))!=(CG_TOKEN_LO|CG_TOKEN_HI))return cg_refuse(s,CG_CONTEXT);
        if(cg_generation==UINT32_MAX)return cg_refuse(s,CG_STALE);
        if(g->flags&CG_LAST){
            if(g->request<g->last_request)return cg_refuse(s,CG_STALE);
            if(g->request==g->last_request)return (g->flags&CG_CHANGED)?cg_refuse(s,CG_STALE):CC_APPLIED;
        }
        int reason=cf_ready(s,g->mode);if(reason)return cg_refuse(s,reason);
        for(unsigned i=0;i<CC_SOURCES;i++)if(&cc_sources[i]!=s && (cc_sources[i].generation.flags&CG_CAPTURE) && cc_sources[i].generation.state==CG_PENDING)return cg_refuse(s,CG_OWNER);
        s->guard.revision=variant_current_revision();s->guard.transport_epoch=seq.transport_epoch;
        s->guard.fingerprint=g->mode?freeze_commit_fingerprint():variation_runtime_fingerprint();
        s->armed=0;g->flags=(uint8_t)((g->flags&~CG_CHANGED)|CG_ARM|CG_LAST);g->last_request=g->request;
        g->result_request=g->request;g->state=CG_ACCEPTED;g->reason=CG_OK;
        if(cg_generation!=UINT32_MAX)cg_generation++;return CC_APPLIED;
    }
    if(field!=30 && field!=31)return CC_UNSUPPORTED;
    if((field==31)!=g->mode)return cg_refuse(s,CG_CONTEXT);
    if(!value){g->flags&=(uint8_t)~CG_HIGH;return CC_APPLIED;}
    if(g->flags&CG_HIGH)return CC_APPLIED;g->flags|=CG_HIGH;
    if((g->flags&CG_LAST) && g->request==g->last_request && !(g->flags&CG_ARM))
        return (g->flags&CG_CHANGED)?cg_refuse(s,CG_STALE):(g->state==CG_APPLIED||g->state==CG_NO_CHANGE||g->state==CG_PENDING)?CC_APPLIED:CC_REJECTED;
    if(!(g->flags&CG_ARM))return cg_refuse(s,CG_CONTEXT);
    int reason=cf_ready(s,g->mode);if(reason)return cg_refuse(s,reason);
    if(s->guard.revision!=variant_current_revision() || s->guard.transport_epoch!=seq.transport_epoch ||
       s->guard.fingerprint!=(g->mode?freeze_commit_fingerprint():variation_runtime_fingerprint()))return cg_refuse(s,CG_STALE);
    int was_held=seq.freeze->state==FREEZE_HELD;
    int rc=g->mode?ui_freeze_commit_checked(g->block,&s->guard):ui_freeze_request();
    g->flags=(uint8_t)((g->flags&~(CG_ARM|CG_CHANGED))|CG_LAST);g->result_request=g->request;
    if(rc!=VR_OK)return cg_refuse(s,rc==VR_PROTECTED?CG_PROTECTED:rc==VR_UNSUPPORTED?CG_CAPTURE_UNSUPPORTED:rc==VR_FULL?CG_UNDO:rc==VR_BUSY?CG_OWNER:CG_INCOMPLETE);
    g->state=g->mode?(freeze_commit_fingerprint()==s->guard.fingerprint?CG_NO_CHANGE:CG_APPLIED):seq.freeze->state==FREEZE_HELD?(was_held?CG_NO_CHANGE:CG_APPLIED):CG_PENDING;g->reason=CG_OK;
    if(g->state!=CG_PENDING)s->guard.transport_epoch=variant_current_revision();
    if(cg_generation!=UINT32_MAX)cg_generation++;return CC_APPLIED;
}
/* Called from the normal main MIDI service, never from a read-only query/ISR. */
void cc_generation_service(void)
{
    for(unsigned i=0;i<CC_SOURCES;i++){
        cc_source_t *s=&cc_sources[i];cc_generation_t *g=&s->generation;
        if(!(g->flags&CG_CAPTURE) || g->drum || g->state!=CG_PENDING)continue;
        if(!cf_context(s) || s->guard.revision!=variant_current_revision() ||
           s->guard.fingerprint!=variation_runtime_fingerprint()){
            cf_cancel_pending(s);cg_refuse(s,CG_STALE);continue;
        }
        /* A completed immutable pass wins over a following Stop epoch. */
        if(freeze_capture_token()){
            g->state=CG_APPLIED;g->reason=CG_OK;s->guard.transport_epoch=variant_current_revision();
            if(cg_generation!=UINT32_MAX)cg_generation++;
        }else if(!seq.playing || seq.freeze->state!=FREEZE_REQUEST || s->guard.transport_epoch!=seq.transport_epoch){
            cf_cancel_pending(s);cg_refuse(s,CG_INCOMPLETE);
        }
    }
}
static POLY_COLD int cf_snapshot(const cc_source_t *s,unsigned page,unsigned index,uint32_t *w)
{
    pattern_freeze_t *f=seq.freeze;if(!f || (page!=5 && page!=6))return -1;
    unsigned n=5,held=freeze_capture_token(),capture=(s->generation.flags&CG_CAPTURE)!=0 && !s->generation.drum;
    if(page==6){
        if(!held || index>=f->bass.len)return -1;
        bstep_t st=f->bass.step[index];
        w[n++]=held;w[n++]=index;w[n++]=st.note;w[n++]=bstep_gate(&st);
        w[n++]=(st.flags&BS_ACCENT)!=0;w[n++]=(st.flags&BS_SLIDE)!=0;w[n++]=st.flags;w[n++]=1;
    }else{
        if(index)return -1;
        uint32_t saved=plat_control_enter();unsigned state=f->state,count=f->count,length=f->bass.len,rate=f->bass.rate;plat_control_leave(saved);
        w[n++]=held;w[n++]=seq.variation.lane;w[n++]=seq.variation.pattern;w[n++]=state;
        w[n++]=length;w[n++]=rate;w[n++]=count;w[n++]=held!=0;
        w[n++]=held && seq.variation.lane>=22;w[n++]=held!=0;w[n++]=cf_context(s);
        w[n++]=capture?s->generation.block:0;w[n++]=capture && s->generation.rsv2;
        w[n++]=capture?s->generation.mode:255;w[n++]=capture?s->generation.seed:0;
    }
    return (int)n;
}
