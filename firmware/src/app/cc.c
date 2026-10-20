/* SPDX-License-Identifier: GPL-3.0-only */
/* Included after ui.c in the platform unity TU. All entrypoints are main-only.
 * Canonical writes are bounded and IRQ-excluded; the existing CQ remains main
 * producer/audio consumer. No new CQ type and no per-CC storage writes. */
#include "cc.h"
#include "../seq/rate_families.h"
#include "cc_bindings.h"
cc_stats_t cc_stats;
int cc_recover(void){if(!engine_queue_space())return 0;seq_stop(&seq);return engine_cc_refresh(1);}
typedef struct {uint8_t pattern,part,drum,bass,step,block_low,block_high,motion_low,motion_high,kit,strip,seed_byte;} cc_context_t;
typedef struct {cc_context_t draft,applied;uint16_t parameter;uint32_t exact;uint8_t ready,exact_parts,action_high;cc_action_guard_t guard;uint16_t rate_tag;uint8_t rate_bin;uint16_t native_high;uint8_t armed,destination_draft,destination_applied,destination_ready;} cc_source_t;
static cc_source_t cc_sources[CC_SOURCES];
static uint8_t cc_action_reason[CC_SOURCES];
void cc_actions_reset(void){ui_cc_job_reset();unsigned s;for(s=0;s<CC_SOURCES;s++)cc_sources[s].armed=0;}
void cc_reset_source(uint8_t source){if(source<CC_SOURCES){plat_midi_clear_cc_source(source);plat_master_detach(source);memset(&cc_sources[source],0,sizeof cc_sources[source]);cc_action_reason[source]=CA_OK;}}
uint32_t cc_source_field(uint8_t source,unsigned field){
    if(source>=CC_SOURCES)return 0xffffffffu;
    if(field<12)return ((const uint8_t *)&cc_sources[source].applied)[field];
    if(field==12)return cc_sources[source].ready;
    if(field==13)return cc_sources[source].parameter;
    if(field==14)return cc_sources[source].exact_parts;
    if(field==15)return cc_sources[source].exact;
    if(field<28)return ((const uint8_t *)&cc_sources[source].draft)[field-16];
    if(field==28)return cc_sources[source].native_high;
    if(field==29)return cc_sources[source].armed;
    if(field==30)return cc_sources[source].destination_applied;
    if(field==31)return cc_action_reason[source];
    return 0xffffffffu;
}
static int cc_lookup(uint16_t number,uint32_t *word){
    unsigned low=0,high=CC_RUN_COUNT;
    if(number>=CC_STRIP_FIRST && number<CC_STRIP_LIMIT){
        unsigned offset=number-CC_STRIP_FIRST,field=offset%CC_STRIP_STRIDE;
        if(field>=CC_STRIP_FIELDS)return CC_UNSUPPORTED;
        *word=CCK_STRIP|((offset/CC_STRIP_STRIDE)<<5)|(field<<14);return CC_APPLIED;
    }
    while(low<high){unsigned mid=(low+high)/2;if(cc_binding_runs[mid].first<=number)low=mid+1;else high=mid;}
    if(!low)return CC_UNSUPPORTED;
    const cc_binding_run_t *r=&cc_binding_runs[low-1];unsigned offset=number-r->first;
    if(offset>=r->count)return CC_UNSUPPORTED;
    *word=r->word+(r->shift==255?0:offset<<r->shift);return CC_APPLIED;
}
int cc_coalescable(uint32_t packet){
    uint32_t word;unsigned cc=(packet>>16)&127u,number=((packet>>8)&15u)*88u+(cc<32?cc:cc-32);
    if(cc>=120||(cc>=32&&cc<64)||cc_lookup((uint16_t)number,&word))return 0;
    return (word&31u)==CCK_SOUND || ((word&31u)==CCK_STRIP && ((word>>14)&31u)<5);
}
static int cc_block(const cc_context_t *x){return x->block_low+128*x->block_high;}
static int cc_lane(const cc_context_t *x){return x->motion_low+128*x->motion_high;}
static int cc_context_valid(const cc_context_t *x){
    return x->pattern<NPAT&&x->part<NPARTS&&x->drum<NDRUM&&x->bass<NBASS&&x->step<NSTEPS&&
        cc_block(x)<NSONG&&cc_lane(x)<NLANE&&x->kit<NKIT&&x->strip<VARIANT_STRIPS&&x->seed_byte<4;
}
static int cc_sparse(uint32_t raw,int fine,int kind,int field){
    uint32_t count=kind==CCK_PAD&&!field?17u:8u;
    unsigned index=(raw*(count-1)+(fine?8191u:63u))/(fine?16383u:127u);
    return index==count-1?255:(int)index;
}
static POLY_COLD int cc_max(int kind,int a,int b,int c,const cc_context_t *x){
    static const uint8_t genmax[]={100,100,100,2,11,TB3PO_NSCALES-1,4,16};
    static const uint8_t selectmax[]={31,4,10,1,63,127,1,127,1,1,24,3,1,127,15,31};
    static const uint8_t stepmax[]={1,1,127,2,1,1,1,8};
    static const uint8_t prefmax[]={1,1,1,7,1,5,127,127,191,127};
    const x0x_param_t *p;
    switch(kind){
    case CCK_SOUND:p=engine_param(a,b,c);return p?p->max:-1;
    case CCK_STRIP:return c<2?20000:c==2?24:c==3?48:c==4?250:c==5?15:1;
    case CCK_SEQ:return ((proj.variant.arrangement.flags&POLY_FLAG) && (c==SQ_DRATE || c==SQ_BRATE))?7:c<NSQ?SEQ_P[c].max:-1;
    case CCK_GEN:return c<NGEN?genmax[c]:-1;
    case CCK_BRKGEN:p=breaks_param(c);return p?p->max:-1;
    case CCK_CHANNEL:return 16;
    case CCK_PREF:return c<10?prefmax[c]:-1;
    case CCK_SONG:return c<5?31:192;
    case CCK_SELECTOR:return c<16?selectmax[c]:-1;
    case CCK_STEP:return c<8?stepmax[c]:-1;
    case CCK_MOTION:
        if(!c){const lane_t *l=&proj.arr.lane[cc_lane(x)];if(!l->used)return -1;
            if(l->t==T_PROB)return PROB_COND+PROB_NCOND-1;
            p=engine_param(l->t,l->v,l->i);return p?p->max:-1;}
        return c==1?T_PROB:c==2?127:c==3?15:c==4?31:c==5?4:1;
    case CCK_PROB:return PROB_COND+PROB_NCOND-1;
    case CCK_PERF:return c==2||c==3||c==4?31:1;
    case CCK_GENACTION:return 1;
    case CCK_SEED:return 255;
    case CCK_ARR:return c==0?1:c==1?192:c==2?11:c==3?HARMONY_CHROMATIC:c==4?255:c==5?7:c==6?32:c==7?31:c==8?1:191;
    case CCK_FOLLOW:return c==0?3:127;
    case CCK_PAD:return c==0?255:127;
    case CCK_EXACT:return c==2?3:c==3?1:127;
    case CCK_ACTION:return c>=1&&c<=11?127:-1;
    case CCK_ACTIONCTL:return c?1:11;
    case CCK_MASTER:return 4096;
    case CCK_TRANSPORT:case CCK_HISTORY:return 1;
    case CCK_RATE:return c==0?1:c==1?20:c==2?3:c==3?1:c==4?2:127;
    case CCK_LANE:return c==0?64:c==1?7:1;
    default:return -1;
    }
}
/* Desired arrangement bytes have one main-context owner; IRQs only read them.
 * Song bytes differ: legacy recording may update those, so their masked writes
 * read the current byte under the publication lock. No full candidate copy or
 * harmony-table validation runs with IRQs excluded. */
static POLY_COLD int cc_arrangement(int kind,int field,uint32_t value,const cc_context_t *x){
    arrangement_settings_t candidate;
    uint8_t *changed=0;uint32_t saved;int block=cc_block(x);
    if(!(kind==CCK_ARR && (field==1 || field==9))){
        candidate=proj.variant.arrangement;
        if(kind==CCK_FOLLOW){
            if(!field){unsigned shift=1+2*x->bass;changed=&candidate.flags;value=(candidate.flags&~(3u<<shift))|(value<<shift);}
            else changed=field==1?&candidate.bass_low[x->bass]:&candidate.bass_high[x->bass];
        }else if(kind==CCK_PAD)changed=(&candidate.pad_channel)+field;
        else if(!field){changed=&candidate.flags;value=(candidate.flags&~ARR_ENABLED)|(value?ARR_ENABLED:0);}
        else if(field==2)changed=&candidate.key;
        else if(field==3)changed=&candidate.mode;
        else if(field==4)changed=&candidate.parent;
        else if(field==5){changed=&candidate.degree[block];value|=candidate.degree[block]&24u;}
        if(changed)*changed=(uint8_t)value;
        if(!arrangement_valid(&candidate))return CC_REJECTED;
    }
    saved=plat_control_enter();
    if(engine_queue_space()<(uint32_t)(kind==CCK_ARR && field==9?2:1)){
        plat_control_leave(saved);return CC_BUSY;
    }
    if(kind==CCK_PAD && !seq_pad_channel_available(&seq,candidate.pad_channel)){
        plat_control_leave(saved);return CC_REJECTED;
    }
    if(kind==CCK_ARR && field==9){
        if(value>=proj.arr.song.len || !(proj.variant.arrangement.flags&ARR_ENABLED)){
            plat_control_leave(saved);return CC_REJECTED;
        }
        (void)engine_arrangement_queue((uint8_t)value);
    }else if(changed){
        ((uint8_t *)&proj.variant.arrangement)[changed-(uint8_t *)&candidate]=(uint8_t)value;
    }else if(field==1)proj.arr.song.len=(uint16_t)value;
    else if(field==6){
        unsigned bars=value-1;
        proj.arr.song.bar[block].mute=(uint8_t)((proj.arr.song.bar[block].mute&31u)|((bars&7u)<<5));
        proj.variant.arrangement.degree[block]=(uint8_t)((proj.variant.arrangement.degree[block]&7u)|(bars&24u));
        if(bars&24u)proj.variant.arrangement.flags|=ARR_EXTENDED;
    }
    else if(field==7)proj.arr.song.bar[block].pat[x->part]=(uint8_t)value;
    else if(field==8){unsigned mask=1u<<x->part;uint8_t *mute=&proj.arr.song.bar[block].mute;*mute=(uint8_t)((*mute&~mask)|(value?mask:0));}
    (void)engine_cc_refresh(2); /* capacity reserved, sole main producer */
    plat_control_leave(saved);ui_external_changed();return CC_APPLIED;
}
static POLY_COLD int cc_apply(int kind,int a,int b,int c,uint32_t value,cc_source_t *source){
    cc_context_t *x=&source->applied;pattern_t *p=&proj.pat[x->pattern];int k,block=cc_block(x),lane=cc_lane(x);
    uint32_t saved;lane_t target;
    if(kind==CCK_SELECTOR){
        if(c<13 || c==15)source->armed=0;
        if(c==15){source->destination_draft=(uint8_t)value;source->destination_ready|=1u;source->ready=0;source->exact_parts=0;return 0;}
        if(c<12){((uint8_t *)&source->draft)[c]=(uint8_t)value;source->ready=0;source->exact_parts=0;return 0;}
        if(c==12){if(!value)return 0;if(!cc_context_valid(&source->draft))return CC_REJECTED;source->applied=source->draft;source->destination_applied=source->destination_draft;source->destination_ready=(uint8_t)((source->destination_ready&1u)?3:0);source->ready=1;source->exact_parts=0;return 0;}
        if(c==13)source->parameter=(uint16_t)((source->parameter&~127u)|value);
        else source->parameter=(uint16_t)((source->parameter&127u)|(value<<7));
        source->exact_parts=0;return 0;
    }
    if(kind==CCK_EXACT){
        if(c<3||c==4){unsigned chunk=c==4?2u:(unsigned)c,shift=chunk*7;source->exact=(source->exact&~(127u<<shift))|(value<<shift);source->exact_parts|=(uint8_t)(1u<<chunk);return 0;}
        if(!value)return 0;
        if(source->exact_parts!=7 || !source->ready)return CC_REJECTED;
        uint32_t target;int result=cc_lookup(source->parameter,&target);
        /* Framing is only accepted through its ordinary CC addresses. Exact
         * redispatch must neither recurse into commit nor alter its own context. */
        if(!result && ((target&31u)==CCK_EXACT || (target&31u)==CCK_SELECTOR))result=CC_REJECTED;
        else if(!result)result=cc_parameter(source->parameter,source->exact,(uint8_t)(source-cc_sources));
        /* The ingress retries this same commit after audio frees CQ space. */
        if(result!=CC_BUSY)source->exact_parts=0;
        return result;
    }
    if(kind==CCK_MASTER)return plat_master_remote(value,(unsigned)(source-cc_sources))?CC_REJECTED:CC_APPLIED;
    if(kind==CCK_ACTIONCTL || kind==CCK_ACTION){
        unsigned id=(unsigned)(source-cc_sources);int reason=CA_OK;
        if(kind==CCK_ACTIONCTL && ((c==0&&!value)||(c==1&&value))){source->armed=0;cc_action_reason[id]=CA_OK;return 0;}
        if(kind==CCK_ACTIONCTL && c==1)return 0;
        if(kind==CCK_ACTION && !value){source->native_high&=(uint16_t)~(1u<<(c-1));return 0;}
        if(kind==CCK_ACTION && (source->native_high&(1u<<(c-1))))return 0;
        int act=kind==CCK_ACTIONCTL?(int)value:c;
        int destructive=act==2||act==3||act==4||act==5||act==7||act==8||act==9;
        if(!source->ready || (act==4&&!(source->destination_ready&2u)))reason=CA_CONTEXT;
        else if(kind==CCK_ACTIONCTL && !destructive)reason=CA_INVALID;
        else reason=ui_cc_action_ready(act);
        if(!reason && kind==CCK_ACTIONCTL){reason=ui_cc_action_arm(act,&source->guard);if(!reason)source->armed=(uint8_t)act;}
        else if(!reason){
            if(destructive && source->armed!=act)reason=CA_CONFIRM;
            else reason=ui_cc_action(act,x->pattern,x->part,source->destination_applied,block,destructive?&source->guard:0);
            if(!reason){source->native_high|=(uint16_t)(1u<<(c-1));source->armed=0;}
        }
        cc_action_reason[id]=(uint8_t)reason;return reason?CC_REJECTED:CC_APPLIED;
    }
    if(kind==CCK_GENACTION || kind==CCK_TRANSPORT || kind==CCK_HISTORY){
        unsigned bit=1u<<(kind==CCK_GENACTION?c:kind==CCK_TRANSPORT?2+c:5+c);
        if(!value){source->action_high&=(uint8_t)~bit;return 0;}
        if(source->action_high&bit)return 0;
        /* Set this only after preflight below. Busy/rejected packets remain
         * retryable and cannot consume the edge before canonical acceptance. */
    }
    if(kind==CCK_HISTORY){
        /* Owner/recording/transport/capacity refusal is terminal for this
         * request, not a retry barrier ahead of the same source's Stop. */
        if(ui_cc_undo(c)!=VR_OK)return CC_REJECTED;
        source->action_high|=(uint8_t)(1u<<(5+c));return CC_APPLIED;
    }
    if(kind==CCK_SOUND){
        int part=engine_motion_part(a,b);if(!engine_queue_space())return CC_BUSY;
        k=motion_find(proj.arr.lane,seq.ppat[part],part,a,b,c);
        if(ui.rec&&seq.playing&&k<0){k=motion_alloc(proj.arr.lane,seq.ppat[part],part,a,b,c);if(k<0)return CC_REJECTED;}
        ui_external_sound(a,b,c,(int)value);
        if(k>=0){if(ui.rec&&seq.playing)engine_motion_rec(k);else engine_motion_hold(k);}return 0;
    }
    if(kind==CCK_STRIP){
        sound_strip_config_t cfg=proj.variant.strip[a];
        if(c>=6){unsigned mask=1u<<(c-6);value=(cfg.flags&~mask)|(value?mask:0);c=VP_FLAGS;}
        if(!variant_field_set(&cfg,c,(int)value))return CC_REJECTED;
        if(!engine_strip_set(a,c,(int)value))return CC_BUSY;
        proj.variant.strip[a]=cfg;ui_external_changed();return 0;
    }
    if(kind==CCK_TRANSPORT){source->action_high|=(uint8_t)(1u<<(2+c));if(c==0)seq_start(&seq);else if(c==1)seq_stop(&seq);else ui.rec=1;return 0;}
    if(kind==CCK_RATE){
        if(!source->ready || !(proj.variant.arrangement.flags&POLY_FLAG))return CC_REJECTED;
        if(!c && (seq.playing||seq.req_stop||seq.req_start||seq.req_cont))return CC_REJECTED;
        if(c==5){
            if(!poly_expanded((const uint8_t *)&proj.sound))return CC_REJECTED;
            unsigned family=(unsigned)poly_rate_config(proj.pat,(const uint8_t *)&proj.sound,x->strip,0),h=(unsigned)poly_rate_config(proj.pat,(const uint8_t *)&proj.sound,x->strip,2);
            uint16_t tag=(uint16_t)(0x8000u|x->pattern|(x->strip<<5)|(family<<10));
            unsigned prior=source->rate_tag==tag?source->rate_bin:RF_INVALID,bin;
            if(prior!=RF_INVALID && rf_ordered(family,prior)!=(unsigned)poly_rate(proj.pat,(const uint8_t *)&proj.sound,x->pattern,x->strip))prior=RF_INVALID;
            bin=rf_sweep(family,value,h,prior);
            if(rf_ordered(family,bin)==(unsigned)poly_rate(proj.pat,(const uint8_t *)&proj.sound,x->pattern,x->strip)){source->rate_tag=tag;source->rate_bin=(uint8_t)bin;return CC_APPLIED;}
            if(!engine_queue_space())return CC_BUSY;
            if(!engine_rate_set(x->pattern,x->strip,1,(int)rf_ordered(family,bin)))return CC_REJECTED;
            source->rate_tag=tag;source->rate_bin=(uint8_t)bin;
        }else {if(!engine_queue_space())return CC_BUSY;if(!engine_rate_set(x->pattern,x->strip,c,(int)value))return CC_REJECTED;source->rate_tag=0;}
        ui_external_changed();return CC_APPLIED;
    }
    if(kind==CCK_LANE){
        if(c<2 && (!source->ready || !(proj.variant.arrangement.flags&POLY_FLAG)))return CC_REJECTED;
        /* Initial conversion is a structural stopped transaction. Ineligible
         * initialization is terminal: it must not pin ingress ahead of Stop.
         * Existing prepared Off/On and all timing fields remain live. */
        if(c==2 && value && !(proj.variant.arrangement.flags&POLY_FLAG) &&
           (seq.playing||seq.req_stop||seq.req_start||seq.req_cont))return CC_REJECTED;
        if(!engine_queue_space())return CC_BUSY;
        if(!(c==2?engine_lane_mode((int)value):engine_lane_set(x->pattern,x->strip,c,(int)value)))return CC_REJECTED;
        ui_external_changed();return CC_APPLIED;
    }
    /* Legacy part timing controls retain their fixed addresses, but cannot
     * write packed header bytes directly once the typed slab is initialized. */
    if(kind==CCK_SEQ && source->ready && (proj.variant.arrangement.flags&POLY_FLAG) && c>=SQ_DLEN && c<=SQ_BRATE){
        int selected=c<=SQ_DRATE?x->kit*11+x->drum:22+x->bass;
        int field=(c==SQ_DRATE || c==SQ_BRATE);
        if(!engine_queue_space())return CC_BUSY;
        if(!engine_lane_set(x->pattern,selected,field,(int)value+(field?0:1)))return CC_REJECTED;
        ui_external_changed();return CC_APPLIED;
    }
    if(kind==CCK_STEP && c>=6 && source->ready && (proj.variant.arrangement.flags&POLY_FLAG)){
        int gate=poly_break_gate(proj.pat,(const uint8_t *)&proj.sound,x->pattern,x->step);
        int slice=poly_break_slice(proj.pat,(const uint8_t *)&proj.sound,x->pattern,x->step);
        if(!engine_queue_space())return CC_BUSY;
        if(!engine_lane_break_step(x->pattern,x->step,c==6?(int)value:gate,c==7?(int)value:slice))return CC_REJECTED;
        ui_external_changed();return CC_APPLIED;
    }

    if(kind==CCK_UNSUPPORTED)return CC_UNSUPPORTED;
    if(!source->ready && !(kind==CCK_SEQ&&c>=7) && kind!=CCK_CHANNEL && !(kind==CCK_PREF&&c!=2&&c!=3))return CC_REJECTED;
    if(kind==CCK_ARR || kind==CCK_FOLLOW || kind==CCK_PAD)return cc_arrangement(kind,c,value,x);
    if(engine_queue_space()<(uint32_t)(kind==CCK_PREF&&c==9?3:1))return CC_BUSY;
    /* Canonical byte/word publication and existing CQ marker are one bounded
     * critical operation. Audio never observes half-written composite fields. */
    saved=plat_control_enter();
    switch(kind){
    case CCK_SEQ:
        switch(c){
        case SQ_SWING:p->swing=(uint8_t)value;break;
        case SQ_DLEN:p->drum[x->kit].len=(uint8_t)(value+1);break;
        case SQ_DRATE:p->drum[x->kit].rate=(uint8_t)value;break;
        case SQ_BLEN:p->bass[x->bass].len=(uint8_t)(value+1);break;
        case SQ_BRATE:p->bass[x->bass].rate=(uint8_t)value;break;
        case SQ_BDIR:p->bass[x->bass].dir=(uint8_t)value;break;
        case SQ_BTRANS:p->bass[x->bass].transpose=(uint8_t)value;break;
        case SQ_TEMPO:seq.bpm=(float)(value+20);proj.set.bpm_x10=(uint16_t)((value+20)*10);break;
        case SQ_ACCENT:seq.accent_q7=proj.set.accent_q7=(uint8_t)value;break;
        case SQ_CLK:seq.send_clock=proj.set.clk_out=(uint8_t)value;break;
        case SQ_NOTES:proj.variant.arrangement.flags=(uint8_t)((proj.variant.arrangement.flags&~ARR_NOTES_OUT)|(value?ARR_NOTES_OUT:0));break;
        case SQ_THEME:proj.set.palette=(uint8_t)value;palette_set(value);break;
        case SQ_KEYLED:proj.set.keyled=(uint8_t)value;break;
        case SQ_KEYSOUND:proj.set.keysound=(uint8_t)value;break;
        case SQ_AUTOSAVE:proj.set.autosave_off=(uint8_t)!value;break;
        default:goto invalid;
        }break;
    case CCK_GEN:if(c>=NGEN)goto invalid;*canonical_gen_byte(p,x->bass,c)=(uint8_t)(value+(c==G_OCTS));ui.gen_stale[x->bass]=1;break;
    case CCK_BRKGEN:p->brk.set[c]=(uint8_t)value;break;
    case CCK_CHANNEL:
        if(!engine_midi_channel_allowed(value?(int)value-1:255))goto invalid;
        seq.ch[a]=(uint8_t)(value?value-1:255);proj.set.midi_ch[a]=(uint8_t)(value?value:17);break;
    case CCK_PREF:
        if(c==0){proj.set.out_loud=(uint8_t)value;engine_out_loud(value);}
        else if(c==1){proj.set.usb_full=(uint8_t)value;engine_usb_full(value);}
        else if(c==2)ui.freeplay[x->bass]=(uint8_t)value;
        else if(c==3){proj.arr.song.bar[block].mute=(uint8_t)((proj.arr.song.bar[block].mute&31u)|(value<<5));proj.variant.arrangement.degree[block]&=7u;}
        else if(c==4){perf.stalls_on=(uint8_t)value;plat_stalls_enable(value);}
        else if(c==6||c==7){if(value>=(uint32_t)engine_brk_nslots())goto invalid; if(c==6)p->brk.slot_a=(uint8_t)value;else p->brk.slot_b=(uint8_t)value;}
        else if(c==8){source->draft.block_low=(uint8_t)(value&127u);source->draft.block_high=(uint8_t)(value>>7);source->ready=0;}
        else if(c==9){
            if(engine_queue_space()<2){plat_control_leave(saved);return CC_BUSY;}
            canonical_tape_set(value);}
        else goto invalid;
        break;
    case CCK_SONG:
        if(c<5)proj.arr.song.bar[block].pat[c]=(uint8_t)value;else proj.arr.song.len=(uint16_t)value;break;
    case CCK_STEP:
        if(c<2){uint32_t *row=c?p->drum[x->kit].accent:p->drum[x->kit].hit[x->drum];uint32_t mask=1u<<(x->step&31);if(value)row[x->step>>5]|=mask;else row[x->step>>5]&=~mask;}
        else if(c<6){bstep_t *step=&p->bass[x->bass].step[x->step];if(c==2)step->note=(uint8_t)value;else {unsigned mask=c==3?BS_GATE_MASK:c==4?BS_ACCENT:BS_SLIDE;step->flags=(uint8_t)((step->flags&~mask)|(c==3?value:value?mask:0));}}
        else {if(x->step>=16)goto invalid;if(c==6){if(value)p->brk.steps|=1u<<x->step;else p->brk.steps&=~(1u<<x->step);}else p->brk.slice[x->step]=(uint8_t)value;}break;
    case CCK_MOTION:
        target=proj.arr.lane[lane];if(!target.used)goto invalid;
        if(c==0){target.val[x->step]=(uint8_t)value;}
        else if(c==1)target.t=(uint8_t)value;else if(c==2)target.v=(uint8_t)value;else if(c==3)target.i=(uint8_t)value;
        else if(c==4)target.pat=(uint8_t)value;else if(c==5)target.part=(uint8_t)value;
        else if(c==6){if(value)engine_motion_rec(lane);break;}else if(c==7){if(value)engine_motion_hold(lane);break;}else if(c==8){if(value)motion_clear(proj.arr.lane,lane);break;}
        if(target.t==T_PROB){if(target.i||target.v>>4>=NPARTS||((target.v&15)>=NDRUM)||((target.v>>4)>=NKIT&&(target.v&15)))goto invalid;}
        else if(!engine_param(target.t,target.v,target.i))goto invalid;
        target.gen++;proj.arr.lane[lane]=target;break;
    case CCK_PROB:
        k=motion_find(proj.arr.lane,x->pattern,x->part,T_PROB,(x->part<<4)|(x->part<2?x->drum:0),0);
        if(k<0)k=motion_alloc(proj.arr.lane,x->pattern,x->part,T_PROB,(x->part<<4)|(x->part<2?x->drum:0),0);
        if(k<0){plat_control_leave(saved);return CC_REJECTED;}proj.arr.lane[k].val[x->step]=(uint8_t)value;break;
    case CCK_PERF:
        if(c==0||c==1){uint32_t bit=1u<<x->strip;if(c==0){if(value)seq.mute|=bit;else seq.mute&=~bit;}else seq.vmute=value?((1u<<25)-1u)&~bit:0;}
        else if(c==2)seq.pcue[x->part]=(uint8_t)value;else if(c==3)seq_chain(&seq,(int)value,seq.chain_b);else if(c==4)seq_chain(&seq,seq.chain_a,(int)value);
        else if(c==5)seq.song_on=(uint8_t)value;else if(c==6)seq.song_rec=(uint8_t)value;
        break;
    case CCK_SEED:{unsigned shift=8u*x->seed_byte;p->bass[x->bass].gen.seed=(p->bass[x->bass].gen.seed&~(255u<<shift))|(value<<shift);break;}
    case CCK_GENACTION:
        plat_control_leave(saved);
        if(value){bpart_t prepared=p->bass[x->bass];
            if(c==0)tb3po_generate(&prepared);else tb3po_mutate(&prepared,&prepared.gen.seed);
            saved=plat_control_enter();p->bass[x->bass]=prepared;
            (void)engine_cc_refresh(2);plat_control_leave(saved);source->action_high|=(uint8_t)(1u<<c);ui_external_changed();}
        return 0;
    default:plat_control_leave(saved);return CC_UNSUPPORTED;
    }
    if(!engine_cc_refresh(kind==CCK_CHANNEL?1:kind==CCK_SEQ?2:0)){
        /* Queue capacity was reserved before canonical mutation; main is the
         * sole producer, so capacity cannot shrink across this operation. */
        plat_control_leave(saved);return CC_BUSY;
    }
    plat_control_leave(saved);
    if(kind==CCK_PREF&&(c==6||c==7))engine_brk_loops();
    ui_external_changed();return 0;
invalid:plat_control_leave(saved);return CC_REJECTED;
}
POLY_COLD int cc_parameter(uint16_t number,uint32_t value,uint8_t source){
    uint32_t word;int kind,a,b,c,max;cc_source_t *s;
    if(source>=CC_SOURCES)return CC_REJECTED;
    if(cc_lookup(number,&word))return CC_UNSUPPORTED;
    s=&cc_sources[source];kind=word&31;a=(word>>5)&31;b=(word>>10)&15;c=(word>>14)&31;
    if(project_write_protected()&&kind!=CCK_MASTER&&kind!=CCK_SELECTOR&&kind!=CCK_EXACT&&!(kind==CCK_TRANSPORT&&c==1)&&!(kind==CCK_HISTORY&&!value)&&kind!=CCK_ACTION&&kind!=CCK_ACTIONCTL)return CC_REJECTED;
    max=cc_max(kind,a,b,c,&s->applied);
    if(kind==CCK_MOTION&&!c&&value==MOT_NONE)return cc_apply(kind,a,b,c,value,s);
    if(kind==CCK_PAD&&!c){if(value>15&&value!=255)return CC_REJECTED;}
    else if(kind==CCK_ARR&&c==4){if(value>6&&value!=255)return CC_REJECTED;}
    else if(max<0)return CC_UNSUPPORTED;
    else if(value>(uint32_t)max)return CC_REJECTED;
    if(kind==CCK_STRIP&&((c<2&&value<20)||(c==4&&value<10)))return CC_REJECTED;
    if(kind==CCK_ARR&&c==6&&!value)return CC_REJECTED;
    if(kind==CCK_LANE&&!c&&!value)return CC_REJECTED;
    return cc_apply(kind,a,b,c,value,s);
}
POLY_COLD int cc_message(uint32_t packet,uint8_t source){
    uint8_t status=(uint8_t)(packet>>8),cc=(uint8_t)(packet>>16),raw=(uint8_t)(packet>>24);
    uint32_t word;uint16_t number;int result,kind,a,b,c,max,value;
    if(source>=CC_SOURCES||(status&240)!=176||cc>127||raw>127)return CC_REJECTED;
    if(cc>=120 || (cc>=32&&cc<64)){cc_stats.unsupported++;return CC_UNSUPPORTED;}
    number=(uint16_t)((status&15)*88+(cc<32?cc:cc-32));
    if(cc_lookup(number,&word)){cc_stats.unsupported++;return CC_UNSUPPORTED;}
    kind=word&31;a=(word>>5)&31;b=(word>>10)&15;c=(word>>14)&31;max=cc_max(kind,a,b,c,&cc_sources[source].applied);
    if(kind==CCK_STRIP&&c<2)value=cc_cutoff_hz[raw];
    else if((kind==CCK_PAD&&!c) || (kind==CCK_ARR&&c==4))value=cc_sparse(raw,0,kind,c);
    else if(kind==CCK_MOTION&&!c){int count=max+2,index=(raw*(count-1)+63)/127;value=index>max?255:index;}
    else value=max<0?0:(int)(((uint32_t)raw*(uint32_t)max+63)/127);
    if(kind==CCK_STRIP&&c==4)value=10+(raw*240+63)/127;
    if(kind==CCK_ARR&&c==6)value=1+(raw*7+63)/127;
    if(kind==CCK_FOLLOW&&!c)value=(raw*2+63)/127; /* preserve legacy coarse mapping; Scale uses exact value3 */
    if(kind==CCK_LANE&&!c)value=1+(raw*63+63)/127;
    result=cc_parameter(number,(uint32_t)value,source);
    if(result==CC_APPLIED)cc_stats.accepted++;else if(result==CC_BUSY)cc_stats.busy++;else if(result==CC_UNSUPPORTED)cc_stats.unsupported++;else cc_stats.rejected++;
    return result;
}
