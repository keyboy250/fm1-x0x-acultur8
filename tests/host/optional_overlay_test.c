/* SPDX-License-Identifier: GPL-3.0-only */
static unsigned guard_enters;
#define OPTIONAL_CONTROL_ENTER_HOOK() (guard_enters++)
#define PATTERN_QUERY_MAIN inherited_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
static uint64_t image_hash=1469598103934665603ull;
static void byte_hash(unsigned b){image_hash=(image_hash^(b&255u))*1099511628211ull;}
static void paint(void){cv_begin(240,138,C_BLACK);for(int b=0;b<2;b++){cv_band(b*69,69);draw_drum(b);}cv_band(0,138);leds();}
int main(void)
{
 for(int format=0;format<3;format++)for(int lane=0;lane<22;lane++){
  mode_setup(format);ui.part=lane/11;ui.sel[ui.part]=lane%11;ui.view=V_GEN;ui.overlay=O_NONE;ui.help=ui.btn=0;proj.set.keyled=1;
  int pat=seq.ppat[ui.part];dpart_t *d=&proj.pat[pat].drum[ui.part];d->len=64;
  if(format)poly_set(proj.pat,(uint8_t *)&proj.sound,pat,lane,64,0);
  seq.variation=(pattern_variation_t){lane,pat,0,8,0,0,0,0};seq.playing=1;seq.mute=seq.vmute=0;
  seq.freeze->state=FREEZE_RECORD;seq.freeze->bass.len=64;
  d->hit[lane%11][0]=d->hit[lane%11][1]=0x55555555u;
#ifndef OPTIONAL_OVERLAY_ORACLE
  CHECK(!ui.live_overlay);pref_t r=PR(R_VARIATION,0,0,7);CHECK(pref_desc(r)==&LIVE_OVERLAY_P&&pref_desc(r)->def==0);
  CHECK(pref_get(r)==0);build_pages();CHECK(pg.r[2][3]==r);
#endif
  for(int step=0;step<64;step++){
   ui.spage=step/16;seq.t[ui.part].started=1;seq.t[ui.part].pos=(step+5)%64;eng_step[TRK_DRUM+ui.part]=(step+5)%64;
   if(format&&lane%11)seq.lane[(lane/11)*10+lane%11-1].pos=step;
   seq.freeze->count=step+1;for(int i=0;i<64;i++)seq.freeze->bass.step[i].flags=(i%2)?G_NOTE:G_REST;
   remember();pattern_freeze_t saved=*seq.freeze;unsigned qw=engine_queue_written(),qr=engine_queue_read();guard_enters=0;
   paint();CHECK(!guard_enters);unchanged(1);CHECK(!memcmp(&saved,seq.freeze,sizeof saved));CHECK(qw==engine_queue_written()&&qr==engine_queue_read());
   for(unsigned i=0;i<240*138;i++){byte_hash(cv_px[i]);byte_hash(cv_px[i]>>8);}for(int i=0;i<4;i++){byte_hash(lit_keys>>(8*i));byte_hash(dim_keys>>(8*i));}
#ifndef OPTIONAL_OVERLAY_ORACLE
   turn_ref(r,1,EN_K1+3);CHECK(pref_get(r)==1);guard_enters=0;paint();CHECK(guard_enters==3);unchanged(1);CHECK(!memcmp(&saved,seq.freeze,sizeof saved));
   turn_ref(r,-1,EN_K1+3);CHECK(pref_get(r)==0);guard_enters=0;paint();CHECK(!guard_enters);unchanged(1);
   CHECK(qw==engine_queue_written()&&qr==engine_queue_read());
#endif
  }
#ifndef OPTIONAL_OVERLAY_ORACLE
  seq.playing=0;turn_ref(r,1,EN_K1+3);CHECK(ui.live_overlay);ui_init();CHECK(!ui.live_overlay);
#endif
 }
 printf("OPTIONAL OVERLAY %s PASS: %d checkpoints; OFF pixel/LED hash=%016llx ui_size=%u step_t0_offset=%u; OFF zero display guards, transient toggles, initialization default.\n",
#ifdef OPTIONAL_OVERLAY_ORACLE
 "PRE-OVERLAY ORACLE",
#else
 "CANDIDATE",
#endif
 checks,(unsigned long long)image_hash,(unsigned)sizeof ui,(unsigned)__builtin_offsetof(ui_t,step_t0));return 0;
}
