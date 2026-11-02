/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
static void block(void){int32_t out[512];engine_render(out,256);}
static void fresh(void){for(unsigned i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;store_tear=-1;memset(stored_known,0,sizeof stored_known);project_discard_rejected();boot();block();}
static void fill(unsigned seed){uint32_t x=seed;for(unsigned p=0;p<NPAT;p++)for(unsigned k=0;k<NKIT;k++)for(unsigned v=0;v<NDRUM;v++)for(unsigned w=0;w<NSW;w++){x^=x<<13;x^=x>>17;x^=x<<5;proj.pat[p].drum[k].hit[v][w]=x;}}
int main(void){
 static pattern_t before[NPAT];
 for(unsigned variant=0;variant<2;variant++)for(unsigned cut=1;cut<4;cut++){
  fresh();if(variant){proj.variant.strip[23].lowpass_hz=10240;proj.variant.strip[23].flags=7;}
  fill(123);assert(project_save()==0);memcpy(before,proj.pat,sizeof before);
  assert(get32(store[OBJ_PAT0])==(variant?4u:2u));
  fill(456);store_tear=(int)cut;int saved=project_save();store_tear=-1;
  if(!saved)continue; /* A cut beyond the last written object is a complete save. */
  assert(project_load()==0);block();assert(!memcmp(before,proj.pat,sizeof before));
  if(variant)assert(proj.variant.strip[23].lowpass_hz==10240);
  assert(project_save()==0);assert(project_load()==0);block();assert(!memcmp(before,proj.pat,sizeof before));
 }
 for(unsigned expanded=0;expanded<2;expanded++){
  fresh();assert(engine_lane_mode(1));block();if(expanded){assert(engine_rate_set(0,0,0,1));block();}
  assert(project_save()==0);assert(get32(store[OBJ_PAT0])==(expanded?6u:5u));
  sound_t sound_before=proj.sound;memcpy(before,proj.pat,sizeof before);
  proj.sound.v[T_303][0][0]^=1;assert(engine_lane_set(0,7,0,63));block();
  store_tear=1;assert(project_save()!=0);store_tear=-1;assert(project_load()==0);block();
  assert(!memcmp(&sound_before,&proj.sound,sizeof sound_before)&&!memcmp(before,proj.pat,sizeof before));
  assert(project_save()==0);assert(project_load()==0);block();assert(!memcmp(&sound_before,&proj.sound,sizeof sound_before));
 }
 fresh();uint32_t held_writes=store_writes;ui_hold_saves(1);assert(project_save()==-9&&project_save_settings()==-9&&store_writes==held_writes);ui_hold_saves(0);
 rejected_save=1;proj.set.one_core^=1;assert(project_save_settings()==-8&&store_writes==held_writes);rejected_save=0;assert(project_save_settings()==0&&store_writes==held_writes+1);
 fresh();proj.variant.strip[23].lowpass_hz=10240;assert(project_strip_default_save(23)==0);uint8_t bank[212];memcpy(bank,store[OBJ_STRIP_DEFAULTS],212);
 assert(project_save()==0);assert(sim_backup("build/host/variant-project.x0b")==0);proj.variant.strip[23].lowpass_hz=9813;assert(project_save()==0);
 assert(sim_restore("build/host/variant-project.x0b")==0);block();assert(proj.variant.strip[23].lowpass_hz==10240);assert(store_len[OBJ_STRIP_DEFAULTS]==212 && !memcmp(bank,store[OBJ_STRIP_DEFAULTS],212));
 ui.part=PART_303B;ui.view=V_PART;build_pages();unsigned page=pg.n-1;ui.page[ui.view][ui.part]=page;ui.btn=1u<<B_ENV;uint32_t writes=store_writes;pref_t action=pg.r[page][1];assert(P_KIND(action)==R_STRIPDEFAULT);/* ENV default must not invoke either explicit action. */
 uint32_t action_fp=project_fingerprint();held_btn=1u<<B_ENV;
 for(unsigned knob=EN_K2;knob<=EN_K3;knob++){enc_acc[knob]=1;ui_input_only();assert(store_writes==writes && project_fingerprint()==action_fp);}
 held_btn=ui.btn=0;assert(store_writes==writes);
 static const uint8_t hello[]={ANN_H0,ANN_H1,ANN_H2,ANN_HELLO,ANN_VER};uint32_t fp=project_fingerprint();assert(ann_frame(hello,sizeof hello,now_ms));sp_front(1);assert(project_fingerprint()==fp && store_writes==writes);
 puts("PASS upstream integration: legacy/variant/LAN1/LAN2 torn-stream and paired-sound recovery, resave/reopen, project backup/restore retains explicit defaults, read-only screen-reader announcement");return 0;
}
