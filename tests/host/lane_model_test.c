/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/x0x.h"
static project_t project;
static uint8_t lengths[32][25],rates[32][25],gates[32][64],slices[32][64];
static uint32_t random_state=0x712ffe1u;
static unsigned random_value(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
int main(void)
{
    uint8_t original[sizeof(sound_t)],seen[sizeof(sound_t)]={0};unsigned cell;int p,l,r,n,s;
    memset(&project,0,sizeof project);
    for(p=0;p<NPAT;p++)pattern_init(&project.pat[p],(unsigned)p+1,(unsigned)p+3);
    memset(&project.sound,0xa5,sizeof project.sound);memcpy(original,&project.sound,sizeof original);
    for(cell=0;cell<POLY_BYTES;cell++){unsigned off=poly_sound_offset(cell);assert(off<sizeof(sound_t) && !seen[off]);seen[off]=1;assert(poly_cell_owned(off));}
    assert(!seen[(T_FX*12+1)*16]); /* hidden TAPE */
    poly_enable(project.pat,(uint8_t *)&project.sound);assert(poly_valid(project.pat,(uint8_t *)&project.sound));
    for(cell=0;cell<sizeof(sound_t);cell++)if(!seen[cell])assert(((uint8_t *)&project.sound)[cell]==original[cell]);
    for(p=0;p<NPAT;p++)for(l=0;l<POLY_LANES;l++)for(r=0;r<8;r++)for(n=1;n<=64;n++){
        poly_set(project.pat,(uint8_t *)&project.sound,p,l,n,r);
        assert(poly_length(project.pat,(uint8_t *)&project.sound,p,l)==n && poly_rate(project.pat,(uint8_t *)&project.sound,p,l)==r);
        if(l>0 && l!=22)assert(poly_length(project.pat,(uint8_t *)&project.sound,p,l-1)==64);
    }
    for(p=0;p<NPAT;p++)for(s=0;s<64;s++)for(n=0;n<=8;n++){
        poly_break_set(project.pat,(uint8_t *)&project.sound,p,s,s&1,n);
        assert(poly_break_gate(project.pat,(uint8_t *)&project.sound,p,s)==(s&1));
        assert(poly_break_slice(project.pat,(uint8_t *)&project.sound,p,s)==n);
    }
    assert(poly_valid(project.pat,(uint8_t *)&project.sound));
    /* Independently indexed expected state, deliberately different across rows
     * and patterns. Recheck ALL rows after each seeded mutation. */
    for(p=0;p<32;p++)for(l=0;l<25;l++){lengths[p][l]=(uint8_t)(1+random_value()%64);rates[p][l]=(uint8_t)(random_value()%8);poly_set(project.pat,(uint8_t *)&project.sound,p,l,lengths[p][l],rates[p][l]);}
    for(n=0;n<300;n++){
        int pat=(int)(random_value()%32),lane=(int)(random_value()%25);
        if(n&1)rates[pat][lane]=(uint8_t)(random_value()%8);else lengths[pat][lane]=(uint8_t)(1+random_value()%64);
        poly_set(project.pat,(uint8_t *)&project.sound,pat,lane,lengths[pat][lane],rates[pat][lane]);
        for(p=0;p<32;p++)for(l=0;l<25;l++){assert(poly_length(project.pat,(uint8_t *)&project.sound,p,l)==lengths[p][l]);assert(poly_rate(project.pat,(uint8_t *)&project.sound,p,l)==rates[p][l]);}
    }
    for(p=0;p<32;p++)for(s=0;s<64;s++){gates[p][s]=(uint8_t)(random_value()%2);slices[p][s]=(uint8_t)(random_value()%9);poly_break_set(project.pat,(uint8_t *)&project.sound,p,s,gates[p][s],slices[p][s]);}
    for(n=0;n<200;n++){
        int pat=(int)(random_value()%32),step=(int)(random_value()%64);
        gates[pat][step]^=1;slices[pat][step]=(uint8_t)(random_value()%9);
        poly_break_set(project.pat,(uint8_t *)&project.sound,pat,step,gates[pat][step],slices[pat][step]);
        for(p=0;p<32;p++)for(s=0;s<64;s++){assert(poly_break_gate(project.pat,(uint8_t *)&project.sound,p,s)==gates[p][s]);assert(poly_break_slice(project.pat,(uint8_t *)&project.sound,p,s)==slices[p][s]);}
    }
    for(p=0;p<32;p++)for(l=0;l<2;l++)assert(project.pat[p].bass[l].gen.seed==(unsigned)(l?p+3:p+1));
    for(cell=0;cell<sizeof(sound_t);cell++)if(!seen[cell])assert(((uint8_t *)&project.sound)[cell]==original[cell]);
    /* Exact malformed header rejection at its actual mapped byte. */
    ((uint8_t *)&project.sound)[poly_sound_offset(0)]^=1;assert(!poly_valid(project.pat,(uint8_t *)&project.sound));
    printf("lane layout: sound=%zu pattern=%zu project=%zu shadow=%zu seq=%zu row=%zu payload=%u header=%u\n",sizeof(sound_t),sizeof(pattern_t),sizeof(project_t),sizeof(project_t)-__builtin_offsetof(project_t,sound),sizeof(seq_t),sizeof(seq_lane_clock_t),POLY_PAYLOAD,POLY_BYTES-POLY_PAYLOAD);
    puts("lane packed model: all32 patterns x25 lanes x8 rates x64 lengths;64 gates/slices;no unowned writes PASS");
    return 0;
}
