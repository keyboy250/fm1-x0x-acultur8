/* SPDX-License-Identifier: GPL-3.0-only */
/* Transient recipe; only the selected drum row is a saved/undoable edit. */
static POLY_COLD int ui_generate_drum_checked(int pattern,int kit,int voice,int density,int positions,int anchor,uint32_t seed,int length,int reserved)
{
    project_t *candidate;int rc=VR_OK,len;
    if(pattern<0 || pattern>=NPAT || kit<0 || kit>=NKIT || voice<0 || voice>=NDRUM ||
       length<0 || length>NSTEPS || density<0 || density>100 || positions<0 || positions>=DR_POS_COUNT || anchor<0 || anchor>=DR_AN_COUNT || !seed)return VR_INVALID;
    if(project_write_protected())return VR_PROTECTED;
    if(external_edit || ui.rec || ui.keys || perf_testing() || save_wait || ui_saves_held() ||
       seq.playing || seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage ||
       (seq.transport_hold && (!reserved || seq.transport_hold!=1)) || engine_queue_read()!=engine_queue_written())return VR_BUSY;
    len=ui_drum_length(&proj.pat[pattern],kit,voice);
    if(length && length!=len && !(proj.variant.arrangement.flags&POLY_FLAG))return VR_UNSUPPORTED;
    if(length)len=length;
    candidate=(project_t *)ui_project_stage(sizeof(project_t));
    if(!candidate)return VR_BUSY;
    if(engine_queue_read()!=engine_queue_written()){ui_project_stage_finish(0);return VR_BUSY;}
    *candidate=proj;
    if(length && (candidate->variant.arrangement.flags&POLY_FLAG))poly_set(candidate->pat,(uint8_t *)&candidate->sound,pattern,kit*11+voice,length,poly_rate(proj.pat,(uint8_t *)&proj.sound,pattern,kit*11+voice));
    if(!pattern_generate_drum(candidate->pat[pattern].drum[kit].hit[voice],len,density,positions,anchor,seed))rc=VR_INVALID;
    if(rc==VR_OK && (!variant_valid(&candidate->variant) || !arrangement_song_valid(&candidate->arr.song) ||
       ((candidate->variant.arrangement.flags&POLY_FLAG) && !poly_valid(candidate->pat,(const uint8_t *)&candidate->sound))))rc=VR_INVALID;
    if(rc==VR_OK && (ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL ||
       ca_undo_bytes((const uint8_t *)candidate+UNDO_OFF,(const uint8_t *)&proj+UNDO_OFF,sizeof undo_shadow)>UNDO_POOL))rc=VR_FULL;
    if(rc!=VR_OK){ui_project_stage_finish(0);return rc;}
    if((!length || length==ui_drum_length(&proj.pat[pattern],kit,voice)) && candidate->pat[pattern].drum[kit].hit[voice][0]==proj.pat[pattern].drum[kit].hit[voice][0] &&
       candidate->pat[pattern].drum[kit].hit[voice][1]==proj.pat[pattern].drum[kit].hit[voice][1]){ui_project_stage_finish(0);return VR_OK;}
    undo_commit(&undo);proj=*candidate;
    if(undo_commit(&undo)>0)ud_put16(undo.pool+undo.at[undo.cur-1]+6,4096u);
    engine_motion_reset();ui.outline_ok=0;mark_dirty();ui.act_t=plat_ms();
    ui_project_stage_finish(0);return VR_OK;
}

static POLY_COLD int ui_generate_drum_size(int pattern,int kit,int voice,int density,int positions,int anchor,uint32_t seed,int length)
{return ui_generate_drum_checked(pattern,kit,voice,density,positions,anchor,seed,length,0);}
static POLY_COLD int ui_generate_drum_remote(int pattern,int kit,int voice,int density,int positions,int anchor,uint32_t seed,int length)
{return ui_generate_drum_checked(pattern,kit,voice,density,positions,anchor,seed,length,1);}
POLY_COLD int ui_generate_drum(int pattern,int kit,int voice,int density,int positions,int anchor,uint32_t seed)
{return ui_generate_drum_size(pattern,kit,voice,density,positions,anchor,seed,0);}
static POLY_COLD void drum_generate_native_action(int confirm,int arg)
{
    int pattern,kit,voice,density,positions,anchor,raw,rc;char first[28],second[28];
    if(!confirm){
        if(!is_drum() || ui.sel[ui.part]>=NDRUM){say("PICK A DRUM TRACK",0);return;}
        pattern=seq.ppat[ui.part];kit=ui.part;voice=ui.sel[kit];
        density=ui.drum_gen[0];positions=ui.drum_gen[1];anchor=ui.drum_gen[2];raw=ui.drum_gen[3];
        if(arg){int next=(int)(tb3po_new_seed(plat_ms()^(ui.frame<<7))&255u);raw=next==raw?(raw+1)&255:next;}
        edit_fingerprint=generation_fingerprint();edit_epoch=seq.transport_epoch;
        char *end=put_s(put_s(put_s(first,PART_N[kit])," "),engine_voice_name(kit?T_808:T_909,voice));
        put_s(put_i(put_s(end," P"),pattern+1)," REPLACE?");
        put_s(put_s(put_i(put_s(second,"SEED "),raw+1)," "),DR_AN_N[anchor]);
        if(random_opts[0])put_i(put_s(put_i(put_s(second,"LEN "),random_opts[0])," SEED "),raw+1);
        ask(ACT_GENERATE_DRUM,pattern|(kit<<5)|(voice<<6)|(density<<10)|(positions<<17)|(anchor<<19)|(raw<<22),first,second);
        return;
    }
    if(edit_fingerprint!=generation_fingerprint() || edit_epoch!=seq.transport_epoch){say("CHANGED: CONFIRM AGAIN",0);return;}
    pattern=arg&31;kit=(arg>>5)&1;voice=(arg>>6)&15;density=(arg>>10)&127;
    positions=(arg>>17)&3;anchor=(arg>>19)&7;raw=(arg>>22)&255;
    rc=ui_generate_drum_size(pattern,kit,voice,density,positions,anchor,(uint32_t)raw+1u,random_opts[0]);
    if(rc==VR_OK)ui.drum_gen[3]=(uint8_t)raw;
    say(rc==VR_OK?(edit_fingerprint==generation_fingerprint()?"UNCHANGED: SAME RHYTHM":"GENERATED: UNDO AVAILABLE"):rc==VR_INVALID?"CONFLICT: CHECK ANCHORS":
        rc==VR_UNSUPPORTED?"USE POLY FOR TRACK LENGTH":rc==VR_FULL?"NO UNDO ROOM: UNCHANGED":rc==VR_PROTECTED?"PROJECT PROTECTED":"STOP / WAIT TO GENERATE",0);
}
