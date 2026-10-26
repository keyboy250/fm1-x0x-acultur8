/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X pattern defaults. */
#include "pattern.h"
#include "tb3po.h"
#include "rate_slice_codec.c"
#include "rate_families.c"
#include "lanes.c" /* shared bounded model; all existing pattern link lists include it */

static const uint8_t BRK_DEFAULTS[BRK_NSET] = {
    25, 60, 20, 50,                   /* complexity, anchor, roll, fill */
    10, 0, 5, 0,                      /* retrig 2x 3x 4x 8x */
    2, 0, 2, 2,                       /* phrase 4 bars, B chance 0, A length 1 bar, B length 1 bar */
};

void pattern_init(pattern_t *p, uint32_t seed_a, uint32_t seed_b)
{
    uint32_t i, b;
    uint8_t *z = (uint8_t *)p;
    for (i = 0; i < sizeof *p; i++)
        z[i] = 0;
    for (i = 0; i < NKIT; i++) {
        p->drum[i].len = 16;
        p->drum[i].rate = RATE_16;
    }
    for (b = 0; b < NBASS; b++) {
        bpart_t *bp = &p->bass[b];
        bp->len = 16;
        bp->rate = RATE_16;
        bp->dir = DIR_FWD;
        bp->transpose = 24;
        tb3po_defaults(&bp->gen, b ? seed_b : seed_a);
        if (b)
            bp->gen.base_oct = 2;     /* 303 B sits an octave above A by default */
        for (i = 0; i < NSTEPS; i++) {
            bp->step[i].note = (uint8_t)(12 * (bp->gen.base_oct + 1) + bp->gen.root);
            bp->step[i].flags = G_REST;
        }
    }
    p->brk.steps = 0;                 /* the break is silent until switched on */
    for (i = 0; i < BRK_NSET; i++)
        p->brk.set[i] = BRK_DEFAULTS[i];
    p->brk.slot_a = 0;                /* the built-in bank's first two: the groove, and the fill for B */
    p->brk.slot_b = 1;
}

/* Local PRNG: deterministic across host/target, without touching global RNG. */
static uint32_t drum_random(uint32_t *s)
{
    uint32_t x=*s;x^=x<<13;x^=x>>17;x^=x<<5;*s=x;return x;
}
int pattern_generate_drum(uint32_t hits[NSW],int len,int density,int positions,int anchor,uint32_t seed)
{
    uint32_t row[NSW];int i,anchors=0;
    if(!hits || len<1 || len>NSTEPS || density<0 || density>100 ||
       positions<0 || positions>=DR_POS_COUNT || anchor<0 || anchor>=DR_AN_COUNT || !seed)return 0;
    for(i=0;i<NSW;i++)row[i]=hits[i];
    for(i=0;i<len;i++){
        int allowed=positions==DR_POS_ALL || (positions==DR_POS_EVEN && (i&1)) ||
            (positions==DR_POS_ODD && !(i&1)) || (positions==DR_POS_OFF && (i&3)==2);
        int beat=(i&3)==0,back=(i&15)==4 || (i&15)==12,old=sm_get(hits,i),on;
        int random_hit=(int)(drum_random(&seed)%100u)<density; /* one draw per position: KEEP repeat is stable */
        int force=(anchor==DR_AN_BEAT && beat) || (anchor==DR_AN_BACK && back) || (anchor==DR_AN_KEEP && old);
        int lock=(anchor==DR_AN_BEATLOCK && beat) || (anchor==DR_AN_BACKLOCK && back);
        anchors+=anchor==DR_AN_BEAT || anchor==DR_AN_BEATLOCK?beat:back;
        if(!allowed && (force || (lock && old)))return 0;
        on=lock?old:force?1:allowed && random_hit;
        if(on)sm_set(row,i);else row[i>>5]&=~(1u<<(i&31));
    }
    if(anchor!=DR_AN_NONE && anchor!=DR_AN_KEEP && !anchors)return 0;
    for(i=0;i<NSW;i++)hits[i]=row[i];
    return 1;
}

int pattern_variation_valid(const pattern_variation_t *v)
{
    return v && (v->lane<24 || v->lane==255) && v->pattern<NPAT && v->every<3 && v->amount<=8 &&
        v->anchor<4 && v->mode<3 && v->options<8 &&
        (v->lane<22 || v->lane==255 || (v->notes && !(v->notes&~0xfffu) && v->low<=v->high && v->high<128));
}
/* Constant work: a seed rotates a bounded run of candidate positions. Every
 * variation is projected from base, never from last cycle's output. */
int pattern_variation_selected(const pattern_variation_t *v,int step,int length,unsigned cycle)
{
    unsigned cadence,window,span,first,rank,offset;uint32_t h;
    if(!pattern_variation_valid(v) || v->lane==255 || !v->amount || length<1 || length>NSTEPS || step<0 || step>=length)return 0;
    cadence=2u<<v->every;if((cycle+1u)&(cadence-1u))return 0;
    if(((v->anchor&1u) && !(step&3)) || ((v->anchor&2u) && ((step&15)==4 || (step&15)==12)))return 0;
    window=v->options>>1;span=window?(2u<<window):(unsigned)length;if(span>(unsigned)length)span=(unsigned)length;
    first=(unsigned)length-span;if((unsigned)step<first)return 0;
    h=(uint32_t)v->seed+1u;if(v->options&1u)h^=((cycle>>(v->every+1u))+1u)*0x9e3779b9u;
    h^=h<<13;h^=h>>17;h^=h<<5;
    offset=h%span;rank=((unsigned)step-first+span-offset)%span;
    if((h>>16)&1u)rank=(span-rank)%span;
    if(rank>=v->amount)return 0;
    return 1;
}
int pattern_variation_hit(const pattern_variation_t *v,int base,int step,int length,unsigned cycle)
{
    base=base!=0;
    if(!pattern_variation_selected(v,step,length,cycle) || v->lane>=22)return base;
    return v->mode==1?1:v->mode==2?0:!base;
}
