/* SPDX-License-Identifier: GPL-3.0-only */
#define MIDI_COPY_NO_MAIN
#include "midi_copy_test.c"

int main(void){
    cp_refusals();
    /* Every stored slot,including valid slot0/31;UI focus stays unrelated. */
    const int scopes[]={0,1,12,23,24};
    for(int source=0;source<32;source++)for(unsigned i=0;i<5;i++){
        int destination=(source+17)%32,scope=scopes[i],port=source%3;cp_setup(2);
        CHECK(ui_pattern_copy_to(source,destination,scope,scope)==VR_OK);project_t golden=proj;
        cp_setup(2);cp_recipe(port,destination,scope,scope,1);CHECK(!midi_parameter(port,1241,cp_raw(source,31)));
        CHECK(!cp_preview(port));uint32_t words[24];CHECK(cc_generation_snapshot(port,10,0,words)==16);CHECK(words[5]==(unsigned)source&&words[6]==(unsigned)destination);
        CHECK(!cp_execute(port));CHECK(!memcmp(&golden,&proj,sizeof proj));
    }
    for(int kind=0;kind<2;kind++){
        cp_setup(2);
        if(!kind){
            for(int i=0;i<NLANE;i++){lane_t *l=&proj.arr.lane[i];memset(l,0,sizeof *l);l->used=1;l->pat=3;l->part=0;l->t=T_909;l->v=2;}
            proj.arr.lane[0].pat=1;proj.arr.lane[0].v=0;settle();
        }else for(unsigned i=0;i<sizeof undo_shadow;i++)undo.shadow[i]=(uint8_t)~undo.region[i];
        project_t base=proj;undo_t journal=undo;uint8_t shadow[sizeof undo_shadow];memcpy(shadow,undo_shadow,sizeof shadow);
        cp_recipe(0,2,1,1,1);CHECK(cp_preview(0)<0);CHECK(cc_sources[0].generation.reason==CG_COPY_ROOM);
        CHECK(!memcmp(&base,&proj,sizeof proj)&&!memcmp(&journal,&undo,sizeof undo)&&!memcmp(shadow,undo_shadow,sizeof shadow));CHECK(cp_execute(0)<0);
    }
    /* Arming with one token byte missing cannot borrow old initialized state. */
    cp_setup(2);cp_recipe(0,2,1,1,1);cc_sources[0].generation.flags&=(uint8_t)~CG_TOKEN_HI;CHECK(cp_preview(0)<0);
    /* Editing either target after preview invalidates the consumed request. */
    cp_setup(2);cp_recipe(0,2,1,1,1);CHECK(!cp_preview(0));CHECK(!midi_parameter(0,1242,cp_raw(3,31)));CHECK(cp_preview(0)<0&&cc_sources[0].generation.reason==CG_STALE);
    printf("COPY GUARDS PASS: %d checkpoints;all32 stored slots,mode/reset/stale/source guards,motion/Undo capacity atomic refusal,missing token,changed target requires fresh ID.\n",checks);return 0;
}
