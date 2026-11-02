/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/dsp/phase303b.h"
#include <assert.h>
static void block(void){int32_t pcm[512];engine_render(pcm,256);}
static void settle(void){for(int i=0;i<8;i++)block();}
static int command(int field,int value){variant_command_t c={1,VC_SET_PHASE,1,1,0,(uint8_t)field,0,value};variant_reply_t r;return variant_command(&c,&r);}
static unsigned hash_bytes(const void *p,unsigned n){const uint8_t *b=p;unsigned h=2166136261u;while(n--)h=(h^*b++)*16777619u;return h;}
int main(void){
 for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
 boot();settle();assert(__builtin_offsetof(project_t,kick)==31356&&sizeof(project_t)==31400);assert(sizeof(bass303_t)==752);
 assert(proj.variant.schema==5);assert(engine_phase_get(0)==0);
 /* An untouched legacy project does not activate or rewrite the extension. */
 proj.arr.song.rsv[0]=0x55;proj.arr.song.rsv[1]=0x23;
 assert(project_save()==0);boot();settle();assert(proj.variant.schema==5);
 assert(engine_phase_get(0)==0&&proj.arr.song.rsv[0]==0x55&&proj.arr.song.rsv[1]==0x23);
 /* Exercise the actual knob preference used by the 303B engine page. */
 turn_ref(PR(R_PHASE,0,0,0),1,EN_K1);settle();assert(engine_phase_get(0)==1);
 turn_ref(PR(R_PHASE,0,0,0),-1,EN_K1);settle();assert(engine_phase_get(0)==0);
 /* Reset to a schema5 fixture before testing the activation Undo boundary. */
 project_defaults();engine_apply_sound(&proj.sound);settle();undo_reset(&undo);
 uint32_t sound_hash=hash_bytes(&proj.sound,sizeof proj.sound),pattern_hash=hash_bytes(proj.pat,sizeof proj.pat);
 /* Original DSP through the adapter is bit-identical, including after switching back. */
 bass303_t original,adapter;float a[256],b[256];bass303_init(&original);bass303_init(&adapter);
 bass303_note_on(&original,48,1,0);slot_note_on(&adapter,48,1,0);
 for(int i=0;i<20;i++){bass303_render(&original,a,256);slot_render(&adapter,b,256);assert(!memcmp(a,b,sizeof a));}
 slot_all_off(&adapter);phase_select(&adapter,0xC0C0u);assert(phase_active(&adapter));phase_select(&adapter,0);assert(!phase_active(&adapter));
 bass303_init(&original);bass303_note_on(&original,48,0,0);slot_note_on(&adapter,48,0,0);
 for(int i=0;i<20;i++){bass303_render(&original,a,256);slot_render(&adapter,b,256);assert(!memcmp(a,b,sizeof a));}
 unsigned wave_hash[8];
 /* Nondefault legacy pots and overload protection survive a round trip. */
 bass303_init(&original);bass303_init(&adapter);
 for(int i=0;i<BASS303_NPARAMS;i++){int v=bass303_param(i)->max/2;bass303_set(&original,i,v);slot_set(&adapter,i,v);}
 bass303_set_lite(&original,1);bass303_set_lite(&adapter,1);
 phase_select(&adapter,0xC0C0u);phase_select(&adapter,0);assert(adapter.lite==1);
 bass303_note_on(&original,55,1,0);slot_note_on(&adapter,55,1,0);
 for(int i=0;i<20;i++){bass303_render(&original,a,256);slot_render(&adapter,b,256);assert(!memcmp(a,b,sizeof a));}
 for(unsigned w=0;w<8;w++){
  phase_select(&adapter,0xC0C0u|w);slot_note_on(&adapter,60,1,0);unsigned h=2166136261u;
  for(int n=0;n<24;n++){slot_render(&adapter,b,256);for(int j=0;j<256;j++){assert(isfinite(b[j]));assert(fabsf(b[j])<1.2f);}h^=hash_bytes(b,sizeof b);h*=16777619u;}
  wave_hash[w]=h;for(unsigned k=0;k<w;k++)assert(wave_hash[k]!=h);
  slot_note_off(&adapter);for(int n=0;n<450;n++)slot_render(&adapter,b,256);assert(adapter.idle);for(int j=0;j<256;j++)assert(fabsf(b[j])<.00001f);
  slot_render(&adapter,b,256);for(int j=0;j<256;j++)assert(b[j]==0.0f);
 }
 /* Every wave/shape/envelope corner remains finite across the MIDI range. */
 for(unsigned w=0;w<8;w++)for(unsigned depth=0;depth<2;depth++)for(unsigned en=0;en<2;en++)for(int note=0;note<128;note+=7){
  unsigned config=0x8000u|128u|w|(en?120u:0u)|(depth?0x7F00u:0u);phase_select(&adapter,config);slot_all_off(&adapter);slot_note_on(&adapter,note,1,0);
  for(int n=0;n<5;n++){slot_render(&adapter,b,256);for(int j=0;j<256;j++){assert(isfinite(b[j]));assert(fabsf(b[j])<1.2f);}}
 }
 undo_reset(&undo);assert(command(0,1)>=0);settle();assert(engine_phase_get(0)==1);assert(proj.variant.schema==6);assert(variant_valid(&proj.variant));assert(undo_commit(&undo)>0);
 seq.playing=1;assert(ui_undo_preflight(0)==VR_BUSY);seq.playing=0;
 engine_bass_on(1,60,1,0);settle();assert(ui_undo_preflight(0)==VR_BUSY);
 engine_bass_off(1);for(int i=0;i<450;i++)block();assert(ui_undo_preflight(0)==VR_OK);
 do_undo(0);settle();assert(engine_phase_get(0)==0);assert(proj.variant.schema==5);
 seq.playing=1;assert(ui_undo_preflight(1)==VR_BUSY);seq.playing=0;
 do_undo(1);settle();assert(engine_phase_get(0)==1);assert(proj.variant.schema==6);
 assert(command(1,6)>=0);settle();assert(command(2,100)>=0);settle();assert(command(3,12)>=0);settle();
 assert(hash_bytes(&proj.sound,sizeof proj.sound)==sound_hash);assert(hash_bytes(proj.pat,sizeof proj.pat)==pattern_hash);
 /* No automatic CC remapping; descriptors and original stored303 pots remain stable. */
 assert(engine_nparams(T_303,0)==12&&engine_nparams(T_303,1)==12);
 variant_snapshot_t s;assert(variant_snapshot(VS_PHASE,1,0,&s)==VR_OK);assert(s.count==5&&s.value[0]==1&&s.value[1]==6&&s.value[2]==100&&s.value[3]==12);
 assert(command(0,2)==VR_INVALID);assert(command(1,8)==VR_INVALID);assert(command(2,128)==VR_INVALID);assert(command(3,16)==VR_INVALID);
 seq.playing=1;unsigned fingerprint=project_fingerprint();assert(command(1,5)==VR_BUSY);assert(project_fingerprint()==fingerprint);seq.playing=0;
 engine_bass_on(1,60,1,0);settle();assert(command(0,0)==VR_BUSY);engine_bass_off(1);for(int i=0;i<450;i++)block();
 assert(project_save()==0);boot();settle();assert(engine_phase_get(0)==1&&engine_phase_get(1)==6&&engine_phase_get(2)==100&&engine_phase_get(3)==12);
 assert(command(0,0)>=0);settle();assert(engine_phase_get(1)==6&&engine_phase_get(2)==100);assert(project_save()==0);boot();settle();assert(engine_phase_get(0)==0&&engine_phase_get(1)==6);
 assert(command(0,1)>=0);settle();assert(project_save()==0);
 /* PHASE and the accepted LAN3 straight/triplet settings coexist on disk. */
 pref_set(PR(R_SYNCMODE,0,0,0),1);settle();
 pref_set(PR(R_SYNCLOCAL,1,0,0),2);settle();
 assert(engine_rate_set(0,0,1,28));settle();assert(project_save()==0);
 boot();settle();assert(engine_phase_get(0)==1&&engine_phase_get(1)==6);
 assert(poly_valid(proj.pat,(const uint8_t *)&proj.sound));
 assert(poly_sync_mode(proj.pat,(const uint8_t *)&proj.sound)==1);
 assert(poly_sync_override(proj.pat,(const uint8_t *)&proj.sound,1)==2);
 assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,0,0)==28);
 /* Save the exact simulated objects for the frozen0.1.32 rejection test. */
 for(int i=0;i<OBJ_NOBJ;i++)if(store_len[i]>=0){char name[80];snprintf(name,sizeof name,"build/phase-tests/obj-%d.bin",i);FILE *f=fopen(name,"wb");assert(f);fwrite(store[i],1,(size_t)store_len[i],f);fclose(f);}
 puts("PHASE303B_PASS: zero-size growth, original303 bit parity,8 distinct waves,finite extremes,release,scope,Undo/Redo,save/reboot,stopped guards");return 0;
}
