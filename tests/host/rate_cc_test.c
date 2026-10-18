#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
static void render(void){int32_t b[512];engine_render(b,256);}
static void context(int source,int p,int lane){assert(!cc_parameter(1010,p,source));assert(!cc_parameter(1069,lane,source));assert(!cc_parameter(1071,1,source));}
static const uint8_t ordered[4][21]={
 {8,9,4,5,0,2,10},
 {11,8,12,9,13,4,14,5,15,0,16,2,17,10},
 {8,18,9,19,4,6,5,3,0,1,2,7,10,20},
 {11,8,12,18,9,13,19,4,14,6,5,15,3,0,16,1,2,17,7,10,20}};
static const unsigned count[4]={7,14,14,21};
static unsigned expected_bin(unsigned f,unsigned value,unsigned h,unsigned prior){
 unsigned target=value*count[f]/128;if(prior>=count[f]||!value||value==127)return target;
 unsigned next=prior;
 for(unsigned bin=prior+1;bin<=target;bin++)if(value>=((bin*128+count[f]-1)/count[f])+h)next=bin;
 for(unsigned bin=prior;bin>target;bin--)if(value+h<((bin*128+count[f]-1)/count[f]))next=bin-1;
 return next;
}
int main(void){
 for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;boot();render();context(0,0,0);
 assert(!cc_parameter(1131,1,0));render();assert(!cc_parameter(1135,1,0));render();assert(poly_expanded((const uint8_t *)&proj.sound));
 for(unsigned source=0;source<3;source++)for(unsigned lane=0;lane<25;lane++)for(unsigned f=0;f<4;f++)for(unsigned h=0;h<3;h++){
  context((int)source,(int)(lane%32),(int)lane);if(engine_queue_space()<4)render();
  assert(!cc_parameter(1137,f,source));assert(!cc_parameter(1139,h,source));render();unsigned prior=255;
  for(unsigned v=0;v<256;v++){
   unsigned value=v<128?v:255-v,bin=expected_bin(f,value,h,prior);int current=poly_rate(proj.pat,(const uint8_t *)&proj.sound,lane%32,lane);
   if(prior<count[f]&&ordered[f][prior]!=current)bin=expected_bin(f,value,h,255);
   uint32_t q=engine_queue_written();if(engine_queue_space()<2)render();q=engine_queue_written();
   assert(!cc_parameter(1140,value,source));assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,lane%32,lane)==ordered[f][bin]);
   if(current==ordered[f][bin])assert(engine_queue_written()==q);
   prior=bin;
  }
 }
 render();context(0,0,0);assert(!cc_parameter(1137,0,0));render();assert(!cc_parameter(1140,127,0));render();
 while(engine_queue_space())assert(engine_strip_set(0,VP_FLAGS,proj.variant.strip[0].flags));
 uint32_t q=engine_queue_written();assert(!cc_parameter(1140,127,0));assert(q==engine_queue_written());render();
 assert(cc_parameter(1129,8,0)==CC_REJECTED);assert(!cc_parameter(1129,1,0));render();assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,0,0)==1);
 assert(!cc_parameter(1140,0,0));render();assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,0,0)==8);
 for(int i=0;i<21;i++){assert(!cc_parameter(1136,i,0));render();assert(poly_rate(proj.pat,(const uint8_t *)&proj.sound,0,0)==i);}
 project_t before=proj;rejected_save=1;assert(cc_parameter(1140,5,0)==CC_REJECTED);assert(!memcmp(&before,&proj,sizeof proj));rejected_save=0;
 puts("Expanded native CC PASS:3 sources x25 lanes x4 families x3 hyst x256 values; strict bins, no-opCQ, preserved legacy IDs, all21 exact choices and protected refusal");
}
