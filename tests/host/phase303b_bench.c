/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include "../../firmware/src/dsp/phase303b.h"
#include <stdio.h>
#include <time.h>
#include <assert.h>
static volatile float checksum;
static double bench(unsigned config){
 bass303_t voice;float out[256];bass303_init(&voice);
 bass303_set(&voice,BASS303_CUTOFF,100);bass303_set(&voice,BASS303_RESO,100);
 phase_select(&voice,config);slot_note_on(&voice,60,1,0);
 clock_t start=clock();
 for(unsigned i=0;i<6000;i++){slot_render(&voice,out,256);checksum+=out[i&255u];}
 return (double)(clock()-start)*1000000.0/((double)CLOCKS_PER_SEC*6000.0*256.0);
}
int main(void){
 double legacy=bench(0),worst=0;printf("HOST_CPU original303_us_per_sample=%.6f\n",legacy);
 for(unsigned wave=0;wave<8;wave++)for(unsigned shape=0;shape<2;shape++){
  double t=bench(0x8080u|wave|(shape?0x7F00u:0u));if(t>worst)worst=t;
  printf("HOST_CPU wave=%u shape=%u us_per_sample=%.6f ratio_to303=%.3f\n",wave,shape*127u,t,t/legacy);
 }
 assert(legacy>0&&worst>0);printf("HOST_CPU worst_ratio=%.3f checksum=%g (host comparison only; device deadlines require physical acceptance)\n",worst/legacy,(double)checksum);
 return 0;
}
