/* SPDX-License-Identifier: GPL-3.0-only */
#include "rate_families.h"
#include "lanes.h"
/* IDs 0..7 are byte-for-byte legacy meanings; added IDs are never sent through
 * legacy lane.rate1129/CC105. kind: straight1 dotted2 triplet4. */
const rf_division_t rf_divisions[RF_COUNT]={
 {24,1,"1/16"},{16,4,"1/16T"},{12,1,"1/32"},{32,4,"1/8T"},
 {96,1,"1/4"},{48,1,"1/8"},{64,4,"1/4T"},{8,4,"1/32T"},
 {384,1,"1/1"},{192,1,"1/2"},{6,1,"1/64"},
 {576,2,"1/1D"},{288,2,"1/2D"},{144,2,"1/4D"},{72,2,"1/8D"},
 {36,2,"1/16D"},{18,2,"1/32D"},{9,2,"1/64D"},
 {256,4,"1/1T"},{128,4,"1/2T"},{4,4,"1/64T"}
};
static const uint8_t order[RF_COUNT]={11,8,12,18,9,13,19,4,14,6,5,15,3,0,16,1,2,17,7,10,20};
POLY_COLD int rf_allowed(unsigned family,unsigned rate)
{
 static const uint8_t mask[RF_FAMILIES]={1,3,5,7};
 return family<RF_FAMILIES && rate<RF_COUNT && (mask[family]&rf_divisions[rate].kind)!=0;
}
POLY_COLD unsigned rf_count(unsigned family){unsigned i,n=0;for(i=0;i<RF_COUNT;i++)n+=rf_allowed(family,i);return n;}
POLY_COLD unsigned rf_ordered(unsigned family,unsigned index)
{unsigned i;for(i=0;i<RF_COUNT;i++)if(rf_allowed(family,order[i])){if(!index--)return order[i];}return RF_INVALID;}
POLY_COLD unsigned rf_nearest(unsigned family,unsigned rate)
{
 unsigned i,best=RF_INVALID,dist=65535;
 if(rate>=RF_COUNT)return RF_INVALID;
 /* slow-first traversal makes exact ties deterministic: choose slower. */
 for(i=0;i<rf_count(family);i++){unsigned r=rf_ordered(family,i),a=rf_divisions[r].pulses4,b=rf_divisions[rate].pulses4,d=a>b?a-b:b-a;if(d<dist){best=r;dist=d;}}
 return best;
}
POLY_COLD unsigned rf_bin(unsigned family,unsigned value)
{unsigned n=rf_count(family);return n && value<128?value*n/128:RF_INVALID;}
POLY_COLD unsigned rf_sweep(unsigned family,unsigned value,unsigned hysteresis,unsigned prior)
{
 unsigned n=rf_count(family),target=rf_bin(family,value),h=hysteresis>2?2:hysteresis;
 if(target==RF_INVALID)return RF_INVALID;
 if(prior>=n || !value || value==127)return target;
 while(target>prior){unsigned boundary=((prior+1)*128+n-1)/n;if(value<boundary+h)break;prior++;}
 while(target<prior){unsigned boundary=(prior*128+n-1)/n;if(value+h>=boundary)break;prior--;}
 return prior;
}
uint32_t rf_external_q8(unsigned rate,uint32_t tick_q8,uint8_t *remainder)
{
 uint64_t total;uint32_t interval;
 if(rate>=RF_COUNT || !tick_q8 || !remainder || *remainder>=4)return 0;
 total=(uint64_t)tick_q8*rf_divisions[rate].pulses4+*remainder;
 if(total/4>UINT32_MAX || total/4==0)return 0;
 interval=(uint32_t)(total/4);*remainder=(uint8_t)(total%4);return interval;
}
uint32_t rf_internal_q8(unsigned rate,unsigned fs,unsigned bpm_x10,uint32_t *remainder)
{
 uint64_t total;uint32_t denominator,interval;
 if(rate>=RF_COUNT || fs<8000 || fs>192000 || bpm_x10<200 || bpm_x10>3000 || !remainder)return 0;
 denominator=bpm_x10*96u;if(*remainder>=denominator)return 0;
 total=(uint64_t)fs*600u*256u*rf_divisions[rate].pulses4+*remainder;
 if(total/denominator>UINT32_MAX || total/denominator==0)return 0;
 interval=(uint32_t)(total/denominator);*remainder=(uint32_t)(total%denominator);return interval;
}
int rf_select(rf_latch_t *s,unsigned rate)
{if(!s || rate>=RF_COUNT || s->policy>RF_BAR)return -1;if(s->selected==rate)return 0;s->selected=(uint8_t)rate;s->pending=s->effective!=rate;s->bar_seen=0;return 1;}
int rf_boundary(rf_latch_t *s,int step,int bar)
{if(!s || s->policy>RF_BAR || s->selected>=RF_COUNT || s->effective>=RF_COUNT)return -1;if(bar)s->bar_seen=1;if(!step || !s->pending || (s->policy==RF_BAR && !s->bar_seen))return 0;s->effective=s->selected;s->pending=0;s->bar_seen=0;return 1;}
