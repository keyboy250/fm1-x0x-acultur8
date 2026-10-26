/* SPDX-License-Identifier: GPL-3.0-only */
/* CAPTURE+drum selects the fifth recipe family,without allocating more RAM.
 * seed=source slot;step=destination;block/length=source/destination scope;
 * seed_mask is explicit staging validity,not musical seed bytes. */
static int cp_active(const cc_generation_t *g)
{return (g->flags&CG_CAPTURE) && g->drum;}
static void cp_family(cc_source_t *s)
{
    cc_generation_t *g=&s->generation;
    if(!cp_active(g)){
        cf_cancel_pending(s);cg_changed(s);g->flags=(uint8_t)((g->flags&~CG_VAR)|CG_CAPTURE);
        g->drum=1;g->seed=0;g->step=g->block=g->length=g->seed_mask=g->mode=g->rsv2=0;
    }
}
static int cp_ready(const cc_source_t *s)
{
    const cc_generation_t *g=&s->generation;
    if(!cp_active(g) || g->seed_mask!=15 || g->seed>=NPAT || g->step>=NPAT ||
       g->block>=PCOPY_COUNT || g->length>=PCOPY_COUNT || pc_group(g->block)!=pc_group(g->length))return CG_CONTEXT;
    int rc=ui_cc_action_ready(ACT_COPY);
    if(rc)return rc==CA_PROTECTED?CG_PROTECTED:rc==CA_STOP?CG_STOP:rc==CA_QUEUE?CG_QUEUE:CG_OWNER;
    if(ui_saves_held())return CG_OWNER;
    return CG_OK;
}
static POLY_COLD int cp_apply(cc_source_t *s,int field,uint32_t value)
{
    cc_generation_t *g=&s->generation;
    if(field>=36 && field<=39){
        cp_family(s);unsigned bit=1u<<(field-36);uint32_t old=field==36?g->seed:field==37?g->step:field==38?g->block:g->length;
        if(old!=value || !(g->seed_mask&bit)){
            cg_changed(s);if(field==36)g->seed=value;else if(field==37)g->step=(uint8_t)value;else if(field==38)g->block=(uint8_t)value;else g->length=(uint8_t)value;
            g->seed_mask|=(uint8_t)bit;
        }
        return CC_APPLIED;
    }
    if(!cp_active(g))return cg_refuse(s,CG_CONTEXT);
    if(field==8 || field==9){
        unsigned shift=field==8?0:7;uint16_t request=(uint16_t)((g->request&~(127u<<shift))|(value<<shift));
        if(g->request!=request){g->flags&=(uint8_t)~CG_ARM;g->request=request;if(cg_generation!=UINT32_MAX)cg_generation++;}
        g->flags|=(uint8_t)(field==8?CG_TOKEN_LO:CG_TOKEN_HI);return CC_APPLIED;
    }
    if(field==11 || (field==10&&!value) || (field==40&&!value)){
        if(value || field!=11){if((g->flags&CG_ARM)||!(g->flags&CG_LAST)){g->state=CG_CANCELLED;g->reason=CG_CANCEL;}g->flags&=(uint8_t)~CG_ARM;if(cg_generation!=UINT32_MAX)cg_generation++;}
        return CC_APPLIED;
    }
    if(field==40){
        if((g->flags&(CG_TOKEN_LO|CG_TOKEN_HI))!=(CG_TOKEN_LO|CG_TOKEN_HI))return cg_refuse(s,CG_CONTEXT);
        if(cg_generation==UINT32_MAX)return cg_refuse(s,CG_STALE);
        if(g->flags&CG_LAST){
            if(g->request<g->last_request)return cg_refuse(s,CG_STALE);
            if(g->request==g->last_request)return (g->flags&CG_CHANGED)?cg_refuse(s,CG_STALE):CC_APPLIED;
        }
        int reason=cp_ready(s);if(reason)return cg_refuse(s,reason);
        cc_action_guard_t guard={variant_current_revision(),seq.transport_epoch,project_fingerprint()};
        int rc=ui_pattern_copy_checked(g->seed,g->step,g->block,g->length,&guard,0);
        if(rc!=VR_OK)return cg_refuse(s,rc==VR_FULL?CG_COPY_ROOM:rc==VR_PROTECTED?CG_PROTECTED:rc==VR_BUSY?CG_OWNER:CG_CONTEXT);
        reason=cp_ready(s);if(reason)return cg_refuse(s,reason);
        s->guard=guard;s->armed=0;g->flags=(uint8_t)((g->flags&~CG_CHANGED)|CG_ARM|CG_LAST);
        g->last_request=g->result_request=g->request;g->state=CG_ACCEPTED;g->reason=CG_OK;
        if(cg_generation!=UINT32_MAX)cg_generation++;return CC_APPLIED;
    }
    if(field!=41)return cg_refuse(s,CG_VALUE);
    if(!value){g->flags&=(uint8_t)~CG_HIGH;return CC_APPLIED;}
    if(g->flags&CG_HIGH)return CC_APPLIED;g->flags|=CG_HIGH;
    if((g->flags&CG_LAST)&&g->request==g->last_request&&!(g->flags&CG_ARM))return (g->flags&CG_CHANGED)?cg_refuse(s,CG_STALE):(g->state==CG_APPLIED||g->state==CG_NO_CHANGE)?CC_APPLIED:CC_REJECTED;
    if(!(g->flags&CG_ARM))return cg_refuse(s,CG_CONTEXT);
    int reason=cp_ready(s);if(reason)return cg_refuse(s,reason);
    if(s->guard.revision!=variant_current_revision() || s->guard.transport_epoch!=seq.transport_epoch || s->guard.fingerprint!=project_fingerprint())return cg_refuse(s,CG_STALE);
    int rc=ui_pattern_copy_checked(g->seed,g->step,g->block,g->length,&s->guard,1);
    g->flags=(uint8_t)((g->flags&~(CG_ARM|CG_CHANGED))|CG_LAST);g->result_request=g->request;
    if(rc!=VR_OK)return cg_refuse(s,rc==VR_FULL?CG_COPY_ROOM:rc==VR_PROTECTED?CG_PROTECTED:rc==VR_BUSY?CG_OWNER:CG_STALE);
    g->state=project_fingerprint()==s->guard.fingerprint?CG_NO_CHANGE:CG_APPLIED;g->reason=CG_OK;s->guard.transport_epoch=variant_current_revision();
    if(cg_generation!=UINT32_MAX)cg_generation++;return CC_APPLIED;
}
static POLY_COLD int cp_snapshot(const cc_source_t *s,uint32_t *w)
{
    const cc_generation_t *g=&s->generation;if(!cp_active(g))return -1;
    unsigned n=5,refs=0,selected=0,source=0;int valid=g->seed_mask==15 && g->seed<NPAT && g->step<NPAT && g->block<PCOPY_COUNT && g->length<PCOPY_COUNT && pc_group(g->block)==pc_group(g->length);
    uint32_t fp=project_fingerprint();
    if(valid){
        for(unsigned bar=0;bar<proj.arr.song.len;bar++)for(int part=0;part<NPARTS;part++)if((!g->length || part==pc_scope_part(g->length)) && proj.arr.song.bar[bar].pat[part]==g->step){refs++;break;}
        for(int i=0;i<NLANE;i++){
            const lane_t *l=&proj.arr.lane[i];int cross=g->block!=g->length;
            if(pc_motion_scope(l,g->seed,g->block) && (!cross || pc_probability(l,g->block)))source++;
            if(pc_motion_scope(l,g->step,g->length) && (!cross || pc_probability(l,g->length)))selected++;
        }
    }
    w[n++]=g->seed;w[n++]=g->step;w[n++]=g->block;w[n++]=g->length;w[n++]=g->seed_mask;
    w[n++]=valid && (g->flags&CG_ARM) && !cp_ready(s) && s->guard.revision==variant_current_revision() && s->guard.transport_epoch==seq.transport_epoch && s->guard.fingerprint==fp;
    w[n++]=fp;w[n++]=refs;w[n++]=source;w[n++]=selected;w[n++]=valid && g->seed==g->step && g->block==g->length;
    return (int)n;
}
