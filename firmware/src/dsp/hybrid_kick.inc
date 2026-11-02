/* SPDX-License-Identifier: GPL-3.0-only */
/* Included by drum808.c. Hybrid layers with per-hit config snapshots; project intent is CC mapped.
 * IMA constants/algorithm follow existing breaks.c / Felucca, Leo Kuroshita. */
static const char *const HK_ON[]={"OFF","ON"};
static const char *const HK_SOURCES[]={"SINE","WARM","HOLLOW","EDGE","USER1"};
static const x0x_param_t HK_PARAMS[HK_COUNT]={
 {"HYBRID",1,0,HK_ON},{"WAVE",4,1,HK_SOURCES},{"WAVE MIX",100,35,0},{"WAVE HZ",100,52,0},
 {"W DECAY",127,58,0},{"W SWEEP",127,53,0},{"K FILTER",1,0,HK_ON},{"K CUTOFF",127,80,0},
 {"V CUTOFF",127,64,0},{"V PITCH",127,64,0},{"V DECAY",127,64,0},{"V BLEND",127,64,0},
 {"LOW VEL",127,65,0},{"MID VEL",127,90,0},{"HIGH VEL",127,110,0},{"J CHANCE",100,10,0},
 {"J RANGE",40,10,0},{"VEL TIERS",1,0,HK_ON},{"S ATTACK",100,0,0},{"S PUNCH",127,64,0},
 {"S P TIME",200,25,0},{"S TAIL",127,0,0},{"S LEVEL",100,50,0},{"V PUNCH",127,64,0},
 {"S ENV",1,0,HK_ON},{"S ATTACK",100,0,0},{"S HOLD",127,0,0},{"S DECAY",127,15,0},{"S CURVE",127,64,0},
 {"W ENV",1,0,HK_ON},{"W ATTACK",100,0,0},{"W HOLD",127,0,0},{"W DECAY",127,15,0},{"W CURVE",127,64,0},
 {"CLICK",60,0,0},{"C TONE",127,86,0},{"C LENGTH",80,12,0},{"CRUNCH",100,0,0},{"SMOOTH",100,0,0}};
const x0x_param_t *hybrid_kick_param(int field){return (unsigned)field<HK_COUNT?&HK_PARAMS[field]:0;}
static void hybrid_kick_defaults(hybrid_kick_t *h){unsigned i;for(i=0;i<HK_COUNT;i++)h->cfg[i]=(uint8_t)HK_PARAMS[i].def;h->rng=808;}
void drum808_hybrid_set(drum808_t *d,int field,int value){const x0x_param_t *p=hybrid_kick_param(field);if(p&&value>=0&&value<=p->max)d->hybrid.cfg[field]=(uint8_t)value;}
void drum808_hybrid_sample(drum808_t *d,const uint8_t *data,uint32_t frames,uint32_t rate){
 d->hybrid.sample=data;d->hybrid.sample_n=frames;d->hybrid.sample_step=(float)rate/SR;
}
static uint32_t hk_rand(hybrid_kick_t *h){h->rng=h->rng*1664525u+1013904223u;return h->rng;}
static float hk_amount(uint8_t value){return value<64?((float)value-64.0f)/64.0f:((float)value-64.0f)/63.0f;}
static float hybrid_kick_resolve_velocity(hybrid_kick_t *h,float vel,int tier){
 int value=(int)(vel*127.0f+0.5f),range=h->cfg[HK_RANGE];
 if(tier>=1&&tier<=3)value=h->cfg[tier==1?HK_LOW:tier==2?HK_MID:HK_HIGH];else if(h->cfg[HK_TIERS])value=h->cfg[value<43?HK_LOW:value<100?HK_MID:HK_HIGH];
 if(range && h->cfg[HK_CHANCE] && hk_rand(h)%100u<h->cfg[HK_CHANCE])value+=(int)(hk_rand(h)%(unsigned)(range*2+1))-range;
 if(value<1)value=1;
 if(value>127)value=127;
 return (float)value/127.0f;
}
static float hybrid_kick_velocity(hybrid_kick_t *h,float vel){return hybrid_kick_resolve_velocity(h,vel,0);}
/* AHD envelopes in seconds. Settings remain immutable while this hit sounds. */
static float hk_env_duration(const hybrid_kick_t *h,int first){
 float scale=first==HK_WA?h->decay_scale:1.0f;
 return clampf((.005f+(float)h->cfg[first+2]*(2.995f/127.0f))*scale,.005f,3.0f);
}
static float hk_envelope(const hybrid_kick_t *h,int first){
 float t=(float)h->elapsed/SR,a=(float)h->cfg[first]*.001f,hold=(float)h->cfg[first+1]*(.5f/127.0f),d=hk_env_duration(h,first),p;
 if(a>0&&t<a){float angle=3.14159265f*t/a;return .5f-.5f*fm_cosf(angle);}
 if(t<a+hold)return 1;
 p=clampf((t-a-hold)/d,0,1);if(p>=1)return 0;
 return fm_powf(1-p,fm_exp2f(2*hk_amount(h->cfg[first+3])))*clampf((1-p)*d/.005f,0,1);
}
static float hk_click(hybrid_kick_t *h){
 float t=(float)h->elapsed/SR,d=(float)(h->cfg[HK_CLEN]?h->cfg[HK_CLEN]:1)*.001f,x,transient,env;
 if(!h->cfg[HK_CLICK]||t>=d)return 0;
 h->click_rng=h->click_rng*1664525u+1013904223u;x=(float)(h->click_rng>>8)*(2.0f/16777216.0f)-1;
 h->click_low+=h->click_hp*(x-h->click_low);transient=x-h->click_low;
 if(h->cfg[HK_CRUNCH]){float q=clampf(transient,-1,1)*h->click_levels;transient=fm_floorf(q+.5f)/h->click_levels;}
 if(h->cfg[HK_SMOOTH]){h->click_z1+=h->click_lp*(transient-h->click_z1);h->click_z2+=h->click_lp*(h->click_z1-h->click_z2);transient=h->click_z2;}
 env=1-t/d;return transient*env*env*clampf(t/.0003f,0,1)*((float)h->cfg[HK_CLICK]*.005f)*h->hit_vel;
}
static void hybrid_kick_trigger(hybrid_kick_t *h,float vel){
 float position=clampf((vel*127.0f-64.0f)/63.0f,-1,1),decay,sweep,cut,punch,tail;
 /* Preserve the outgoing sample across phase/envelope reset. The64-sample
  * residual ramp is only active when a sounding hit has nonzero output;
  * isolated hits retain their original sample path exactly. */
 /* Do not alter legacy unshaped kicks or native-synth-only bypass. */
 h->retrigger_carry=(h->active&&h->cfg[HK_WENV]&&h->mix>0)?h->retrigger_last:0;
 h->retrigger_left=fm_fabsf(h->retrigger_carry)>1e-7f?64:0;
 h->hit_vel=vel;h->decay_scale=fm_exp2f(2.0f*hk_amount(h->cfg[HK_VDECAY])*position);
 h->click_rng=808;h->click_low=h->click_z1=h->click_z2=0;
 h->click_hp=1-d8_exp(-6.28318531f*(800.0f*fm_powf(15.0f,(float)h->cfg[HK_CTONE]/127.0f))/SR);
 h->click_lp=1-d8_exp(-6.28318531f*(18000.0f*fm_powf(800.0f/18000.0f,(float)h->cfg[HK_SMOOTH]*.01f))/SR);
 h->click_levels=fm_exp2f(16.0f-13.0f*(float)h->cfg[HK_CRUNCH]*.01f)-1;
 h->phase=0;h->elapsed=0;h->sample_i=0;h->sample_fraction=0;h->pred=0;h->idx=0;h->previous=0;h->z1=h->z2=0;
 h->hz=h->cfg[HK_PITCH]<30?30.0f:(float)h->cfg[HK_PITCH];
 decay=(0.06f+(float)h->cfg[HK_DECAY]*(0.64f/127.0f))*fm_exp2f(2.0f*hk_amount(h->cfg[HK_VDECAY])*position);
 decay=clampf(decay,0.03f,0.7f);h->decay=d8_exp(-1.0f/(SR*decay));h->env=1;
 sweep=(float)h->cfg[HK_SWEEP]*(180.0f/127.0f)+180.0f*hk_amount(h->cfg[HK_VPITCH])*position;
 h->sweep=clampf(sweep,0,500);h->mix=clampf((float)h->cfg[HK_BLEND]+50.0f*hk_amount(h->cfg[HK_VBLEND])*position,0,100)*.01f;
 cut=80.0f*fm_powf(225.0f,(float)h->cfg[HK_CUTOFF]/127.0f)*fm_exp2f(4.0f*hk_amount(h->cfg[HK_VCUT])*position);
 cut=clampf(cut,80,18000);h->cutoff=h->cfg[HK_FILTER]?1.0f-d8_exp(-6.28318531f*cut/SR):0;
 h->vol=2.5f*vel;h->left=66150;
 if(h->cfg[HK_WENV]&&h->cfg[HK_SOURCE]!=4){float duration=(float)h->cfg[HK_WA]*.001f+(float)h->cfg[HK_WH]*(.5f/127.0f)+hk_env_duration(h,HK_WA);uint32_t frames=(uint32_t)(duration*SR)+1;if(frames>h->left)h->left=frames;}
 if(h->cfg[HK_SOURCE]==4){
  if(!h->sample || !h->sample_n || !(h->sample_step>0)){h->mix=0;}
  else {float duration=(float)h->sample_n/h->sample_step;h->left=(uint32_t)(duration>220500.0f?220500.0f:duration);}
  h->vol*=(float)h->cfg[HK_LEVEL]*.01f;
 }
 punch=clampf(hk_amount(h->cfg[HK_PUNCH])+hk_amount(h->cfg[HK_VPUNCH])*position,-1,1);
 h->punch=fm_exp2f(1.99315686f*punch)-1.0f;
 h->punch_decay=d8_exp(-1.0f/(SR*(float)(h->cfg[HK_PTIME]?h->cfg[HK_PTIME]:1)*.001f));
 tail=(float)h->cfg[HK_TAIL]*(5.0f/127.0f)*fm_exp2f(2.0f*hk_amount(h->cfg[HK_VDECAY])*position);
 h->tail=1;h->tail_decay=h->cfg[HK_TAIL]?d8_exp(-6.0f/(SR*clampf(tail,.02f,5.0f))):1;h->active=1;
}
static const int16_t HK_TABLE[4][256]={
{0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767,32757,32728,32678,32609,32521,32412,32285,32137,31971,31785,31580,31356,31113,30852,30571,30273,29956,29621,29268,28898,28510,28105,27683,27245,26790,26319,25832,25329,24811,24279,23731,23170,22594,22005,21403,20787,20159,19519,18868,18204,17530,16846,16151,15446,14732,14010,13279,12539,11793,11039,10278,9512,8739,7962,7179,6393,5602,4808,4011,3212,2410,1608,804,0,-804,-1608,-2410,-3212,-4011,-4808,-5602,-6393,-7179,-7962,-8739,-9512,-10278,-11039,-11793,-12539,-13279,-14010,-14732,-15446,-16151,-16846,-17530,-18204,-18868,-19519,-20159,-20787,-21403,-22005,-22594,-23170,-23731,-24279,-24811,-25329,-25832,-26319,-26790,-27245,-27683,-28105,-28510,-28898,-29268,-29621,-29956,-30273,-30571,-30852,-31113,-31356,-31580,-31785,-31971,-32137,-32285,-32412,-32521,-32609,-32678,-32728,-32757,-32767,-32757,-32728,-32678,-32609,-32521,-32412,-32285,-32137,-31971,-31785,-31580,-31356,-31113,-30852,-30571,-30273,-29956,-29621,-29268,-28898,-28510,-28105,-27683,-27245,-26790,-26319,-25832,-25329,-24811,-24279,-23731,-23170,-22594,-22005,-21403,-20787,-20159,-19519,-18868,-18204,-17530,-16846,-16151,-15446,-14732,-14010,-13279,-12539,-11793,-11039,-10278,-9512,-8739,-7962,-7179,-6393,-5602,-4808,-4011,-3212,-2410,-1608,-804},
{0,1397,2791,4180,5560,6929,8283,9621,10939,12235,13507,14752,15967,17151,18302,19417,20494,21532,22530,23485,24397,25263,26084,26859,27586,28265,28896,29478,30011,30495,30931,31319,31659,31952,32198,32399,32555,32667,32738,32767,32757,32708,32623,32503,32350,32165,31950,31708,31439,31146,30830,30494,30139,29768,29381,28981,28570,28149,27719,27283,26842,26397,25950,25502,25055,24608,24164,23724,23287,22856,22430,22010,21597,21191,20792,20400,20016,19640,19271,18909,18554,18206,17865,17529,17199,16874,16553,16236,15922,15611,15301,14992,14683,14374,14064,13752,13437,13119,12797,12471,12139,11802,11459,11109,10751,10387,10015,9634,9246,8849,8444,8031,7610,7180,6742,6297,5844,5384,4918,4445,3966,3482,2994,2501,2005,1506,1005,503,0,-503,-1005,-1506,-2005,-2501,-2994,-3482,-3966,-4445,-4918,-5384,-5844,-6297,-6742,-7180,-7610,-8031,-8444,-8849,-9246,-9634,-10015,-10387,-10751,-11109,-11459,-11802,-12139,-12471,-12797,-13119,-13437,-13752,-14064,-14374,-14683,-14992,-15301,-15611,-15922,-16236,-16553,-16874,-17199,-17529,-17865,-18206,-18554,-18909,-19271,-19640,-20016,-20400,-20792,-21191,-21597,-22010,-22430,-22856,-23287,-23724,-24164,-24608,-25055,-25502,-25950,-26397,-26842,-27283,-27719,-28149,-28570,-28981,-29381,-29768,-30139,-30494,-30830,-31146,-31439,-31708,-31950,-32165,-32350,-32503,-32623,-32708,-32757,-32767,-32738,-32667,-32555,-32399,-32198,-31952,-31659,-31319,-30931,-30495,-30011,-29478,-28896,-28265,-27586,-26859,-26084,-25263,-24397,-23485,-22530,-21532,-20494,-19417,-18302,-17151,-15967,-14752,-13507,-12235,-10939,-9621,-8283,-6929,-5560,-4180,-2791,-1397},
{0,2381,4748,7086,9380,11618,13786,15871,17863,19752,21528,23184,24713,26110,27372,28496,29482,30331,31045,31627,32081,32415,32636,32750,32767,32697,32549,32335,32065,31750,31401,31028,30642,30253,29870,29502,29156,28839,28558,28316,28119,27967,27865,27811,27805,27846,27932,28059,28224,28420,28645,28890,29152,29422,29696,29966,30227,30472,30697,30896,31065,31201,31300,31360,31380,31360,31300,31201,31065,30896,30697,30472,30227,29966,29696,29422,29152,28890,28645,28420,28224,28059,27932,27846,27805,27811,27865,27967,28119,28316,28558,28839,29156,29502,29870,30253,30642,31028,31401,31750,32065,32335,32549,32697,32767,32750,32636,32415,32081,31627,31045,30331,29482,28496,27372,26110,24713,23184,21528,19752,17863,15871,13786,11618,9380,7086,4748,2381,0,-2381,-4748,-7086,-9380,-11618,-13786,-15871,-17863,-19752,-21528,-23184,-24713,-26110,-27372,-28496,-29482,-30331,-31045,-31627,-32081,-32415,-32636,-32750,-32767,-32697,-32549,-32335,-32065,-31750,-31401,-31028,-30642,-30253,-29870,-29502,-29156,-28839,-28558,-28316,-28119,-27967,-27865,-27811,-27805,-27846,-27932,-28059,-28224,-28420,-28645,-28890,-29152,-29422,-29696,-29966,-30227,-30472,-30697,-30896,-31065,-31201,-31300,-31360,-31380,-31360,-31300,-31201,-31065,-30896,-30697,-30472,-30227,-29966,-29696,-29422,-29152,-28890,-28645,-28420,-28224,-28059,-27932,-27846,-27805,-27811,-27865,-27967,-28119,-28316,-28558,-28839,-29156,-29502,-29870,-30253,-30642,-31028,-31401,-31750,-32065,-32335,-32549,-32697,-32767,-32750,-32636,-32415,-32081,-31627,-31045,-30331,-29482,-28496,-27372,-26110,-24713,-23184,-21528,-19752,-17863,-15871,-13786,-11618,-9380,-7086,-4748,-2381},
{0,2440,4857,7229,9536,11757,13875,15874,17741,19466,21041,22461,23727,24838,25799,26618,27303,27867,28321,28680,28960,29174,29339,29468,29576,29675,29775,29885,30012,30161,30334,30532,30752,30990,31242,31501,31757,32004,32230,32428,32588,32703,32764,32767,32707,32582,32392,32138,31824,31456,31040,30586,30104,29606,29103,28608,28133,27690,27289,26941,26654,26433,26283,26205,26199,26261,26385,26563,26785,27038,27307,27578,27832,28054,28226,28331,28353,28277,28090,27781,27342,26767,26053,25199,24209,23090,21849,20499,19054,17531,15947,14322,12676,11031,9406,7823,6299,4853,3501,2255,1128,128,-741,-1474,-2072,-2537,-2876,-3095,-3205,-3217,-3143,-2998,-2796,-2550,-2276,-1985,-1692,-1405,-1136,-890,-673,-489,-339,-222,-136,-77,-39,-16,0,16,39,77,136,222,339,489,673,890,1136,1405,1692,1985,2276,2550,2796,2998,3143,3217,3205,3095,2876,2537,2072,1474,741,-128,-1128,-2255,-3501,-4853,-6299,-7823,-9406,-11031,-12676,-14322,-15947,-17531,-19054,-20499,-21849,-23090,-24209,-25199,-26053,-26767,-27342,-27781,-28090,-28277,-28353,-28331,-28226,-28054,-27832,-27578,-27307,-27038,-26785,-26563,-26385,-26261,-26199,-26205,-26283,-26433,-26654,-26941,-27289,-27690,-28133,-28608,-29103,-29606,-30104,-30586,-31040,-31456,-31824,-32138,-32392,-32582,-32707,-32767,-32764,-32703,-32588,-32428,-32230,-32004,-31757,-31501,-31242,-30990,-30752,-30532,-30334,-30161,-30012,-29885,-29775,-29675,-29576,-29468,-29339,-29174,-28960,-28680,-28321,-27867,-27303,-26618,-25799,-24838,-23727,-22461,-21041,-19466,-17741,-15874,-13875,-11757,-9536,-7229,-4857,-2440}
};
static const int16_t HK_IMA_STEP[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
    4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767};
static int hk_ima(hybrid_kick_t *h){
 unsigned byte=h->sample[h->sample_i>>1],code=(h->sample_i&1u)?byte>>4:byte&15u;
 int step=HK_IMA_STEP[h->idx],delta=step>>3;static const int8_t index[]={-1,-1,-1,-1,2,4,6,8};
 if(code&4u)delta+=step;
 if(code&2u)delta+=step>>1;
 if(code&1u)delta+=step>>2;
 h->pred+=(code&8u)?-delta:delta;if(h->pred<-32768)h->pred=-32768;if(h->pred>32767)h->pred=32767;
 h->idx+=index[code&7u];if(h->idx<0)h->idx=0;if(h->idx>88)h->idx=88;h->sample_i++;return h->pred;
}
static float hk_sample_tick(hybrid_kick_t *h){
 float out;if(!h->sample || h->sample_n<2)return 0;
 if(!h->sample_i){h->previous=(int16_t)hk_ima(h);(void)hk_ima(h);}
 out=((float)h->previous+((float)h->pred-h->previous)*h->sample_fraction)*(1.0f/32768.0f);
 h->sample_fraction+=h->sample_step;
 while(h->sample_fraction>=1.0f){h->sample_fraction-=1.0f;h->previous=(int16_t)h->pred;if(h->sample_i<h->sample_n)(void)hk_ima(h);else h->pred=0;}
 return out;
}
static float hybrid_kick_tick(hybrid_kick_t *h,float synth,int synth_active){
 float wave=0,out;
 if(h->left){
  if(h->cfg[HK_SOURCE]==4){
   float attack=(float)h->cfg[HK_ATTACK]*(SR*.001f),fade=attack>0?clampf((float)h->elapsed/attack,0,1):1;
   wave=hk_sample_tick(h)*fade*(1.0f+h->punch)*h->tail;
   if(h->cfg[HK_WENV])wave*=hk_envelope(h,HK_WA);
   h->punch*=h->punch_decay;h->tail*=h->tail_decay;
  }else{
   const int16_t *table=HK_TABLE[h->cfg[HK_SOURCE]&3u];float p,frac;unsigned k;
   h->phase+=(h->hz+h->sweep)/SR;if(h->phase>=1)h->phase-=1;
   p=h->phase*256.0f;k=(unsigned)p;frac=p-(float)k;
   wave=((float)table[k]*(1.0f-frac)+(float)table[(k+1)&255u]*frac)*(1.0f/32768.0f);
   if(h->cfg[HK_WENV])wave*=hk_envelope(h,HK_WA);else wave=wave*h->env*clampf((float)h->elapsed/66.15f,0,1);
   h->sweep*=0.9990933819f;h->env*=h->decay;
  }
  wave*=h->vol*clampf((float)h->left/88.0f,0,1);h->left--;
 }
 if(h->cfg[HK_SENV])synth*=hk_envelope(h,HK_SA);
 out=synth*(1.0f-h->mix)+wave*h->mix;
 if(h->cutoff){h->z1+=h->cutoff*(out-h->z1);h->z2+=h->cutoff*(h->z1-h->z2);out=h->z2;}
 out+=hk_click(h);
 if(h->retrigger_left){out+=h->retrigger_carry*((float)h->retrigger_left*(1.0f/64.0f));h->retrigger_left--;}
 h->retrigger_last=out;
 h->elapsed++;
 if(!h->left&&!synth_active&&fm_fabsf(out)<1e-8f)h->active=0;
 return out;
}
