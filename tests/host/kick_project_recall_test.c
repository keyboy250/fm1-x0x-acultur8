#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
static void block(void){int32_t pcm[512];engine_render(pcm,256);}
static void settle(void){for(int i=0;i<16;i++)block();}
int main(void){
 for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
 boot();settle();assert(sizeof(project_t)==31400);assert(project_save()==0);boot();settle();assert(proj.kick.active==0&&engine_kick_get(HK_ENABLE)==0);
 for(int f=0;f<HK_COUNT;f++){if(f==HK_SOURCE)continue;const x0x_param_t *p=engine_kick_param(f);assert(engine_kick_set(f,p->max));settle();}
 assert(project_save()==0);assert(proj.kick.active==1);unsigned char saved[sizeof proj];memcpy(saved,&proj,sizeof proj);
 boot();settle();assert(!memcmp(saved,&proj,sizeof proj));for(int f=0;f<HK_COUNT;f++)assert(engine_kick_get(f)==engine_kick_applied(f));
 assert(engine_kick_set(HK_BLEND,17));settle();assert(project_save()==0);boot();settle();assert(engine_kick_applied(HK_BLEND)==17);
 assert(project_kick_preset_save(0)==VR_OK);kick_preset_t preset;assert(project_kick_preset_get(0,&preset)==VR_OK);assert(preset.kick.cfg[HK_BLEND]==17);
 for(int slot=1;slot<16;slot++){assert(engine_kick_set(HK_BLEND,slot));settle();assert(project_kick_preset_save(slot)==VR_OK);}
 assert(project_kick_preset_get(0,&preset)==VR_OK&&preset.kick.cfg[HK_BLEND]==17);assert(project_kick_preset_get(15,&preset)==VR_OK&&preset.kick.cfg[HK_BLEND]==15);
 assert(engine_kick_step_set(0,0,1));assert(engine_kick_step_set(0,1,2));assert(engine_kick_step_set(0,2,3));assert(project_save()==0);boot();settle();assert(engine_kick_step_get(0,0)==1&&engine_kick_step_get(0,1)==2&&engine_kick_step_get(0,2)==3&&engine_kick_step_get(0,3)==0);
 assert(project_kick_preset_get(0,&preset)==VR_OK&&preset.kick.cfg[HK_BLEND]==17);
 /* Preset library upgrades the independent bank andpreserves all25 strip defaults. */
 sound_strip_config_t defaults[25];for(int i=0;i<25;i++){defaults[i]=proj.variant.strip[i];defaults[i].boost_db=(uint8_t)(i%25);proj.variant.strip[i]=defaults[i];assert(project_strip_default_save(i)==VR_OK);}
 assert(project_kick_preset_save(1)==VR_OK);for(int i=0;i<25;i++){sound_strip_config_t out;assert(project_strip_default_get(i,&out)==VR_OK&&!memcmp(&out,&defaults[i],sizeof out));}
 unsigned char bank[PLAT_STORE_MAX];int banklen=store_len[7];memcpy(bank,store[7],sizeof bank);assert(engine_kick_set(HK_BLEND,31));settle();store_tear=0;assert(project_kick_preset_save(0)==VR_IO);store_tear=-1;assert(store_len[7]==banklen&&!memcmp(bank,store[7],sizeof bank));
 /* A real nativeconfirmation can cancel without replacing a slot. */
 ready_label: ui.dirty=save_wait=0;ui.btn=ui.keys=0;ui.overlay=O_NONE;undo_reset(&undo);pref_set(PR(R_KPRESET,0,0,0),0);pref_set(PR(R_KPRESET,0,0,1),1);assert(ui.overlay==O_ASK);button_tap(B_HOME);assert(!memcmp(bank,store[7],sizeof bank));
 pref_set(PR(R_KPRESET,0,0,2),1);assert(ui.overlay==O_ASK);button_tap(B_SEL);settle();assert(engine_kick_applied(HK_BLEND)==17);assert(ui_external_undo(0)==VR_OK);settle();assert(engine_kick_applied(HK_BLEND)==31);assert(ui_external_undo(1)==VR_OK);settle();assert(engine_kick_applied(HK_BLEND)==17);
 /* Missing/changed USER1 refuses recall before musical bytes change. */
 slot_nz[0]=1;slot_n[0][0]=16;slot_rate[0][0]=44100;slot_data[0][0]=calloc(1,9);assert(slot_data[0][0]);engine_brk_loops();settle();assert(engine_kick_set(HK_SOURCE,4));settle();assert(project_kick_preset_save(15)==VR_OK);assert(engine_kick_set(HK_SOURCE,1));settle();unsigned char before_missing[sizeof proj];memcpy(before_missing,&proj,sizeof proj);slot_data[0][0][0]=17;ui.dirty=save_wait=0;undo_reset(&undo);assert(ui_kick_preset_action(15,1)==VR_MISSING&&!memcmp(before_missing,&proj,sizeof proj));
 assert(engine_kick_set(HK_BLEND,41));settle();assert(project_save()==0);FILE *fixture=fopen("build/kick-tests/format7-store.bin","wb");assert(fixture);for(int i=0;i<OBJ_NOBJ;i++){assert(fwrite(&store_len[i],4,1,fixture)==1);assert(fwrite(store[i],1,PLAT_STORE_MAX,fixture)==PLAT_STORE_MAX);}fclose(fixture);
 /* Every object-write boundary offormat7 retains a whole old/new kick transaction. */
 unsigned cuts_old=0,cuts_new=0;
 for(int cut=0;cut<10;cut++){
  for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
  store_tear=-1;memset(stored_known,0,sizeof stored_known);project_discard_rejected();boot();settle();
  assert(engine_kick_set(HK_ENABLE,1)&&engine_kick_set(HK_BLEND,17));settle();assert(project_save()==0);
  kick_settings_t oldkick=proj.kick;uint8_t oldbase=proj.sound.v[T_808][0][0];sound_strip_config_t oldstrip=proj.variant.strip[11];
  assert(engine_kick_set(HK_BLEND,91));proj.sound.v[T_808][0][0]^=1;proj.variant.strip[11].boost_db=11;
  assert(engine_kick_step_set(0,0,3));settle();kick_settings_t newkick=proj.kick;
  store_tear=cut;int rc=project_save();store_tear=-1;assert(project_load()==0);settle();
  if(rc){cuts_old++;assert(!memcmp(&proj.kick,&oldkick,sizeof oldkick)&&proj.sound.v[T_808][0][0]==oldbase&&!memcmp(&proj.variant.strip[11],&oldstrip,sizeof oldstrip)&&engine_kick_step_get(0,0)==0);}
  else{cuts_new++;assert(!memcmp(&proj.kick,&newkick,sizeof newkick)&&proj.sound.v[T_808][0][0]==(oldbase^1)&&proj.variant.strip[11].boost_db==11&&engine_kick_step_get(0,0)==3);}
 }
 assert(cuts_old&&cuts_new);
 puts("KICK_PROJECT_RECALL_PASS:legacy default project,39-control format7 save/reboot and applied DSP recall");return 0;
}
