/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int save_fail;
#define HOST_STORE_PROBE(writing) ((writing)&&save_fail ? -9 : 0)
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
static int action(unsigned lane,int apply){variant_command_t c={1,apply?21:20,0,(uint8_t)lane,0,0,0,0};variant_reply_t r;return variant_command(&c,&r);}
static void render(void){int32_t output[256];engine_render(output,128);}
int main(void){
 sound_strip_config_t cfg[25],got,prior;uint8_t bank[212];uint32_t fingerprint,writes;
 for(unsigned i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;boot();render();
 memset(&got,0xA5,sizeof got);prior=got;assert(project_strip_default_get(0,&got)==VR_MISSING&&!memcmp(&got,&prior,sizeof got));
 fingerprint=project_fingerprint();assert(action(0,1)==VR_MISSING&&project_fingerprint()==fingerprint);
 for(unsigned i=0;i<25;i++){
  cfg[i]=(sound_strip_config_t){(uint16_t)(31+i*7),(uint16_t)(19000-i*101),(uint8_t)(i%25),(uint8_t)(i%49),(uint8_t)(10+i*9),(uint8_t)(i%16)};
  for(unsigned j=0;j<6;j++){variant_command_t c={1,2,0,(uint8_t)i,0,(uint8_t)j,0,variant_field_get(&cfg[i],j)};variant_reply_t r;assert(variant_command(&c,&r)==1);}render();
  fingerprint=project_fingerprint();assert(action(i,0)==0&&project_fingerprint()==fingerprint);assert(project_strip_default_get(i,&got)==0&&!memcmp(&got,&cfg[i],sizeof got));
 }
 memcpy(bank,store[7],sizeof bank);writes=store_writes;assert(action(24,0)==0&&store_writes==writes);
 /* Fresh project creation preserves reusable defaults, without applying them implicitly. */
 boot();render();for(unsigned i=0;i<25;i++){assert(project_strip_default_get(i,&got)==0&&!memcmp(&got,&cfg[i],sizeof got));assert(memcmp(&proj.variant.strip[i],&cfg[i],sizeof got));}
 for(unsigned i=0;i<25;i++){
  sound_strip_config_t before[25];memcpy(before,proj.variant.strip,sizeof before);assert(action(i,1)==0);render();assert(!memcmp(engine_strip_config(i),&cfg[i],sizeof got));
  for(unsigned j=0;j<25;j++)assert(!memcmp(&proj.variant.strip[j],j==i?&cfg[i]:&before[j],sizeof got));
 }
 assert(project_save()==0);boot();render();for(unsigned i=0;i<25;i++)assert(!memcmp(&proj.variant.strip[i],&cfg[i],sizeof got));
 /* One whole-strip recall is one undoable gesture. */
 prior=proj.variant.strip[0];proj.variant.strip[0].release_ms=250;ui_external_changed();ui_external_edit(0);assert(action(0,1)==0);render();assert(proj.variant.strip[0].release_ms==prior.release_ms);assert(ui_external_undo(0)==0);render();assert(proj.variant.strip[0].release_ms==250);assert(ui_external_undo(1)==0);render();assert(proj.variant.strip[0].release_ms==prior.release_ms);
 /* Exact object length is independent of its valid SDF1 prefix. Real host
  * storage truncates to the caller buffer: request one extra byte or an
  * oversized/future record would silently load and later be rewritten. */
 for(unsigned case_no=0;case_no<3;case_no++){
  unsigned n=(unsigned[]){213,256,PLAT_STORE_MAX}[case_no];
  uint8_t future[PLAT_STORE_MAX];memset(future,0xA5,sizeof future);memcpy(future,bank,sizeof bank);
  memcpy(store[7],future,n);store_len[7]=(int)n;fingerprint=project_fingerprint();writes=store_writes;
  memset(&got,0x5A,sizeof got);prior=got;int rc=project_strip_default_get(0,&got);
  fprintf(stderr,"defaults oversized OBJ7 length%u: get=%d expected=%d\n",n,rc,VR_UNSUPPORTED);
  assert(rc==VR_UNSUPPORTED&&!memcmp(&got,&prior,sizeof got));
  assert(action(0,1)==VR_UNSUPPORTED&&action(0,0)==VR_UNSUPPORTED);
  assert(project_fingerprint()==fingerprint&&store_writes==writes&&store_len[7]==(int)n&&!memcmp(store[7],future,n));
 }
 memcpy(store[7],bank,sizeof bank);store_len[7]=sizeof bank;
 /* I/O refusal, malformed/future bank and busy transport never claim a saved default. */
 proj.variant.strip[0].release_ms=240;save_fail=1;assert(action(0,0)==VR_IO);save_fail=0;assert(!memcmp(bank,store[7],sizeof bank));
 store[7][4]=2;fingerprint=project_fingerprint();assert(action(0,0)==VR_UNSUPPORTED&&action(0,1)==VR_UNSUPPORTED&&project_fingerprint()==fingerprint&&store[7][4]==2);memcpy(store[7],bank,sizeof bank);
 seq.req_start=1;assert(action(0,0)==VR_BUSY&&action(0,1)==VR_BUSY);seq.req_start=0;
 rejected_save=1;assert(action(0,0)==VR_PROTECTED&&action(0,1)==VR_PROTECTED);rejected_save=0;
 ui_external_edit(1);assert(action(0,0)==VR_BUSY&&action(0,1)==VR_BUSY);ui_external_edit(0);
 while(engine_queue_space()>5)engine_set(T_909,0,0,0);fingerprint=project_fingerprint();assert(action(0,1)==VR_BUSY);assert(engine_transport_reserve());assert(!engine_strip_restore(0,&cfg[0]));engine_transport_release();assert(project_fingerprint()==fingerprint);render();
 /* Actual native pages expose filter/limiter settings and defaults for all25 sounds. */
 for(unsigned i=0;i<25;i++){ui.part=(uint8_t)poly_part(i);if(i<22)ui.sel[i/11]=(uint8_t)(i%11);ui.view=V_PART;build_pages();assert(pg.n>=3);assert(P_KIND(pg.r[pg.n-1][1])==R_STRIPDEFAULT);}
 /* Legacy BREAK selection stays first; strip pages append without stealing its knobs. */
 ui.part=PART_BRK;ui.view=V_PART;build_pages();assert(pg.n==4);
 assert(!strcmp(pg.title[0],"LOOPS")&&P_KIND(pg.r[0][0])==R_BRKSLOT&&P_A(pg.r[0][0])==0);
 assert(P_KIND(pg.r[1][0])==R_STRIP&&P_A(pg.r[1][0])==24&&P_C(pg.r[1][0])==0);
 assert(P_KIND(pg.r[2][0])==R_STRIP&&P_A(pg.r[2][0])==24&&P_C(pg.r[2][0])==2);
 assert(P_KIND(pg.r[3][1])==R_STRIPDEFAULT&&P_A(pg.r[3][1])==24);
 puts("Strip defaults actual engine:25 independent defaults, reboot/project reopen, isolated recall, undo/redo, no-op save, I/O/format/protection/transport/gesture/CQ refusals PASS");return 0;
}
