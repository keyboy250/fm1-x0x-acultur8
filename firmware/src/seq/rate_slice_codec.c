/* SPDX-License-Identifier: GPL-3.0-only */
#include "rate_slice_codec.h"
#include "lanes.h"
POLY_COLD int rf_slice_pack(uint8_t p[26],const uint8_t s[64])
{
 unsigned i,g;
 if(!p || !s)return 0;
 /* Validate entire source first: invalid input never changes destination. */
 for(i=0;i<64;i++)if(s[i]>8)return 0;
 for(i=0;i<26;i++)p[i]=0;
 for(g=0;g<16;g++){
  unsigned word=0,bit=g*13,byte=bit/8,shift=bit%8;
  for(i=4;i-->0;)word=word*9+s[g*4+i];
  word<<=shift;p[byte]|=(uint8_t)word;p[byte+1]|=(uint8_t)(word>>8);
  if(byte+2<26)p[byte+2]|=(uint8_t)(word>>16);
 }
 return 1;
}
POLY_COLD int rf_slice_get(const uint8_t p[26],unsigned step)
{
 unsigned group,bit,byte,shift,word,i;
 if(!p || step>=64)return -1;
 group=step/4;bit=group*13;byte=bit/8;shift=bit%8;
 word=p[byte]|((unsigned)p[byte+1]<<8);
 if(byte+2<26)word|=(unsigned)p[byte+2]<<16;
 word=(word>>shift)&8191;
 if(word>=6561)return -1;
 for(i=0;i<step%4;i++)word/=9;
 return (int)(word%9);
}
POLY_COLD int rf_slice_unpack(uint8_t s[64],const uint8_t p[26])
{
 unsigned i;
 if(!s || !p)return 0;
 /* Validate all groups before touching destination. */
 for(i=0;i<64;i+=4)if(rf_slice_get(p,i)<0)return 0;
 for(i=0;i<64;i++)s[i]=(uint8_t)rf_slice_get(p,i);
 return 1;
}
