/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define X0X_WEB 1
#include <stdint.h>
static void web_audio(const int32_t *p,uint32_t n){(void)p;(void)n;}
#define plat_control_enter host_unmodeled_enter
#define plat_control_leave host_unmodeled_leave
#include "../../host/x0x_host.c"
#undef plat_control_enter
#undef plat_control_leave
#include "control_guard_model.h"
#include <assert.h>
void web_seq_tick(uint32_t n){(void)n;}
void web_seq_transport(int start,int top){(void)start;(void)top;}
void web_slice_audio(uint32_t n){(void)n;}
#include "../../firmware/src/app/engine.c"
static int32_t audio[512];
static uint32_t packet(unsigned st,unsigned note,unsigned vel){return(st>=248?15u:st>>4)|(st<<8)|(note<<16)|(vel<<24);}
static void render(void){unsigned guard=512;do{plat_midi_service();engine_render(audio,256);assert(guard--);}while(mi_r!=mi_w);}
static unsigned request_boundary_armed, boundary_kind;
void clock_run_request_boundary(seq_t *s){if(request_boundary_armed){request_boundary_armed=0;
 if(boundary_kind){uint32_t generation;
  if(boundary_kind==3){host_web_midi_ready(0);host_web_midi_ready(1);assert(s->clock.flags&CLOCK_RETIRED);return;}
  if(boundary_kind==1)assert(!engine_clock_source_retire(CLOCK_PROTOCOL,1,&generation));
  else assert(!engine_clock_request(CLOCK_PROTOCOL,CLOCK_STANDALONE,0,0,&generation));
  render();assert(!s->clock.pending&&s->clock.generation==generation);
 }else{render();assert(s->clock.pending&&s->out_stop_pending);}}}
static unsigned consume(void){unsigned n=0;while(host_web_midi_pending()){int k=host_web_midi_batch(64);if(k<0)k=web_midi_latched;for(int i=0;i<k;i++)if(!(host_web_midi_packet(i)>>31))n++;assert(!host_web_midi_ack(k));}return n;}
static void fresh(void){host_web_midi_ready(0);mi_w=mi_r=mi_ready=0;memset(mi_clock_retired,0,sizeof mi_clock_retired);web_midi_w=web_midi_r=web_midi_latched=web_midi_cancel_pending=web_midi_transport_cancel_pending=0;project_defaults();boot();host_web_midi_ready(1);seq.send_clock=1;seq.send_notes=0;render();consume();}
static void ingest(unsigned source,unsigned st){assert(host_web_midi_ingest(packet(st,0,0),source)==CC_APPLIED);}
static uint32_t mode(unsigned role,unsigned source){uint32_t g;assert(!engine_clock_request(CLOCK_PROTOCOL,role,source,0,&g));assert(seq.clock.generation!=g);render();assert(seq.clock.generation==g&&(seq.clock.flags&CLOCK_CONFIG_APPLIED));assert((seq.clock.role_source&3)==role&&!seq.playing);consume();return g;}
int main(void){
 fresh();uint32_t g=seq.clock.generation,w=cq_w,out[10];
 assert(engine_clock_request(0,CLOCK_FOLLOW,0,0,&g)==-1);
 assert(engine_clock_request(CLOCK_PROTOCOL,4,0,0,&g)==-1);
 assert(engine_clock_request(CLOCK_PROTOCOL,CLOCK_FOLLOW,0,1,&g)==-1);
 assert(engine_clock_status(0,out)==-1&&cq_w==w);
 /* Two-word command commits together; producer refuses occupied/full queue. */
 cq_w=1024;cq_r=0;assert(engine_clock_request(CLOCK_PROTOCOL,3,1,0,&g)==-2);cq_w=cq_r=0xffffffffu;
 assert(!engine_clock_request(CLOCK_PROTOCOL,3,1,0,&g));assert(cq_w==1u);
 assert(engine_clock_request(CLOCK_PROTOCOL,2,0,0,0)==-2);render();assert(seq.clock.generation==g&&seq.clock.role_source==7&&cq_r==cq_w);consume();
 /* Only selected source transport/clock; notes remain independently admitted. */
 ingest(0,250);ingest(2,248);render();assert(!seq.playing&&seq.clock.tick_lo==0);
 ingest(1,250);ingest(1,248);render();assert(seq.playing&&seq.ext&&seq.clock.tick_lo==1);assert(consume()==0);
 ingest(0,252);ingest(2,251);render();assert(seq.playing);
 assert(host_web_midi_ingest(packet(0x91,60,100),2)==CC_APPLIED);render();assert(bass_held[0]==60);
 assert(host_web_midi_ingest(packet(0x81,60,0),0)==CC_APPLIED);render();assert(bass_held[0]==255);
 assert(cc_parameter(954,3,2)==0);render();assert(seq.ch[TRK_BASS0]==2&&seq.playing&&seq.clock.role_source==7);
 seq.t[0].pos=7;seq.t[0].started=1;seq.t[0].bars=12;ingest(1,252);render();assert(!seq.playing&&seq.t[0].pos==7&&seq.t[0].bars==12);
 ingest(1,251);ingest(1,248);render();assert(seq.playing&&seq.t[0].pos==8&&seq.t[0].bars==12);
 /* Loss freezes, releases ownership, never falls back; clocks alone never run. */
 seq.bass_notes_out[0]=70;seq.bass_channels_out[0]=2;g=seq.clock.generation;for(int i=0;i<90;i++)render();
 assert(!seq.playing&&seq.ext&&seq.clock.state==CLOCK_LOST&&seq.clock.generation==g+1&&seq.bass_notes_out[0]==255);assert(consume()==1);
 uint32_t ticks=seq.clock.tick_lo;ingest(0,250);ingest(1,248);render();assert(!seq.playing&&seq.clock.tick_lo==ticks+1&&seq.clock.state==CLOCK_LOST);
 ingest(1,251);ingest(1,248);render();assert(seq.playing);
 /* Generation retirement before immediate reconnect blocks queued old RT;
  * fresh packets before the retirement acknowledgment are discarded too. */
 g=seq.clock.generation;ingest(1,250);host_web_midi_ready(0);host_web_midi_ready(1);ingest(1,248);render();assert(!seq.playing&&seq.clock.generation==g+1&&seq.clock.state==CLOCK_LOST);
 ingest(1,250);ingest(1,248);render();assert(seq.playing);
 /* Cached external deadline keeps all transition/cursor/loss/reset semantics.
  * clk_to_next is never an internal pulse timer while explicit Follow is active. */
 fresh();mode(CLOCK_FOLLOW,1);assert(seq.ext&&!seq.playing);seq_start(&seq);render();
 assert(seq.playing&&(seq.clk_to_next>>8)==FS/2u+1u-seq.ext_since);
 seq_stop(&seq);render();assert(!seq.playing&&seq.ext);seq_continue(&seq);render();
 assert(seq.playing&&(seq.clk_to_next>>8)==FS/2u+1u-seq.ext_since);
 ingest(1,248);render();assert((seq.clk_to_next>>8)==FS/2u+1u-seq.ext_since);
 assert(!engine_clock_source_retire(CLOCK_PROTOCOL,1,&g));render();assert(!seq.playing&&seq.ext);
 ingest(1,248);render();assert(!seq.playing);ingest(1,250);render();assert(seq.playing&&(seq.clk_to_next>>8)==FS/2u+1u-seq.ext_since);
 mode(CLOCK_LEAD,0);assert(!seq.ext&&!seq.playing&&seq.clk_to_next==0);
 seq.send_clock=0;seq_start(&seq);render();assert(seq.playing&&seq.clk_to_next>0);consume();
 project_defaults();boot();render();assert((seq.clock.role_source&3u)==CLOCK_LEGACY&&!seq.playing);
 /* Exact native audio deadline: equality is still acquired; +1 sample freezes. */
 fresh();mode(CLOCK_FOLLOW,1);ingest(1,250);ingest(1,248);render();seq.ext_since=0;seq.clock.age=0;g=seq.clock.generation;
 seq_advance(&seq,FS/2,&SINK);assert(seq.playing&&seq.ext_since==FS/2);assert(seq_until_event(&seq,16)==1);
 seq_advance(&seq,1,&SINK);assert(!seq.playing&&seq.ext&&seq.clock.state==CLOCK_LOST&&seq.clock.generation==g+1&&seq.clock.age==FS/2+1);
 /* Explicit selected-input retirement is a bounded applied-generation intent.
  * Unrelated source retirement preserves the selected source's queued clock. */
 fresh();mode(CLOCK_FOLLOW,1);ingest(1,250);ingest(1,248);render();ticks=seq.clock.tick_lo;
 ingest(2,248);ingest(1,248);assert(!engine_clock_source_retire(CLOCK_PROTOCOL,2,&g));render();assert(seq.playing&&seq.clock.generation==g&&seq.clock.tick_lo==ticks+1);
 ingest(1,250);assert(!engine_clock_source_retire(CLOCK_PROTOCOL,1,&g));render();assert(!seq.playing&&seq.clock.generation==g&&(seq.clock.flags&CLOCK_INVALIDATED));
 ingest(1,248);render();assert(!seq.playing);ingest(1,250);render();assert(seq.playing);
 /* A retired source and an immediately committed role both invalidate
  * intent captured before the interrupt, even with pending already clear. */
 for(unsigned enabled=0;enabled<2;enabled++)for(unsigned nested=0;nested<2;nested++)
 for(boundary_kind=1;boundary_kind<=3;boundary_kind++)for(unsigned route=0;route<3;route++){
  const char *only=getenv("CLOCK_BOUNDARY_CASE");
  if(only&&atoi(only)!=(int)boundary_kind)continue;
  fresh();mode(CLOCK_FOLLOW,1);test_icfg=enabled?0x300u:0x200u;
  uint32_t outer=nested?plat_control_enter():0,prior=test_icfg;request_boundary_armed=1;
  if(route==0)seq_start(&seq);
  else if(route==1)seq_continue(&seq);
  else{held_btn=0;ui_input_only();held_btn=1u<<B_PLAY;ui_input_only();held_btn=0;ui_input_only();}
  assert(test_icfg==prior);if(nested){plat_control_leave(outer);assert(test_icfg==(enabled?0x300u:0x200u));}
  assert(!request_boundary_armed&&!seq.req_start&&!seq.req_cont);
  render();assert(!seq.playing&&consume()==0);
  /* New explicit intent after the boundary remains usable. */
  seq_start(&seq);render();assert(seq.playing);consume();
 }
 boundary_kind=0;
 /* Role change waits for old FC and owned cleanup under full output. */
 fresh();mode(CLOCK_LEAD,0);seq_start(&seq);render();consume();
 for(unsigned i=0;i<64;i++)assert(plat_midi_out(packet(248,0,0)));
 seq.pad_channel_out=8;seq.pad_notes_out[0]=60;seq.pad_notes_out[1]=64;seq.pad_notes_out[2]=67;
 assert(!engine_clock_request(CLOCK_PROTOCOL,CLOCK_FOLLOW,2,0,&g));render();assert(!seq.playing&&seq.clock.pending&&seq.out_stop_pending&&seq.clock.generation!=g);assert(consume()==0);
 render();consume();render();assert(seq.clock.generation==g&&seq.clock.role_source==11&&seq.pad_channel_out==255&&!seq.playing);
 /* Sequential ordinary inputs while pending are already rejected. A test-only
  * IRQ boundary proves late helper/UI publication cannot emit old-role FA/FB. */
 for(unsigned route=0;route<5;route++){
  fresh();mode(CLOCK_LEAD,0);seq_start(&seq);render();consume();
  if(route==2){seq_stop(&seq);render();consume();} /* physical PLAY chooses Start while stopped */
  for(unsigned i=0;i<64;i++)assert(plat_midi_out(packet(248,0,0)));
  assert(!engine_clock_request(CLOCK_PROTOCOL,CLOCK_FOLLOW,2,0,&g));
  if(route<3){
   request_boundary_armed=1;
   if(route==0)seq_start(&seq);
   else if(route==1)seq_continue(&seq);
   else{held_btn=0;ui_input_only();held_btn=1u<<B_PLAY;ui_input_only();held_btn=0;ui_input_only();}
   assert(!request_boundary_armed);
  }else{
   render();seq_start(&seq);seq_continue(&seq);assert(!seq.req_start&&!seq.req_cont);
   if(route==3){assert(host_web_midi_ingest(packet(0xbc,95,127),0)==CC_APPLIED);render();assert(!seq.req_start&&(cc_sources[0].action_high&4u));uint32_t stopped=seq.stop_epoch;assert(host_web_midi_ingest(packet(0xbc,96,127),0)==CC_APPLIED);render();assert(seq.stop_epoch==stopped+1&&!seq.playing);}
   else{held_btn=0;ui_input_only();held_btn=1u<<B_PLAY;ui_input_only();held_btn=0;ui_input_only();assert(!seq.req_start);ingest(2,250);ingest(2,251);assert(host_web_midi_ingest(packet(0x90|seq.ch[TRK_BASS0],60,100),0)==CC_APPLIED);}
  }
  assert(consume()==0);render();assert(seq.clock.generation==g&&seq.clock.role_source==11&&!seq.playing&&!seq.req_start&&!seq.req_cont);
  int n=host_web_midi_batch(64);unsigned fc=0,run=0;assert(n>0);
  for(int i=0;i<n;i++){uint32_t p=host_web_midi_packet(i);if(!(p>>31)){unsigned st=(p>>8)&255u;fc+=st==252u;run+=st==250u||st==251u;}}
  assert(fc==1&&run==0);assert(!host_web_midi_ack(n));if(route==4){assert(bass_held[0]==60);assert(host_web_midi_ingest(packet(0x80|seq.ch[TRK_BASS0],60,0),1)==CC_APPLIED);render();assert(bass_held[0]==255&&!seq.playing);}
 }
 /* Follow cleanup is also a run barrier even though it emits no realtime. */
 fresh();mode(CLOCK_FOLLOW,1);ingest(1,250);ingest(1,248);render();
 for(unsigned i=0;i<64;i++)assert(plat_midi_out(packet(128,i,0)));
 seq.pad_channel_out=8;seq.pad_notes_out[0]=60;seq.pad_notes_out[1]=64;seq.pad_notes_out[2]=67;
 ingest(1,252);ingest(1,250);render();assert(!seq.playing&&seq.req_start&&seq.pad_channel_out!=255);
 consume();render();assert(seq.playing&&seq.pad_channel_out==255);assert(consume()==3);
 /* Start after waiting longer than the loss threshold grants one fresh clock
  * deadline; stale prior age cannot cancel a new explicit owner intent. */
 fresh();mode(CLOCK_FOLLOW,1);for(int i=0;i<90;i++)render();ingest(1,250);render();assert(seq.playing&&seq.ext);ingest(1,248);render();assert(seq.playing);
 /* Main reset retires both halves before audio dispatch and preserves count. */
 fresh();mode(CLOCK_LEAD,0);seq.send_clock=0;seq_start(&seq);render();assert(seq.clock.tick_lo>0);consume();ticks=seq.clock.tick_lo;g=seq.clock.generation;
 assert(!engine_clock_request(CLOCK_PROTOCOL,CLOCK_FOLLOW,1,0,0));project_defaults();boot();render();assert(!seq.playing&&seq.clock.role_source==CLOCK_LEGACY&&seq.clock.tick_lo==ticks&&seq.clock.generation>g&&!seq.clock.pending);
 /* A session retirement between enqueue/drain cannot accidentally apply the
  * old transaction even if the new generation equals its expected generation. */
 fresh();assert(!engine_clock_request(CLOCK_PROTOCOL,CLOCK_FOLLOW,1,0,&g));seq_transport_retire(&seq);render();assert(seq.clock.role_source==0&&!seq.clock.pending&&(seq.clock.flags&CLOCK_INVALIDATED)&&!(seq.clock.flags&CLOCK_CONFIG_APPLIED));
 /* Standalone emits no RT; Lead ignores all input RT despite send_clock0. */
 fresh();mode(CLOCK_STANDALONE,0);seq.send_clock=0;ingest(2,250);render();assert(!seq.playing);seq_start(&seq);render();assert(seq.playing&&seq.clock.tick_lo>0&&consume()==0);
 fresh();mode(CLOCK_LEAD,0);seq.send_clock=0;seq_start(&seq);render();assert(seq.playing&&consume()>=2);ingest(1,252);ingest(1,248);render();assert(seq.playing&&!seq.ext);
 /* Legacy acquisition/fallback preserved, absolute ticks independent of Start. */
 fresh();ingest(0,250);ingest(2,248);render();assert(seq.playing&&seq.ext);for(int i=0;i<90;i++)render();assert(seq.playing&&!seq.ext);assert(!engine_clock_status(CLOCK_PROTOCOL,out));assert(out[0]==CLOCK_PROTOCOL&&out[5]==seq.clock.tick_lo);
 seq.clock.tick_lo=seq.clock.tick_hi=UINT32_MAX;seq.ext_pending=1;seq_advance(&seq,0,&SINK);assert(seq.clock.tick_lo==UINT32_MAX&&seq.clock.tick_hi==UINT32_MAX);
 printf("PASS actual engine clock roles: source collision, independent notes, Start/Continue cursor, loss freeze/Off, non-auto reconnect, blocked mode cleanup, CQ full/wrap/reset/retirement, transition Start/Continue/UI publication races and raw CC Stop priority, commit ingress cutoff, explicit Lead/Standalone, legacy fallback; runtime metadata=%zu bytes\n",sizeof(clock_runtime_t));
 return 0;
}
