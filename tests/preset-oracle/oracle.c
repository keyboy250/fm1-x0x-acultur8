#include <stdlib.h>
#define drum808_quiet p35_drum808_quiet
#define drum808_init p35_drum808_init
#define drum808_trigger p35_drum808_trigger
#define drum808_render p35_drum808_render
#define drum808_render_st p35_drum808_render_st
#define drum808_pan_settle p35_drum808_pan_settle
#define drum808_nparams p35_drum808_nparams
#define drum808_param p35_drum808_param
#define drum808_set p35_drum808_set
#define drum808_get p35_drum808_get
#define drum808_active p35_drum808_active
#define drum808_strip_set p35_drum808_strip_set
#define drum808_strip_meter p35_drum808_strip_meter
#define drum808_strip_meter_clear p35_drum808_strip_meter_clear
#define hybrid_kick_param p35_hybrid_kick_param
#define drum808_hybrid_set p35_drum808_hybrid_set
#define drum808_hybrid_sample p35_drum808_hybrid_sample
#include "drum808.c"
void *p35_new(void){drum808_t *d=calloc(1,sizeof *d);p35_drum808_init(d);return d;}
void p35_cfg(void *d,int f,int v){p35_drum808_hybrid_set(d,f,v);}
void p35_base(void *d,int f,int v){p35_drum808_set(d,0,f,v);}
void p35_strip(void *d,const sound_strip_config_t *s){p35_drum808_strip_set(d,0,s);}
void p35_hit(void *d,float v){p35_drum808_trigger(d,0,v);}
void p35_render(void *d,float *a,float *b,float *c,float *e,int n){p35_drum808_render_st(d,a,b,c,e,n);}
