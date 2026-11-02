/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-context, bounded read of the existing accepted-event recorder. No new
 * audio writes, allocation, static RAM, prediction or project mutation. */
typedef struct { uint32_t bits[2]; int step,count,active; } live_drum_overlay_t;
static POLY_COLD live_drum_overlay_t live_drum_overlay(void)
{
    live_drum_overlay_t a={{0,0},-1,0,0};
    if(!ui.live_overlay || !is_drum())return a; /* OFF: no guard or capture reads */
    unsigned lane=ui.part*NDRUM+ui.sel[ui.part],saved=plat_control_enter();
    const pattern_freeze_t *f=seq.freeze;
    if(seq.playing && seq.variation.lane==lane && seq.variation.pattern==seq.ppat[ui.part]){
        a.active=1;a.step=seq_lane_position(&seq,(int)lane);
        int len=seq.lane_enabled?seq_lane_length(&seq,(int)lane):seq.pat[seq.ppat[ui.part]].drum[ui.part].len;
        if(f && f->count<=NSTEPS && f->count<=f->bass.len && f->bass.len==len &&
           !((seq.mute|seq.vmute)&(1u<<lane))){
            a.count=f->count;
            for(int i=0;i<a.count;i++)if(bstep_gate(&f->bass.step[i])==G_NOTE)a.bits[i/32]|=1u<<(i&31);
        }
    }
    plat_control_leave(saved);return a;
}
static int live_drum_hit(const live_drum_overlay_t *a,int step)
{return step>=0 && step<a->count && step<NSTEPS && ((a->bits[step/32]>>(step&31))&1u);}
