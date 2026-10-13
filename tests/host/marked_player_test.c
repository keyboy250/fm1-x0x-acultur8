/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../firmware/src/dsp/marked_player.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    marked_player_t p;
    float pcm[128], l[128], r[128];
    marked_slice_t s[]={{0,4,0},{8,13,11},{16,21,23}};
    int i;
    for(i=0;i<64;i++){pcm[i*2]=(float)i/100;pcm[i*2+1]=-(float)i/100;}
    assert(marked_load(&p,pcm,64,44100,2,s,3,32)==0);
    marked_play(&p,0);marked_render(&p,l,r,32);
    assert(p.triggers==3&&p.last_frame==23&&!p.playing);
    assert(l[3]==.03f&&l[4]==0&&l[10]==0&&l[11]==.08f&&l[15]==.12f&&l[16]==0&&l[23]==.16f&&r[23]==-.16f);
    marked_play(&p,1);marked_render(&p,l,r,64);assert(p.triggers==6&&p.cursor==0);
    marked_stop(&p);marked_render(&p,l,r,32);for(i=0;i<32;i++)assert(l[i]==0&&r[i]==0);
    assert(marked_trigger(&p,2)==0);marked_render(&p,l,r,8);assert(l[0]==.16f&&l[4]==.20f&&l[5]==0);
    assert(marked_load(&p,pcm,64,44100,2,s,65,32)<0);
    s[1].end=100;assert(marked_load(&p,pcm,64,44100,2,s,3,32)<0);
    puts("marked player: irregular timing, boundaries, stereo, loop, pad, stop and invalid metadata passed");
    return 0;
}
