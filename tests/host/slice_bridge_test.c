/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual host-only exports; no marked-player or CC state stubs. */
#define _POSIX_C_SOURCE 200809L
#include "../../web/emu/x0x_web.c"
#include <assert.h>
int main(void){
 web_boot();web_render(256);assert(web_slice_cc_version()==0x10001u);
 assert(web_marked_repeat(-1)==-1&&web_marked_repeat(2)==-1&&web_marked_repeat(0)==-2);
 for(int i=0;i<200;i++)marked_pcm[i]=0.2f;
 marked_meta[0]=(marked_slice_t){0,200,0};
 assert(!web_marked_load(200,44100,1,1,200));web_marked_play(1);web_marked_render(51);
 uint32_t cursor=marked.cursor,triggers=marked.triggers,pos=marked.voice[0].pos;
 assert(!web_marked_repeat(0));assert(marked.cursor==cursor&&marked.triggers==triggers&&marked.voice[0].pos==pos);
 web_marked_render(149);assert(!marked.playing&&marked.cursor==200); /* next boundary, not cursor reset */
 assert(!cc_parameter(1010,7,2)&&!cc_parameter(1011,0,2)&&!cc_parameter(1132,19,2)&&!cc_parameter(1071,1,2));
 assert(!cc_parameter(1133,2,2));assert(cc_sources[2].armed==2);
 assert(!cc_parameter(1072,1107&127,2)&&!cc_parameter(1073,1107>>7,2));
 uint32_t value=123456;assert(!cc_parameter(1074,value&127,2)&&!cc_parameter(1075,(value>>7)&127,2)&&!cc_parameter(1130,value>>14,2));
 cc_source_t before=cc_sources[2];assert(web_cc_take_exact(1107,0x200000)==-1&&!memcmp(&before,&cc_sources[2],sizeof before));
 assert(web_cc_take_exact(0,value)==-1&&!memcmp(&before,&cc_sources[2],sizeof before));
 assert(web_cc_take_exact(1108,value)==-2&&!memcmp(&before,&cc_sources[2],sizeof before));
 assert(web_cc_take_exact(1107,value+1)==-2&&!memcmp(&before,&cc_sources[2],sizeof before));
 assert(!web_cc_take_exact(1107,value));before.exact_parts=0;assert(!memcmp(&before,&cc_sources[2],sizeof before));
 assert(web_cc_take_exact(1107,value)==-2);assert(cc_sources[2].armed==2&&cc_sources[2].destination_applied==19);
 ui_external_edit(1);assert(web_slice_control_state()&8u);ui_external_edit(0);
 seq.req_start=1;assert(web_slice_control_state()&2u);seq.req_start=0;
 seq.transport_hold=1;assert(web_slice_control_state()&4u);seq.transport_hold=0;
 seq.playing=1;assert(web_slice_control_state()&1u);seq.playing=0;
 perf.test=0;perf.phase=0;assert(web_slice_control_state()&16u);perf.test=-1;perf.phase=0;
 engine_set(T_909,0,0,13);assert(web_slice_control_state()&32u);web_render(256);
 puts("Slice HOST ABI1: exact21bit mismatch/consume preserves selectors/native arms/destination; real repeat next-boundary cursor/tails and authoritative gates PASS");return 0;
}
