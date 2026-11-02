#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "/parent/host/x0x_host.c"
#undef main
#include <assert.h>
int main(void){for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;boot();project_t before=proj;FILE *f=fopen("build/kick-tests/format7-store.bin","rb");assert(f);for(int i=0;i<OBJ_NOBJ;i++){assert(fread(&store_len[i],4,1,f)==1);assert(fread(store[i],1,PLAT_STORE_MAX,f)==PLAT_STORE_MAX);}fclose(f);unsigned char bytes[sizeof store];memcpy(bytes,store,sizeof store);int lengths[OBJ_NOBJ];memcpy(lengths,store_len,sizeof lengths);assert(project_load()==-8&&project_write_protected());assert(!memcmp(&before,&proj,sizeof proj));assert(project_save()==-8);sound_strip_config_t strip;assert(project_strip_default_get(0,&strip)==VR_UNSUPPORTED);assert(!memcmp(bytes,store,sizeof store)&&!memcmp(lengths,store_len,sizeof lengths));puts("OLD035_REFUSAL_PASS: format7 project andformat2 preset bank refused without project/store writes");return 0;}
