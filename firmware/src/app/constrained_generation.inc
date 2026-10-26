/* SPDX-License-Identifier: GPL-3.0-only */
/* Snapshot generation: selected block context is read only at the stopped commit.
 * Playback follow and legacy/automatic TB-3PO mutation remain independent. */
static POLY_COLD uint16_t generation_mask(const project_t *p,int mode,int block)
{
    uint16_t mask=0,chord_mask=0;int i;uint8_t note;harmony_triad_t chord;
    const arrangement_settings_t *a=&p->variant.arrangement;
    if(mode==GEN_FREE)return 0xfffu;
    if(mode==GEN_KEY){
        for(i=0;i<12;i++)if(harmony_scale_note(a->key,a->mode,i,0,0,11,&note) && note==i)mask|=(uint16_t)(1u<<i);
        return mask;
    }
    if(mode==GEN_CHORD || mode==GEN_BOTH){
        if(block<0 || block>=p->arr.song.len)return 0;
        harmony_spec_t spec={a->key,a->mode,ARR_DEGREE(a->degree[block]),a->parent};
        if(!harmony_resolve(&spec,60,0,127,&chord))return 0;
        for(i=0;i<3;i++)chord_mask|=(uint16_t)(1u<<chord.pitch_class[i]);
    }
    return chord_mask;
}

static POLY_COLD int ui_generate_random_checked(int pattern,int bass,int mode,int block,uint32_t seed,int length,int step,int reserved)
{
    project_t *candidate;const bpart_t *original;uint16_t mask;int low,high,rc=VR_OK;
    if(pattern<0 || pattern>=NPAT || bass<0 || bass>=NBASS || mode<0 || mode>=GEN_MODES || !seed || length<0 || length>NSTEPS || step<0 || step>NSTEPS)return VR_INVALID;
    if(project_write_protected())return VR_PROTECTED;
    if(external_edit || ui.rec || ui.keys || perf_testing() || save_wait || ui_saves_held() ||
       seq.playing || seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage ||
       (seq.transport_hold && (!reserved || seq.transport_hold!=1)) || engine_queue_read()!=engine_queue_written())return VR_BUSY;
    original=&proj.pat[pattern].bass[bass];
    if(original->gen.mutate_bars)return VR_UNSUPPORTED;
    if(original->gen.base_oct>4 || original->gen.oct_range<1 || original->gen.oct_range>3 ||
       original->gen.root>11 || original->gen.scale>=TB3PO_NSCALES)return VR_INVALID;
    low=12*(original->gen.base_oct+1);high=low+12*original->gen.oct_range-1;
    mask=generation_mask(&proj,mode,block);
    if(mode==GEN_SCALE)mask=tb3po_scale_mask(&original->gen);
    if(mode==GEN_BOTH)mask&=tb3po_scale_mask(&original->gen);
    if(!mask)return VR_MISSING;
    int effective=(proj.variant.arrangement.flags&POLY_FLAG)?poly_length(proj.pat,(uint8_t *)&proj.sound,pattern,22+bass):original->len;
    int chosen=-1;
    if(step){
        if(step>effective)return VR_INVALID;
        if(bstep_gate(&original->step[step-1])!=G_NOTE)return VR_UNSUPPORTED;
        uint8_t allowed[128];int n=0,trans=(int)original->transpose-24;
        for(int note=0;note<128;note++){
            int final=note+trans;
            if(final>=low && final<=high && final>=0 && final<128 && (mask&(1u<<(final%12))) && note!=original->step[step-1].note)allowed[n++]=(uint8_t)note;
        }
        if(!n)return VR_OK; /* No legal alternative: true no-op, including seed/Undo. */
        chosen=allowed[tb3po_new_seed(seed^((uint32_t)step*73u))%(unsigned)n];
    }
    candidate=(project_t *)ui_project_stage(sizeof(project_t));
    if(!candidate)return VR_BUSY;
    if(engine_queue_read()!=engine_queue_written()){ui_project_stage_finish(0);return VR_BUSY;}
    *candidate=proj;
    bpart_t *out=&candidate->pat[pattern].bass[bass];
    if(step)out->step[step-1].note=(uint8_t)chosen;
    else {
        out->gen.seed=seed;
        if(length){
            out->len=(uint8_t)length;
            if(candidate->variant.arrangement.flags&POLY_FLAG)poly_set(candidate->pat,(uint8_t *)&candidate->sound,pattern,22+bass,length,poly_rate(proj.pat,(uint8_t *)&proj.sound,pattern,22+bass));
        }
        uint16_t rawmask=mask;int lo=low,hi=high;
        if(length){
            int trans=(int)original->transpose-24;rawmask=0;
            for(int pc=0;pc<12;pc++)if(mask&(1u<<((pc+trans+120)%12)))rawmask|=(uint16_t)(1u<<pc);
            lo-=trans;hi-=trans;if(lo<0)lo=0;if(hi>127)hi=127;
        }
        if(!tb3po_generate_mask(out,rawmask,lo,hi))rc=VR_INVALID;
        if(length)for(int i=0;i<NSTEPS;i++){
            if(i>=length)out->step[i]=original->step[i];
            else out->step[i].flags=(out->step[i].flags&~BS_ACCENT)|(original->step[i].flags&BS_ACCENT);
        }
    }
    if(rc==VR_OK && (!variant_valid(&candidate->variant) || !arrangement_song_valid(&candidate->arr.song) ||
       ((candidate->variant.arrangement.flags&POLY_FLAG) && !poly_valid(candidate->pat,(const uint8_t *)&candidate->sound))))rc=VR_INVALID;
    if(rc==VR_OK && (ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL ||
       ca_undo_bytes((const uint8_t *)candidate+UNDO_OFF,(const uint8_t *)&proj+UNDO_OFF,sizeof undo_shadow)>UNDO_POOL))rc=VR_FULL;
    if(rc!=VR_OK){ui_project_stage_finish(0);return rc;}
    undo_commit(&undo);proj=*candidate;
    if(undo_commit(&undo)>0)ud_put16(undo.pool+undo.at[undo.cur-1]+6,4096u);
    engine_motion_reset();ui.outline_ok=0;mark_dirty();ui.act_t=plat_ms();ui.gen_stale[bass]=0;
    ui_project_stage_finish(0);return VR_OK;
}

static POLY_COLD int ui_generate_random(int pattern,int bass,int mode,int block,uint32_t seed,int length,int step)
{return ui_generate_random_checked(pattern,bass,mode,block,seed,length,step,0);}
static POLY_COLD int ui_generate_remote(int pattern,int bass,int mode,int block,uint32_t seed,int length,int step)
{return ui_generate_random_checked(pattern,bass,mode,block,seed,length,step,1);}
POLY_COLD int ui_generate_constrained(int pattern,int bass,int mode,int block,uint32_t seed)
{return ui_generate_random(pattern,bass,mode,block,seed,0,0);}
static uint32_t generation_fingerprint(void)
{return (project_fingerprint()^((uint32_t)random_opts[0]<<8)^((uint32_t)random_opts[1]<<16));}

static POLY_COLD void generate_native_action(int confirm,int arg)
{
    int pattern,bass,mode,block,new_seed,rc;char first[28],second[28];uint32_t seed;
    if(!confirm){
        if(!is_303()){say("PICK 303 A OR B",0);return;}
        pattern=seq.ppat[ui.part];bass=bidx();mode=ui.gen_mode[bass];block=ui.song_sel;
        if(mode>=GEN_MODES){say("INVALID NOTE CONSTRAINTS",0);return;}
        /* This guard is shared with copy because only one ASK overlay can exist. */
        edit_fingerprint=generation_fingerprint();edit_epoch=seq.transport_epoch;
        put_s(put_i(put_s(put_s(first,PART_N[ui.part])," P"),pattern+1)," REPLACE NOTES?");
        if(random_opts[1])put_s(put_i(put_s(put_i(put_s(put_s(first,PART_N[ui.part])," P"),pattern+1)," RANDOM STEP "),random_opts[1]),"?");
        else if(random_opts[0])put_s(put_i(put_s(put_i(put_s(put_s(first,PART_N[ui.part])," P"),pattern+1)," NEW "),random_opts[0])," STEPS?");
        char *end=put_s(second,GEN_MODE_LONG[mode]);
        if(mode==GEN_CHORD || mode==GEN_BOTH)put_i(put_s(end," B"),block+1);
        ask(ACT_GENERATE_CONSTRAINED,pattern|(bass<<5)|(mode<<6)|(block<<9)|((arg!=0)<<17),first,second);
        return;
    }
    if(edit_fingerprint!=generation_fingerprint() || edit_epoch!=seq.transport_epoch){say("CHANGED: CONFIRM AGAIN",0);return;}
    pattern=arg&31;bass=(arg>>5)&1;mode=(arg>>6)&7;block=(arg>>9)&255;new_seed=(arg>>17)&1;
    seed=new_seed?tb3po_new_seed(plat_ms()^(ui.frame<<7)):proj.pat[pattern].bass[bass].gen.seed;
    uint32_t before=project_fingerprint();
    rc=ui_generate_random(pattern,bass,mode,block,seed,random_opts[0],random_opts[1]);
    say(rc==VR_OK?(before==project_fingerprint()?"UNCHANGED: NO LEGAL NOTE":random_opts[1]?"STEP RANDOMIZED: UNDO":"GENERATED: UNDO AVAILABLE"):rc==VR_UNSUPPORTED?"NOTE STEP / MUTATE OFF":rc==VR_MISSING?"NO CHORD / SHARED NOTES":
        rc==VR_FULL?"NO UNDO ROOM: UNCHANGED":rc==VR_PROTECTED?"PROJECT PROTECTED":rc==VR_INVALID?"INVALID NOTE CONSTRAINTS":"STOP / WAIT TO GENERATE",0);
}
