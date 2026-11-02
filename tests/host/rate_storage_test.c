#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/lanes.h"
static pattern_t patterns[NPAT];
static uint8_t sound[1344],before[1344],lengths[32][25],rates[32][25],slices[32][64],gates[32][64];
int main(void){
 unsigned p,l,i;memset(sound,0xa5,sizeof sound);
 for(p=0;p<32;p++){for(l=0;l<2;l++){patterns[p].drum[l].len=16;patterns[p].drum[l].rate=(uint8_t)l;patterns[p].bass[l].len=32;}patterns[p].brk.steps=0x5555;}
 poly_enable(patterns,sound);assert(poly_valid(patterns,sound));
 for(p=0;p<32;p++){
  for(l=0;l<25;l++){lengths[p][l]=(uint8_t)((p*25+l)%64+1);rates[p][l]=(uint8_t)((p+l)%8);poly_set(patterns,sound,p,l,lengths[p][l],rates[p][l]);}
  for(i=0;i<64;i++){slices[p][i]=(uint8_t)((p*64+i)%9);gates[p][i]=(uint8_t)((p+i)%2);poly_break_set(patterns,sound,p,i,gates[p][i],slices[p][i]);}
 }
 memcpy(before,sound,sizeof sound);assert(poly_expand(patterns,sound));assert(poly_expanded(sound));assert(poly_valid(patterns,sound));
 for(i=0;i<1344;i++)if(!poly_cell_owned(i))assert(sound[i]==before[i]);
 for(p=0;p<32;p++){
  for(l=0;l<25;l++){assert(poly_length(patterns,sound,p,l)==lengths[p][l]);assert(poly_rate(patterns,sound,p,l)==rates[p][l]);}
  for(i=0;i<64;i++){assert(poly_break_slice(patterns,sound,p,i)==slices[p][i]);assert(poly_break_gate(patterns,sound,p,i)==gates[p][i]);}
 }
 for(l=0;l<25;l++){poly_rate_config_set(patterns,sound,l,0,l%4);poly_rate_config_set(patterns,sound,l,1,l%2);poly_rate_config_set(patterns,sound,l,2,l%3);}
 for(p=0;p<32;p++)for(l=0;l<25;l++){
  assert(poly_rate_config(patterns,sound,l,0)==(int)(l%4));assert(poly_rate_config(patterns,sound,l,1)==(int)(l%2));assert(poly_rate_config(patterns,sound,l,2)==(int)(l%3));
  for(i=0;i<21;i++){poly_set(patterns,sound,p,l,lengths[p][l],i);assert(poly_rate(patterns,sound,p,l)==(int)i);assert(poly_rate_config(patterns,sound,l,0)==(int)(l%4));}
 }
 for(p=0;p<32;p++)for(i=0;i<64;i++)assert(poly_break_slice(patterns,sound,p,i)==slices[p][i]);
 assert(poly_valid(patterns,sound));poly_rate_config_set(patterns,sound,24,2,3);assert(!poly_valid(patterns,sound));
 puts("LAN2 PASS: actual migration32x25x64, all21 rate IDs, disjoint sound/config ownership and invalid config rejection");
}
