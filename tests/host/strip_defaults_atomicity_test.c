/* SPDX-License-Identifier: GPL-3.0-only */
/* Real UI/project/engine, with Stop injected at two actual host boundaries. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int stop_read, stop_ms;
static void read_boundary(void);
static void ms_boundary(void);
#define HOST_STORE_PROBE(writing) ((writing)?0:(read_boundary(),0))
#define HOST_MS_PROBE() ms_boundary()
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
static void read_boundary(void) {
 if(stop_read){stop_read=0;assert(seq.transport_hold);seq_stop(&seq);}
}
static void ms_boundary(void) {
 if(stop_ms && external_edit){stop_ms=0;assert(seq.transport_hold);seq_stop(&seq);}
}
static int action(unsigned lane,int apply){
 variant_command_t c={1,apply?21:20,0,(uint8_t)lane,0,0,0,0};variant_reply_t r;
 return variant_command(&c,&r);
}
static void render(void){int32_t audio[256];engine_render(audio,128);}
static void set(unsigned lane,unsigned field,int value){
 variant_command_t c={1,VC_SET_STRIP,0,(uint8_t)lane,0,(uint8_t)field,0,value};variant_reply_t r;
 assert(variant_command(&c,&r)==VR_QUEUED);render();
}
int main(void){
 for(unsigned i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
 boot();render();assert(action(0,0)==VR_OK);
 sound_strip_config_t saved=proj.variant.strip[0];
 set(0,VP_RELEASE,250);ui_external_edit(0);
 sound_strip_config_t current=proj.variant.strip[0];
 sound_strip_config_t other=proj.variant.strip[1];
 /* An unrelated ordinary edit is still pending in the quiet journal. */
 set(1,VP_BOOST,11);
 assert(!external_edit&&undo_ready&&memcmp(undo.region,undo.shadow,undo.len));
 undo_t journal=undo;uint8_t shadow[sizeof undo_shadow];memcpy(shadow,undo_shadow,sizeof shadow);
 project_t before=proj;uint32_t written=engine_queue_written(),reads=engine_queue_read(),writes=store_writes;
 stop_read=1;assert(action(0,1)==VR_BUSY&&!stop_read);
 assert(seq.req_stop&&!seq.playing&&!seq.transport_hold);
 assert(!memcmp(&before,&proj,sizeof proj)&&!memcmp(&journal,&undo,sizeof undo));
 assert(!memcmp(shadow,undo_shadow,sizeof shadow));
 assert(written==engine_queue_written()&&reads==engine_queue_read()&&writes==store_writes);
 render();assert(!seq.req_stop&&!seq.playing);
 /* Accept before committing pending history. A later Stop is ordered after
  * acceptance and must neither reject recall nor be consumed by its hold. */
 unsigned prior_n=undo.n;stop_ms=1;written=engine_queue_written();
 assert(action(0,1)==VR_OK&&!stop_ms);
 assert(seq.req_stop&&!seq.playing&&!seq.req_start&&!seq.req_cont&&!seq.transport_hold);
 assert(engine_queue_written()==written+VP_COUNT);
 assert(!memcmp(&proj.variant.strip[0],&saved,sizeof saved));
 assert(proj.variant.strip[1].boost_db==11&&undo.n==prior_n+2);
 render();assert(!seq.playing&&!seq.req_stop&&!seq.req_start&&!seq.req_cont);
 assert(!memcmp(engine_strip_config(0),&saved,sizeof saved));
 assert(ui_external_undo(0)==VR_OK);render();
 assert(!memcmp(&proj.variant.strip[0],&current,sizeof current));
 assert(proj.variant.strip[1].boost_db==11);
 assert(ui_external_undo(0)==VR_OK);render();
 assert(!memcmp(&proj.variant.strip[1],&other,sizeof other));
 assert(!memcmp(&proj.variant.strip[0],&current,sizeof current));
 puts("Strip defaults actual UI atomicity: read-Stop refuses without journal mutation; accepted recall retains late Stop and independent Undo PASS");
 return 0;
}
