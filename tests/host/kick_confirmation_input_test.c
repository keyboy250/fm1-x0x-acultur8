/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#ifdef CONFIRM_PARENT
#include "/parent/host/x0x_host.c"
#else
#include "../../host/x0x_host.c"
#endif
#undef main
#include <assert.h>
static void block(void){int32_t pcm[512];engine_render(pcm,256);}
static void settle(void){for(int i=0;i<16;i++)block();}
static void tap(unsigned b){held_btn=1u<<b;ui_input_only();block();held_btn=0;ui_input_only();block();}
static void chord(unsigned b){held_btn=1u<<B_HOME;ui_input_only();held_btn|=1u<<b;ui_input_only();block();held_btn=1u<<B_HOME;ui_input_only();held_btn=0;ui_input_only();block();}
static void ask_recall(void){pref_set(PR(R_KPRESET,0,0,0),0);pref_set(PR(R_KPRESET,0,0,2),1);assert(ui.overlay==O_ASK);}
static void fresh(void){for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;store_tear=-1;held_btn=held_keys=0;boot();settle();ui.dirty=save_wait=0;ui.rec=ui.btn=ui.keys=0;ui.overlay=O_NONE;assert(engine_kick_set(HK_ENABLE,1)&&engine_kick_set(HK_BLEND,100));settle();assert(project_kick_preset_save(0)==VR_OK);assert(engine_kick_set(HK_BLEND,50));settle();undo_reset(&undo);}
int main(void){
 fresh();project_t before=proj;kick_preset_t original;assert(project_kick_preset_get(0,&original)==VR_OK);uint32_t writes=store_writes;
 ask_recall();tap(B_HOME);assert(proj.kick.cfg[HK_BLEND]==50&&store_writes==writes);
 ask_recall();held_btn=1u<<B_SEL;ui_input_only();assert(ui.overlay==O_ASK&&proj.kick.cfg[HK_BLEND]==50);held_btn=0;ui_input_only();block();assert(ui.overlay==O_NONE);
#ifdef CONFIRM_PARENT
 assert(proj.kick.cfg[HK_BLEND]==50&&engine_kick_applied(HK_BLEND)==50);puts("OLD036_CONFIRM_BUG_REPRODUCED:actual SELECT down/up leaves kick at50;direct-handler fixture missed stale button snapshot");return 0;
#else
 assert(proj.kick.cfg[HK_BLEND]==100&&engine_kick_applied(HK_BLEND)==100);before.kick.cfg[HK_BLEND]=100;assert(!memcmp(&before,&proj,sizeof proj));
 kick_preset_t after;assert(project_kick_preset_get(0,&after)==VR_OK&&!memcmp(&original,&after,sizeof original));
 /* Native STORE uses the same physical SELECT release; original slot0 stays intact. */
 pref_set(PR(R_KPRESET,0,0,0),1);pref_set(PR(R_KPRESET,0,0,1),1);assert(ui.overlay==O_ASK);tap(B_SEL);assert(project_kick_preset_get(1,&after)==VR_OK&&!memcmp(&original,&after,sizeof original));
 chord(B_REC);settle();assert(!seq.playing&&proj.kick.cfg[HK_BLEND]==50&&engine_kick_applied(HK_BLEND)==50);
 chord(B_PLAY);settle();assert(!seq.playing&&proj.kick.cfg[HK_BLEND]==100&&engine_kick_applied(HK_BLEND)==100);
 /* An actually held key/button must still refuse confirmation. */
 assert(engine_kick_set(HK_BLEND,50));settle();ask_recall();held_btn=1u<<B_SEL;ui_input_only();held_btn=1u<<B_EDIT;ui_input_only();block();assert(proj.kick.cfg[HK_BLEND]==50);held_btn=0;ui_input_only();
 ask_recall();ui.keys=held_keys=1u;tap(B_SEL);assert(proj.kick.cfg[HK_BLEND]==50);held_keys=0;ui_input_only();
 /* Programmatic/MIDI caller cannot borrow the native SELECT-release exception. */
 ui.btn=1u<<B_SEL;assert(ui_kick_preset_action(0,1)==VR_BUSY&&proj.kick.cfg[HK_BLEND]==50);ui.btn=0;
 assert(project_kick_preset_get(0,&after)==VR_OK&&!memcmp(&original,&after,sizeof original));
 puts("KICK_CONFIRM_INPUT_PASS:real input down/up native recall/store/cancel,one-step Undo/Redo,whole-project isolation,held button/key refusal,remote owner guard andpreset preservation");return 0;
#endif
}
