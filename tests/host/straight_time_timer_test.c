/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../../firmware/src/seq/sequencer.c"
static pattern_t patterns[NPAT];static uint8_t sound[1344];static uint16_t units[25];
int main(void){seq_t s;for(int p=0;p<NPAT;p++)pattern_init(&patterns[p],1,2);seq_init(&s,patterns);s.rate_units=units;s.lane_sound=sound;poly_enable(patterns,sound);assert(poly_sync_upgrade(patterns,sound));
 for(int lane=0;lane<25;lane++){
  for(uint64_t q=UINT32_MAX-1ull;q<=UINT32_MAX+2ull;q++){units[lane]=200;uint32_t low=rate_store(&s,lane,q);assert(rate_remaining(&s,lane,low)==q);assert(rate_unit(&s,lane)==800);}
  for(unsigned high=0;high<7;high++){uint64_t q=((uint64_t)high<<32)|123;uint32_t low=rate_store(&s,lane,q);assert(rate_remaining(&s,lane,low)==q);}
  uint32_t low=rate_idle(&s,lane);assert(rate_remaining(&s,lane,low)==UINT64_MAX);assert(rate_unit(&s,lane)==800);
  low=rate_store(&s,lane,0);assert(rate_remaining(&s,lane,low)==0);
 }
 puts("STRAIGHT_TIMER_CARRY_PASS: uint32 wrap, explicit idle sentinel and BPM unit preservation on all25 lanes");
}
