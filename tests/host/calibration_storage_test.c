/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/plat.h"
#include "../../firmware/src/storage.c"
static uint8_t flash[1<<20],baseline[1<<20];static int budget=-1;
static int st_read(uint32_t off,void *dst,uint32_t n){assert(off+n<=sizeof flash);memcpy(dst,flash+off,n);return 0;}
static int st_erase(uint32_t off){assert(off==0xDC000||off==0xDD000);memset(flash+off,255,4096);return 0;}
static int st_prog(uint32_t off,const void *src,uint32_t n){
 assert((off>=0xDC000&&off+n<=0xDD000)||(off>=0xDD000&&off+n<=0xDE000));
 for(uint32_t i=0;i<n;i++){if(budget==0)return -1;if(budget>0)budget--;flash[off+i]&=((const uint8_t *)src)[i];}return 0;
}
int plat_store_load(uint32_t o,void *p,uint32_t n){return st_load(o,p,n);}
int plat_store_save(uint32_t o,const void *p,uint32_t n){return st_save(o,p,n);}
#include "../../firmware/src/timing_store.h"
static const uint8_t panel[32]={0x35,0x4e,0x41,0x50,2,3,4,5,6,7,8,9,10,11,12,13,0,1,0,1,6,2,3,4,5,1,1,1,1,1,1,1};
int main(void){
 memset(flash,255,sizeof flash);assert(timing_store_read().status==TC_MISSING);
 assert(!timing_panel_write(panel));assert(timing_store_read().status==TC_LEGACY);
 assert(!timing_store_write(panel,44120,0,0,99));timing_record_t r=timing_store_read();assert(r.rate==44120&&r.generation==1);
 uint8_t old_reader[32];assert(st_load(OBJ_PANEL,old_reader,32)==32&&!memcmp(old_reader,panel,32));
 memcpy(baseline,flash,sizeof flash);
 for(int cut=0;cut<(int)(TC_RECORD_SIZE+sizeof(st_hdr_t));cut++){
  memcpy(flash,baseline,sizeof flash);budget=cut;
  assert(timing_store_write(panel,44119,1,99,99)<0);
  r=timing_store_read();assert(r.status==TC_OK&&r.rate==44120&&r.generation==1);
 }
 memcpy(flash,baseline,sizeof flash);budget=-1;assert(!timing_store_write(panel,44119,1,99,99));
 r=timing_store_read();assert(r.rate==44119&&r.generation==2&&r.unit==99);
 uint8_t unknown[TC_RECORD_SIZE+1];memset(unknown,0,sizeof unknown);memcpy(unknown,panel,32);
 assert(!st_save(OBJ_PANEL,unknown,sizeof unknown));assert(timing_store_read().status==TC_FUTURE);
 memcpy(baseline,flash,sizeof flash);assert(timing_store_write(panel,44120,0,0,77)==-2);assert(!memcmp(flash,baseline,sizeof flash));
 memset(flash,0,sizeof flash);assert(timing_store_read().status==TC_DAMAGED);
 for(unsigned i=0;i<0xDC000;i++)assert(baseline[i]==255);
 for(unsigned i=0xDE000;i<sizeof flash;i++)assert(baseline[i]==255);
 puts("CALIBRATION STORAGE PASS:actual A/B sectors,88 power-cut positions,legacy reader,panel preservation,future/corrupt refusal,other flash unchanged.");
 return 0;
}
