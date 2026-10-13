/* SPDX-License-Identifier: GPL-3.0-only */
/* Compiled independently against the pinned pre-lane and current source trees. */
#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
int main(int argc,char **argv)
{
    FILE *out;int i,v,s;int32_t pcm[512];
    assert(argc==2);out=fopen(argv[1],"wb");assert(out);
    for(i=0;i<OBJ_NOBJ;i++)store_len[i]=-1;boot();engine_render(pcm,256);
    engine_guard_enable(0);seq.send_clock=seq.send_notes=0;
    for(i=0;i<2;i++){
        proj.pat[0].drum[i].len=(uint8_t)(i?23:19);proj.pat[0].drum[i].rate=(uint8_t)(i?3:1);
        for(v=0;v<11;v++)for(s=0;s<64;s++)if((s*3+v+i)%7<2)sm_set(proj.pat[0].drum[i].hit[v],s);
        proj.pat[0].drum[i].accent[0]=0x4a211102u;
    }
    proj.pat[0].swing=37;proj.pat[0].brk.steps=0xa529;
    for(s=0;s<16;s++)proj.pat[0].brk.slice[s]=(uint8_t)(s%9);
    for(i=0;i<2;i++){
        bpart_t *b=&proj.pat[0].bass[i];b->len=(uint8_t)(i?7:31);b->rate=(uint8_t)(i?2:0);b->dir=(uint8_t)(i?DIR_REV:DIR_RANDOM);
        for(s=0;s<64;s++){b->step[s].note=(uint8_t)(36+(s*5+i*7)%36);b->step[s].flags=(uint8_t)(s%7==2?G_TIE:s%9==0?G_REST:G_NOTE);if(s%5==0)b->step[s].flags|=BS_SLIDE;if(s%3==0)b->step[s].flags|=BS_ACCENT;}
    }
    seq_start(&seq);
    for(i=0;i<1024;i++){engine_render(pcm,256);assert(fwrite(pcm,sizeof pcm,1,out)==1);}
    assert(!fclose(out));return 0;
}
