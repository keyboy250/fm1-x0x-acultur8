/* SPDX-License-Identifier: GPL-3.0-only */
/* No new audio-path writes or RAM allocation: reuse Freeze's accepted-event
 * prefix. It is not MIDI-delivery proof and is not a history of older passes. */
POLY_COLD int engine_runtime_snapshot(unsigned lane,uint32_t w[RUNTIME_DIAGNOSTIC_WORDS])
{
    pattern_variation_t v;unsigned part,pat,i,count=0;uint32_t saved;
    if(lane>=24 || !w)return -1;
    part=lane<22?lane/11:lane-20;
    saved=plat_control_enter();
    pat=seq.ppat[part];
    if(pat>=NPAT || !seq.pat){plat_control_leave(saved);return -1;}
    for(i=0;i<RUNTIME_DIAGNOSTIC_WORDS;i++)w[i]=0;
    v=seq.variation;
    w[0]=variant_current_revision();w[1]=engine_queue_written();w[2]=engine_queue_read();
    w[3]=seq.transport_epoch;w[4]=seq.playing;w[5]=seq.req_start;w[6]=seq.req_stop;w[7]=seq.req_cont;
    w[8]=seq.transport_hold;w[9]=seq.transport_stage;w[10]=lane;w[11]=pat;
    w[12]=seq.lane_enabled;w[13]=seq.lane_sound!=0;w[14]=seq.lane_sound?poly_expanded(seq.lane_sound):0;
    w[15]=seq.arrangement.flags;w[16]=seq.song_on;w[17]=seq.song_pos;w[18]=seq.chain_a;w[19]=seq.chain_b;
    w[20]=v.lane;w[21]=v.pattern;w[22]=v.every;w[23]=v.amount;w[24]=v.seed;w[25]=v.anchor;
    w[26]=v.mode;w[27]=v.options;w[28]=v.notes;w[29]=v.low;w[30]=v.high;
    w[31]=seq_lane_position(&seq,(int)lane);w[32]=seq_lane_cycles(&seq,(int)lane);
    w[35]=lane<22?seq.pat[pat].drum[part].len:seq.pat[pat].bass[lane-22].len;
    w[33]=(unsigned)seq_part_len(&seq,(int)part);
    if(lane<22 && seq.lane_enabled)w[33]=(unsigned)seq_lane_length(&seq,(int)lane);
    w[51]=seq.lane_enabled?(unsigned)seq_lane_length(&seq,(int)lane):w[35];
    w[34]=seq.lane_sound?(unsigned)seq_lane_latched_rate(&seq,(int)(lane<22 && !seq.lane_enabled?part*11:lane)):lane<22?seq.pat[pat].drum[part].rate:seq.pat[pat].bass[lane-22].rate;
    w[36]=seq.t[part].bars;
    w[41]=(v.lane==lane && v.pattern==pat);
    if(seq.freeze){
        w[37]=seq.freeze->state;w[38]=seq.freeze->count;w[39]=seq.freeze->bass.len;
        if(v.lane==lane && v.pattern==pat && w[38]<=NSTEPS && w[38]<=w[39] && w[39]<=NSTEPS){
            count=w[38];w[40]=1;
            for(i=0;i<count;i++)if(bstep_gate(&seq.freeze->bass.step[i])==G_NOTE)w[44+i/32]|=1u<<(i&31);
        }
    }
    if(lane<22){w[42]=seq.pat[pat].drum[part].hit[lane%11][0];w[43]=seq.pat[pat].drum[part].hit[lane%11][1];}
    else for(i=0;i<NSTEPS;i++)if(bstep_gate(&seq.pat[pat].bass[lane-22].step[i])==G_NOTE)w[42+i/32]|=1u<<(i&31);
    w[46]=seq.mute;w[47]=seq.vmute;w[48]=prob[part][lane<22?lane%11:0];
    plat_control_leave(saved);
    /* Calculated intent, NOT observed triggers; outside the IRQ guard. */
    if(lane<22 && w[33]<=NSTEPS && w[51]<=NSTEPS)for(i=0;i<w[33];i++){
        int hit=(w[42+i/32]>>(i&31))&1u;
        if(w[40] && w[37]==FREEZE_HELD)hit=(w[44+i/32]>>(i&31))&1u;
        else if(v.lane==lane && v.pattern==pat)hit=pattern_variation_hit(&v,hit,(int)i,(int)w[51],w[32]);
        if(hit)w[49+i/32]|=1u<<(i&31);
    }
    return 0;
}
