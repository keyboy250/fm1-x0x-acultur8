#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>
static uint8_t ed_out[276];static unsigned ed_n;
static void ed_b(uint32_t v){assert(ed_n<sizeof ed_out);ed_out[ed_n++]=(uint8_t)(v&127);}
static void ed_pq_u32(uint32_t v){for(int i=0;i<5;i++){ed_b(v);v>>=7;}}
#define st_crc32(p,n) sim_crc32(0,p,n)
#include "../../firmware/src/app/generation_query.inc"
static void dump_query(int page,int index){uint8_t request[]={1,77,0,(uint8_t)page,(uint8_t)index};uint8_t header[]={240,125,70,76,51};memcpy(ed_out,header,5);ed_n=5;uint32_t fingerprint=project_fingerprint(),writes=store_writes;ed_generation_query(request,5);ed_out[ed_n++]=247;assert(ed_out[10]==0);assert(project_fingerprint()==fingerprint&&store_writes==writes);char path[128];snprintf(path,sizeof path,"build/kick-tests/query-%d-%d.bin",page,index);FILE *f=fopen(path,"wb");assert(f);assert(fwrite(ed_out,1,ed_n,f)==ed_n);fclose(f);}
static void block(void){int32_t audio[512];engine_render(audio,256);}
static void raw(unsigned number,unsigned value,unsigned source){unsigned controller=number%88;if(controller>=32)controller+=32;uint32_t packet=11u|((0xb0u+number/88)<<8)|(controller<<16)|(value<<24);assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,packet,(uint8_t)source));plat_midi_service();block();plat_midi_service();}
static void scaled(unsigned n,unsigned value,unsigned max,unsigned source){raw(n,(value*127u+max/2u)/max,source);}
static void context(unsigned source,unsigned pattern,unsigned step,unsigned destination){scaled(1010,pattern,31,source);scaled(1011,1,4,source);raw(1012,0,source);scaled(1014,step,63,source);raw(1068,127,source);scaled(1132,destination,31,source);raw(1071,127,source);}
static void exact(unsigned n,unsigned value,unsigned source){raw(1072,n&127u,source);scaled(1073,n>>7,15,source);raw(1074,value&127u,source);raw(1075,(value>>7)&127u,source);raw(1130,(value>>14)&127u,source);raw(1077,127,source);}
static void ready(void){for(int i=0;i<12;i++)block();ui.dirty=save_wait=0;undo_reset(&undo);}
int main(void){
 for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;boot();ready();for(int i=0;i<3;i++)cc_reset_source(i);context(0,0,0,0);
 /* Real packet ingress,including source-owned exact-value framing. */
 for(int f=0;f<HK_COUNT;f++){if(f==HK_SOURCE)continue;unsigned value=(unsigned)engine_kick_param(f)->max;exact(1250u+f,value,0);assert(proj.kick.cfg[f]==value);assert(engine_kick_applied(f)==(int)value);exact(1250u+f,(unsigned)engine_kick_param(f)->def,0);}
 exact(1250+HK_ENABLE,1,0);exact(1250+HK_BLEND,73,0);ready();uint32_t words[24];assert(cc_generation_snapshot(0,12,0,words)==18&&words[5+HK_BLEND]==73);assert(cc_generation_snapshot(0,13,0,words)==18&&words[5+HK_BLEND]==73);
 raw(1294,1,0);raw(1295,0,0);raw(1290,127,0);assert(cc_sources[0].generation.state==CG_ACCEPTED);raw(1291,127,0);assert(cc_sources[0].generation.state==CG_APPLIED);kick_preset_t saved;assert(project_kick_preset_get(0,&saved)==VR_OK&&saved.kick.cfg[HK_BLEND]==73);
 raw(1291,127,0);assert(cc_sources[0].generation.state==CG_APPLIED); /* held/repeated execute does not write again */
 exact(1250+HK_BLEND,21,0);ready();context(0,0,0,0);raw(1294,2,0);raw(1295,0,0);raw(1292,127,0);raw(1293,0,0);raw(1293,127,0);assert(cc_sources[0].generation.state==CG_APPLIED);assert(engine_kick_applied(HK_BLEND)==73);
 ready();context(0,0,0,0);raw(1294,3,0);raw(1295,0,0);raw(1292,127,0);exact(1250+HK_BLEND,22,0);raw(1293,0,0);raw(1293,127,0);assert(cc_sources[0].generation.state==CG_REJECTED&&engine_kick_applied(HK_BLEND)==22);
 /* Source1 cannot execute source0's arm. */
 ready();context(0,0,0,0);context(1,0,0,0);raw(1294,4,0);raw(1295,0,0);raw(1292,127,0);raw(1294,4,1);raw(1295,0,1);raw(1293,127,1);assert(cc_sources[1].generation.state==CG_REJECTED);
 ready();for(unsigned st=0;st<3;st++){context(0,0,st,0);scaled(1296,st+1,3,0);assert(engine_kick_step_get(0,(int)st)==(int)st+1);}
 exact(1250+HK_CHANCE,0,0);exact(1250+HK_LOW,65,0);exact(1250+HK_MID,90,0);exact(1250+HK_HIGH,110,0);ready();assert(project_save()==0);boot();ready();assert(engine_kick_step_get(0,2)==3&&engine_kick_applied(HK_BLEND)==22);
 /* Observe actual sequencer MIDI andthe velocity used bythe internal kick. */
 memset(proj.pat[0].drum[1].hit,0,sizeof proj.pat[0].drum[1].hit);proj.pat[0].drum[1].hit[0][0]=7;seq.ppat[1]=0;seq.send_notes=1;seq.send_clock=0;midi_log=tmpfile();assert(midi_log);seq_start(&seq);unsigned observed=0,last_note_count=note_ons;int expected[]={65,90,110};
 for(int i=0;i<100&&observed<3;i++){block();if(note_ons>last_note_count){assert(note_ons==last_note_count+1);assert((int)(d808.hybrid.hit_vel*127+.5f)==expected[observed]);observed++;last_note_count=note_ons;}}
 assert(observed==3);seq_stop(&seq);block();rewind(midi_log);char line[128];unsigned found=0;while(fgets(line,sizeof line,midi_log)){unsigned ms,status,note,vel;if(sscanf(line,"%u ms: %x %x %x",&ms,&status,&note,&vel)==4&&(status&240)==144&&vel){assert(found<3&&vel==(unsigned)expected[found]);found++;}}assert(found==3);fclose(midi_log);midi_log=0;
 /* Live CC changes queue next-hit intent without changingthe current tail. */
 for(int i=0;i<400;i++)block();ready();context(0,0,0,0);seq_start(&seq);block();int old_current=engine_kick_current(HK_BLEND);exact(1250+HK_BLEND,61,0);assert(seq.playing&&engine_kick_applied(HK_BLEND)==61&&engine_kick_current(HK_BLEND)==old_current);
 unsigned before_live=note_ons;for(int i=0;i<100&&note_ons==before_live;i++)block();assert(note_ons>before_live&&engine_kick_current(HK_BLEND)==61);seq_stop(&seq);block();
 /* Jitter is drawn once andshared byte-for-byte withthe outgoing sequence. */
 for(int i=0;i<400;i++)block();ready();context(0,0,0,0);exact(1250+HK_CHANCE,100,0);exact(1250+HK_RANGE,10,0);ready();assert(engine_kick_recall_commit());block();uint32_t rng=808;
 for(int j=0;j<3;j++){int base=j==0?65:j==1?90:110;rng=rng*1664525u+1013904223u;rng=rng*1664525u+1013904223u;expected[j]=base+(int)(rng%21)-10;}
 midi_log=tmpfile();assert(midi_log);seq_start(&seq);observed=0;last_note_count=note_ons;
 for(int i=0;i<100&&observed<3;i++){block();if(note_ons>last_note_count){assert((int)(d808.hybrid.hit_vel*127+.5f)==expected[observed]);observed++;last_note_count=note_ons;}}
 assert(observed==3&&d808.hybrid.rng==rng);seq_stop(&seq);block();rewind(midi_log);found=0;while(fgets(line,sizeof line,midi_log)){unsigned ms,status,note,vel;if(sscanf(line,"%u ms: %x %x %x",&ms,&status,&note,&vel)==4&&(status&240)==144&&vel){assert(found<3&&vel==(unsigned)expected[found]);found++;}}assert(found==3);fclose(midi_log);midi_log=0;
 /* Cancel,detach andprotected writes must leave both project andbank untouched. */
 for(int i=0;i<400;i++)block();ready();context(0,0,0,0);raw(1294,5,0);raw(1295,0,0);raw(1290,127,0);
 uint32_t writes_before=store_writes,fp_before=project_fingerprint();raw(1290,0,0);assert(cc_sources[0].generation.state==CG_CANCELLED);raw(1291,0,0);raw(1291,127,0);assert(cc_sources[0].generation.state==CG_REJECTED&&store_writes==writes_before&&project_fingerprint()==fp_before);
 ready();context(0,0,0,0);raw(1294,6,0);raw(1295,0,0);raw(1290,127,0);cc_reset_source(0);raw(1291,0,0);raw(1291,127,0);assert(cc_sources[0].generation.state==CG_REJECTED&&store_writes==writes_before&&project_fingerprint()==fp_before);
 rejected_save=1;assert(!engine_kick_set(HK_BLEND,44)&&!engine_kick_step_set(0,0,3)&&ui_kick_preset_action(0,0)==VR_PROTECTED);rejected_save=0;
 assert(project_fingerprint()==fp_before&&store_writes==writes_before);
 /* Saturated audio queue refuses a control without publishing canonical intent. */
 while(cq_w-cq_r<CQ)assert(cq_put(C_PHASE,0,0,0));assert(!engine_kick_set(HK_BLEND,44)&&project_fingerprint()==fp_before);block();
 /* Preset I/O reserves onlythe kick voice;incoming hits cannot race recall. */
 for(int i=0;i<400;i++)block();ready();assert(engine_transport_reserve());assert(engine_kick_preset_reserve());
 midi_in(9u|((0x90u+seq.ch[1])<<8)|(36u<<16)|(110u<<24));assert(engine_kick_idle());
 engine_kick_preset_release();engine_transport_release();s_drum(0,1,0,1);assert(!engine_kick_idle());
 /* An unchanged sample reference refresh must not truncate a built-in tail. */
 uint8_t active=d808.hybrid.active;kick_sample_refresh();assert(d808.hybrid.active==active);
 for(int i=0;i<400;i++)block();ready();context(0,0,0,0);for(int page=11;page<=19;page++){int count=page==12||page==13||page==19?3:page==14?4:page==17?2:1;for(int index=0;index<count;index++)dump_query(page,index);}
 puts("KICK_CC_PASS:39 production-ingress controls,desired/audio-next/current-hit readback,guarded recall,stale/source isolation,project reboot andmatching internal/external step velocities");return 0;
}
