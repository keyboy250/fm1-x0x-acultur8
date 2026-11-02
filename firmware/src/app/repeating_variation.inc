/* SPDX-License-Identifier: GPL-3.0-only */
/* Runtime recipe only. Never dirty the project or enter the Undo journal. */
static POLY_COLD int ui_variation_config_checked(const pattern_variation_t *cfg,int reserved)
{
    if(!pattern_variation_valid(cfg))return VR_INVALID;
    if(cfg->lane>=22 && cfg->lane<24 && proj.pat[cfg->pattern].bass[cfg->lane-22].gen.mutate_bars)return VR_UNSUPPORTED;
    if(external_edit || ui.rec || ui.keys || perf_testing() || save_wait || ui_saves_held() ||
       seq.playing || seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage || (seq.transport_hold && (!reserved || seq.transport_hold!=1)) ||
       engine_queue_read()!=engine_queue_written())return VR_BUSY;
    if(!reserved && !engine_transport_reserve())return VR_BUSY;
    seq.variation=*cfg;
    if(seq.freeze){seq.freeze->state=FREEZE_RECORD;seq.freeze->count=0;}
    if(!reserved)engine_transport_release();return VR_OK;
}
static POLY_COLD int ui_variation_config(const pattern_variation_t *cfg)
{return ui_variation_config_checked(cfg,0);}
static POLY_COLD int ui_variation_remote(int pattern,int lane,int every,int amount,int seed,int anchor,int mode,int options,int constraint,int block,int off)
{
    pattern_variation_t v;
    if(off){v=seq.variation;v.lane=255;}
    else{
        v=(pattern_variation_t){(uint8_t)lane,(uint8_t)pattern,(uint8_t)every,(uint8_t)amount,(uint8_t)seed,(uint8_t)anchor,(uint8_t)mode,(uint8_t)options,0,0,0};
        if(lane>=22 && lane<24){
            const bpart_t *bp=&proj.pat[pattern].bass[lane-22];
            if(constraint>=GEN_FREE || bp->gen.root>11 || bp->gen.scale>=TB3PO_NSCALES || bp->gen.base_oct>4 || bp->gen.oct_range<1 || bp->gen.oct_range>3)return VR_INVALID;
            v.notes=generation_mask(&proj,constraint,block);
            if(constraint==GEN_SCALE)v.notes=tb3po_scale_mask(&bp->gen);
            if(constraint==GEN_BOTH)v.notes&=tb3po_scale_mask(&bp->gen);
            if(!v.notes)return VR_MISSING;
            v.low=(uint8_t)(12*(bp->gen.base_oct+1));v.high=(uint8_t)(v.low+12*bp->gen.oct_range-1);
        }
    }
    return ui_variation_config_checked(&v,1);
}
static int variation_control(int f)
{
    const pattern_variation_t *v=&seq.variation;
    switch(f){case 0:return v->every;case 1:return v->amount;case 2:return v->anchor;case 3:return v->seed;
        case 4:return v->mode;case 5:return v->options&1u;default:return v->options>>1;}
}
static int variation_set_control(int f,int value)
{
    pattern_variation_t v=seq.variation;
    switch(f){case 0:v.every=(uint8_t)value;break;case 1:v.amount=(uint8_t)value;break;case 2:v.anchor=(uint8_t)value;break;
        case 3:v.seed=(uint8_t)value;break;case 4:v.mode=(uint8_t)value;break;
        case 5:v.options=(uint8_t)((v.options&6u)|(unsigned)value);break;default:v.options=(uint8_t)((v.options&1u)|((unsigned)value<<1));break;}
    return ui_variation_config(&v)==VR_OK;
}
/* Include transient settings in the confirmation guard, not just project data. */
static uint32_t variation_runtime_fingerprint(void)
{
    uint32_t h=project_fingerprint();const uint8_t *p=(const uint8_t *)&seq.variation;
    for(unsigned i=0;i<sizeof seq.variation;i++)h=(h^p[i])*16777619u;
    return h;
}
static uint32_t variation_fingerprint(void)
{
    return variation_runtime_fingerprint()^ui.gen_mode[0]^((uint32_t)ui.gen_mode[1]<<8)^((uint32_t)ui.song_sel<<16);
}
static POLY_COLD void variation_native_action(int confirm,int off,int arg)
{
    pattern_variation_t v=seq.variation;char first[28],second[28];int rc;
    if(!confirm){
        if(!off && ((!is_drum() && !is_303()) || (is_drum() && ui.sel[ui.part]>=NDRUM))){say("PICK DRUM OR 303",0);return;}
        edit_fingerprint=variation_fingerprint();edit_epoch=seq.transport_epoch;
        if(off){ask(ACT_VARIATION_OFF,0,"TURN LIVE VARIATION OFF?","BASE PATTERN IS UNCHANGED");return;}
        v.lane=(uint8_t)(is_303()?22+bidx():ui.part*11+ui.sel[ui.part]);v.pattern=seq.ppat[ui.part];
        char *end=put_s(first,PART_N[ui.part]);if(is_drum())end=put_s(put_s(end," "),engine_voice_name(ui.part?T_808:T_909,ui.sel[ui.part]));
        put_s(put_i(put_s(end," P"),v.pattern+1)," LIVE VAR?");
        put_s(put_i(put_s(put_i(put_s(second,"EVERY "),2<<v.every)," MAX "),v.amount)," CHANGES");
        if(is_303()){
            int mode=ui.gen_mode[bidx()];
            if(mode>=GEN_FREE){say("CHOOSE SCALE OR CHORD",0);return;}
            char *q=put_s(second,GEN_MODE_N[mode]);if(mode==GEN_CHORD || mode==GEN_BOTH)q=put_i(put_s(q," B"),ui.song_sel+1);
            put_s(q," SNAP / PITCH");
        }
        ask(ACT_VARIATION_ARM,v.lane|(v.pattern<<5)|(v.every<<10)|(v.amount<<12)|(v.seed<<16)|(v.anchor<<24)|(v.mode<<26)|(v.options<<28),first,second);return;
    }
    if(edit_fingerprint!=variation_fingerprint() || edit_epoch!=seq.transport_epoch){say("CHANGED: CONFIRM AGAIN",0);return;}
    if(off)v.lane=255;
    else {v.lane=(uint8_t)(arg&31);v.pattern=(uint8_t)((arg>>5)&31);v.every=(uint8_t)((arg>>10)&3);v.amount=(uint8_t)((arg>>12)&15);
        v.seed=(uint8_t)((arg>>16)&255);v.anchor=(uint8_t)((arg>>24)&3);v.mode=(uint8_t)((arg>>26)&3);v.options=(uint8_t)((arg>>28)&7);}
    if(!off && v.lane>=22 && v.lane<24){
        const bpart_t *bp=&proj.pat[v.pattern].bass[v.lane-22];int mode=ui.gen_mode[v.lane-22];
        if(mode>=GEN_FREE || bp->gen.root>11 || bp->gen.scale>=TB3PO_NSCALES || bp->gen.base_oct>4 || bp->gen.oct_range<1 || bp->gen.oct_range>3){say("INVALID NOTE CONSTRAINTS",0);return;}
        v.notes=generation_mask(&proj,mode,ui.song_sel);
        if(mode==GEN_SCALE)v.notes=tb3po_scale_mask(&bp->gen);
        if(mode==GEN_BOTH)v.notes&=tb3po_scale_mask(&bp->gen);
        if(!v.notes){say("NO CHORD / SHARED NOTES",0);return;}
        v.low=(uint8_t)(12*(bp->gen.base_oct+1));v.high=(uint8_t)(v.low+12*bp->gen.oct_range-1);
    }
    rc=ui_variation_config(&v);
    say(rc==VR_OK?(off?"VAR OFF: BASE UNCHANGED":"VAR ARMED: BASE UNCHANGED"):rc==VR_UNSUPPORTED?"TURN AUTO MUTATE OFF":"STOP / WAIT TO CONFIGURE",0);
}
