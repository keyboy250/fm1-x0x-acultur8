/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../firmware/src/app/engine.c"
static void render(void){int32_t audio[512];engine_render(audio,256);}
static void init(int adc,int q){master_control=(master_control_t)MC_INITIAL(q,adc?MC_ADC:MC_POINTER,adc?MC_HAS_ADC:0);}
static void sample(unsigned q){assert(!master_control_adc(&master_control,q));}
static void remote(unsigned q,unsigned s){assert(!cc_parameter(1100,q,(uint8_t)s));}
static void pickup(void){
    /* ADC filtering/curve stays outside the arbiter and remains exactly the old gain when locally owned. */
    init(1,2048);for(unsigned a=0;a<1024;a++){unsigned q=(a*a)>>8;sample(q);assert(plat_master()==q&&master_control_owner(&master_control)==MC_ADC);}
    init(1,1000);sample(1000);remote(2048,0);assert(master_control_flags(&master_control)&MC_PICKUP);
    for(unsigned q=1000;q<2039;q+=7){sample(q);assert(plat_master()==2048&&master_control_owner(&master_control)==MC_USB);}
    sample(2050);assert(plat_master()==2050&&master_control_owner(&master_control)==MC_ADC&&!(master_control_flags(&master_control)&MC_PICKUP));
    sample(2100);assert(plat_master()==2100);
    init(1,3000);sample(3000);remote(2048,1);
    for(unsigned q=3000;q>2057;q-=7){sample(q);assert(plat_master()==2048&&master_control_owner(&master_control)==MC_TRS);}
    sample(2040);assert(plat_master()==2040&&master_control_owner(&master_control)==MC_ADC);
    /* Fast sweeps may skip the tolerance window; crossing is deliberate takeover. */
    init(1,1000);sample(1000);remote(2048,2);sample(3000);assert(plat_master()==3000&&master_control_owner(&master_control)==MC_ADC);
    remote(2048,2);sample(1000);assert(plat_master()==1000&&master_control_owner(&master_control)==MC_ADC);
    /* Unrelated source reset cannot relinquish owner; owner detach holds until pickup. */
    remote(3072,2);cc_reset_source(0);assert(master_control_owner(&master_control)==MC_WEB);cc_reset_source(2);
    assert(plat_master()==3072&&master_control_owner(&master_control)==MC_DETACHED&&(master_control_flags(&master_control)&MC_PICKUP));
    for(int i=0;i<100;i++){sample(1000+(i&1));assert(plat_master()==3072&&master_control_owner(&master_control)==MC_DETACHED);}
    sample(3070);assert(master_control_owner(&master_control)==MC_ADC&&plat_master()==3070);
    /* No previous reading means no imaginary crossing. New remote ownership rearms. */
    init(1,2048);remote(1024,0);sample(3000);assert(plat_master()==1024&&master_control_owner(&master_control)==MC_USB);
    remote(3500,1);sample(1024);assert(plat_master()==3500&&master_control_owner(&master_control)==MC_TRS);sample(3495);assert(master_control_owner(&master_control)==MC_ADC);
    init(1,1024);sample(1024);remote(4096,0);sample(4088);assert(master_control_owner(&master_control)==MC_ADC&&plat_master()==4088);
    remote(0,1);sample(9);assert(plat_master()==0&&master_control_owner(&master_control)==MC_TRS);sample(0);assert(master_control_owner(&master_control)==MC_ADC&&plat_master()==0);
    master_control_t before=master_control;assert(master_control_adc(&master_control,4097)==-1);assert(!memcmp(&before,&master_control,sizeof before));
    init(0,2800);before=master_control;assert(master_control_adc(&master_control,1000)==-1);assert(!memcmp(&before,&master_control,sizeof before));
    puts("MASTER physical pickup: unchanged local ADC curve, both directions/window/fast crossing, endpoints/jitter, source ownership/detach/rearm PASS");
}
static void stationary_pickup(void){
    /* Automation crossing a stationary knob is not physical intent, even at mute. */
    const unsigned positions[]={0,5,2048,4088};
    for(unsigned s=0;s<3;s++)for(unsigned p=0;p<4;p++){
        unsigned pot=positions[p];init(1,pot);sample(pot);
        for(unsigned q=0;q<=4096;q++){remote(q,s);sample(pot);assert(plat_master()==q&&master_control_owner(&master_control)==MC_USB+s);}
        for(int q=4096;q>=0;q--){remote((unsigned)q,s);sample(pot);assert(plat_master()==(unsigned)q&&master_control_owner(&master_control)==MC_USB+s);}
    }
    /* First sample only seeds the movement anchor, including near endpoints. */
    for(unsigned p=0;p<4;p++){
        unsigned pot=positions[p];init(1,2048);remote(pot==4088?4096:0,2);sample(pot);
        assert(plat_master()==(pot==4088?4096u:0u)&&master_control_owner(&master_control)==MC_WEB);
    }
    init(1,5);sample(5);remote(0,2);
    for(int i=0;i<1000;i++){sample(4+(i%3));assert(plat_master()==0&&master_control_owner(&master_control)==MC_WEB);}
    /* Anchor-relative +/-7 jitter never hands off; >=8 movement can pick up. */
    init(1,2000);sample(2000);remote(2000,2);
    for(int i=0;i<1000;i++){sample((i&1)?1993:2007);assert(plat_master()==2000&&master_control_owner(&master_control)==MC_WEB);}
    sample(2008);assert(plat_master()==2008&&master_control_owner(&master_control)==MC_ADC);
    init(1,2000);sample(2000);remote(2000,1);
    for(unsigned d=1;d<8;d++){sample(2000-d);assert(plat_master()==2000&&master_control_owner(&master_control)==MC_TRS);}
    sample(1992);assert(plat_master()==1992&&master_control_owner(&master_control)==MC_ADC);
    /* Slow movement accumulates; repeated remote targets must not reset its anchor. */
    init(1,2000);sample(2000);
    for(unsigned d=0;d<8;d++){remote(2000+d,0);sample(2000+d);assert(master_control_owner(&master_control)==MC_USB);}
    remote(2008,0);sample(2008);assert(plat_master()==2008&&master_control_owner(&master_control)==MC_ADC);
    /* Detach preserves the same movement requirement and held target. */
    init(1,2000);sample(2000);remote(2000,2);cc_reset_source(2);
    for(int i=0;i<100;i++){sample((i&1)?1993:2007);assert(plat_master()==2000&&master_control_owner(&master_control)==MC_DETACHED);}
    sample(1992);assert(plat_master()==1992&&master_control_owner(&master_control)==MC_ADC);
    /* A sub-deadband endpoint movement deliberately holds remote mute. Move away
     * and return far enough to constitute pickup before following the pot. */
    init(1,5);sample(5);remote(0,1);sample(0);assert(plat_master()==0&&master_control_owner(&master_control)==MC_TRS);
    sample(13);assert(plat_master()==0&&master_control_owner(&master_control)==MC_TRS);
    sample(0);assert(plat_master()==0&&master_control_owner(&master_control)==MC_ADC);
    puts("MASTER stationary ADC: all3sources/all4097 targets both directions at4 pot positions, mute/jitter, first sample, accumulated movement, retargeting and detach PASS");
}
static void canonical(void){
    project_t prior=proj;uint32_t writes=store_writes,q=engine_queue_written();int dirty=ui.dirty;
    init(0,2800);
    for(unsigned s=0;s<3;s++)for(unsigned v=0;v<=4096;v++){remote(v,s);assert(plat_master()==v&&master_control_owner(&master_control)==MC_USB+s&&!(master_control_flags(&master_control)&MC_PICKUP));}
    master_control_t before=master_control;assert(cc_parameter(1100,4097,0)==CC_REJECTED);assert(cc_parameter(1100,0xffffffffu,0)==CC_REJECTED);assert(cc_parameter(1100,2048,3)==CC_REJECTED);assert(!memcmp(&before,&master_control,sizeof before));
    for(unsigned raw=0;raw<128;raw++){uint32_t packet=11u|(0xbcu<<8)|(76u<<16)|(raw<<24);assert(!cc_message(packet,2));assert(plat_master()==(raw*4096+63)/127);}
    assert(!cc_parameter(1071,1,2));assert(!cc_parameter(1072,1100&127,2));assert(!cc_parameter(1073,1100>>7,2));
    assert(!cc_parameter(1074,1234&127,2));assert(!cc_parameter(1075,(1234>>7)&127,2));assert(!cc_parameter(1130,0,2));assert(!cc_parameter(1077,1,2));assert(plat_master()==1234);
    rejected_save=1;remote(0,1);remote(4096,1);rejected_save=0;
    assert(!memcmp(&prior,&proj,sizeof proj)&&writes==store_writes&&q==engine_queue_written()&&dirty==ui.dirty);
    ui_external_edit(1);remote(2048,2);assert(external_edit);ui_external_edit(0);
    cc_reset_source(2);assert(plat_master()==2048&&master_control_owner(&master_control)==MC_DETACHED);
    assert(!master_control_set(&master_control,2112,MC_POINTER));assert(plat_master()==2112&&master_control_owner(&master_control)==MC_POINTER);
    project_defaults();assert(plat_master()==2112); /* project reset does not reset runtime gain */
    puts("MASTER CC all3x4097 exact values/all128 CC7, exact transaction, protected/runtime bypass, no journal/project/storage/CQ writes, project reset retention PASS");
}
static void glide(void){
    int32_t block[512],tap[512];init(0,4096);engine_usb_full(1);vol_after=-1;
    for(int i=0;i<512;i++)block[i]=tap[i]=2097152;
    engine_volume(block,256);assert(!memcmp(block,tap,sizeof block));
    remote(2048,2);for(int i=0;i<512;i++)block[i]=tap[i];engine_volume(block,256);
    assert(block[0]<tap[0]&&block[0]>2090000); /* existing ~10ms glide, no half-scale step */
    for(int i=2;i<512;i+=2){assert(block[i]<=block[i-2]&&block[i]==block[i+1]);assert(tap[i]==2097152);}
    for(int n=0;n<20;n++){for(int i=0;i<512;i++)block[i]=tap[i];engine_volume(block,256);}
    assert(abs(block[510]-1048576)<=32&&tap[510]==2097152);
    engine_usb_full(0);for(int i=0;i<512;i++)block[i]=tap[i];engine_volume(block,256);assert(!memcmp(block,tap,sizeof block));
    puts("MASTER unchanged USB-full tap and linked downstream gain glide, no new sample processing PASS");
}
int main(void){for(int i=0;i<OBJ_NOBJ;i++)store_len[i]=-1;boot();render();stationary_pickup();pickup();canonical();glide();assert(sizeof(master_control_t)==4);puts("MASTER prototype native PASS; packed state4B replaces4B, no physical proof");return 0;}
