/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_QUERY_MAIN parent_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
static void properties(void)
{
    unsigned cases=0,evolved=0;
    for(int len=1;len<=64;len++)for(int every=0;every<3;every++)for(int an=0;an<4;an++)for(int mode=0;mode<3;mode++)for(int range=0;range<4;range++)for(int shape=0;shape<2;shape++)for(int n=0;n<4;n++){
        const int amounts[]={0,1,3,8};pattern_variation_t v={7,31,(uint8_t)every,(uint8_t)amounts[n],(uint8_t)(len*3+n*17),(uint8_t)an,(uint8_t)mode,(uint8_t)(range*2+shape)},old=v;
        int changed=0,different=0,cadence=2<<every,span=range?(2<<range):len;if(span>len)span=len;
        for(int st=0;st<len;st++){
            int base=(st%3)!=1,a=pattern_variation_hit(&v,base,st,len,(unsigned)(cadence-1));
            CHECK(pattern_variation_hit(&v,base,st,len,0)==base);
            CHECK(pattern_variation_hit(&v,base,st,len,(unsigned)cadence)==base);
            CHECK(a==pattern_variation_hit(&v,base,st,len,(unsigned)(cadence-1)));
            int next=pattern_variation_hit(&v,base,st,len,(unsigned)(2*cadence-1));
            if(!shape)CHECK(next==a);else different+=next!=a;
            if(((an&1)&&st%4==0)||((an&2)&&(st%16==4||st%16==12))||st<len-span)CHECK(a==base);
            if(mode==1)CHECK(a>=base);if(mode==2)CHECK(a<=base);changed+=a!=base;
        }
        CHECK(changed<=amounts[n]);CHECK(!memcmp(&v,&old,sizeof v));evolved+=different!=0;cases++;
    }
    CHECK(evolved>100);
    for(int f=0;f<7;f++){
        pattern_variation_t v={0,0,1,2,0,0,0,0};uint8_t *p=(uint8_t *)&v;
        if(f==0)p[0]=22;if(f==1)p[1]=32;if(f==2)p[2]=3;if(f==3)p[3]=9;if(f==4)p[5]=4;if(f==5)p[6]=3;if(f==6)p[7]=8;
        CHECK(!pattern_variation_valid(&v));CHECK(pattern_variation_hit(&v,1,3,16,3)==1);CHECK(pattern_variation_hit(&v,0,3,16,3)==0);
    }
    pattern_variation_t v={0,0,2,8,255,0,0,1};CHECK(pattern_variation_valid(&v));
    for(int st=0;st<64;st++){CHECK(pattern_variation_hit(&v,1,st,64,65536)==1);CHECK(pattern_variation_hit(&v,0,st,64,65535)==pattern_variation_hit(&v,0,st,64,65535));}
    v.lane=255;CHECK(pattern_variation_hit(&v,1,1,16,7)==1);CHECK(pattern_variation_hit(0,1,1,16,7)==1);v.lane=0;
    CHECK(pattern_variation_hit(&v,1,0,0,7)==1);CHECK(pattern_variation_hit(&v,1,64,64,7)==1);CHECK(pattern_variation_hit(&v,1,-1,16,7)==1);
    printf("VARIATION PROPERTIES: %u recipes; fixed/evolving, strict change cap, locks, ranges, no drift, invalid inputs/counter wrap PASS\n",cases);
}
#define CYCLES 8
static struct { uint32_t hits[22][CYCLES][2],ticks[22][CYCLES][64];uint8_t vel[22][CYCLES][64];unsigned on,off; } observed;
static uint32_t elapsed;static int last_lane,last_vel,pending_on;
static const uint8_t expected_gm[2][11]={{36,38,41,45,50,37,39,42,46,49,51},{36,38,41,45,50,37,39,56,49,46,42}};
static void variation_snapshot(const char *name){FILE *f=fopen(name,"wb");CHECK(f!=0);pattern(f,0);pattern(f,1);fclose(f);}
static int audit_drum(void *ctx,int kit,int voice,float vel)
{
    int lane=kit*11+voice,st=seq_lane_position(&seq,lane);unsigned cycle=seq_lane_cycles(&seq,lane);
    int accepted=s_drum_seq(ctx,kit,voice,vel);CHECK(accepted);
    if(cycle<CYCLES && st>=0 && st<64){sm_set(observed.hits[lane][cycle],st);observed.vel[lane][cycle][st]=(uint8_t)(vel*127.0f);observed.ticks[lane][cycle][st]=elapsed;}
    CHECK(!pending_on);last_lane=lane;last_vel=(int)(vel*127.0f);return accepted;
}
static void audit_midi(void *ctx,uint8_t st,uint8_t d1,uint8_t d2)
{
    (void)ctx;
    if((st&240)==144){CHECK(!pending_on);CHECK(st==(0x90|seq.ch[last_lane/11]));CHECK(d1==expected_gm[last_lane/11][last_lane%11]);CHECK(d2==last_vel);pending_on=1;observed.on++;}
    if((st&240)==128){CHECK(pending_on);CHECK(st==(0x80|seq.ch[last_lane/11]));CHECK(d1==expected_gm[last_lane/11][last_lane%11]);CHECK(d2==0);pending_on=0;observed.off++;}
}
static void prepare(int format,int lane)
{
    save_hold=0;fresh();pending_on=0;
    for(int p=0;p<NPAT;p++)pattern_init(&proj.pat[p],123,456);
    if(format){CHECK(engine_lane_mode(1));render();}if(format==2){CHECK(engine_rate_set(1,0,0,1));render();}
    for(int p=0;p<2;p++){
        for(int k=0;k<2;k++){
            proj.pat[p].drum[k].len=16;proj.pat[p].drum[k].rate=RATE_16;proj.pat[p].drum[k].accent[0]=0x9292u;
            for(int v=0;v<11;v++)proj.pat[p].drum[k].hit[v][0]=(uint32_t)(0x1111u ^ (v*73u+p*19u));
        }
        if(format)for(int l=0;l<25;l++)poly_set(proj.pat,(uint8_t *)&proj.sound,p,l,16,RATE_16);
        proj.pat[p].swing=40;
    }
    for(int t=0;t<5;t++)seq.ppat[t]=0;
    /* Other melodic parts are deliberately empty: no unrelated auto-mutation. */
    proj.set.notes_out=1;proj.variant.arrangement.flags|=ARR_NOTES_OUT;proj.set.bpm_x10=1200;proj.set.clk_out=0;
    seq.send_notes=1;seq.send_clock=0;seq.bpm=120;seq.mute=seq.vmute=0;seq.chain_a=seq.chain_b=0;seq.song_on=0;
    settle();seq.variation=(pattern_variation_t){(uint8_t)lane,0,1,3,42,3,0,0};
}
static void run_samples(unsigned samples)
{
    seq_sink_t sink=SINK;sink.drum=audit_drum;sink.midi=audit_midi;sink.midi_try=0;sink.midi_pair_try=0;sink.midi_batch_try=0;
    int feature=seq.lane_sound || (seq.arrangement.flags&ARR_ENABLED);
    while(samples){unsigned n=samples>256?256:samples;if(feature)seq_advance_arrangement(&seq,0,&sink);else seq_advance(&seq,0,&sink);
        while(n){unsigned k=feature?seq_until_event_arrangement(&seq,n):seq_until_event(&seq,n);elapsed+=k;if(feature)seq_advance_arrangement(&seq,k,&sink);else seq_advance(&seq,k,&sink);n-=k;samples-=k;}}
}
static void playback(int format,int lane,int options_case)
{
    prepare(format,lane);if(options_case){seq.variation.mode=1;seq.variation.options=5;}before=proj;int history=undo.cur;uint32_t fingerprint=project_fingerprint();memset(&observed,0,sizeof observed);elapsed=0;
    if(format==2&&lane==1)variation_snapshot("build/repeating-variation/before-play.bin");
    seq_start(&seq);run_samples(88200u*CYCLES-1u);
    unsigned total=0,selected_changes=0;
    for(int l=0;l<22;l++)for(int cycle=0;cycle<CYCLES;cycle++)for(int st=0;st<16;st++){
        int base=sm_get(before.pat[0].drum[l/11].hit[l%11],st),got=sm_get(observed.hits[l][cycle],st);
        int want=l==lane?pattern_variation_hit(&seq.variation,base,st,16,(unsigned)cycle):base;CHECK(got==want);total+=got;
        if(l==lane){selected_changes+=got!=base;if(cycle!=3&&cycle!=7)CHECK(got==base);}
        if(got){CHECK(observed.vel[l][cycle][st]==((0x9292u>>st&1u)?127:seq.accent_q7));}
        if(!options_case&&l==lane&&cycle==7)CHECK(got==sm_get(observed.hits[l][3],st));
    }
    CHECK(selected_changes>0);CHECK(observed.on==total&&observed.off==total);CHECK(!memcmp(&proj,&before,sizeof proj));CHECK(undo.cur==history);CHECK(project_fingerprint()==fingerprint);
    if(format==2){
        char path[100];snprintf(path,sizeof path,"build/repeating-variation/expected-%d-%d.json",lane,options_case);FILE *f=fopen(path,"w");CHECK(f!=0);fprintf(f,"{\"rows\":[");
        for(int l=0;l<22;l++){if(l)fputc(',',f);fputc('[',f);for(int cy=0;cy<CYCLES;cy++){if(cy)fputc(',',f);fprintf(f,"%u",observed.hits[l][cy][0]);}fputc(']',f);}fprintf(f,"]}\n");fclose(f);
    }
    /* Start resets phase; Continue preserves it. */
    seq_stop(&seq);run_samples(1);unsigned oldcycle=seq_lane_cycles(&seq,lane);seq_continue(&seq);run_samples(1);CHECK(seq_lane_cycles(&seq,lane)>=oldcycle&&seq_lane_cycles(&seq,lane)<=oldcycle+1);
    seq_stop(&seq);run_samples(1);seq_start(&seq);run_samples(1);CHECK(seq_lane_cycles(&seq,lane)==0);
    seq_stop(&seq);run_samples(1);CHECK(!memcmp(&proj,&before,sizeof proj));if(format==2&&lane==1)variation_snapshot("build/repeating-variation/after-play.bin");
}
static void polymeter(void)
{
    for(int format=1;format<=2;format++)for(int every=0;every<3;every++){
        int lane=17;prepare(format,lane);poly_set(proj.pat,(uint8_t *)&proj.sound,0,lane,7,RATE_8);seq.variation.every=(uint8_t)every;seq.variation.anchor=0;
        before=proj;memset(&observed,0,sizeof observed);elapsed=0;seq_start(&seq);run_samples(7u*11025u*CYCLES-1u);
        for(int cy=0;cy<CYCLES;cy++)for(int st=0;st<7;st++){
            int base=sm_get(before.pat[0].drum[1].hit[6],st);CHECK(sm_get(observed.hits[lane][cy],st)==pattern_variation_hit(&seq.variation,base,st,7,(unsigned)cy));
        }
        CHECK(!memcmp(&proj,&before,sizeof proj));seq_stop(&seq);run_samples(1);
    }
}
static void external_playback(void)
{
    for(int format=0;format<3;format++){
        int lane=7;prepare(format,lane);seq.ext=1;seq.ext_bpm=120;before=proj;memset(&observed,0,sizeof observed);elapsed=0;seq_start(&seq);
        for(int tick=0;tick<96*CYCLES;tick++){seq_ext_clock(&seq);run_samples(919);}
        for(int cy=0;cy<CYCLES;cy++)for(int st=0;st<16;st++){int base=sm_get(before.pat[0].drum[0].hit[7],st);CHECK(sm_get(observed.hits[lane][cy],st)==pattern_variation_hit(&seq.variation,base,st,16,(unsigned)cy));}
        CHECK(observed.on==observed.off);CHECK(!pending_on);CHECK(!memcmp(&proj,&before,sizeof proj));seq_stop(&seq);run_samples(1);
    }
}
static void menu_and_storage(void)
{
    prepare(2,255);ui.part=PART_909;ui.sel[0]=1;set_view(V_PART);button_tap(B_ARP);button_tap(B_ARP);CHECK(ui.view==V_GEN&&cur_page()==1&&pg.n==5);
    remember();for(int f=0;f<7;f++)pref_set(PR(R_VARIATION,0,0,f),f==0?1:f==1?3:f==2?3:f==3?42:0);unchanged(0);
    before=proj;int history=undo.cur;uint32_t off_sig=knobs_sig();button_tap(B_OCTUP);CHECK(ui.overlay==O_ASK);CHECK(strstr(ui.ask_q[0],"909 SD P1"));button_tap(B_HOME);CHECK(seq.variation.lane==255);CHECK(!memcmp(&proj,&before,sizeof proj));
    button_tap(B_OCTUP);CHECK(ui.overlay==O_ASK);ui_frame();png_write("build/repeating-variation/confirmation.png");button_tap(B_SEL);CHECK(seq.variation.lane==1&&seq.variation.pattern==0);CHECK(knobs_sig()!=off_sig);char tag[28];knob_tag(tag);CHECK(!strcmp(tag,"909 SD P1 LIVE"));CHECK(undo.cur==history);CHECK(!memcmp(&proj,&before,sizeof proj));
    ui.msg_until=0;ui_frame();png_write("build/repeating-variation/page.png");
    CHECK(project_save()==0);before=proj;fixture_store("build/repeating-variation/baseline.json");variation_snapshot("build/repeating-variation/armed.bin");CHECK(seq.variation.lane==1);
    /* The save payload is exactly the same with runtime overlay off. */
    button_tap(B_OCTDN);CHECK(ui.overlay==O_ASK);button_tap(B_SEL);CHECK(seq.variation.lane==255);CHECK(!memcmp(&proj,&before,sizeof proj));CHECK(project_save()==0);fixture_store("build/repeating-variation/off.json");variation_snapshot("build/repeating-variation/off.bin");
    button_tap(B_OCTUP);button_tap(B_SEL);CHECK(seq.variation.lane==1);boot();render();CHECK(seq.variation.lane==255);CHECK(!memcmp(&proj,&before,sizeof proj));
    for(int flag=0;flag<13;flag++){
        prepare(2,255);pattern_variation_t cfg=seq.variation;cfg.lane=1;
        if(flag==0)seq.playing=1;if(flag==1)seq.req_start=1;if(flag==2)seq.req_stop=1;if(flag==3)seq.req_cont=1;if(flag==4)seq.transport_stage=1;if(flag==5)seq.transport_hold=1;
        if(flag==6)ui.rec=1;if(flag==7)ui.keys=1;if(flag==8)external_edit=1;if(flag==9)save_wait=1;if(flag==10)save_hold=1;if(flag==11){perf.test=0;perf.phase=1;}if(flag==12)start_before_reserve=1;
        remember();CHECK(ui_variation_config(&cfg)==VR_BUSY);unchanged(flag!=12);
    }
    save_hold=0;prepare(2,255);ui.part=PART_808;ui.sel[1]=10;seq.ppat[1]=31;set_view(V_GEN);*page_of(V_GEN)=2;build_pages();
    seq.variation=(pattern_variation_t){255,0,2,8,255,3,2,7};button_tap(B_OCTUP);CHECK(ui.overlay==O_ASK);CHECK(strstr(ui.ask_q[0],"P32"));button_tap(B_SEL);CHECK(seq.variation.lane==21&&seq.variation.pattern==31&&seq.variation.seed==255&&seq.variation.options==7&&seq.variation.mode==2);
    prepare(2,255);pattern_variation_t cfg=seq.variation;cfg.lane=22;remember();CHECK(ui_variation_config(&cfg)==VR_INVALID);unchanged(1);
    prepare(2,255);engine_set(T_909,0,0,50);remember();cfg=seq.variation;cfg.lane=1;CHECK(ui_variation_config(&cfg)==VR_BUSY);unchanged(1);
    prepare(2,255);ui.part=0;ui.sel[0]=1;set_view(V_GEN);*page_of(V_GEN)=1;build_pages();button_tap(B_OCTUP);seq.transport_epoch++;button_tap(B_SEL);CHECK(strstr(ui.msg,"CHANGED"));CHECK(seq.variation.lane==255);
    button_tap(B_OCTUP);proj.pat[2].swing++;button_tap(B_SEL);CHECK(strstr(ui.msg,"CHANGED"));CHECK(seq.variation.lane==255);
}
static void cue_priority(void)
{
    for(int format=0;format<3;format++){
        prepare(format,1);before=proj;memset(&observed,0,sizeof observed);elapsed=0;seq_start(&seq);run_samples(1000);seq_cue_part(&seq,0,1);run_samples(88200);CHECK(seq.ppat[0]==1);run_samples(88200u*CYCLES);
        for(int cy=1;cy<CYCLES;cy++)for(int st=0;st<16;st++)CHECK(sm_get(observed.hits[1][cy],st)==sm_get(before.pat[1].drum[0].hit[1],st));
        CHECK(!memcmp(&proj,&before,sizeof proj));seq_stop(&seq);run_samples(1);
    }
}
int main(void)
{
    properties();for(int f=0;f<3;f++)for(int lane=0;lane<22;lane++)playback(f,lane,0);playback(2,1,1);polymeter();external_playback();cue_priority();menu_and_storage();
    printf("REPEATING VARIATION PASS: %d checkpoints;66 format/lane playback cases; normal repeats/every4th variation, internal/MIDI accepted-event parity, Start/Continue, cue priority, whole-project/query/storage/Undo isolation and stopped menu/refusals.\n",checks);return 0;
}
