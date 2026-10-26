/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "app/plat.h"
#define TC_PANEL_SIZE 32u
#define TC_RECORD_SIZE 56u
#define TC_MAGIC 0x314C4354u
#define TC_MIN_HZ 43900u
#define TC_MAX_HZ 44300u
enum { TC_OK, TC_MISSING, TC_LEGACY, TC_DAMAGED, TC_FUTURE };
typedef struct { uint32_t rate, generation, unit, status; } timing_record_t;
static uint32_t tc_get(const uint8_t *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void tc_put(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8)); }
static uint32_t tc_crc(const uint8_t *p,uint32_t n) {
 uint32_t c=~0u;while(n--){c^=*p++;for(unsigned i=0;i<8;i++)c=(c>>1)^(0xEDB88320u&-(c&1u));}return ~c;
}
static int tc_panel_valid(const uint8_t *p) {
 if(tc_get(p)!=0x50414E35u)return 0;
 for(unsigned i=4;i<18;i++)if(p[i]>=14)return 0;
 for(unsigned i=18;i<25;i++)if(p[i]>=7)return 0;
 for(unsigned i=25;i<32;i++)if(p[i]!=1&&p[i]!=255)return 0;
 return 1;
}
static timing_record_t timing_store_read(void) {
 uint8_t p[TC_RECORD_SIZE+1];timing_record_t r={0,0,0,TC_MISSING};
 int n=plat_store_load(OBJ_PANEL,p,sizeof p);
 if(n==-1)return r;
 r.status=TC_DAMAGED;
 if(n<0||n<(int)TC_PANEL_SIZE||!tc_panel_valid(p))return r;
 if(n==(int)TC_PANEL_SIZE){r.status=TC_LEGACY;return r;}
 r.status=TC_FUTURE;
 if(n!=(int)TC_RECORD_SIZE||tc_get(p+32)!=TC_MAGIC||tc_get(p+36)!=1)return r;
 r.status=TC_DAMAGED;
 if(tc_crc(p,52)!=tc_get(p+52))return r;
 r.rate=tc_get(p+40);r.generation=tc_get(p+44);r.unit=tc_get(p+48);
 if((r.rate&&(r.rate<TC_MIN_HZ||r.rate>TC_MAX_HZ))||!r.generation||!r.unit){r.rate=r.generation=r.unit=0;return r;}
 r.status=TC_OK;return r;
}
/* The existing A/B panel object commits atomically. Never allocate another
 * sector or alter any project/sound/strip-default object. Unknown records
 * are read-only; a failed write does not activate a new in-RAM rate.
 */
static int timing_store_write(const void *current_panel,uint32_t rate,uint32_t generation,uint32_t old_unit,uint32_t new_unit) {
 uint8_t p[TC_RECORD_SIZE],check[TC_RECORD_SIZE+1];
 timing_record_t old=timing_store_read();
 if(old.status>TC_LEGACY)return -2;
 if(old.generation!=generation||old.unit!=old_unit||generation==UINT32_MAX)return -3;
 if((rate&&(rate<TC_MIN_HZ||rate>TC_MAX_HZ))||!new_unit||(old.unit&&old.unit!=new_unit))return -2;
 int n=plat_store_load(OBJ_PANEL,p,TC_PANEL_SIZE);
 if(n==-1){for(unsigned i=0;i<TC_PANEL_SIZE;i++)p[i]=((const uint8_t *)current_panel)[i];}
 else if(n!=(int)TC_PANEL_SIZE)return -2;
 if(!tc_panel_valid(p))return -2;
 tc_put(p+32,TC_MAGIC);tc_put(p+36,1);tc_put(p+40,rate);tc_put(p+44,generation+1);tc_put(p+48,new_unit);tc_put(p+52,tc_crc(p,52));
 if(plat_store_save(OBJ_PANEL,p,sizeof p))return -1;
 if(plat_store_load(OBJ_PANEL,check,sizeof check)!=(int)sizeof p)return -1;
 for(unsigned i=0;i<sizeof p;i++)if(p[i]!=check[i])return -1;
 return 0;
}
/* Re-teaching physical buttons/encoders preserves the timing extension. */
static int timing_panel_write(const void *panel_bytes) {
 uint8_t p[TC_RECORD_SIZE];timing_record_t r=timing_store_read();
 if(r.status>TC_LEGACY||!tc_panel_valid(panel_bytes))return -2;
 for(unsigned i=0;i<TC_PANEL_SIZE;i++)p[i]=((const uint8_t *)panel_bytes)[i];
 if(r.status==TC_OK){tc_put(p+32,TC_MAGIC);tc_put(p+36,1);tc_put(p+40,r.rate);tc_put(p+44,r.generation);tc_put(p+48,r.unit);tc_put(p+52,tc_crc(p,52));}
 return plat_store_save(OBJ_PANEL,p,r.status==TC_OK?TC_RECORD_SIZE:TC_PANEL_SIZE);
}
