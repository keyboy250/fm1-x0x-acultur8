/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define X0X_WEB 1
#include <stdint.h>
static void web_audio(const int32_t *p,uint32_t n){(void)p;(void)n;}
#include "../../host/x0x_host.c"
#include <assert.h>
void web_seq_tick(uint32_t n){(void)n;}
void web_seq_transport(int start,int top){(void)start;(void)top;}
void web_slice_audio(uint32_t n){(void)n;}
#include "../../firmware/src/app/engine.c"
static uint32_t pk(unsigned s,unsigned n,unsigned v){return(s>=248?15u:s>>4)|(s<<8)|(n<<16)|(v<<24);}
static uint32_t wire[1024];static unsigned nw;
static void consume(unsigned limit){while(limit&&host_web_midi_pending()){int n=host_web_midi_batch(16);if(n<0)n=(int)web_midi_latched;assert(n>0);if((unsigned)n>limit)n=(int)limit;for(int i=0;i<n;i++){uint32_t p=host_web_midi_packet(i);if(!(p>>31))wire[nw++]=p;}assert(!host_web_midi_ack(n));limit-=(unsigned)n;}}
static unsigned st(uint32_t p){return(p>>8)&255;}
static void fresh(void){host_web_midi_ready(0);web_midi_w=web_midi_r=web_midi_latched=web_midi_cancel_pending=web_midi_transport_cancel_pending=0;nw=0;project_defaults();boot();host_web_midi_ready(1);seq.send_clock=1;seq.send_notes=0;seq.ext=0;seq_midi_prepare(&seq,1,&SINK);}
static void advance(void){seq_advance(&seq,0,&SINK);}
int main(void){
 fresh();seq_start(&seq);advance();assert(seq.playing);consume(64);nw=0;
 /* Real shared sequencer + C FIFO: blocked Stop freezes position and cancels
  * fetches without changing slots, prefix lengths or ACK-only read cursor. */
 for(unsigned i=0;i<64;i++){assert(plat_midi_out(pk(248,0,0)));}
 uint32_t before[64];memcpy(before,web_midi_ring,sizeof before);uint32_t r=web_midi_r;assert(host_web_midi_batch(16)==16);
 seq.t[0].pos=7;seq.t[0].bars=12;seq_stop(&seq);advance();assert(!seq.playing&&seq.out_stop_pending);assert(seq.t[0].pos==7&&seq.t[0].bars==12);assert(web_midi_r==r&&web_midi_latched==16&&!memcmp(before,web_midi_ring,sizeof before));
 for(int i=0;i<16;i++){assert(host_web_midi_packet(i)>>31);}
 seq_continue(&seq);seq_start(&seq);advance();assert(!seq.playing&&seq.req_start&&!seq.req_cont&&seq.t[0].pos==7);consume(64);assert(nw==0);advance();consume(64);assert(seq.playing&&wire[0]==pk(252,0,0)&&wire[1]==pk(250,0,0));assert(seq.t[0].pos==0&&seq.t[0].bars==0);for(unsigned i=2;i<nw;i++)assert(st(wire[i])!=252);
 /* Repeated Stop cancels a queued Start and latest Stop never leaves FA/FB. */
 seq_stop(&seq);advance();seq_start(&seq);advance();seq_stop(&seq);advance();nw=0;consume(64);assert(!seq.playing);assert(nw==1&&st(wire[0])==252);
 /* Continue retains cursor/bar before release, then advances next legacy step. */
 seq.t[0].pos=7;seq.t[0].started=1;seq.t[0].bars=12;for(unsigned i=0;i<64;i++)assert(plat_midi_out(pk(128,i,0)));seq_stop(&seq);seq_continue(&seq);advance();assert(!seq.playing&&seq.req_cont&&seq.t[0].pos==7&&seq.t[0].bars==12);nw=0;consume(64);advance();consume(64);assert(seq.playing&&seq.t[0].pos==8&&seq.t[0].bars==12);assert(st(wire[64])==252&&st(wire[65])==251);
 /* Triad cleanup cannot be overtaken by a new run, including latched prefixes. */
 fresh();seq.playing=1;seq.pad_channel_out=8;seq.pad_notes_out[0]=60;seq.pad_notes_out[1]=64;seq.pad_notes_out[2]=67;seq.bass_notes_out[0]=70;seq.bass_channels_out[0]=2;for(unsigned i=0;i<64;i++)assert(plat_midi_out(pk(248,0,0)));seq_stop(&seq);seq_start(&seq);advance();assert(!seq.playing);consume(64);advance();consume(64);advance();consume(64);assert(seq.playing);unsigned off=0,fa=999;for(unsigned i=0;i<nw;i++){if((st(wire[i])&240)==128)off++;if(st(wire[i])==250)fa=i;}assert(off==4&&fa!=999);for(unsigned i=fa+1;i<nw;i++)assert((st(wire[i])&240)!=128);
 /* C retained slots after partial ACK carry cancellation independent of the
  * new batch's epoch. Owned Off survives; old Stop/Start and attack do not. */
 fresh();assert(plat_midi_out(pk(128,60,0)));assert(plat_midi_out(pk(248,0,0)));assert(plat_midi_out(pk(252,0,0)));assert(plat_midi_out(pk(250,0,0)));assert(plat_midi_out(pk(144,62,90)));assert(host_web_midi_batch(16)==5);assert(!host_web_midi_ack(1));r=web_midi_r;uint32_t e=host_web_midi_epoch();host_web_midi_cancel();assert(host_web_midi_epoch()==e+1&&web_midi_r==r&&web_midi_latched==4);for(int i=0;i<4;i++)assert(host_web_midi_packet(i)>>31);consume(64);assert(nw==0);
 /* Disconnect then immediate re-enable invalidates pending FC/run without
  * relying on another render; reset/load cleanup has the same request barrier. */
 fresh();seq.playing=1;for(unsigned i=0;i<64;i++)assert(plat_midi_out(pk(248,0,0)));seq_stop(&seq);seq_start(&seq);advance();assert(seq.out_stop_pending&&seq.req_start);host_web_midi_ready(0);host_web_midi_ready(1);assert(!seq.out_stop_pending&&!seq.req_start);consume(64);advance();consume(64);assert(!seq.playing&&nw==0);seq_start(&seq);engine_notes_clear();assert(seq.req_start);host_web_midi_cancel();assert(!seq.req_start);seq_continue(&seq);project_defaults();assert(!seq.req_cont);
 /* Wrap cannot misclassify new packets as old cancellation. */
 fresh();web_midi_w=web_midi_r=0xfffffffdu;assert(plat_midi_out(pk(144,62,90)));assert(host_web_midi_batch(16)==1);assert(!(host_web_midi_packet(0)>>31));assert(host_web_midi_ack(1)==0);assert(plat_midi_out(pk(248,0,0)));assert(plat_midi_out(pk(128,62,0)));assert(host_web_midi_batch(16)==2);host_web_midi_cancel();assert(host_web_midi_packet(0)>>31);assert(!(host_web_midi_packet(1)>>31));assert(host_web_midi_ack(2)==0);assert(web_midi_cancel_pending==0);assert(plat_midi_out(pk(250,0,0)));assert(host_web_midi_batch(16)==1);assert(!(host_web_midi_packet(0)>>31));assert(host_web_midi_ack(1)==0);
 /* Run the same blocked ordering through actual engine audio dispatch. */
 fresh();seq_start(&seq);int32_t audio[512];engine_render(audio,256);consume(64);nw=0;for(unsigned i=0;i<64;i++)assert(plat_midi_out(pk(248,0,0)));seq_stop(&seq);seq_start(&seq);engine_render(audio,256);assert(!seq.playing&&seq.out_stop_pending);consume(64);assert(nw==0);engine_render(audio,256);consume(64);assert(seq.playing&&st(wire[0])==252&&st(wire[1])==250);for(unsigned i=2;i<nw;i++)assert(st(wire[i])!=252);
 /* Reviewer regression: ordinary channel refresh must not retire pending FC,
  * staged run or an already-queued FC, nor invalidate its transport epoch. */
 fresh();seq.playing=1;for(unsigned i=0;i<64;i++)assert(plat_midi_out(pk(248,0,0)));seq_stop(&seq);seq_start(&seq);advance();assert(seq.out_stop_pending&&seq.req_start);uint32_t rt_epoch=host_web_midi_transport_epoch();assert(cc_parameter(954,3,0)==0);engine_render(audio,256);assert(seq.out_stop_pending&&seq.req_start&&!seq.playing);assert(host_web_midi_transport_epoch()==rt_epoch);consume(64);engine_render(audio,256);consume(64);assert(seq.playing&&st(wire[0])==252&&st(wire[1])==250);
 fresh();seq.playing=1;seq_stop(&seq);advance();assert(!seq.out_stop_pending&&host_web_midi_pending()==1);rt_epoch=host_web_midi_transport_epoch();assert(cc_parameter(954,4,0)==0);engine_render(audio,256);assert(host_web_midi_transport_epoch()==rt_epoch);assert(host_web_midi_batch(16)==1);assert(host_web_midi_packet(0)==pk(252,0,0));assert(host_web_midi_ack(1)==0);
 /* Start while running is an explicit cleanup/restart barrier; Continue while
  * already running is ignored, avoiding an owned-note cleanup deadlock. */
 fresh();seq.playing=1;seq.t[0].pos=7;seq.bass_notes_out[0]=70;seq.bass_channels_out[0]=2;seq_continue(&seq);assert(!seq.req_cont);seq_start(&seq);advance();consume(64);assert(seq.playing&&seq.t[0].pos==0);assert(st(wire[0])==130&&st(wire[1])==252&&st(wire[2])==250);
 /* Qualified native-action witnesses remain audio-owned/saturating. Blocked
  * output retries must not repeatedly advance a staged request's epoch. */
 fresh();seq.playing=1;for(unsigned i=0;i<64;i++)assert(plat_midi_out(pk(248,0,0)));uint32_t te=seq.transport_epoch,se=seq.stop_epoch;seq_stop(&seq);seq_start(&seq);advance();assert(seq.transport_epoch==te+1&&seq.stop_epoch==se+1);advance();assert(seq.transport_epoch==te+1&&seq.stop_epoch==se+1);consume(64);advance();assert(seq.transport_epoch==te+2&&seq.stop_epoch==se+1);consume(64);seq.transport_epoch=seq.stop_epoch=UINT32_MAX;seq_stop(&seq);advance();assert(seq.transport_epoch==UINT32_MAX&&seq.stop_epoch==UINT32_MAX);seq_transport_retire(&seq);assert(seq.transport_epoch==UINT32_MAX&&seq.stop_epoch==UINT32_MAX);
 /* An unconsumed old Stop remains a local safety Stop after route retirement,
  * but may not send FC to an immediate replacement connection. */
 fresh();seq.playing=1;seq_stop(&seq);host_web_midi_ready(0);host_web_midi_ready(1);assert(seq.req_stop&&seq.out_stop_pending==2);advance();consume(64);assert(!seq.playing&&nw==0&&seq.stop_epoch==1);seq_stop(&seq);advance();consume(64);assert(nw==1&&st(wire[0])==252);
 /* Trace consumed by the production bridge protocol fixture. */
 fresh();uint32_t trace_words[]={pk(128,60,0),pk(128,64,0),pk(248,0,0),pk(252,0,0),pk(250,0,0),pk(144,62,90)};for(unsigned i=0;i<6;i++)assert(plat_midi_out(trace_words[i]));assert(host_web_midi_batch(16)==6);uint32_t old_epoch=host_web_midi_epoch();FILE *f=fopen("build/host/c-prefix-trace.json","w");assert(f);fprintf(f,"{\"oldEpoch\":%u,\"oldFlight\":[",old_epoch);for(int i=0;i<6;i++)fprintf(f,"%s%u",i?",":"",host_web_midi_packet(i));host_web_midi_cancel();fprintf(f,"],\"newEpoch\":%u,\"cancelledFlight\":[",host_web_midi_epoch());for(int i=0;i<6;i++)fprintf(f,"%s%u",i?",":"",host_web_midi_packet(i));assert(host_web_midi_ack(1)==0);fprintf(f,"],\"retainedSuffix\":[");for(int i=0;i<5;i++)fprintf(f,"%s%u",i?",":"",host_web_midi_packet(i));fprintf(f,"],\"pendingAfterPartialAck\":%u,\"latchedAfterPartialAck\":%u}\n",host_web_midi_pending(),web_midi_latched);fclose(f);assert(web_midi_latched==5&&host_web_midi_ack(5)==0);
 puts("PASS actual engine: ACK-only cursor, retained-slot cutoff, blocked Stop/Start/Continue exact positions, repeated Stop, FC-before-run, triad/bass Off-before-new FA, immediate disconnect/re-enable and reset/load request invalidation");return 0;}
