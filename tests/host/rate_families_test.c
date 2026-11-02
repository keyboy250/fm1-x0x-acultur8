/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/src/seq/rate_families.h"
#include "../../firmware/src/seq/rate_slice_codec.h"
#include <string.h>
int main(void)
{
 static const unsigned legacy[8]={24,16,12,32,96,48,64,8},counts[4]={7,14,14,21};
 unsigned f,i,v,h,r,b;
 static const unsigned exact[21]={24,16,12,32,96,48,64,8,384,192,6,576,288,144,72,36,18,9,256,128,4};
 for(i=0;i<21;i++)assert(rf_divisions[i].pulses4==exact[i]);
 for(i=0;i<8;i++)assert(rf_divisions[i].pulses4==legacy[i]);
 for(f=0;f<4;f++){
  unsigned n=rf_count(f),previous=65535;assert(n==counts[f]);
  for(i=0;i<n;i++){r=rf_ordered(f,i);assert(rf_allowed(f,r));assert(rf_divisions[r].pulses4<previous);previous=rf_divisions[r].pulses4;}
  assert(rf_ordered(f,n)==RF_INVALID);
  for(r=0;r<21;r++){unsigned nearest=rf_nearest(f,r);assert(rf_allowed(f,nearest));if(rf_allowed(f,r))assert(nearest==r);}
  for(h=0;h<=2;h++){
   b=RF_INVALID;
   for(v=0;v<128;v++){unsigned next=rf_sweep(f,v,h,b);assert(next<n);assert(b==RF_INVALID || next>=b);b=next;assert(rf_allowed(f,rf_ordered(f,b)));}assert(b==n-1);
   for(v=128;v-->0;){unsigned next=rf_sweep(f,v,h,b);assert(next<=b);b=next;}assert(b==0);
   for(i=1;i<n;i++){unsigned boundary=(i*128+n-1)/n;
    assert(rf_sweep(f,boundary-1,h,i-1)==i-1);
    assert(rf_sweep(f,boundary+h,h,i-1)==i);
    assert(rf_sweep(f,boundary,h,i)==i);
    assert(rf_sweep(f,boundary-1-h,h,i)==i-1);
   }
  }
  for(v=0;v<128;v++){b=rf_bin(f,v);assert(rf_sweep(f,v,2,RF_INVALID)==b);assert(rf_sweep(f,v,2,b)==b);}
 }
 assert(rf_count(RF_FAMILIES)==0 && rf_bin(RF_FAMILIES,1)==RF_INVALID && rf_bin(0,128)==RF_INVALID);
 for(r=0;r<21;r++){
  static const unsigned ticks[]={1,2,257,1234567,10000000};
  for(i=0;i<5;i++){uint8_t rem=0;uint64_t sum=0;
   for(v=1;v<=10000;v++){uint32_t interval=rf_external_q8(r,ticks[i],&rem);assert(interval>0);sum+=interval;assert(sum==((uint64_t)v*ticks[i]*rf_divisions[r].pulses4)/4);}
  }
  {static const unsigned bpm[]={200,201,1250,2999,3000};
   for(i=0;i<5;i++){uint32_t rem=0;uint64_t sum=0,den=(uint64_t)bpm[i]*96;
    for(v=1;v<=10000;v++){uint32_t interval=rf_internal_q8(r,48000,bpm[i],&rem);assert(interval>0);sum+=interval;assert(sum==((uint64_t)v*48000*600*256*rf_divisions[r].pulses4)/den);assert(rem<den);}
   }
  }
 }
 {uint8_t rem=0;assert(!rf_external_q8(RF_COUNT,100,&rem));assert(!rf_external_q8(11,UINT32_MAX,&rem));assert(rem==0);}
 {rf_latch_t s={0,0,0,RF_STEP,0};assert(rf_select(&s,8)==1);assert(s.effective==0);assert(!rf_boundary(&s,0,1));assert(rf_boundary(&s,1,0));assert(s.effective==8 && !s.pending);s.policy=RF_BAR;assert(rf_select(&s,11)==1);assert(!rf_boundary(&s,1,0));assert(!rf_boundary(&s,0,1));assert(rf_boundary(&s,1,0));assert(s.effective==11);assert(rf_select(&s,11)==0);}
 {uint8_t slices[64],decoded[64],packed[26],before[26];unsigned word,k;
  for(word=0;word<6561;word++){unsigned q=word;for(k=0;k<4;k++){slices[k]=(uint8_t)(q%9);q/=9;}for(k=4;k<64;k++)slices[k]=slices[k%4];
   assert(rf_slice_pack(packed,slices));assert(rf_slice_unpack(decoded,packed));assert(!memcmp(slices,decoded,64));for(k=0;k<64;k++)assert(rf_slice_get(packed,k)==slices[k]);}
  {uint32_t random=44;unsigned record;
   for(record=0;record<10000;record++){for(k=0;k<64;k++){random^=random<<13;random^=random>>17;random^=random<<5;slices[k]=(uint8_t)(random%9);}assert(rf_slice_pack(packed,slices));assert(rf_slice_unpack(decoded,packed));assert(!memcmp(slices,decoded,64));}}
  memcpy(before,packed,26);slices[63]=9;assert(!rf_slice_pack(packed,slices));assert(!memcmp(before,packed,26));
  memset(packed,0,26);packed[0]=161;packed[1]=25;memset(decoded,0xa5,64);assert(rf_slice_get(packed,0)<0);assert(!rf_slice_unpack(decoded,packed));for(k=0;k<64;k++)assert(decoded[k]==0xa5);
  assert(rf_slice_get(packed,64)<0);
 }
 puts("FM1-44 foundation PASS: legacy IDs;21 rational divisions;four families;128 sweeps both directions;hysteresis;10000 intervals/rate/period/tempo;boundary latching");
 return 0;
}
