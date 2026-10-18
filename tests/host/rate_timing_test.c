#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"
#include "../../firmware/src/seq/rate_families.h"
static pattern_t patterns[32];static uint8_t sound[1344];static seq_t seq;
static uint16_t units[25];
static uint64_t frames,numerator;static uint32_t denominator,counts[25];static int checking;
static void observe(int lane){uint64_t expected=(uint64_t)counts[lane]*numerator/denominator;if(checking){uint64_t error=frames>expected?frames-expected:expected-frames;assert(error<=1);}counts[lane]++;}
static int drum(void *ctx,int kit,int voice,float vel){(void)ctx;(void)vel;observe(kit*11+voice);return 1;}
static void step(void *ctx,int part,int position){(void)ctx;(void)position;if(part>=2)observe(part+20);}
static const seq_sink_t sink={.drum=drum,.step=step};
static void fixture(unsigned rate,unsigned bpm){
 memset(patterns,0,sizeof patterns);memset(sound,0,sizeof sound);memset(counts,0,sizeof counts);frames=0;checking=1;
 for(int p=0;p<32;p++){for(int kit=0;kit<2;kit++){patterns[p].drum[kit].len=64;for(int v=0;v<11;v++)for(int w=0;w<2;w++)patterns[p].drum[kit].hit[v][w]=~0u;patterns[p].bass[kit].len=64;}}
 poly_enable(patterns,sound);assert(poly_expand(patterns,sound));for(int lane=0;lane<25;lane++)poly_set(patterns,sound,0,lane,64,rate);
 seq_init(&seq,patterns);memset(units,0,sizeof units);seq.rate_units=units;seq.lane_sound=sound;seq.lane_enabled=1;seq.bpm=(float)bpm/10.0f;seq.send_clock=seq.send_notes=0;
 numerator=(uint64_t)FS*150*rf_divisions[rate].pulses4;denominator=bpm*24;
}
static void run(uint32_t length){while(length){uint32_t k=seq_until_event_poly(&seq,length<257?length:257);frames+=k;seq_advance_poly(&seq,k,&sink);length-=k;}}
int main(void){
 static const unsigned tempos[]={200,1250,3000,2001};
 for(unsigned r=0;r<21;r++)for(unsigned b=0;b<4;b++){
  fixture(r,tempos[b]);seq_start(&seq);seq_advance_poly(&seq,0,&sink);
  while(counts[0]<128)run(10000);
  for(int lane=0;lane<25;lane++)assert(counts[lane]==counts[0]);
 }
 for(unsigned r=0;r<21;r++){
  fixture(r,1250);seq.ext=1;seq.ext_period=882u*256u;numerator=(uint64_t)882*rf_divisions[r].pulses4;denominator=4;
  seq_start(&seq);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);
  while(counts[0]<128){run(882);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);}
  for(int lane=0;lane<25;lane++)assert(counts[lane]==counts[0]);
 }
 fixture(11,200);seq_start(&seq);seq_advance_poly(&seq,0,&sink);checking=0;
 uint32_t remaining=seq.t[0].to_next;seq.bpm=300.0f;seq_advance_poly(&seq,0,&sink);assert(seq.t[0].to_next==remaining && units[0]==200);run(10000);assert(seq.t[0].to_next==remaining-10000u*4800u);assert(counts[0]==1);
 fixture(0,1250);seq_start(&seq);seq_advance_poly(&seq,0,&sink);run(1000);checking=0;
 uint8_t position=seq_lane_position(&seq,0);poly_set(patterns,sound,0,0,64,11);seq_lane_mark_pending(&seq,0,1);
 assert(seq_lane_latched_rate(&seq,0)==0);run(4291);assert(seq_lane_position(&seq,0)==position);run(1);assert(seq_lane_latched_rate(&seq,0)==11);
 fixture(0,1250);seq_start(&seq);seq_advance_poly(&seq,0,&sink);run(1000);checking=0;
 poly_rate_config_set(patterns,sound,0,1,1);poly_set(patterns,sound,0,0,64,9);seq_lane_mark_pending(&seq,0,1);
 run(70000);assert(seq_lane_latched_rate(&seq,0)==0);run(25000);assert(seq_lane_latched_rate(&seq,0)==9);
 fixture(0,1250);seq.ext=1;seq.ext_period=882u*256u;seq_start(&seq);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);checking=0;
 seq_ext_clock(&seq);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);assert(!seq.playing);assert(seq.clock.state==CLOCK_LOST);
 fixture(11,1250);seq.ext=1;seq.ext_period=0;seq.ext_since=FS-1;seq_start(&seq);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);assert(!seq.playing && seq.clock.state==CLOCK_LOST);
 fixture(11,1250);seq.ext=1;seq.ext_since=FS-1;seq_start(&seq);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);checking=0;assert(!seq.playing && seq.ext_period==0);
 seq.ext_since=882;seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);assert(!seq.playing);seq_start(&seq);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);assert(seq.playing && seq.ext_period==882u*256u);
 for(unsigned tempo=0;tempo<2;tempo++){
  fixture(11,tempo?200:3000);seq.ext=1;seq.ext_since=tempo?5513:367;seq.ext_period=0;seq_start(&seq);seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);checking=0;assert(seq.playing);
  for(unsigned tick=0;tick<96;tick++){run(tempo?(tick%2?5513:5512):(tick%2?367:368));seq_ext_clock(&seq);seq_advance_poly(&seq,0,&sink);assert(seq.playing);}
 }
 puts("LAN2 timing PASS: all21x25 lanes,128 intervals at4 tempos, fractional external pulses, step/bar latching and ambiguous queued-clock freeze");
}
