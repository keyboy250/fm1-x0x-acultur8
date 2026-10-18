/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
int main(void) {
    for(unsigned i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
    boot();run_ms(40);
    ui.part=(uint8_t)poly_part(23);ui.view=V_PART;ui.btn=ui.keys=0;
    build_pages();
    pref_set(PR(R_STRIPDEFAULT,23,0,0),1);
    assert(!strcmp(ui.msg,"DEFAULT SAVED"));
    run_ms(40);
    pref_set(PR(R_STRIPDEFAULT,23,0,1),1);
    assert(!strcmp(ui.msg,"DEFAULT APPLIED"));
    assert((int32_t)(ui.msg_until-plat_ms())>0);
    draw_header();
    unsigned black=0,coloured=0;
    for(unsigned y=1;y<HDR_H-1;y++)for(unsigned x=0;x<240;x++)
        if(fb[y*240+x])coloured++;else black++;
    assert(black>20&&coloured>20);
    png_write("build/native-apply-default-feedback.png");
    puts("PASS host native 303B Save/Apply uses DEFAULT SAVED/APPLIED and rasterizes text; physical blank-feedback report remains unresolved.");
    return 0;
}
