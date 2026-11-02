/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_QUERY_MAIN inherited_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
static uint16_t pixel(int x,int y){return swap16(cv_px[y*240+x]);}
static void paint(void){cv_begin(240,138,C_BLACK);for(int band=0;band<2;band++){cv_band(band*69,69);draw_drum(band);}cv_band(0,138);}
int main(void)
{
 for(int format=0;format<3;format++)for(int lane=0;lane<22;lane++){
  mode_setup(format);ui.live_overlay=1;ui.part=lane/11;ui.sel[ui.part]=lane%11;ui.view=V_GEN;ui.overlay=O_NONE;ui.help=ui.btn=0;proj.set.keyled=1;
  int pat=seq.ppat[ui.part];dpart_t *d=&proj.pat[pat].drum[ui.part];d->len=64;
  if(format)poly_set(proj.pat,(uint8_t *)&proj.sound,pat,lane,64,0);
  seq.variation=(pattern_variation_t){lane,pat,0,8,0,0,0,0};seq.playing=1;seq.mute=seq.vmute=0;
  seq.freeze->state=FREEZE_RECORD;seq.freeze->bass.len=64;
  for(int step=0;step<64;step++){
   ui.spage=step/16;seq.t[ui.part].started=1;seq.t[ui.part].pos=(format&&lane%11)?(step+5)%64:step;
   if(format&&lane%11)seq.lane[(lane/11)*10+lane%11-1].pos=step;
   /* Opposite stored/accepted bits explicitly exercise added and suppressed hits. */
   for(int i=0;i<64;i++)seq.freeze->bass.step[i].flags=(i%2)?G_NOTE:G_REST;
   d->hit[lane%11][0]=d->hit[lane%11][1]=0x55555555u;seq.freeze->count=step+1;
   remember();pattern_freeze_t f=*seq.freeze;unsigned qw=engine_queue_written(),qr=engine_queue_read();
   live_drum_overlay_t live=live_drum_overlay();CHECK(live.active&&live.step==step&&live.count==step+1);
   paint();leds();unchanged(1);CHECK(!memcmp(&f,seq.freeze,sizeof f));CHECK(qw==engine_queue_written()&&qr==engine_queue_read());
   unsigned mask=1u<<WHITE_KEY[step%16];CHECK(!!(lit_keys&mask)==!!(step%2));
   CHECK(!!(dim_keys&mask)==!(step%2));
   for(int col=0;col<16;col++){
    int st=ui.spage*16+col,x=col_x(col)+4,y=(lane%11)*DROW+1;
    CHECK((pixel(x,y+DROW-4)==C_AMB)==(st<=step&&st%2));
    CHECK((pixel(x,y+3)==C_WHITE)==(st==step&&st%2));
    /* A base hit's outer fill remains visible even if its trigger is suppressed. */
    if(!(st%2))CHECK(pixel(col_x(col)+1,y+3)==part_col());
   }
  }
  seq.mute=1u<<lane;CHECK(!live_drum_hit(&(live_drum_overlay_t){{0,0},0,0,0},0));
  CHECK(live_drum_overlay().active&&!live_drum_overlay().count);seq.mute=0;
  seq.freeze->count=65;CHECK(!live_drum_overlay().count);seq.freeze->count=64;
  seq.freeze->bass.len=63;CHECK(!live_drum_overlay().count);seq.freeze->bass.len=64;
  seq.freeze->state=FREEZE_HELD;CHECK(live_drum_overlay().count==64);
  seq.variation.pattern^=1;CHECK(!live_drum_overlay().active);seq.variation.pattern^=1;
  seq.variation.lane=255;CHECK(!live_drum_overlay().active);seq.variation.lane=lane;
  ui.sel[ui.part]=(lane%11+1)%11;CHECK(!live_drum_overlay().active);ui.sel[ui.part]=lane%11;
  seq.playing=0;CHECK(!live_drum_overlay().active);leds();
  for(int col=0;col<16;col++)CHECK(!!(lit_keys&(1u<<WHITE_KEY[col]))==!(col%2));
  ui.btn=1u<<B_ENV;d->accent[1]=0xaaaau<<16;leds();
  for(int col=0;col<16;col++)CHECK(!!(lit_keys&(1u<<WHITE_KEY[col]))==!!(col%2));ui.btn=0;
 }
 printf("LIVE TRIGGER OVERLAY PASS: %d checkpoints;22 lanes/3 formats/all64 positions, actual grid pixels and key LEDs, added/suppressed hits, base retained, prefix tail/mute/held/Stop/OFF/mismatch and project/Undo/seq/capture/queue/storage invariants.\n",checks);
 return 0;
}
