#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../firmware/src/dsp/hybrid_kick.h"
static void block(void){int32_t pcm[512];engine_render(pcm,256);}
static void settle(void){for(int i=0;i<12;i++)block();}
int main(void){
 for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
 boot();settle();assert(__builtin_offsetof(project_t,kick)==31356&&sizeof(project_t)==31400);assert(engine_kick_get(HK_ENABLE)==0);
 assert(project_save()==0);boot();settle(); /* canonical saved-project baseline */
 unsigned char *before=malloc(sizeof proj);assert(before);memcpy(before,&proj,sizeof proj);
 ui.part=PART_808;ui.sel[PART_808]=0;ui.view=V_PART;build_pages();int found=0;
 for(int i=0;i<pg.n;i++)if(!strcmp(pg.title[i],"KICK RAM"))found=1;assert(found);
 assert(P_KIND(pg.r[pg.n-1][1])==R_STRIPDEFAULT);
 const char *names[]={"SYNTH ENV","WAVE ENV","ENV CURVES","CLICK","CLICK FX"};
 for(int f=0;f<HK_COUNT;f++){int ok=0;for(int i=0;i<pg.n;i++)for(int k=0;k<4;k++)if(P_KIND(pg.r[i][k])==R_KICK&&P_C(pg.r[i][k])==f)ok=1;for(int i=0;i<pg.nmore;i++)if(P_KIND(pg.more[i])==R_KICK&&P_C(pg.more[i])==f)ok=1;assert(ok);}
 for(int n=0;n<5;n++){int ok=0;for(int i=0;i<pg.n;i++)if(!strcmp(pg.title[i],names[n]))ok=1;assert(ok);}
 for(int f=HK_SENV;f<HK_COUNT;f++){const x0x_param_t *p=engine_kick_param(f);assert(engine_kick_set(f,p->max));settle();assert(engine_kick_get(f)==p->max);assert(!memcmp(before,&proj,__builtin_offsetof(project_t,kick)));assert(engine_kick_set(f,p->def));settle();}

 turn_ref(PR(R_KICK,0,0,HK_ENABLE),1,EN_K1);settle();assert(engine_kick_get(HK_ENABLE)==1);
 assert(!memcmp(before,&proj,__builtin_offsetof(project_t,kick)));assert(engine_kick_set(HK_BLEND,50));settle();
 engine_drum(1,0,1);block();assert(engine_kick_set(HK_BLEND,30));block();assert(engine_kick_applied(HK_BLEND)==30&&engine_kick_current(HK_BLEND)==50);
 for(int i=0;i<800;i++)block();assert(engine_kick_set(HK_BLEND,30));settle();
 assert(!engine_kick_set(HK_SOURCE,4)); /* empty USER1 cannot be selected */
 assert(project_save()==0);boot();settle();assert(engine_kick_get(HK_ENABLE)==1);assert(engine_kick_get(HK_BLEND)==30);
 assert(!memcmp(before,&proj,__builtin_offsetof(project_t,kick)));
 free(before);puts("HYBRID_UI_PASS: native pages/knobs,live next-hit guard,unchanged legacy project prefix,saved39-control recall and empty-sample refusal");return 0;
}
