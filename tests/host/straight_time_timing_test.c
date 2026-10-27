/* SPDX-License-Identifier: GPL-3.0-only */
#define main legacy_rate_timing_main
#include "rate_timing_test.c"
#undef main
int main(void){
 static const unsigned tempos[]={200,1250,2001,3000};
 for(unsigned r=21;r<RF_COUNT;r++)for(unsigned b=0;b<4;b++){
  fixture(0,tempos[b]);assert(poly_sync_upgrade(patterns,sound));
  for(int lane=0;lane<25;lane++)poly_set(patterns,sound,0,lane,64,r);
  numerator=(uint64_t)FS*150*rf_divisions[r].pulses4;
  seq_start(&seq);seq_advance_poly(&seq,0,&sink);
  while(counts[0]<3)run(10000);
  for(int lane=0;lane<25;lane++)assert(counts[lane]==counts[0]);
  assert(poly_valid(patterns,sound));
 }
 for(unsigned r=21;r<RF_COUNT;r++){
  fixture(0,1250);assert(poly_sync_upgrade(patterns,sound));
  for(int lane=0;lane<25;lane++)poly_set(patterns,sound,0,lane,64,r);
  seq.ext=1;seq.ext_period=882u*256u;
  numerator=(uint64_t)882*rf_divisions[r].pulses4;denominator=4;
  seq_start(&seq);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);
  while(counts[0]<3){run(882);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);}
  for(int lane=0;lane<25;lane++)assert(counts[lane]==counts[0]);
 }
 puts("STRAIGHT_LONG_TIMING_PASS: all eight multi-bar rates, 25 lanes, 20/125/200.1/300 BPM and external clock; <=1-sample event error");
}
