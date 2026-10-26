/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-only generation controls. No UI selection, flash or transport writes. */
enum { CG_ARM=1,CG_HIGH=2,CG_LAST=4,CG_CHANGED=8,CG_TOKEN_LO=16,CG_TOKEN_HI=32,CG_VAR=64,CG_CAPTURE=128 };
enum { CG_IDLE,CG_ACCEPTED,CG_PENDING,CG_APPLIED,CG_NO_CHANGE,CG_REJECTED,CG_UNSUPPORTED,CG_CANCELLED };
enum { CG_OK,CG_CONTEXT,CG_STALE,CG_STOP,CG_QUEUE,CG_PROTECTED,CG_OWNER,CG_VALUE,CG_MISSING,CG_MUTATE,CG_UNDO,CG_CANCEL,CG_INCOMPLETE,CG_CAPTURE_UNSUPPORTED,CG_DESTINATION,CG_COPY_ROOM };
static uint32_t cg_generation X0X_POOL;
static void cf_cancel_pending(cc_source_t *s);
static int cp_apply(cc_source_t *s,int field,uint32_t value);
static int cp_snapshot(const cc_source_t *s,uint32_t *w);
static int cf_apply(cc_source_t *s,int field,uint32_t value);
static int cf_snapshot(const cc_source_t *s,unsigned page,unsigned index,uint32_t *w);
static void cg_changed(cc_source_t *s)
{
    s->generation.flags=(uint8_t)((s->generation.flags&~CG_ARM)|CG_CHANGED);
    if(cg_generation!=UINT32_MAX)cg_generation++;
}
static int cg_context(const cc_source_t *s)
{
    if((s->generation.drum && !(s->generation.flags&CG_VAR)) || ((s->generation.flags&(CG_VAR|CG_CAPTURE)) && s->applied.part<2))return s->ready && s->applied.part<2 && s->applied.kit==s->applied.part &&
        s->applied.drum<11 && s->applied.strip==s->applied.kit*11+s->applied.drum;
    return s->ready && s->applied.part>=2 && s->applied.part<=3 &&
        s->applied.bass==s->applied.part-2 && s->applied.strip==22+s->applied.bass;
}
static int cg_refuse(cc_source_t *s,int reason)
{
    cf_cancel_pending(s);
    s->generation.state=CG_REJECTED;s->generation.reason=(uint8_t)reason;
    s->generation.result_request=s->generation.request;
    if(!(s->generation.flags&CG_LAST) || s->generation.request>s->generation.last_request)s->generation.last_request=s->generation.request;
    s->guard.transport_epoch=variant_current_revision(); /* unarmed:receipt revision */
    s->generation.flags=(uint8_t)((s->generation.flags&~CG_ARM)|CG_LAST);
    if(cg_generation!=UINT32_MAX)cg_generation++;
    return CC_REJECTED;
}
/* One active recipe family per source; switching explicitly invalidates arm,
 * seed validity and the old recipe, while preserving monotonic request history. */
static void cg_family(cc_source_t *s,unsigned drum){
    cc_generation_t *g=&s->generation;
    if((g->flags&(CG_VAR|CG_CAPTURE)) || g->drum!=drum){cf_cancel_pending(s);cg_changed(s);g->flags&=(uint8_t)~(CG_VAR|CG_CAPTURE);g->drum=(uint8_t)drum;g->seed=0;g->seed_mask=0;
        g->mode=g->step=g->length=0;g->block=(uint8_t)(drum?50:0);}
}
/* A variation recipe reuses the existing16B source union,never project data.
 * seed packs raw seed[0:7],shape/range[8:10],constraint[11:12],block[16:23]. */
static void cv_family(cc_source_t *s){
    cc_generation_t *g=&s->generation;
    if(!(g->flags&CG_VAR)){cf_cancel_pending(s);cg_changed(s);g->flags=(uint8_t)((g->flags&~CG_CAPTURE)|CG_VAR);g->drum=0;g->mode=0;g->block=1;g->length=2;g->step=3;g->seed=0;g->seed_mask=0;}
}
static POLY_COLD int cg_apply(cc_source_t *s,int field,uint32_t value)
{
    cc_generation_t *g=&s->generation;
    if(field>=36 || ((g->flags&CG_CAPTURE) && g->drum && field>=8 && field<=11 && !(field==10 && (value==6 || value==7))))return cp_apply(s,field,value);
    if(field>=29 || (field==10 && (value==6 || value==7)) || ((g->flags&CG_CAPTURE) && field>=8 && field<=11))return cf_apply(s,field,value);
    if((field>=18 && field<=24) || field==27 || field==28){
        cv_family(s);uint32_t old=g->seed;unsigned shift=0,mask=0;int changed=0;
        if(field==18){changed=g->block!=value;g->block=(uint8_t)value;}
        else if(field==19){changed=g->length!=value;g->length=(uint8_t)value;}
        else if(field==20){changed=g->step!=value;g->step=(uint8_t)value;}
        else if(field==22){changed=g->mode!=value;g->mode=(uint8_t)value;}
        else{shift=field==21?0:field==23?8:field==24?9:field==27?11:16;
            mask=field==21||field==28?255u:field==23?1u:3u;
            g->seed=(g->seed&~(mask<<shift))|(value<<shift);changed=g->seed!=old;
            if(field==21){changed|=!g->seed_mask;g->seed_mask=1;}}
        if(changed)cg_changed(s);return CC_APPLIED;
    }
    if(field>=13 && field<=16){
        cg_family(s,1);
        if(field==16){if(g->seed!=value+1u || g->seed_mask!=1){cg_changed(s);g->seed=value+1u;g->seed_mask=1;}}
        else{uint8_t current=field==13?g->block:field==14?g->mode:g->step;
            if(current!=value){cg_changed(s);if(field==13)g->block=(uint8_t)value;else if(field==14)g->mode=(uint8_t)value;else g->step=(uint8_t)value;}}
        return CC_APPLIED;
    }
    if(field<8){
        if(field!=2 || (g->flags&(CG_VAR|CG_CAPTURE)))cg_family(s,0);
        if(field==0){if(g->mode!=value){cg_changed(s);g->mode=(uint8_t)value;}}
        else if(field<4){uint8_t *p=field==1?&g->block:field==2?&g->length:&g->step;
            if(*p!=value){cg_changed(s);*p=(uint8_t)value;}
        }else{unsigned shift=(unsigned)(field-4)*8;uint32_t seed=(g->seed&~(255u<<shift))|(value<<shift);
            if(seed!=g->seed || !(g->seed_mask&(1u<<(field-4)))){cg_changed(s);g->seed=seed;g->seed_mask|=(uint8_t)(1u<<(field-4));}}
        return CC_APPLIED;
    }
    if(field==8 || field==9){
        unsigned shift=field==8?0:7;uint16_t request=(uint16_t)((g->request&~(127u<<shift))|(value<<shift));
        if(g->request!=request){g->flags&=(uint8_t)~CG_ARM;g->request=request;if(cg_generation!=UINT32_MAX)cg_generation++;}
        g->flags|=(uint8_t)(field==8?CG_TOKEN_LO:CG_TOKEN_HI);return CC_APPLIED;
    }
    if(field==11 || (field==10&&!value)){
        if(value || field==10){if((g->flags&CG_ARM)||!(g->flags&CG_LAST)){g->state=CG_CANCELLED;g->reason=CG_CANCEL;}g->flags&=(uint8_t)~CG_ARM;if(cg_generation!=UINT32_MAX)cg_generation++;}
        return CC_APPLIED;
    }
    if(field==10){
        if(value==4 || value==5){cv_family(s);if(g->drum!=(value==5)){cg_changed(s);g->drum=(uint8_t)(value==5);}}
        if(value!=(uint32_t)((g->flags&CG_VAR)?(g->drum?5:4):g->drum?3:2)){cg_refuse(s,CG_VALUE);g->state=CG_UNSUPPORTED;return CC_UNSUPPORTED;}
        if(!cg_context(s) || (g->flags&(CG_TOKEN_LO|CG_TOKEN_HI))!=(CG_TOKEN_LO|CG_TOKEN_HI))return cg_refuse(s,CG_CONTEXT);
        if(g->flags&CG_VAR){
            if(!g->drum && g->seed_mask!=1)return cg_refuse(s,CG_VALUE);
            if(g->drum && seq.variation.lane!=255 && (seq.variation.lane!=s->applied.strip || seq.variation.pattern!=s->applied.pattern))return cg_refuse(s,CG_CONTEXT);
        }else if(g->seed_mask!=(g->drum?1:15) || !g->seed)return cg_refuse(s,CG_VALUE);
        if(cg_generation==UINT32_MAX)return cg_refuse(s,CG_STALE);
        if(g->flags&CG_LAST){
            if(g->request<g->last_request)return cg_refuse(s,CG_STALE);
            if(g->request==g->last_request)return (g->flags&CG_CHANGED)?cg_refuse(s,CG_STALE):CC_APPLIED;
        }
        int reason=ui_cc_action_arm(ACT_COPY,&s->guard);
        if(reason)return cg_refuse(s,reason==CA_PROTECTED?CG_PROTECTED:reason==CA_STOP?CG_STOP:reason==CA_QUEUE?CG_QUEUE:CG_OWNER);
        if(g->flags&CG_VAR)s->guard.fingerprint=variation_runtime_fingerprint();
        s->armed=0;g->flags=(uint8_t)((g->flags&~CG_CHANGED)|CG_ARM|CG_LAST);g->last_request=g->request;
        g->result_request=g->request;g->state=CG_ACCEPTED;g->reason=CG_OK;
        if(cg_generation!=UINT32_MAX)cg_generation++;
        return CC_APPLIED;
    }
    if(field==25 || field==26){if(!(g->flags&CG_VAR) || (g->flags&CG_CAPTURE) || (field==26)!=g->drum)return cg_refuse(s,CG_CONTEXT);}
    else if(field==12 || field==17){if((g->flags&(CG_VAR|CG_CAPTURE)) || (field==17)!=g->drum)return cg_refuse(s,CG_CONTEXT);}
    else return CC_UNSUPPORTED;
    if(!value){g->flags&=(uint8_t)~CG_HIGH;return CC_APPLIED;}
    if(g->flags&CG_HIGH)return CC_APPLIED;
    g->flags|=CG_HIGH;
    if((g->flags&CG_LAST)&&g->request==g->last_request && !(g->flags&CG_ARM))
        return (g->flags&CG_CHANGED)?cg_refuse(s,CG_STALE):(g->state==CG_APPLIED||g->state==CG_NO_CHANGE)?CC_APPLIED:CC_REJECTED;
    if(!(g->flags&CG_ARM) || !cg_context(s))return cg_refuse(s,CG_CONTEXT);
    int reason=ui_cc_action_ready(ACT_COPY);
    if(reason)return cg_refuse(s,reason==CA_PROTECTED?CG_PROTECTED:reason==CA_STOP?CG_STOP:reason==CA_QUEUE?CG_QUEUE:CG_OWNER);
    if(!engine_transport_reserve())return cg_refuse(s,CG_STOP);
    if(s->guard.revision!=variant_current_revision() || s->guard.transport_epoch!=seq.transport_epoch ||
       s->guard.fingerprint!=((g->flags&CG_VAR)?variation_runtime_fingerprint():project_fingerprint())){
        engine_transport_release();return cg_refuse(s,CG_STALE);
    }
    /* ui_generate_random checks the same stopped ownership under our transport
     * reservation. Its transport_hold preflight is waived only in this wrapper. */
    int rc=(g->flags&CG_VAR)?ui_variation_remote(s->applied.pattern,s->applied.strip,g->block,g->length,g->seed&255u,g->step,g->mode,(g->seed>>8)&7u,(g->seed>>11)&3u,(g->seed>>16)&255u,g->drum):g->drum?ui_generate_drum_remote(s->applied.pattern,s->applied.kit,s->applied.drum,g->block,g->mode,g->step,g->seed,g->length):ui_generate_remote(s->applied.pattern,s->applied.bass,g->mode,g->block,g->seed,g->length,g->step);
    uint32_t after=(g->flags&CG_VAR)?variation_runtime_fingerprint():project_fingerprint();engine_transport_release();
    g->flags=(uint8_t)((g->flags&~(CG_ARM|CG_CHANGED))|CG_LAST);
    g->last_request=g->request;g->result_request=g->request;s->guard.transport_epoch=variant_current_revision();
    if(rc==VR_OK){g->state=(after==s->guard.fingerprint)?CG_NO_CHANGE:CG_APPLIED;g->reason=CG_OK;}
    else{g->state=CG_REJECTED;g->reason=rc==VR_PROTECTED?CG_PROTECTED:rc==VR_UNSUPPORTED?CG_MUTATE:rc==VR_MISSING?CG_MISSING:rc==VR_FULL?CG_UNDO:rc==VR_BUSY?CG_OWNER:CG_VALUE;}
    if(cg_generation!=UINT32_MAX)cg_generation++;
    return rc==VR_OK?CC_APPLIED:CC_REJECTED;
}
void cc_generation_reset(void)
{
    unsigned i;for(i=0;i<CC_SOURCES;i++)memset(&cc_sources[i].generation,0,sizeof(cc_generation_t));
    if(cg_generation!=UINT32_MAX)cg_generation++;
}
/* Snapshot uses only main-owned values. Caller adds CRC and request echo. */
POLY_COLD int cc_generation_snapshot(unsigned source,unsigned page,unsigned index,uint32_t *w)
{
    unsigned n=0;if(source>=CC_SOURCES || (index && page!=6) || cg_generation==UINT32_MAX)return -1;
    const cc_source_t *s=&cc_sources[source];const cc_generation_t *g=&s->generation;
    w[n++]=variant_current_revision();w[n++]=engine_queue_written();w[n++]=engine_queue_read();
    w[n++]=seq.transport_epoch;w[n++]=cg_generation;
    if(page==10)return cp_snapshot(s,w);
    if(page==5 || page==6)return cf_snapshot(s,page,index,w);
    if(page==0){w[n++]=1;w[n++]=1;w[n++]=63;w[n++]=CC_SOURCES;w[n++]=9000128;}
    else if(page==1){w[n++]=s->applied.pattern;w[n++]=s->applied.part;w[n++]=s->applied.strip;
        w[n++]=s->ready;w[n++]=(g->flags&CG_ARM)!=0;w[n++]=g->request;w[n++]=g->seed_mask;}
    else if(page==2){if(g->drum || (g->flags&(CG_VAR|CG_CAPTURE)))return -1;w[n++]=g->mode;w[n++]=g->block;w[n++]=g->length;w[n++]=g->step;w[n++]=g->seed;}
    else if(page==3){if(!g->drum || (g->flags&(CG_VAR|CG_CAPTURE)))return -1;w[n++]=g->block;w[n++]=g->mode;w[n++]=g->step;
        w[n++]=g->seed_mask?g->seed-1u:0;w[n++]=g->seed;w[n++]=g->length;w[n++]=g->seed_mask!=0;}
    else if(page==8){if(!(g->flags&CG_VAR))return -1;
        w[n++]=g->block;w[n++]=g->length;w[n++]=g->step;w[n++]=g->seed&255u;w[n++]=g->mode;
        w[n++]=(g->seed>>8)&1u;w[n++]=(g->seed>>9)&3u;w[n++]=(g->seed>>11)&3u;
        w[n++]=(g->seed>>16)&255u;w[n++]=g->seed_mask!=0;w[n++]=g->drum;}
    else if(page==9){const pattern_variation_t *v=&seq.variation;
        w[n++]=v->lane;w[n++]=v->pattern;w[n++]=v->every;w[n++]=v->amount;w[n++]=v->seed;w[n++]=v->anchor;
        w[n++]=v->mode;w[n++]=v->options&1u;w[n++]=v->options>>1;w[n++]=v->notes;w[n++]=v->low;w[n++]=v->high;
        w[n++]=s->ready && v->lane==s->applied.strip && v->pattern==s->applied.pattern;}
    else if(page==7){w[n++]=ui.cc_mode;w[n++]=(g->flags&CG_CAPTURE)?(g->drum?4:3):(g->flags&CG_VAR)?2:g->drum;w[n++]=cc_stats.accepted;w[n++]=cc_stats.rejected;
        w[n++]=cc_stats.unsupported;w[n++]=cc_stats.busy;w[n++]=cc_stats.coalesced;w[n++]=cc_stats.overflow;}
    else if(page==4){w[n++]=(g->flags&CG_LAST)!=0;w[n++]=g->result_request;w[n++]=g->request;
        w[n++]=g->state;w[n++]=g->reason;w[n++]=s->guard.revision;w[n++]=(g->flags&CG_ARM)?0:s->guard.transport_epoch;
        w[n++]=s->guard.fingerprint;w[n++]=(g->flags&CG_CHANGED)!=0;}
    else return -1;
    return (int)n;
}

#include "cc_capture.inc"

#include "cc_copy.inc"
