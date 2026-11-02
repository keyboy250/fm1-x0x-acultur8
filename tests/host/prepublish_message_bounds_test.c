/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
static void check(const char *a,const char *b) {
    char expected[28];unsigned n=0;
    for(const char *s=a;*s&&n<27;s++)expected[n++]=*s;
    if(b)for(const char *s=b;*s&&n<27;s++)expected[n++]=*s;
    expected[n]=0;
    ui.touched=3;ui.overlay=9;
    memset(ui.msg,0xA5,sizeof ui.msg);
    ui_say(a,b);
    assert(memchr(ui.msg,0,sizeof ui.msg));
    assert(!strcmp(ui.msg,expected));
    assert(ui.msg_until==plat_ms()+1300u);
    assert(ui.touched==3&&ui.overlay==9);
}
int main(void) {
    for(unsigned i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
    boot();run_ms(40);
    check("",0);check("DEFAULT APPLIED",0);
    check("123456789012345678901234567",0);
    check("1234567890123456789012345678",0);
    check("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",0);
    check("DEFAULT ","ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    check("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz","MORE");
    now_ms=0xFFFFFF00u;check("WRAP ","TEST");
    ui.overlay=0;proj.variant.arrangement.flags|=POLY_FLAG;
    run_action(ACT_PERF_TEST,0);
    assert(memchr(ui.msg,0,sizeof ui.msg));
    assert(!strcmp(ui.msg,"USE LEGACY PROJECT FOR TEST"));
    puts("PASS native message bounds: empty/exact/overlong/split inputs, timer wrap, adjacent fields and actual expanded-project benchmark refusal.");
    return 0;
}
