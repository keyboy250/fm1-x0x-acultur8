/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual shared physical platform service + USB consumer + sequencer. */
#define CLOCK_USB_NO_MAIN
#include "clock_usb_consumer_test.c"
#include "../../firmware/src/seq/sequencer.h"
seq_t seq;static unsigned source_resets;static pattern_t patterns[NPAT];
void cc_reset_source(uint8_t source){assert(source==0);source_resets++;plat_midi_clear_cc_source(source);}
static int inject_reset_observation;
static void reset_reconfigure(void){usb.resets++;midi_usb_state(0,0,1);midi_usb_state(1,0,0);}
static void observation_hook(void){if(inject_reset_observation){assert(lock_depth==1&&!irq_enabled);inject_reset_observation=0;deferred_irq=reset_reconfigure;}}
#define MIDI_SERVICE_OBSERVE_HOOK() observation_hook()
#include "../../firmware/src/app/midi_fm1.inc"
static void send_midi(void *x,uint8_t st,uint8_t n,uint8_t v){(void)x;(void)plat_midi_out(pk(st,n,v));}
static int try_midi(void *x,uint8_t st,uint8_t n,uint8_t v){(void)x;return plat_midi_out(pk(st,n,v));}
static void cancel_notes(void *x){(void)x;plat_midi_cancel();}
static void cancel_transport(void *x){(void)x;plat_midi_transport_cancel();}
static const seq_sink_t sink={.midi=send_midi,.midi_try=try_midi,.midi_cancel=cancel_notes,.midi_transport_cancel=cancel_transport};
static void fresh(void){reset();seq_init(&seq,patterns);seq.send_notes=0;seq.midi_ready=1;plat_midi_service();nw=0;}
int main(void){
 fresh();seq.playing=1;for(int i=0;i<64;i++)assert(plat_midi_out(pk(248,0,0)));seq_stop(&seq);seq_start(&seq);seq_advance(&seq,0,&sink);assert(seq.out_stop_pending==1&&seq.req_start&&!seq.playing);unsigned resets=source_resets;midi_usb_state(1,1,0);plat_midi_service();assert(!seq.out_stop_pending&&!seq.req_start&&source_resets==resets+1);midi_usb_state(1,0,0);assert(!plat_midi_connected()&&!plat_midi_out(pk(252,0,0)));plat_midi_service();for(int i=0;i<4;i++)next();seq_advance(&seq,0,&sink);next();assert(!seq.playing&&nw==0);
 /* Reset/reconfigure between main-service calls is caught by reset generation,
  * even though connected remains true. Old request is local-only, never FC. */
 fresh();seq.playing=1;seq_stop(&seq);resets=source_resets;reset_reconfigure();assert(!plat_midi_connected());plat_midi_service();assert(seq.req_stop&&seq.out_stop_pending==2&&source_resets==resets+1);seq_advance(&seq,0,&sink);next();assert(!seq.playing&&nw==0&&seq.stop_epoch==1);seq_stop(&seq);seq_advance(&seq,0,&sink);next();assert(nw==1&&status(wire[0])==252);
 /* Owned Off survives cancellation/reconnect; Start/Continue never revive. */
 fresh();assert(plat_midi_out(pk(128,60,0)));assert(plat_midi_out(pk(250,0,0)));seq_continue(&seq);reset_reconfigure();plat_midi_service();assert(!seq.req_cont);next();assert(nw==1&&wire[0]==pk(128,60,0));
 /* The retirement subcritical section restores prior disabled state. Service
  * itself is main-only; CC handlers here are transport fixtures, not IRQ proof. */
 fresh();irq_enabled=0;reset_reconfigure();plat_midi_service();assert(!irq_enabled&&!lock_depth);
 /* Original TOCTOU boundary: a reset requested between compare and adoption is
  * deferred by the guard. Generation then differs, blocking any audio retry
  * until the next serialized observation retires old transport. */
 fresh();seq.out_stop_pending=1;seq.req_start=1;inject_reset_observation=1;plat_midi_service();assert(usb_midi_generation!=usb_midi_observed_generation&&!plat_midi_connected());seq_advance(&seq,0,&sink);assert(seq.out_stop_pending&&seq.req_start&&!seq.playing);next();assert(nw==0);plat_midi_service();assert(!seq.out_stop_pending&&!seq.req_start&&plat_midi_connected());seq_advance(&seq,0,&sink);next();assert(nw==0);
 /* Both kinds of disconnect/reconnect entirely between main calls are seen. */
 fresh();seq.out_stop_pending=1;seq.req_cont=1;midi_usb_state(0,0,0);midi_usb_state(1,0,0);assert(!plat_midi_connected());plat_midi_service();assert(!seq.out_stop_pending&&!seq.req_cont);fresh();seq.out_stop_pending=1;seq.req_start=1;midi_usb_state(1,1,0);midi_usb_state(1,0,0);assert(!plat_midi_connected());plat_midi_service();assert(!seq.out_stop_pending&&!seq.req_start);
 fresh();usb_midi_generation=UINT32_MAX-1;usb_midi_observed_generation=UINT32_MAX-1;midi_usb_state(0,0,0);midi_usb_state(1,0,0);plat_midi_service();assert(usb_midi_generation==UINT32_MAX&&!plat_midi_connected());
 /* Exact old ingress prefix: old USB realtime cannot resurrect playback; notes,
  * CC and TRS realtime remain independent. Fresh USB Start after retirement is
  * admitted. Generation mismatch also drops USB realtime before main observes. */
 fresh();assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,pk(250,0,0),0));assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,pk(248,0,0),0));assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,pk(128,60,0),0));assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,pk(248,0,0),1));reset_reconfigure();plat_midi_service();uint32_t input;assert(plat_midi_in(&input)&&status(input)==128);assert(plat_midi_in(&input)&&status(input)==248&&((input>>4)&15)==1);assert(!plat_midi_in(&input)&&mi_r==mi_w);assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,pk(250,0,0),0));plat_midi_service();assert(plat_midi_in(&input)&&status(input)==250);
 fresh();assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,pk(251,0,0),0));cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);reset_reconfigure();assert(!plat_midi_in(&input));plat_midi_service();assert(!seq.req_start&&!seq.req_cont);
 /* Full/wrapped ingress, partly consumed before retirement, and fresh Start
  * appended behind the captured prefix. CC review is preserved independently. */
 fresh();reviewed_cc=0;mi_w=mi_r=mi_ready=0xfffffff0u;for(unsigned i=0;i<64;i++){
  unsigned source=i%2?1:0;uint32_t p=pk(248,0,0);
  if(i<48&&i%4==2){p=pk(128,60,0);source=0;}
  if(i<48&&i%4==3){p=11u|(176u<<8)|(1u<<16)|(42u<<24);source=1;}
  assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,p,(uint8_t)source));
 }cc_ingress_service(midi_in_q,&mi_w,&mi_r,MQ);for(int i=0;i<3;i++)assert(plat_midi_in(&input));assert(mi_r==0xfffffff3u);reset_reconfigure();plat_midi_service();assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,pk(250,0,0),0));unsigned off_count=0,trs_count=0,start_count=0,guard=0;
 while(mi_r!=mi_w){assert(guard++<80);while(plat_midi_in(&input)){if(status(input)==128)off_count++;else if(status(input)==248){assert(((input>>4)&15u)==1);trs_count++;}else{assert(status(input)==250&&((input>>4)&15u)==0);start_count++;}}plat_midi_service();}
 assert(off_count==11&&trs_count==19&&start_count==1&&reviewed_cc==12&&mi_retired_clock[0]==0);
 /* Source reset deliberately tombstones unreviewed old USB CC, while other
  * source CC still reaches the real reviewed-prefix service. */
 fresh();reviewed_cc=0;uint32_t cc=11u|(176u<<8)|(1u<<16)|(42u<<24);assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,cc,0));assert(cc_ingress_put(midi_in_q,&mi_w,&mi_r,MQ,cc,1));reset_reconfigure();plat_midi_service();assert(reviewed_cc==1);while(plat_midi_in(&input)){}assert(mi_r==mi_w);
 printf("PASS actual physical platform service: serialized reset observation/race, config0->1 and suspend->resume between calls, generation gates audio before retirement, exact retired USB realtime ingress prefix/independent notes+TRS/freshStart, local-only Stop/no staleFC/freshFC, ownedOff, exact IRQ state and saturation; output prefix counters=%zu + session/observed generations=%zu + ingress counters=%zu declared bytes; target placement and physical delivery not tested\n",sizeof mo_cancel_attacks+sizeof mo_cancel_transport,sizeof usb_midi_generation+sizeof usb_midi_observed_generation,sizeof mi_retired_clock);return 0;}
