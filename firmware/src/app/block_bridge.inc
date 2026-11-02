/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-only, stopped transaction. Uses the existing staged project; failures
 * never publish its partial contents. No audio-path or saved-format changes. */
static POLY_COLD int bridge_bank_blank(const project_t *p,int bank)
{
    pattern_t empty;
    const pattern_t *q=&p->pat[bank];
    int part,v,k;
    pattern_init(&empty,0x3B0u+(uint32_t)bank*977u,0x5A1u+(uint32_t)bank*613u);
    if(!(p->variant.arrangement.flags&POLY_FLAG))return !memcmp(q,&empty,sizeof empty);
    if(q->swing)return 0;
    for(part=0;part<2;part++){
        if(memcmp(q->drum[part].hit,empty.drum[part].hit,sizeof q->drum[part].hit) ||
           memcmp(q->drum[part].accent,empty.drum[part].accent,sizeof q->drum[part].accent) ||
           memcmp(&q->bass[part],&empty.bass[part],sizeof(bpart_t)))return 0;
    }
    for(v=0;v<25;v++)if(poly_length(p->pat,(const uint8_t *)&p->sound,bank,v)!=16 ||
        poly_rate(p->pat,(const uint8_t *)&p->sound,bank,v)!=RATE_16)return 0;
    if(memcmp(q->brk.set,empty.brk.set,sizeof q->brk.set) || q->brk.slot_a!=0 || q->brk.slot_b!=1)return 0;
    for(k=0;k<64;k++)if(poly_break_gate(p->pat,(const uint8_t *)&p->sound,bank,k) ||
        poly_break_slice(p->pat,(const uint8_t *)&p->sound,bank,k))return 0;
    return 1;
}

/* Canonical model only. protected_banks includes the currently selected part
 * patterns, even if they have no song references. Return copied bank on success. */
static POLY_COLD int bridge_stage_copy(project_t *p,int block,uint32_t protected_banks)
{
    song_t *s=&p->arr.song;
    uint8_t *degrees=p->variant.arrangement.degree;
    int bank,k,part,free_lanes=0,needed=0;
    uint32_t used=protected_banks;
    if(block<0 || block>=s->len || s->len>NSONG)return VR_INVALID;
    if(s->len==NSONG)return VR_FULL;
    /* Preserve all dormant preparation, not only the active prefix. */
    if(degrees[NSONG-1])return VR_FULL;
    for(part=0;part<NTRACKS;part++)if(s->bar[NSONG-1].pat[part])return VR_FULL;
    if(s->bar[NSONG-1].mute)return VR_FULL;
    for(k=0;k<NSONG;k++)for(part=0;part<NTRACKS;part++){
        if(s->bar[k].pat[part]>=NPAT)return VR_INVALID;
        used|=1u<<s->bar[k].pat[part];
    }
    for(k=0;k<NLANE;k++){
        const lane_t *l=&p->arr.lane[k];
        if(!l->used){free_lanes++;continue;}
        if(l->pat>=NPAT || l->part>=NTRACKS)return VR_INVALID;
        used|=1u<<l->pat;
        if(l->pat==s->bar[block].pat[l->part])needed++;
    }
    if(needed>free_lanes)return VR_FULL;
    for(bank=0;bank<NPAT;bank++)if(!(used&(1u<<bank)) && bridge_bank_blank(p,bank))break;
    if(bank==NPAT)return VR_FULL;
    for(part=0;part<NTRACKS;part++){
        int src=s->bar[block].pat[part];
        edit_copy_notes(p,bank,src,part);
        if(edit_copy_motion(p,bank,src,part))return VR_FULL;
    }
    for(k=NSONG-1;k>block+1;k--){s->bar[k]=s->bar[k-1];degrees[k]=degrees[k-1];}
    s->bar[block+1]=s->bar[block];degrees[block+1]=degrees[block];
    for(part=0;part<NTRACKS;part++)s->bar[block+1].pat[part]=(uint8_t)bank;
    s->len++;
    return bank;
}

static int bridge_shift_selection(int value,int after)
{return value>after && value<NSONG ? value+1 : value;}

static int bridge_history_index(int value,int source,int redo)
{
    if(redo)return bridge_shift_selection(value,source);
    if(value==source+1)return source;
    return value>source+1 && value<NSONG ? value-1 : value;
}
#define BRIDGE_DELETE 512u
#define BRIDGE_START_DELETED 1024u
#define BRIDGE_POSITION_DELETED 2048u
static int bridge_delete_index(int value,int source,int length)
{
    if(value==source)return length? (source<length?source:length-1):0;
    return value>source && value<NSONG ? value-1 : value;
}
static void bridge_history_remap(unsigned structural,int redo)
{
    int source=(int)((structural>>1)&255u);
    if(structural&BRIDGE_DELETE){
        int length=song()->len;
        if(redo){
            seq.song_start=(uint8_t)bridge_delete_index(seq.song_start,source,length);
            seq.song_pos=(uint16_t)bridge_delete_index(seq.song_pos,source,length);
            ui.song_sel=(uint8_t)(length?(source<length?source:length-1):0);
        }else{
            int fallback=length>1?(source<length-1?source:length-2):0;
            int start=seq.song_start,pos=seq.song_pos;
            seq.song_start=(uint8_t)((structural&BRIDGE_START_DELETED) && start==fallback ? source : start>=source && start<NSONG-1?start+1:start);
            seq.song_pos=(uint16_t)((structural&BRIDGE_POSITION_DELETED) && pos==fallback ? source : pos>=source && pos<NSONG-1?pos+1:pos);
            ui.song_sel=(uint8_t)source;
        }
        seq.arrangement.editing=ui.song_sel;
        return;
    }
    seq.song_start=(uint8_t)bridge_history_index(seq.song_start,source,redo);
    seq.song_pos=(uint16_t)bridge_history_index(seq.song_pos,source,redo);
    ui.song_sel=(uint8_t)(redo?source+1:bridge_history_index(ui.song_sel,source,0));
    seq.arrangement.editing=ui.song_sel;
}

static POLY_COLD int bridge_stage_delete(project_t *p,int block)
{
    song_t *s=&p->arr.song;
    uint8_t *degree=p->variant.arrangement.degree;
    int k;
    if(block<0 || block>=s->len || s->len>NSONG)return VR_INVALID;
    for(k=block;k+1<NSONG;k++){s->bar[k]=s->bar[k+1];degree[k]=degree[k+1];}
    memset(&s->bar[NSONG-1],0,sizeof(song_bar_t));degree[NSONG-1]=0;
    s->len--;
    return VR_OK;
}

static POLY_COLD int bridge_structural_edit(int block,int deleting)
{
    project_t *candidate;
    uint32_t used=0;
    unsigned structural=1u|((unsigned)block<<1);
    int part,bank;
    if(project_write_protected())return VR_PROTECTED;
    if(external_edit || ui.rec || ui.keys || perf_testing() || save_wait || ui_saves_held() ||
       seq.playing || seq.req_start || seq.req_stop || seq.req_cont || seq.transport_stage ||
       seq.transport_hold || engine_queue_read()!=engine_queue_written())return VR_BUSY;
    if(block<0 || block>=song()->len)return VR_INVALID;
    for(part=0;part<NTRACKS;part++)if(seq.ppat[part]>=NPAT)return VR_INVALID;
    candidate=(project_t *)ui_project_stage(sizeof(project_t));
    if(!candidate)return VR_BUSY;
    *candidate=proj;
    for(part=0;part<NTRACKS;part++)used|=1u<<seq.ppat[part];
    bank=deleting?bridge_stage_delete(candidate,block):bridge_stage_copy(candidate,block,used);
    if(bank>=0 && (!variant_valid(&candidate->variant) || !arrangement_song_valid(&candidate->arr.song) ||
       ((candidate->variant.arrangement.flags&POLY_FLAG) && !poly_valid(candidate->pat,(const uint8_t *)&candidate->sound))))bank=VR_INVALID;
    if(bank>=0 && (ca_undo_bytes(undo.region,undo.shadow,undo.len)>UNDO_POOL ||
       ca_undo_bytes((const uint8_t *)candidate+UNDO_OFF,(const uint8_t *)&proj+UNDO_OFF,sizeof undo_shadow)>UNDO_POOL))bank=VR_FULL;
    if(bank<0){ui_project_stage_finish(0);return bank;}
    if(deleting){
        structural|=BRIDGE_DELETE;
        if(seq.song_start==block)structural|=BRIDGE_START_DELETED;
        if(seq.song_pos==block)structural|=BRIDGE_POSITION_DELETED;
    }
    undo_commit(&undo);
    proj=*candidate;
    if(undo_commit(&undo)>0)ud_put16(undo.pool+undo.at[undo.cur-1]+6,structural);
    if(deleting)bridge_history_remap(structural,1);
    else{
        seq.song_start=(uint8_t)bridge_shift_selection(seq.song_start,block);
        seq.song_pos=(uint16_t)bridge_shift_selection(seq.song_pos,block);
        seq.arrangement.editing=ui.song_sel=(uint8_t)(block+1);
    }
    /* Stopped refresh retires queued selection; Start uses the remapped start. */
    engine_cc_refresh(2);engine_motion_reset();
    ui.outline_ok=0;mark_dirty();ui.act_t=plat_ms();
    ui_project_stage_finish(0);
    return bank;
}
int ui_duplicate_block(int block){return bridge_structural_edit(block,0);}
int ui_delete_block(int block){return bridge_structural_edit(block,1);}

static POLY_COLD int bridge_native_block(int kind,int value,int part)
{
    variant_command_t c={VARIANT_ABI,VC_SET_BLOCK,0,ui.song_sel,0,0,0,0};
    variant_reply_t r;
    song_bar_t b;
    int i,degree,bars;
    if(ui.song_sel>=song()->len || ((kind==R_SPAT || kind==R_BMUTE) && (part<0 || part>=NTRACKS)))return VR_INVALID;
    b=song()->bar[ui.song_sel];degree=ARR_DEGREE(proj.variant.arrangement.degree[ui.song_sel]);
    bars=(int)ARR_BARS(&b,proj.variant.arrangement.degree[ui.song_sel])-1;
    if(kind==R_SPAT)b.pat[part]=(uint8_t)value;
    else if(kind==R_SREP){bars=value;b.mute=(uint8_t)((b.mute&31u)|((value&7)<<5));}
    else if(kind==R_BDEGREE)degree=value;
    else if(kind==R_BMUTE)b.mute^=(uint8_t)(1u<<part);
    c.voice=(uint8_t)((bars&7)|(degree<<3)|((bars>>3)<<6));c.index=b.mute&31u;
    for(i=0;i<NTRACKS;i++)c.value|=(int32_t)b.pat[i]<<(5*i);
    return variant_command(&c,&r);
}

static POLY_COLD void bridge_native_action(int act)
{
    int rc,part;
    char text[24];
    if(act==ACT_BLOCK_DEL){
        rc=ui_delete_block(ui.song_sel);
        if(rc==VR_OK)say(song()->len?"BLOCK DELETED":"ARRANGEMENT EMPTY",0);
        else say(rc==VR_PROTECTED?"PROJECT PROTECTED":rc==VR_FULL?"UNDO FULL: BLOCK KEPT":rc==VR_INVALID?"SELECT AN EXISTING BLOCK":"STOP / WAIT TO DELETE",0);
    }else if(act==ACT_BLOCK_DUP){
        rc=ui_duplicate_block(ui.song_sel);
        if(rc>=0){char *q=put_i(put_s(text,"COPY P"),rc+1);put_i(put_s(q," -> BLOCK "),ui.song_sel+1);say(text,0);}
        else say(rc==VR_FULL?"NO ROOM: ORIGINAL KEPT":rc==VR_PROTECTED?"PROJECT PROTECTED":rc==VR_INVALID?"SELECT AN EXISTING BLOCK":"STOP / WAIT TO COPY",0);
    }else if(act==ACT_BLOCK_QUEUE){
        if(!(proj.variant.arrangement.flags&ARR_ENABLED) || ui.song_sel>=song()->len){say("ENABLE ARRANGEMENT",0);return;}
        if(engine_arrangement_queue(ui.song_sel))say(seq.playing?"QUEUED: NEXT BAR":"START BLOCK SELECTED",0);
        else say("CONTROL QUEUE BUSY",0);
    }else{
        if(ui.song_sel>=song()->len || !engine_transport_reserve()){say("STOP TO EDIT NOTES",0);return;}
        if(engine_queue_read()!=engine_queue_written()){engine_transport_release();say("CONTROL QUEUE BUSY",0);return;}
        for(part=0;part<NTRACKS;part++)seq_cue_part(&seq,part,song()->bar[ui.song_sel].pat[part]);
        engine_transport_release();set_view(V_HOME);say("BLOCK NOTES: EDIT A PART",0);
    }
}
