/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual UI and CC canonical setters, explicit independent pattern contexts. */
#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
int main(void){
    int32_t audio[512];for(int i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;boot();engine_render(audio,256);
    /* Actual production pref_set/get and exact CC setters with separate contexts. */
    ui.part=PART_303A;
    for(int pattern=0;pattern<NPAT;pattern++)for(int bass=0;bass<NBASS;bass++)for(int field=0;field<NGEN;field++){
        for(int part=0;part<NPARTS;part++)seq.ppat[part]=(uint8_t)pattern;
        cc_sources[0].applied.pattern=(uint8_t)((pattern+7)%NPAT);cc_sources[0].applied.bass=(uint8_t)bass;cc_sources[0].ready=1;
        for(int value=0;value<=GEN_P[field].max;value++){
            uint32_t seed=proj.pat[pattern].bass[bass].gen.seed;
            ui.gen_stale[bass]=0;pref_set(PR(R_GEN,bass,0,field),value);
            assert(pref_get(PR(R_GEN,bass,0,field))==value);
            assert(proj.pat[pattern].bass[bass].gen.seed==seed);
            assert(ui.gen_stale[bass]==(field!=G_MUT));
            assert(cc_parameter((uint16_t)(920+field),(uint32_t)value,0)==CC_APPLIED);
            assert(((uint8_t *)&proj.pat[(pattern+7)%NPAT].bass[bass].gen)[field]==value+(field==G_OCTS));
            assert(ui.gen_stale[bass]==1);engine_render(audio,1);
        }
    }
    for(int value=0;value<128;value++){
        engine_render(audio,1);pref_set(PR(R_TAPE,0,0,0),value);
        assert(pref_get(PR(R_TAPE,0,0,0))==value);
        assert(proj.sound.v[T_FX][0][FX_DL_TYPE]==(value>=16));
        if(value>=16)assert(proj.sound.v[T_FX][0][FX_DL_WEAR]==(value-16)*127/111);
        engine_render(audio,1);assert(cc_parameter(1022,(uint32_t)value,0)==CC_APPLIED);engine_render(audio,1);
    }
    for(int free=0;free<3;free++){
        engine_render(audio,1);
        while(engine_queue_space()>(uint32_t)free)engine_set(T_MIX,0,MX_LEVEL,40);
        project_t saved=proj;uint8_t shadow[sizeof undo_shadow];memcpy(shadow,undo_shadow,sizeof shadow);
        variant_snapshot_t initial,final;assert(variant_snapshot(VS_INFO,0,0,&initial)==VR_OK);undo_t journal=undo;
        assert(cc_parameter(1022,110,0)==CC_BUSY);
        assert(!memcmp(&saved,&proj,sizeof proj)&&!memcmp(shadow,undo_shadow,sizeof shadow)&&!memcmp(&journal,&undo,sizeof undo));
        assert(variant_snapshot(VS_INFO,0,0,&final)==VR_OK && final.revision==initial.revision);engine_render(audio,1);
    }
    puts("canonical setters: GEN32patterns/two basses/eight full domains, UI/CC selection/stale/seed parity;Tape128/CQ0..2 journal/canonical preservation PASS");
    return 0;
}
