/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define X0X_WEB 1
#define PLAT_STORE_TEST 1
#include <stdint.h>
#include <assert.h>
static int flash_play_race;
static int calibration_probe(int writing);
#define HOST_STORE_PROBE(w) calibration_probe(w)
static void web_audio(const int32_t *p,uint32_t n){(void)p;(void)n;}
#include "../../host/x0x_host.c"
void web_seq_tick(uint32_t n){(void)n;}
void web_seq_transport(int a,int b){(void)a;(void)b;}
void web_slice_audio(uint32_t n){(void)n;}
#include "../../firmware/src/app/engine.c"
static int calibration_probe(int writing) {
 if(writing&&flash_play_race){flash_play_race=0;assert(seq.transport_hold);seq_start(&seq);}
 return 0;
}
static uint8_t ed_out[276];static unsigned ed_n;static int tx_idle=1;
static void ed_b(uint32_t v){assert(ed_n<sizeof ed_out);ed_out[ed_n++]=(uint8_t)(v&127);}
#define ED_TIMING_TX_IDLE() (tx_idle&&plat_midi_tx_idle())
#include "../../firmware/src/app/timing_query.inc"
static int32_t audio[512];
static void render(void){engine_render(audio,256);}
static void fresh(void){
 for(unsigned i=0;i<OBJ_NOBJ;i++)store_len[i]=store_prev_len[i]=-1;
 store_fail_after=-1;store_writes=0;project_defaults();boot();render();
}
static void query(const uint8_t *a,unsigned n){
 ed_n=0;ed_b(0x70); /* F0 byte is irrelevant to CRC; serializer normally writes it. */
 ed_b(0x7d);ed_b(0x46);ed_b(0x4c);ed_b(52);ed_timing_query(a,n);
 assert(ed_n==65);
}
static void tests(void){
 fresh();assert(timing_rate_get()==44100&&timing_rate_state.record_status==TC_MISSING);
 project_t before=proj;uint32_t w[10],writes=store_writes;
 engine_timing_status(w);assert(!w[8]&&store_writes==writes);
 assert(!engine_timing_update(44119,44100,0,0,17));assert(timing_rate_get()==44119);
 assert(store_len[OBJ_PANEL]==TC_RECORD_SIZE&&timing_rate_state.generation==1);
 assert(!memcmp(&before,&proj,sizeof proj));assert(tc_panel_valid(store[OBJ_PANEL]));
 timing_record_t rec=timing_store_read();assert(rec.rate==44119&&rec.unit==17&&rec.generation==1);
 writes=store_writes;assert(!engine_timing_update(44119,44119,1,17,17)&&store_writes==writes);
 assert(engine_timing_update(44120,44100,1,17,17)==3&&store_writes==writes);
 assert(engine_timing_update(44120,44119,0,17,17)==3&&store_writes==writes);
 assert(engine_timing_update(43899,44119,1,17,17)==1);
 seq.playing=1;assert(engine_timing_update(44120,44119,1,17,17)==2);seq.playing=0;
 seq.req_start=1;assert(engine_timing_update(44120,44119,1,17,17)==2);seq.req_start=0;
 seq.transport_stage=1;assert(engine_timing_update(44120,44119,1,17,17)==2);seq.transport_stage=0;
 cq_w=cq_r+1;assert(engine_timing_update(44120,44119,1,17,17)==2);cq_w=cq_r;
 seq.t[0].to_next=100000;seq.clk_to_next=1000;seq.ext=0;
 assert(!engine_timing_update(44200,44119,1,17,17));assert(seq.t[0].to_next==timing_rescale(100000,44119,44200));
 timing_rate_activate(44000);engine_timing_init();assert(timing_rate_get()==44200);
 project_defaults();assert(timing_rate_get()==44200);boot();assert(timing_rate_get()==44200);render();
 uint8_t p[32];memcpy(p,store[OBJ_PANEL],32);uint8_t b=p[4];p[4]=p[5];p[5]=b;
 assert(!timing_panel_write(p));rec=timing_store_read();assert(rec.rate==44200&&rec.generation==2&&rec.unit==17);
 assert(!engine_timing_update(0,44200,2,17,17));assert(timing_rate_get()==44100&&timing_rate_state.generation==3);
 engine_timing_init();assert(timing_rate_get()==44100&&timing_rate_state.unit==17);
 seq.ext=1;seq.ext_period=882u*256u;seq.t[0].to_next=12345;
 assert(!engine_timing_update(44119,44100,3,17,17));assert(seq.t[0].to_next==12345&&seq.ext_period==882u*256u);seq.ext=0;
 store_fail_after=0;assert(engine_timing_update(44120,44119,4,17,17)==4);assert(timing_rate_get()==44119);
 store_fail_after=-1;engine_timing_init();assert(timing_rate_state.record_status==0&&timing_rate_state.generation==4);
 flash_play_race=1;assert(!engine_timing_update(44120,44119,4,17,17));assert(!seq.playing&&seq.req_start);
 render();assert(seq.playing);seq_stop(&seq);render();
 store[OBJ_PANEL][52]^=1;engine_timing_init();assert(timing_rate_get()==44100&&timing_rate_state.record_status==TC_DAMAGED);
 writes=store_writes;assert(engine_timing_update(44119,44100,0,0,17)==6&&store_writes==writes);
 fresh();uint8_t malformed[61]={1,9,1};
 for(unsigned n=0;n<=60;n++){if(n==3)malformed[2]=0;else malformed[2]=1;query(malformed,n);assert(store_writes==0);}
 uint8_t apply[33]={1,9,1},wire[32]={0x7d,0x46,0x4c,52};uint32_t args[]={44119,44100,0,0,17};
 for(unsigned i=0;i<5;i++)for(unsigned j=0;j<5;j++)apply[3+i*5+j]=(uint8_t)((args[i]>>(j*7))&127);
 memcpy(wire+4,apply,28);uint32_t crc=ed_tc_crc(wire,32);for(unsigned j=0;j<5;j++)apply[28+j]=(uint8_t)((crc>>(j*7))&127);
 tx_idle=0;query(apply,33);assert(ed_out[8]==2&&store_writes==0);tx_idle=1;
 web_midi_w=1;web_midi_r=0;query(apply,33);assert(ed_out[8]==2&&store_writes==0);web_midi_w=web_midi_r=0;
 uint8_t read[]={1,3,0};query(read,3);assert(ed_out[8]==0&&store_writes==0);
 read[0]=2;query(read,3);assert(ed_out[8]==7&&store_writes==0);
 puts("CALIBRATION PASS: unit-only storage,Apply/Reset,stale/busy/bounds,project isolation,cold reload,panel preservation,failed-write recovery,queued PLAY and read-only protocol.");
}
int main(int argc,char **argv){
 if(argc>1&&!strcmp(argv[1],"--wire")){
  fresh();char line[1024];
  while(fgets(line,sizeof line,stdin)){
   if(!strncmp(line,"REBOOT",6)){boot();render();puts("REBOOTED");fflush(stdout);continue;}
   uint8_t a[128];unsigned n=0,v;char *p=line;int used;
   while(sscanf(p,"%2x%n",&v,&used)==1&&n<sizeof a){a[n++]=(uint8_t)v;p+=used;}
   assert(n>=4&&a[0]==0x7d&&a[1]==0x46&&a[2]==0x4c&&a[3]==52);query(a+4,n-4);
   for(unsigned i=1;i<ed_n;i++)printf("%02x",ed_out[i]);puts("");fflush(stdout);
  }
  return 0;
 }
 tests();return 0;
}
