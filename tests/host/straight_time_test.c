/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
static void render(void){int32_t block[512];engine_render(block,256);}
static void drain(void){for(int i=0;i<3;i++)render();}
static const unsigned grid[13]={0,5,4,9,8,21,22,23,24,25,26,27,28};
static void sweep(pref_t ref,int pattern,int lane){
 assert(engine_rate_set(pattern,lane,1,0));drain();
 for(unsigned i=1;i<13;i++){turn_ref(ref,1,EN_K1);drain();assert((unsigned)poly_rate(proj.pat,(const uint8_t *)&proj.sound,pattern,lane)==grid[i]);}
 turn_ref(ref,1,EN_K1);drain();assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,pattern,lane)==28);
 for(unsigned i=12;i-->0;){turn_ref(ref,-1,EN_K1);drain();assert((unsigned)poly_rate(proj.pat,(const uint8_t *)&proj.sound,pattern,lane)==grid[i]);}
}
int main(void){
 for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
 boot();drain();assert(poly_sync_mode(proj.pat,(const uint8_t *)&proj.sound)==0);
 open_global();int moves=0;while(moves<ui.list_n && P_KIND(list_rows[moves])!=R_SYNCMODE)moves++;
 assert(moves<ui.list_n);int selectable=0;for(int i=0;i<moves;i++)if(!list_skip(i))selectable++;
 FILE *navigation=fopen("build/straight-time/navigation.json","w");assert(navigation);fprintf(navigation,"{\"gloMoves\":%d}\n",selectable);fclose(navigation);ui.overlay=O_NONE;build_pages();
 pref_set(PR(R_SYNCMODE,0,0,0),1);drain();assert(poly_sync_enabled((const uint8_t *)&proj.sound));assert(poly_valid(proj.pat,(const uint8_t *)&proj.sound));
 assert(rf_count(RF_GRID)==13);
 for(unsigned lane=0;lane<25;lane++){
  assert(poly_sync_override(proj.pat,(const uint8_t *)&proj.sound,lane)==0);
  int part=poly_part((int)lane);pref_t ref=PR(R_RATEEXACT,lane,0,0);
  if(lane<22){ui.sel[part]=lane%11;ref=PR(R_DRATE,part,0,0);}
  else if(lane<24)ref=PR(R_BRATE,lane-22,0,0);
  sweep(ref,seq.ppat[part],lane);
 }
 /* Changing only the selected snare's strip policy leaves every pattern rate alone. */
 uint8_t rates[32][25];for(int p=0;p<32;p++)for(int l=0;l<25;l++)rates[p][l]=poly_rate(proj.pat,(const uint8_t *)&proj.sound,p,l);
 pref_set(PR(R_SYNCLOCAL,1,0,0),2);drain();
 assert(poly_sync_family(proj.pat,(const uint8_t *)&proj.sound,0)==RF_GRID);
 assert(poly_sync_family(proj.pat,(const uint8_t *)&proj.sound,1)==RF_STRAIGHT_TRIPLET);
 ui.sel[0]=1;assert(engine_rate_set(0,1,1,0));drain();turn_ref(PR(R_DRATE,0,0,0),-1,EN_K1);drain();assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,0,1)==1);
 rates[0][1]=1;
 pref_set(PR(R_SYNCMODE,0,0,0),0);drain();
 for(int p=0;p<32;p++)for(int l=0;l<25;l++)assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,p,l)==rates[p][l]);
 assert(poly_sync_family(proj.pat,(const uint8_t *)&proj.sound,1)==RF_STRAIGHT_TRIPLET);
 pref_set(PR(R_SYNCMODE,0,0,0),1);drain();
 /* Shared delay offers its supported straight choices, without inventing long buffers. */
 pref_t delay=PR(R_ENG,T_FX,0,FX_DL_TIME);pref_set(delay,2);drain();
 for(int i=0;i<3;i++){turn_ref(delay,1,EN_K1);drain();assert(pref_get(delay)==5+i*3);}
 for(int i=0;i<3;i++){turn_ref(delay,-1,EN_K1);drain();assert(pref_get(delay)==8-i*3);}
 /* Native CC sweep follows the same global policy and per-sound override. */
 assert(!cc_parameter(1010,0,0));assert(!cc_parameter(1069,0,0));assert(!cc_parameter(1071,1,0));
 for(unsigned v=0;v<128;v++){assert(!cc_parameter(1140,v,0));drain();assert(rf_allowed(RF_GRID,poly_rate(proj.pat,(const uint8_t *)&proj.sound,0,0)));}
 /* Saved LAN3 survives the ordinary store/reboot path with long rate and local override. */
 assert(engine_rate_set(0,0,1,28));drain();assert(project_save()==0);boot();drain();
 assert(poly_valid(proj.pat,(const uint8_t *)&proj.sound));assert(poly_sync_mode(proj.pat,(const uint8_t *)&proj.sound)==1);
 assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,0,0)==28);assert(poly_sync_override(proj.pat,(const uint8_t *)&proj.sound,1)==2);
 /* The real Undo journal restores policy and its redo without changing notes. */
 undo_reset(&undo);pref_set(PR(R_SYNCMODE,0,0,0),0);drain();assert(undo_commit(&undo)>0);
 do_undo(0);drain();assert(poly_sync_mode(proj.pat,(const uint8_t *)&proj.sound)==1);
 do_undo(1);drain();assert(poly_sync_mode(proj.pat,(const uint8_t *)&proj.sound)==0);
 pref_set(PR(R_SYNCMODE,0,0,0),1);drain();undo_reset(&undo);
 pref_set(PR(R_SYNCLOCAL,1,0,0),0);drain();assert(undo_commit(&undo)>0);
 do_undo(0);drain();assert(poly_sync_override(proj.pat,(const uint8_t *)&proj.sound,1)==2);
 do_undo(1);drain();assert(poly_sync_override(proj.pat,(const uint8_t *)&proj.sound,1)==0);
 /* Policy edits refuse playback; no partial project writes. */
 project_t before=proj;seq.playing=1;assert(!engine_sync_mode(-1,0));assert(!memcmp(&before,&proj,sizeof proj));seq.playing=0;
 puts("STRAIGHT_TIME_PASS: production knob path on 25 lanes, 13 exact stops, snare triplet override, no pattern rewrites, delay filter, CC sweep, saved reboot, busy refusal");
}
