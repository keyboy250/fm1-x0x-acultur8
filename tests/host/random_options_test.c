/* SPDX-License-Identifier: GPL-3.0-only */
#define GENERATION_NO_MAIN
#include "generation_test.c"

static uint16_t wanted_mask(int b,int mode)
{
    uint16_t scale=oracle_scale(&proj.pat[1].bass[b].gen);
    return mode==GEN_SCALE?scale:mode==GEN_KEY?0xab5u:mode==GEN_FREE?0xfffu:mode==GEN_BOTH?(scale&(1u<<7|1u<<11|1u<<2)):(1u<<7|1u<<11|1u<<2);
}
static void option_setup(int format,int b)
{
    gen_setup(format);bpart_t *bp=&proj.pat[1].bass[b];bp->len=64;bp->transpose=24;bp->gen.base_oct=3;bp->gen.oct_range=2;bp->gen.scale=5;bp->gen.root=0;bp->gen.density=100;
    for(int i=0;i<64;i++)bp->step[i]=(bstep_t){60,(uint8_t)(G_NOTE|((i&1)?BS_ACCENT:0)|((i%3==0)?BS_SLIDE:0))};
    if(format)poly_set(proj.pat,(uint8_t *)&proj.sound,1,22+b,64,poly_rate(proj.pat,(uint8_t *)&proj.sound,1,22+b));
    ui.part=(uint8_t)(PART_303A+b);seq.ppat[2+b]=1;settle();
}
static void whole_lengths(void)
{
    const int lengths[]={1,8,16,32,64},transposes[]={12,17,24,25,31,36};
    for(int f=0;f<3;f++)for(int b=0;b<2;b++)for(int mode=0;mode<5;mode++)for(unsigned ix=0;ix<5;ix++)for(unsigned ti=0;ti<6;ti++){
        option_setup(f,b);int len=lengths[ix],tr=transposes[ti];proj.pat[1].bass[b].transpose=(uint8_t)tr;settle();
        uint16_t mask=wanted_mask(b,mode);before=proj;int h=undo.cur;
        CHECK(ui_generate_random(1,b,mode,0,12345,len,0)==VR_OK);after=proj;
        bpart_t *out=&proj.pat[1].bass[b];CHECK(out->len==len);CHECK(out->transpose==tr);CHECK(out->rate==before.pat[1].bass[b].rate);CHECK(out->dir==before.pat[1].bass[b].dir);
        for(int i=0;i<64;i++){
            CHECK((out->step[i].flags&BS_ACCENT)==(before.pat[1].bass[b].step[i].flags&BS_ACCENT));
            if(i>=len)CHECK(!memcmp(&out->step[i],&before.pat[1].bass[b].step[i],sizeof(bstep_t)));
            else {int final=out->step[i].note+tr-24;CHECK(final>=48&&final<=71);CHECK(mask&(1u<<(final%12)));}
        }
        original=before;original.pat[1].bass[b]=*out;
        if(f)poly_set(original.pat,(uint8_t *)&original.sound,1,22+b,len,poly_rate(before.pat,(uint8_t *)&before.sound,1,22+b));
        CHECK(!memcmp(&original,&proj,sizeof proj));CHECK(undo.cur==h+1);
        CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
        CHECK(ui_external_undo(1)==VR_OK);render();CHECK(!memcmp(&proj,&after,sizeof proj));
    }
}
static void one_note(void)
{
    for(int b=0;b<2;b++)for(int mode=0;mode<5;mode++)for(int st=1;st<=64;st++)for(int tr=17;tr<=31;tr+=7){
        option_setup(2,b);proj.pat[1].bass[b].transpose=(uint8_t)tr;settle();uint16_t mask=wanted_mask(b,mode);before=proj;
        CHECK(ui_generate_random(1,b,mode,0,987654,8,st)==VR_OK);
        int note=proj.pat[1].bass[b].step[st-1].note;CHECK(note!=60);int final=note+tr-24;CHECK(final>=48&&final<=71);CHECK(mask&(1u<<(final%12)));
        original=before;original.pat[1].bass[b].step[st-1].note=(uint8_t)note;
        CHECK(!memcmp(&original,&proj,sizeof proj));CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    }
}
static void refused_options(void)
{
    option_setup(2,0);remember();CHECK(ui_generate_random(1,0,GEN_SCALE,0,1,65,0)==VR_INVALID);unchanged(1);
    remember();CHECK(ui_generate_random(1,0,GEN_SCALE,0,1,8,65)==VR_INVALID);unchanged(1);
    poly_set(proj.pat,(uint8_t *)&proj.sound,1,22,8,0);settle();remember();CHECK(ui_generate_random(1,0,GEN_SCALE,0,1,32,9)==VR_INVALID);unchanged(1);
    for(int gate=G_REST;gate<=G_TIE;gate+=2){proj.pat[1].bass[0].step[2].flags=(uint8_t)gate;settle();remember();CHECK(ui_generate_random(1,0,GEN_SCALE,0,1,0,3)==VR_UNSUPPORTED);unchanged(1);}
    option_setup(2,0);proj.pat[1].bass[0].transpose=48;proj.pat[1].bass[0].gen.base_oct=0;proj.pat[1].bass[0].gen.oct_range=1;settle();remember();CHECK(ui_generate_random(1,0,GEN_SCALE,0,7,0,1)==VR_OK);unchanged(1);
    option_setup(2,0);set_view(V_GEN);*page_of(V_GEN)=2;build_pages();before=proj;
    pref_set(PR(R_RANDOM,0,0,0),32);pref_set(PR(R_RANDOM,0,0,1),7);CHECK(!memcmp(&before,&proj,sizeof proj));
    button_tap(B_OCTDN);CHECK(ui.overlay==O_ASK);CHECK(strstr(ui.ask_q[0],"STEP 7"));button_tap(B_HOME);CHECK(!memcmp(&before,&proj,sizeof proj));
    button_tap(B_OCTDN);random_opts[1]=8;button_tap(B_SEL);CHECK(strstr(ui.msg,"CHANGED"));CHECK(!memcmp(&before,&proj,sizeof proj));
    ui_init();CHECK(!random_opts[0]&&!random_opts[1]);
}
static void drum_lengths(void)
{
    for(int lane=0;lane<22;lane++)for(int len=8;len<=32;len*=2){
        mode_setup(2);before=proj;int k=lane/11,v=lane%11;
        CHECK(ui_generate_drum_size(1,k,v,50,DR_POS_ALL,DR_AN_NONE,77,len)==VR_OK);
        CHECK(ui_drum_length(&proj.pat[1],k,v)==len);
        CHECK(!memcmp(proj.pat[1].drum[k].accent,before.pat[1].drum[k].accent,sizeof proj.pat[1].drum[k].accent));
        for(int st=len;st<64;st++)CHECK(sm_get(proj.pat[1].drum[k].hit[v],st)==sm_get(before.pat[1].drum[k].hit[v],st));
        original=before;memcpy(original.pat[1].drum[k].hit[v],proj.pat[1].drum[k].hit[v],sizeof original.pat[1].drum[k].hit[v]);poly_set(original.pat,(uint8_t *)&original.sound,1,lane,len,poly_rate(before.pat,(uint8_t *)&before.sound,1,lane));CHECK(!memcmp(&original,&proj,sizeof proj));
        CHECK(ui_external_undo(0)==VR_OK);render();CHECK(!memcmp(&proj,&before,sizeof proj));
    }
    mode_setup(0);remember();CHECK(ui_generate_drum_size(1,0,0,50,DR_POS_ALL,DR_AN_NONE,77,8)==VR_UNSUPPORTED);unchanged(1);
}
static void random_fixtures(void)
{
    for(int b=0;b<2;b++)for(int single=0;single<2;single++){
        option_setup(2,b);CHECK(project_save()==0);char path[128];
        snprintf(path,sizeof path,"build/random-step-options/before-%d-%d.json",b,single);fixture_store(path);
        set_view(V_GEN);*page_of(V_GEN)=2;build_pages();random_opts[0]=single?0:32;random_opts[1]=single?7:0;
        button_tap(B_OCTDN);CHECK(ui.overlay==O_ASK);button_tap(B_SEL);CHECK(strstr(ui.msg,single?"RANDOMIZED":"GENERATED"));
        CHECK(project_save()==0);after=proj;snprintf(path,sizeof path,"build/random-step-options/after-%d-%d.json",b,single);fixture_store(path);
        CHECK(project_load()==0);boot();render();CHECK(!memcmp(&after,&proj,sizeof proj));CHECK(!random_opts[0]&&!random_opts[1]);
    }
}
#ifndef RANDOM_OPTIONS_NO_MAIN
int main(void)
{
    whole_lengths();one_note();refused_options();drum_lengths();random_fixtures();
    printf("RANDOM OPTIONS PASS: %d checkpoints;1/8/16/32/64 steps,both303/5constraints/3formats/transposition,all64 single notes,22drum lengths,exact scope/AC/swing/sounds/tail,Undo/Redo/save/reboot/Cancel/stale/refusals.\n",checks);return 0;
}
#endif
