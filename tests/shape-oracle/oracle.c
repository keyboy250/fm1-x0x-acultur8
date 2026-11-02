#include <stdlib.h>
#define drum808_quiet shape_ref_quiet
#define drum808_init shape_ref_init
#define drum808_trigger shape_ref_trigger
#define drum808_render shape_ref_render
#define drum808_render_st shape_ref_render_st
#define drum808_pan_settle shape_ref_pan_settle
#define drum808_nparams shape_ref_nparams
#define drum808_param shape_ref_param
#define drum808_set shape_ref_set
#define drum808_get shape_ref_get
#define drum808_active shape_ref_active
#define drum808_strip_set shape_ref_strip_set
#define drum808_strip_meter shape_ref_strip_meter
#define drum808_strip_meter_clear shape_ref_strip_meter_clear
#define hybrid_kick_param shape_ref_kick_param
#define drum808_hybrid_set shape_ref_kick_set
#define drum808_hybrid_sample shape_ref_kick_sample
#include "drum808.c"
void *shape_shape_ref_new(void){drum808_t *d=calloc(1,sizeof(*d));shape_ref_init(d);return d;}
void shape_shape_ref_hit(void *d,int t,float vel){shape_ref_trigger(d,t,vel);}
void shape_shape_ref_set(void *d,int t,int i,int v){shape_ref_set(d,t,i,v);}
void shape_shape_ref_render(void *d,float *l,float *r,float *rev,float *dly,int n){shape_ref_render_st(d,l,r,rev,dly,n);}

void shape_ref_kick(void *p,int f,int v){shape_ref_kick_set(p,f,v);}
