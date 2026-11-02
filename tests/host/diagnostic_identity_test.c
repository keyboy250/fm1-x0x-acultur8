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
 for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
 boot();ready();uint32_t words[24];
 uint32_t fingerprint=project_fingerprint(),writes=store_writes;
 assert(cc_generation_snapshot(0,0,0,words)==10);
 assert(words[9]==X0X_FIRMWARE_ID);
 assert(cc_generation_snapshot(0,11,0,words)==17);
 assert(words[10]==X0X_FIRMWARE_ID);
 dump_query(0,0);dump_query(11,0);
 assert(project_fingerprint()==fingerprint&&store_writes==writes);
 printf("DIAGNOSTIC_IDENTITY_PASS %u pages0,11;no project writes\n",(unsigned)X0X_FIRMWARE_ID);
 return 0;
}
