/* SPDX-License-Identifier: GPL-3.0-only */
#define PATTERN_QUERY_MAIN parent_query_main
#include "pattern_query_test.c"
#undef PATTERN_QUERY_MAIN
typedef struct { unsigned tick,cycle;int bass,step,note,accent,slide; } event_t;
static event_t events[2048],base_events[2048];
static unsigned nevents,elapsed,ons,offs;static int held[16][128],reject_step=-1;
static int audit_bass(void *ctx,int b,int note,int accent,int slide)
{
    int p=seq.t[TRK_BASS0+b].pos;unsigned cy=seq.t[TRK_BASS0+b].variation_cycle;
    if(p==reject_step)return 0;
    int accepted=s_bon_checked(ctx,b,note,accent,slide);CHECK(accepted);
    CHECK(nevents<2048);events[nevents++]=(event_t){elapsed,cy,b,p,note,accent,slide};return accepted;
}
static void audit_midi(void *ctx,uint8_t status,uint8_t note,uint8_t vel)
{
    (void)ctx;int ch=status&15;
    if((status&240)==144 && vel){CHECK(!held[ch][note]);held[ch][note]=1;ons++;}
    if((status&240)==128 || ((status&240)==144 && !vel)){CHECK(held[ch][note]);held[ch][note]=0;offs++;}
}
static void run_samples(unsigned n)
{
    seq_sink_t sink=SINK;sink.bass_trigger=audit_bass;sink.midi=audit_midi;sink.midi_try=0;sink.midi_pair_try=0;sink.midi_batch_try=0;
    int feature=seq.lane_sound || (seq.arrangement.flags&ARR_ENABLED);
    if(feature)seq_advance_arrangement(&seq,0,&sink);else seq_advance(&seq,0,&sink);
    while(n){unsigned k=feature?seq_until_event_arrangement(&seq,n):seq_until_event(&seq,n);elapsed+=k;n-=k;
        if(feature)seq_advance_arrangement(&seq,k,&sink);else seq_advance(&seq,k,&sink);}
}
static void setup(int format,int bass,int mode,int pitch_mode,int direction,int articulation)
{
    save_hold=0;fresh();nevents=elapsed=ons=offs=0;reject_step=-1;memset(held,0,sizeof held);
    for(int p=0;p<NPAT;p++)pattern_init(&proj.pat[p],123,456);
    if(format){CHECK(engine_lane_mode(1));render();}if(format==2){CHECK(engine_rate_set(1,0,0,1));render();}
    proj.arr.song.len=1;memset(&proj.arr.song.bar[0],0,sizeof(song_bar_t));
    arrangement_settings_t *a=&proj.variant.arrangement;a->key=0;a->mode=HARMONY_IONIAN;a->parent=HARMONY_NO_PARENT;a->degree[0]=1;
    for(int b=0;b<2;b++){
        bpart_t *bp=&proj.pat[0].bass[b];bp->len=16;bp->rate=RATE_16;bp->dir=(uint8_t)direction;bp->transpose=24;
        bp->gen.root=0;bp->gen.scale=5;bp->gen.base_oct=3;bp->gen.oct_range=2;bp->gen.mutate_bars=0;
        for(int st=0;st<16;st++)bp->step[st]=(bstep_t){(uint8_t)(60+(st%3==1?4:st%3==2?7:0)),(uint8_t)(G_NOTE|((st%4==1)?BS_ACCENT:0))};
        if(articulation){bp->step[0].flags|=BS_SLIDE;bp->step[2].flags=G_TIE;bp->step[3].flags=G_REST;bp->step[7].flags=G_TIE;}
    }
    if(format)for(int l=0;l<25;l++)poly_set(proj.pat,(uint8_t *)&proj.sound,0,l,16,RATE_16);
    proj.pat[0].swing=40;proj.set.notes_out=1;a->flags|=ARR_NOTES_OUT;proj.set.bpm_x10=1200;proj.set.clk_out=0;
    for(int p=0;p<5;p++)seq.ppat[p]=0;
    seq.midi_ready=1;seq.ch[2]=0;seq.ch[3]=1;seq.send_notes=1;seq.send_clock=0;seq.bpm=120;seq.mute=seq.vmute=0;seq.chain_a=seq.chain_b=0;seq.song_on=0;
    settle();ui.part=(uint8_t)(PART_303A+bass);ui.gen_mode[bass]=(uint8_t)mode;ui.song_sel=0;
    set_view(V_GEN);*page_of(V_GEN)=3;build_pages();CHECK(pg.n==6);
    seq.variation=(pattern_variation_t){255,0,0,8,42,1,(uint8_t)pitch_mode,0,0,0,0};
}
static void arm(void){button_tap(B_OCTUP);CHECK(ui.overlay==O_ASK);button_tap(B_SEL);CHECK(seq.variation.lane==22+bidx());}
static void snapshot303(const char *name){FILE *f=fopen(name,"wb");CHECK(f!=0);pattern(f,0);pattern(f,1);fclose(f);}
static unsigned play303(void)
{
    nevents=elapsed=ons=offs=0;memset(held,0,sizeof held);seq_start(&seq);run_samples(88200u*8u-1u);seq_stop(&seq);run_samples(1);
    CHECK(ons>20);CHECK(ons==offs);for(int c=0;c<16;c++)for(int n=0;n<128;n++)CHECK(!held[c][n]);return nevents;
}
static void playback(int format,int bass,int mode,int pm,int direction,int articulation)
{
    setup(format,bass,mode,pm,direction,articulation);unsigned count=play303();memcpy(base_events,events,sizeof events);
    /* Start resets sequence RNG and local pass counters. Same base events/timing. */
    setup(format,bass,mode,pm,direction,articulation);arm();before=proj;int history=undo.cur;uint32_t fp=project_fingerprint();
    unsigned writes_before=store_writes;CHECK(play303()==count);unsigned changes=0;
    const unsigned expected=mode==GEN_CHORD || mode==GEN_BOTH?0x91u:0xab5u;
    CHECK(seq.variation.notes==expected);
    for(unsigned i=0;i<count;i++){
        event_t *a=&base_events[i],*v=&events[i];CHECK(a->tick==v->tick&&a->cycle==v->cycle&&a->bass==v->bass&&a->step==v->step&&a->accent==v->accent&&a->slide==v->slide);
        if(a->note!=v->note){CHECK(v->bass==bass);CHECK(v->cycle%2==1);CHECK(v->step%4!=0);CHECK(v->note>=48&&v->note<=71);CHECK(expected&(1u<<(v->note%12)));CHECK(abs(v->note-a->note)<=12);if(pm==1)CHECK(v->note>a->note);if(pm==2)CHECK(v->note<a->note);changes++;}
        if(a->bass!=bass || !(a->cycle%2) || !(a->step%4))CHECK(v->note==a->note);
    }
    if(!changes)fprintf(stderr,"no changes format%d bass%d mode%d pitch%d direction%d art%d mask%u low%u high%u count%u firstnote%d cycles%u/%u\n",format,bass,mode,pm,direction,articulation,seq.variation.notes,seq.variation.low,seq.variation.high,count,events[0].note,events[0].cycle,events[count-1].cycle);
    CHECK(changes>0);CHECK(!memcmp(&proj,&before,sizeof proj));CHECK(undo.cur==history);CHECK(project_fingerprint()==fp);CHECK(store_writes==writes_before);
    if(format==2&&direction==DIR_FWD&&!articulation){
        char path[120];snprintf(path,sizeof path,"build/303-variation/events-%d-%d-%d.json",bass,mode,pm);FILE *f=fopen(path,"w");CHECK(f!=0);fprintf(f,"{\"events\":[");
        for(unsigned i=0;i<count;i++){event_t *v=&events[i];fprintf(f,"%s[%u,%u,%d,%d,%d,%d,%d]",i?",":"",v->tick,v->cycle,v->bass,v->step,v->note,v->accent,v->slide);}fprintf(f,"]}\n");fclose(f);
    }
}
static void menus_and_storage(void)
{
    for(int b=0;b<2;b++)for(int m=0;m<4;m++){
        setup(2,b,m,0,DIR_FWD,0);CHECK(project_save()==0);before=proj;char path[100];snprintf(path,sizeof path,"build/303-variation/baseline-%d-%d.json",b,m);fixture_store(path);
        uint32_t sig=knobs_sig();button_tap(B_OCTUP);CHECK(ui.overlay==O_ASK);button_tap(B_HOME);CHECK(seq.variation.lane==255);CHECK(!memcmp(&proj,&before,sizeof proj));
        arm();CHECK(knobs_sig()!=sig);char tag[28];knob_tag(tag);CHECK(strstr(tag,b?"303B P1 LIVE":"303A P1 LIVE"));
        if(!b&&!m){ui.msg_until=0;ui_frame();png_write("build/303-variation/page.png");snapshot303("build/303-variation/before.bin");}
        CHECK(project_save()==0);CHECK(!memcmp(&proj,&before,sizeof proj));snprintf(path,sizeof path,"build/303-variation/armed-%d-%d.json",b,m);fixture_store(path);
        play303();CHECK(!memcmp(&proj,&before,sizeof proj));
        if(!b&&!m)snapshot303("build/303-variation/after.bin");
        button_tap(B_OCTDN);CHECK(ui.overlay==O_ASK);button_tap(B_SEL);CHECK(seq.variation.lane==255);CHECK(project_save()==0);
        CHECK(!memcmp(&proj,&before,sizeof proj));snprintf(path,sizeof path,"build/303-variation/off-%d-%d.json",b,m);fixture_store(path);
        arm();boot();render();CHECK(seq.variation.lane==255);CHECK(!memcmp(&proj,&before,sizeof proj));
    }
}
static void refusals_and_boundaries(void)
{
    setup(2,0,GEN_FREE,0,DIR_FWD,0);before=proj;button_tap(B_OCTUP);CHECK(ui.overlay!=O_ASK&&seq.variation.lane==255);CHECK(strstr(ui.msg,"SCALE OR CHORD"));CHECK(!memcmp(&proj,&before,sizeof proj));
    setup(2,0,GEN_CHORD,0,DIR_FWD,0);proj.variant.arrangement.degree[0]=0;button_tap(B_OCTUP);button_tap(B_SEL);CHECK(seq.variation.lane==255);CHECK(strstr(ui.msg,"NO CHORD"));
    setup(2,0,GEN_BOTH,0,DIR_FWD,0);proj.variant.arrangement.key=2;proj.pat[0].bass[0].gen.scale=3;before=proj;button_tap(B_OCTUP);button_tap(B_SEL);CHECK(seq.variation.lane==255);CHECK(strstr(ui.msg,"SHARED NOTES"));CHECK(!memcmp(&proj,&before,sizeof proj));
    setup(2,0,GEN_SCALE,0,DIR_FWD,0);proj.pat[0].bass[0].gen.mutate_bars=1;button_tap(B_OCTUP);button_tap(B_SEL);CHECK(seq.variation.lane==255);CHECK(strstr(ui.msg,"AUTO MUTATE"));
    setup(2,0,GEN_SCALE,0,DIR_FWD,0);button_tap(B_OCTUP);ui.gen_mode[0]=GEN_KEY;button_tap(B_SEL);CHECK(seq.variation.lane==255);CHECK(strstr(ui.msg,"CHANGED"));
    setup(2,0,GEN_SCALE,0,DIR_FWD,0);arm();pattern_variation_t cfg=seq.variation;seq.playing=1;remember();CHECK(ui_variation_config(&cfg)==VR_BUSY);unchanged(1);seq.playing=0;
    /* Selected source parameters cannot introduce base mutation while armed. */
    proj.pat[0].bass[0].gen.mutate_bars=1;before=proj;play303();CHECK(!memcmp(&proj,&before,sizeof proj));
    /* Captured constraints stay captured when the project key changes. */
    setup(2,0,GEN_KEY,0,DIR_FWD,0);arm();proj.variant.arrangement.key=1;CHECK(seq.variation.notes==0xab5u);before=proj;play303();CHECK(!memcmp(&proj,&before,sizeof proj));
    /* Probability-rejected NOTE cannot be revived by its following TIE. */
    setup(2,0,GEN_SCALE,0,DIR_FWD,1);arm();reject_step=1;play303();for(unsigned i=0;i<nevents;i++)CHECK(events[i].step!=1&&events[i].step!=2);
}
static void external_and_reconcile(void)
{
    for(int format=0;format<3;format++)for(int b=0;b<2;b++){
        setup(format,b,GEN_CHORD,0,DIR_FWD,1);arm();seq.ext=1;seq.ext_bpm=120;before=proj;seq_start(&seq);
        for(int tick=0;tick<96*8;tick++){seq_ext_clock(&seq);run_samples(919);}
        seq_stop(&seq);run_samples(1);CHECK(ons>20&&ons==offs);unsigned changed=0;
        for(unsigned i=0;i<nevents;i++)if(events[i].bass==b){event_t *e=&events[i];int base=60+(e->step%3==1?4:e->step%3==2?7:0);
            if(e->note!=base){CHECK(e->cycle%2==1&&e->step%4);CHECK(0x91u&(1u<<(e->note%12)));changed++;}}
        CHECK(changed>0);CHECK(!memcmp(&proj,&before,sizeof proj));
    }
    for(int b=0;b<2;b++){
        setup(2,b,GEN_KEY,0,DIR_FWD,0);arm();seq_start(&seq);
        for(unsigned k=0;k<2000 && !(seq.t[2+b].sounding && (seq.t[2+b].source_note&128));k++)run_samples(256);
        CHECK(seq.t[2+b].sounding && (seq.t[2+b].source_note&128));
        seq_sink_t sink=SINK;sink.midi=audit_midi;sink.midi_try=0;sink.midi_pair_try=0;sink.midi_batch_try=0;
        int pitch=seq.t[2+b].held_note&127;uint8_t source=seq.t[2+b].source_note;before=proj;
        seq_cc_reconcile(&seq,&sink);CHECK((seq.t[2+b].held_note&127)==pitch);CHECK(seq.t[2+b].source_note==source);CHECK(!memcmp(&proj,&before,sizeof proj));
        /* Deliberate live-follow edit: a held projected source must not escape its snapshot. */
        proj.variant.arrangement.key=1;proj.variant.arrangement.bass_low[b]=proj.variant.arrangement.bass_high[b]=61;
        seq.arrangement.flags=(uint8_t)(BASS_FOLLOW_SCALE<<(1+2*b));seq.arrangement.intent=&proj.variant.arrangement;before=proj;
        seq_cc_reconcile(&seq,&sink);CHECK(!seq.t[2+b].sounding);CHECK(!memcmp(&proj,&before,sizeof proj));
        seq_stop(&seq);run_samples(1);CHECK(ons==offs);
        setup(2,b,GEN_KEY,0,DIR_FWD,0);arm();seq.t[2+b].variation_cycle=255;seq.t[2+b].variation_step=16;
        seq.t[2+b].started=1;seq.t[2+b].pos=15;seq_continue(&seq);run_samples(1);CHECK(seq.t[2+b].variation_cycle==0);
        seq_stop(&seq);run_samples(1);
    }
}
static void context_and_phase(void)
{
    for(int b=0;b<2;b++)for(int follow=BASS_FOLLOW_ROOT;follow<=BASS_FOLLOW_SCALE;follow++){
        setup(2,b,GEN_KEY,0,DIR_FWD,1);arm();
        proj.variant.arrangement.flags|=(uint8_t)(ARR_ENABLED|(follow<<(1+2*b)));
        proj.variant.arrangement.bass_low[b]=48;proj.variant.arrangement.bass_high[b]=71;
        proj.variant.arrangement.degree[0]=5;settle();before=proj;
        seq.arrangement.flags=proj.variant.arrangement.flags;seq.arrangement.intent=&proj.variant.arrangement;seq.arrangement.key=0;seq.arrangement.mode=HARMONY_IONIAN;seq.arrangement.parent=HARMONY_NO_PARENT;seq.arrangement.degree=5;
        play303();unsigned legal=follow==BASS_FOLLOW_ROOT?(1u<<7):follow==BASS_FOLLOW_CHORD?((1u<<7)|(1u<<11)|(1u<<2)):0xab5u;
        for(unsigned i=0;i<nevents;i++)if(events[i].bass==b)CHECK(legal&(1u<<(events[i].note%12)));
        CHECK(!memcmp(&proj,&before,sizeof proj));
    }
    for(int b=0;b<2;b++)for(int transpose=0;transpose<=48;transpose+=24){
        setup(2,b,GEN_SCALE,0,DIR_FWD,0);proj.pat[0].bass[b].transpose=(uint8_t)transpose;arm();before=proj;play303();
        for(unsigned i=0;i<nevents;i++)if(events[i].bass==b&&events[i].cycle%2&&events[i].step%4){
            int n=events[i].note;CHECK((0xab5u&(1u<<(n%12)))!=0);CHECK(n>=0&&n<128);
        }
        CHECK(!memcmp(&proj,&before,sizeof proj));
    }
    /* A singleton range with no alternative keeps the base event; never invent rests. */
    setup(2,0,GEN_SCALE,0,DIR_FWD,0);for(int st=0;st<16;st++)proj.pat[0].bass[0].step[st].note=60;arm();
    seq.variation.notes=1;seq.variation.low=seq.variation.high=60;before=proj;play303();
    for(unsigned i=0;i<nevents;i++)if(!events[i].bass)CHECK(events[i].note==60);CHECK(!memcmp(&proj,&before,sizeof proj));
    for(int bad=0;bad<4;bad++){
        pattern_variation_t v=seq.variation;if(bad==0)v.notes=0;if(bad==1)v.notes=0x8000;if(bad==2)v.high=128;if(bad==3)v.low=61;
        remember();CHECK(ui_variation_config(&v)==VR_INVALID);unchanged(1);
    }
    /* Seven-step/eighth-note local passes, different directions, evolving last4 fills. */
    for(int b=0;b<2;b++)for(int d=0;d<NDIRS;d++){
        setup(2,b,GEN_KEY,0,d,0);poly_set(proj.pat,(uint8_t *)&proj.sound,0,22+b,7,RATE_8);arm();
        seq.variation.every=1;seq.variation.options=3;seq.variation.anchor=0;before=proj;play303();unsigned changed=0;
        for(unsigned i=0;i<nevents;i++)if(events[i].bass==b){event_t *e=&events[i];int base=60+(e->step%3==1?4:e->step%3==2?7:0);
            if(e->note!=base){CHECK(e->cycle%4==3);CHECK(e->step>=3&&e->step<7);changed++;}}
        CHECK(changed>0);CHECK(!memcmp(&proj,&before,sizeof proj));
        unsigned cy=seq.t[2+b].variation_cycle;seq_continue(&seq);run_samples(1);CHECK(seq.t[2+b].variation_cycle==cy||seq.t[2+b].variation_cycle==(uint8_t)(cy+1));
        seq_stop(&seq);run_samples(1);seq_start(&seq);run_samples(1);CHECK(seq.t[2+b].variation_cycle==0);seq_stop(&seq);run_samples(1);
        seq_cue_part(&seq,2+b,1);seq_start(&seq);run_samples(88200u*2u);CHECK(seq.ppat[2+b]==1);CHECK(seq.t[2+b].variation_cycle<3);seq_stop(&seq);run_samples(1);
        CHECK(!memcmp(&proj,&before,sizeof proj));
    }
}
#ifndef VARIATION303_NO_MAIN
int main(void)
{
    for(int f=0;f<3;f++)for(int b=0;b<2;b++)for(int m=0;m<4;m++)for(int pm=0;pm<3;pm++)for(int art=0;art<2;art++)playback(f,b,m,pm,DIR_FWD,art);
    for(int d=DIR_REV;d<NDIRS;d++)for(int b=0;b<2;b++)playback(2,b,GEN_CHORD,0,d,1);
    menus_and_storage();refusals_and_boundaries();context_and_phase();external_and_reconcile();
    printf("303 VARIATION PASS: %d checkpoints;150 playback cases; both303/3 formats/4 constraints/3 pitch modes; pitch-only timing/articulation/MIDI ownership/base/query/save/Undo invariants, native menus and refusals.\n",checks);return 0;
}

#endif
