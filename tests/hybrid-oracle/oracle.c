#include <stdlib.h>
#define drum808_quiet old_quiet
#define drum808_init old_init
#define drum808_trigger old_trigger
#define drum808_render old_render
#define drum808_render_st old_render_st
#define drum808_pan_settle old_pan_settle
#define drum808_nparams old_nparams
#define drum808_param old_param
#define drum808_set old_set
#define drum808_get old_get
#define drum808_active old_active
#define drum808_strip_set old_strip_set
#define drum808_strip_meter old_strip_meter
#define drum808_strip_meter_clear old_strip_meter_clear
#include "drum808.c"
void *oracle_new(void){drum808_t *d=calloc(1,sizeof(*d));old_init(d);return d;}
void oracle_hit(void *d,int t,float vel){old_trigger(d,t,vel);}
void oracle_set(void *d,int t,int i,int v){old_set(d,t,i,v);}
void oracle_render(void *d,float *l,float *r,float *rev,float *dly,int n){old_render_st(d,l,r,rev,dly,n);}
