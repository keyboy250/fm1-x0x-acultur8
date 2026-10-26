/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_QUERY_MAIN parent_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
int main(void)
{
    for(int format=0;format<3;format++){
        save_hold=0;fresh();CHECK(ui.view==V_HOME&&ui.overlay==O_NONE);CHECK(!seq.playing&&seq.variation.lane==255);
        if(format){CHECK(engine_lane_mode(1));render();}if(format==2){CHECK(engine_rate_set(1,0,0,1));render();}
        proj.pat[4].bass[0].step[0]=(bstep_t){38,G_NOTE|BS_ACCENT|BS_SLIDE};
        proj.pat[4].bass[1].step[3]=(bstep_t){45,G_NOTE};proj.pat[4].swing=40;proj.variant.strip[22].boost_db=3;
        settle();CHECK(project_save()==0);before=proj;unsigned n=store_writes;
        /* Reinitialization, project load and complete boot all use the same startup policy. */
        button_tap(B_EDIT);CHECK(ui.view==V_PART);button_tap(B_ARP);CHECK(ui.view==V_GEN);
        ui_init();CHECK(ui.view==V_HOME);CHECK(!memcmp(&proj,&before,sizeof proj));
        CHECK(project_load()==0);CHECK(!memcmp(&proj,&before,sizeof proj));boot();render();CHECK(ui.view==V_HOME);
        CHECK(!seq.playing&&seq.variation.lane==255);CHECK(!memcmp(&proj,&before,sizeof proj));CHECK(store_writes==n);
        button_tap(B_EDIT);CHECK(ui.view==V_PART);button_tap(B_HOME);CHECK(ui.view==V_HOME);CHECK(!memcmp(&proj,&before,sizeof proj));
        if(format==2){fixture_store("build/home-boot/saved.json");ui.msg_until=0;ui_frame();png_write("build/home-boot/home-native.png");
            FILE *f=fopen("build/home-boot/home-title.rgb565","wb");CHECK(f!=0);
            for(int y=0;y<24;y++)CHECK(fwrite(&fb[y*240],2,60,f)==60);fclose(f);}
    }
    printf("HOME BOOT PASS: %d checkpoints; factory/reinitialize/saved boots across3 formats; HOME visible, stopped/variation OFF, project/sounds/accent/swing/storage unchanged and EDIT/HOME navigation.\n",checks);return 0;
}
