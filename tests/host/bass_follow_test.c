/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual shared sequencer, bass303, application, canvas staging and storage. */
#define _POSIX_C_SOURCE 200809L
#define PLAT_CONTROL_TEST
#define plat_midi_out host_midi_out
#define plat_midi_pair host_midi_pair
#define plat_midi_batch host_midi_batch
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#undef plat_midi_out
#undef plat_midi_pair
#undef plat_midi_batch
#include <assert.h>
#include "../../firmware/src/midi_ring.h"
static uint32_t trace[8192];static int trace_n,use_real_ring;
static uint32_t out_ring[64];static volatile uint32_t out_write,out_read;
static void accepted_packet(uint32_t packet) {(void)host_midi_out(packet);if(trace_n<8192)trace[trace_n++]=packet;}
int plat_midi_out(uint32_t packet) {
    if(!midi_connected)return 0;
    if(use_real_ring && !midi_ring_try_put(out_ring,&out_write,&out_read,64,packet,midi_connected))return 0;
    accepted_packet(packet);return 1;
}
int plat_midi_pair(uint32_t first,uint32_t second) {
    if(!midi_connected)return 0;
    if(use_real_ring && !midi_ring_try_pair(out_ring,&out_write,&out_read,64,first,second,midi_connected))return 0;
    accepted_packet(first);accepted_packet(second);return 1;
}
int plat_midi_batch(const uint32_t *packets,uint32_t count);
#include "../../firmware/src/app/engine.c"
int plat_midi_batch(const uint32_t *packets,uint32_t count) {
    uint32_t i;
    if(!midi_connected || !count || count>6)return 0;
    if(use_real_ring && !midi_ring_try_batch(out_ring,&out_write,&out_read,64,packets,count,midi_connected))return 0;
    for(i=0;i<count;i++)accepted_packet(packets[i]);
    return 1;
}
static int locked,irq_start;
uint32_t plat_control_enter(void) {
    assert(!locked);
    if(irq_start) {irq_start=0;seq_start(&seq);seq_advance_arrangement(&seq,0,&SINK);assert(seq.playing);}
    locked=1;return 19;
}
void plat_control_leave(uint32_t saved) {assert(locked && saved==19);locked=0;}
static void render_block(void) {int32_t b[512];engine_render(b,256);}
static void fresh(void) {
    int i;use_real_ring=0;out_write=out_read=0;for(i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;
    memset(stored_known,0,sizeof stored_known);midi_connected=1;project_discard_rejected();boot();render_block();trace_n=0;
}
static int command(int op,int target,int voice,int index,int value) {
    variant_command_t c={1,(uint16_t)op,90,(uint8_t)target,(uint8_t)voice,(uint8_t)index,0,value};variant_reply_t r;
    int rc=variant_command(&c,&r);assert(r.result==rc && r.request==90);return rc;
}
static void app_config(void) {
    assert(command(VC_SET_ARR_LENGTH,0,0,0,2)==VR_QUEUED);
    assert(command(VC_SET_BLOCK,0,1<<3,0,0)==VR_QUEUED);
    assert(command(VC_SET_BLOCK,1,4<<3,0,0)==VR_QUEUED);
    assert(command(VC_SET_BASS_FOLLOW,0,0,0,1)==VR_QUEUED);
    assert(command(VC_SET_BASS_FOLLOW,1,0,0,2)==VR_QUEUED);
    assert(command(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_QUEUED);render_block();
}
static uint32_t ui_revision(void) { variant_snapshot_t v;assert(variant_snapshot(VS_INFO,0,0,&v)==VR_OK);return v.revision; }
static void storage_commands(void) {
    project_t before;undo_t journal;uint8_t shadow[sizeof undo_shadow];uint32_t packed,writes,revision,queued;
    variant_snapshot_t view;
    fresh();assert(sizeof(variant_settings_t)==412 && sizeof(arrangement_settings_t)==204);app_config();
    assert(proj.variant.schema==5 && proj.variant.arrangement.flags==19);
    assert(command(VC_SET_ARR_GLOBAL,0,0,0,0)==VR_QUEUED && proj.variant.arrangement.flags==18);
    assert(command(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_QUEUED && proj.variant.arrangement.flags==19);
    assert(variant_snapshot(VS_INFO,0,0,&view)==VR_OK && view.value[0]==(1535u|2048u) && view.value[5]==5);
    assert(variant_snapshot(VS_BASS_FOLLOW,0,0,&view)==VR_OK && view.count==10 && view.value[0]==1 && view.value[5]==255 && view.value[6]==255 && view.value[7]==255);
    assert(variant_snapshot(VS_BASS_FOLLOW,2,0,&view)==VR_INVALID);
    before=proj;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);revision=ui_revision();queued=engine_queue_written();
    int bad[][4]={{2,0,0,1},{0,1,0,1},{0,0,3,1},{0,0,0,3},{0,0,1,-1},{0,0,2,128},{0,0,1,73},{0,0,2,23}};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++)assert(command(13,bad[i][0],bad[i][1],bad[i][2],bad[i][3])==VR_INVALID);
    variant_command_t reserved={1,13,91,0,0,0,1,1};variant_reply_t reply;
    assert(variant_command(&reserved,&reply)==VR_INVALID);
    assert(command(9,0,0,0,19)==VR_INVALID); /* legacy checkbox cannot write full packed flags */
    assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo) && !memcmp(shadow,undo_shadow,sizeof shadow));
    assert(ui_revision()==revision && engine_queue_written()==queued);
    render_block();while(engine_strip_set(0,VP_BOOST,0)){};queued=engine_queue_written();
    assert(command(13,0,0,0,2)==VR_BUSY && !memcmp(&before,&proj,sizeof proj) && queued==engine_queue_written());render_block();
    seq.req_start=1;assert(command(13,0,0,0,2)==VR_BUSY);seq.req_start=0;
    seq.req_cont=1;assert(command(13,0,0,1,12)==VR_BUSY);seq.req_cont=0;
    seq.req_stop=1;assert(command(13,1,0,2,100)==VR_BUSY);seq.req_stop=0;
    irq_start=1;assert(command(13,0,0,0,2)==VR_BUSY && !memcmp(&before,&proj,sizeof proj));seq_stop(&seq);render_block();
    assert(command(VC_SAVE,0,0,0,0)==VR_OK);project_defaults();assert(project_load()==0);render_block();
    assert(proj.variant.schema==5 && proj.variant.arrangement.flags==19);
    /* Independently encode old schema2/format4 with its old flags contract. */
    proj.variant.schema=2;proj.variant.arrangement.flags=1;proj.variant.strip[22].boost_db=7;
    assert(stream_measure(RAW_LEN,RAW_OFF,&packed)==0 && save_stream(RAW_LEN,RAW_OFF,4,packed)==0);
    assert(project_load()==0 && proj.variant.schema==5 && proj.variant.arrangement.flags==1 && proj.variant.strip[22].boost_db==7);
    /* Old raw schema1 prefix still appends default Off ranges, without rewriting its notes. */
    proj.variant.schema=1;proj.variant.bytes=208;assert(stream_measure(SCHEMA1_RAW_LEN,RAW_OFF,&packed)==0 && save_stream(SCHEMA1_RAW_LEN,RAW_OFF,3,packed)==0);
    assert(project_load()==0 && proj.variant.schema==5 && proj.variant.arrangement.flags==0 && proj.variant.arrangement.bass_low[0]==24);
    fresh();assert(project_save()==0 && get32(store[OBJ_PAT0])==2); /* canonical default stays legacy byte format */
    for(int kind=0;kind<9;kind++) {
        fresh();app_config();before=proj;
        if(kind==0)proj.variant.arrangement.flags|=64;
        if(kind==1)proj.variant.arrangement.flags=(uint8_t)((19&~6)|6);
        if(kind==2)proj.variant.arrangement.flags=(uint8_t)((19&~24)|24);
        if(kind==3)proj.variant.arrangement.bass_high[0]=128;
        if(kind==4)proj.variant.arrangement.bass_low[1]=73;
        if(kind==5)proj.variant.schema=6;
        if(kind==6){proj.variant.schema=2;proj.variant.arrangement.flags=3;} /* invalid old flags cannot masquerade as migration */
        if(kind==7)proj.variant.bytes=411;
        if(kind==8){proj.variant.schema=3;proj.variant.arrangement.flags|=32;} /* old schema cannot smuggle the new output bit */
        assert(stream_measure(RAW_LEN,RAW_OFF,&packed)==0 && save_stream(RAW_LEN,RAW_OFF,4,packed)==0);proj=before;
        journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);writes=store_writes;revision=ui_revision();
        seq.bass_notes_out[0]=0;seq.bass_channels_out[0]=7;trace_n=0;
        assert(project_load()==-8 && project_write_protected());
        assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo) && !memcmp(shadow,undo_shadow,sizeof shadow));
        assert(writes==store_writes && ui_revision()==revision && !seq.transport_hold && !seq.transport_stage);
        assert(seq.bass_notes_out[0]==0 && seq.bass_channels_out[0]==7 && trace_n==0); /* rejected load retains actual ownership */
    }
    puts("bass storage: same412B schema5, explicit schema2/schema1/default migrations, bounded/IRQ-safe stopped commands, queue pressure and9 CRC-valid protected rejection cases PASS");
}
static pattern_t ptn[NPAT];static song_t song_fixture;static arrangement_settings_t intent;static seq_t player;
static int actual[2],ons[2],offs[2],last_accent[2],last_slide[2],retunes[2],reject_trigger;
static uint8_t note_trace[4096][3];static int note_trace_n;
static void played(void*x,int b,int note,int accent,int slide) {(void)x;actual[b]=note;ons[b]++;last_accent[b]=accent;last_slide[b]=slide;}
static void stopped(void*x,int b) {(void)x;offs[b]++;actual[b]=255;}
static void retuned(void*x,int b,int note,int accent) {played(x,b,note,accent,1);retunes[b]++;}
static int checked(void*x,int b,int note,int accent,int slide) {if(reject_trigger)return 0;played(x,b,note,accent,slide);return 1;}
static void emitted(void*x,uint8_t st,uint8_t n,uint8_t v) {(void)x;if(note_trace_n<4096){note_trace[note_trace_n][0]=st;note_trace[note_trace_n][1]=n;note_trace[note_trace_n++][2]=v;}}
static const seq_sink_t TEST_SINK={.bass_on=played,.bass_off=stopped,.midi=emitted,.bass_retune=retuned,.bass_trigger=checked};
static void player_fresh(int mode,int key,int degree,int a,int b,int low,int high,int transpose) {
    memset(ptn,0,sizeof ptn);memset(&song_fixture,0,sizeof song_fixture);arrangement_defaults(&intent);
    for(int i=0;i<NPAT;i++) {for(int k=0;k<2;k++){ptn[i].drum[k].len=16;ptn[i].bass[k].len=1;ptn[i].bass[k].transpose=(uint8_t)(24+transpose);ptn[i].bass[k].step[0]=(bstep_t){64,G_NOTE|BS_SLIDE|BS_ACCENT};}}
    song_fixture.len=2;intent.flags=(uint8_t)(1|(a<<1)|(b<<3));intent.key=(uint8_t)key;intent.mode=(uint8_t)mode;
    intent.parent=mode==7?HARMONY_IONIAN:mode==8?HARMONY_AEOLIAN:255;intent.degree[0]=intent.degree[1]=(uint8_t)degree;
    intent.bass_low[0]=intent.bass_low[1]=(uint8_t)low;intent.bass_high[0]=intent.bass_high[1]=(uint8_t)high;
    seq_init(&player,ptn);player.song=&song_fixture;player.send_clock=0;player.send_notes=1;seq_arrangement_apply(&player,&intent);
    memset(ons,0,sizeof ons);memset(offs,0,sizeof offs);memset(retunes,0,sizeof retunes);actual[0]=actual[1]=255;note_trace_n=0;reject_trigger=0;
}
static const uint8_t scales[7][7]={{0,2,4,5,7,9,11},{0,2,3,5,7,9,10},{0,1,3,5,7,8,10},{0,2,4,6,7,9,11},{0,2,4,5,7,9,10},{0,2,3,5,7,8,10},{0,1,3,5,6,8,10}};
static int permitted(int note,int mode,int key,int degree,int follow) {
    int parent=mode==7?0:mode==8?5:mode;
    for(int j=0;j<(follow==1?1:3);j++)if(note%12==(key+scales[parent][(degree-1+2*j)%7])%12)return 1;
    return 0;
}
static void matrix(void) {
    const int ranges[][2]={{0,127},{24,72},{48,60},{61,61}};unsigned cases=0;
    for(int mode=0;mode<9;mode++)for(int key=0;key<12;key++)for(int degree=1;degree<=7;degree++)for(int a=0;a<3;a++)for(int b=0;b<3;b++)for(int r=0;r<4;r++)for(int transpose=-24;transpose<=24;transpose+=24) {
        player_fresh(mode,key,degree,a,b,ranges[r][0],ranges[r][1],transpose);seq_start(&player);seq_advance_arrangement(&player,0,&TEST_SINK);
        for(int part=0;part<2;part++) {
            int follow=part?b:a,exists=0;for(int n=ranges[r][0];n<=ranges[r][1];n++)if(permitted(n,mode,key,degree,follow))exists=1;
            if(!follow){assert(actual[part]==64+transpose && ons[part]==1);}
            else if(exists){assert(ons[part]==1 && actual[part]>=ranges[r][0] && actual[part]<=ranges[r][1] && permitted(actual[part],mode,key,degree,follow));}
            else assert(ons[part]==0 && !player.t[TRK_BASS0+part].sounding && player.bass_notes_out[part]==255);
            if(ons[part])assert(player.bass_notes_out[part]==actual[part] && player.bass_channels_out[part]==player.ch[TRK_BASS0+part]);
        }
        cases++;
    }
    /* Disabled arrangement and explicit no-chord are exact absolute legacy pitch, ignoring follow range. */
    for(int degree=0;degree<2;degree++) {player_fresh(0,0,degree,2,2,0,0,24);if(degree)player.arrangement.flags&=(uint8_t)~1;seq_start(&player);if(degree)seq_advance(&player,0,&TEST_SINK);else seq_advance_arrangement(&player,0,&TEST_SINK);assert(actual[0]==88 && actual[1]==88);}
    puts("bass playback:81648 cases of9 A/B follow pairs x9 modes x12 keys x7 degrees x4 ranges x3 transposes, final synth/MIDI pitches and no-chord/disabled legacy absolute PASS");assert(cases==81648);
}
static void tie_and_midi(void) {
    player_fresh(0,0,1,2,1,0,127,0);ptn[0].bass[0].len=3;ptn[0].bass[0].step[0]=(bstep_t){64,G_NOTE|BS_ACCENT};ptn[0].bass[0].step[1]=(bstep_t){99,G_TIE};ptn[0].bass[0].step[2]=(bstep_t){65,G_NOTE};
    seq_start(&player);seq_advance_arrangement(&player,0,&TEST_SINK);int held=actual[0];
    player.t[TRK_BASS0].to_next=0;seq_advance_arrangement(&player,0,&TEST_SINK);
    assert(actual[0]==held && (player.t[TRK_BASS0].held_note&127)==held && ons[0]==1); /* TIE payload99 is never played */
    player.t[TRK_BASS0].to_next=0;seq_advance_arrangement(&player,0,&TEST_SINK);assert(last_slide[0]==0);
    player_fresh(0,0,1,1,1,61,61,0);ptn[0].bass[0].len=2;ptn[0].bass[0].step[1]=(bstep_t){61,G_TIE};
    seq_start(&player);seq_advance_arrangement(&player,0,&TEST_SINK);assert(!ons[0]);
    player.arrangement.degree=0;player.t[TRK_BASS0].to_next=0;seq_advance_arrangement(&player,0,&TEST_SINK);assert(!ons[0]); /* failure cannot revive on no-chord TIE */
    player.t[TRK_BASS0].to_next=0;seq_advance_arrangement(&player,0,&TEST_SINK);assert(actual[0]==64); /* next NOTE revives absolute legacy */
    player_fresh(0,0,0,0,0,0,127,0);ptn[0].bass[0].step[0].note=0;
    seq_start(&player);seq_advance_arrangement(&player,0,&TEST_SINK);assert(player.bass_notes_out[0]==0 && player.bass_channels_out[0]==1);
    int count=note_trace_n;player.t[TRK_BASS0].to_next=0;seq_advance_arrangement(&player,0,&TEST_SINK);assert(note_trace_n==count); /* same pitch, no duplicate On */
    player.send_notes=0;player.ch[TRK_BASS0]=7;seq_midi_sync(&player,1,&TEST_SINK);
    assert(note_trace[note_trace_n-2][0]==0x81 && note_trace[note_trace_n-2][1]==0);assert(player.bass_notes_out[0]==255 && player.bass_channels_out[0]==255);
    for(int why=0;why<5;why++) {
        player_fresh(0,0,0,0,0,0,127,0);ptn[0].bass[1].step[0].flags=G_REST;
        seq_start(&player);seq_advance_arrangement(&player,0,&TEST_SINK);assert(player.bass_notes_out[0]==64);note_trace_n=0;
        if(why==0){player.ch[TRK_BASS0]=255;seq_midi_sync(&player,1,&TEST_SINK);}
        if(why==1){player.send_notes=0;seq_stop(&player);seq_advance_arrangement(&player,0,&TEST_SINK);}
        if(why==2){player.mute|=1u<<MUTE_BASS0;seq_midi_sync(&player,1,&TEST_SINK);assert(!player.t[TRK_BASS0].sounding);}
        if(why==3)seq_midi_sync(&player,0,&TEST_SINK);
        if(why==4)seq_notes_clear(&player,&TEST_SINK);
        assert(note_trace_n==1 && note_trace[0][0]==0x81 && note_trace[0][1]==64 && player.bass_notes_out[0]==255);
    }
    puts("bass ties/MIDI: actual pitch preserved, failed chord stays silent until NOTE, note0 ownership, samepitch balance, original-channel off after disable/channeloff/mute/Stop/reset/disconnect PASS");
}
static int actual_triggers,actual_retunes;
static int counted_trigger(void*x,int b,int n,int a,int sl){actual_triggers++;return s_bon_checked(x,b,n,a,sl);}
static void counted_retune(void*x,int b,int n,int a){actual_retunes++;s_bretune(x,b,n,a);}
static void actual_engine(void) {
    float raw[256];seq_sink_t sink=SINK;sink.bass_trigger=counted_trigger;sink.bass_retune=counted_retune;
    fresh();app_config();seq.send_notes=1;seq.send_clock=0;proj.pat[0].bass[0].len=2;proj.pat[0].bass[0].step[0]=(bstep_t){64,G_NOTE|BS_ACCENT|BS_SLIDE};proj.pat[0].bass[0].step[1]=(bstep_t){99,G_TIE};
    actual_triggers=actual_retunes=0;seq_start(&seq);seq_advance_arrangement(&seq,0,&sink);assert(seq.t[TRK_BASS0].sounding && b303[0].gate);
    bass303_render(&b303[0],raw,256);float env=b303[0].env_y,slew=b303[0].slew_y,acc=b303[0].accent_gain;int amp_acc=b303[0].amp_acc;
    uint32_t random=prob_rng;seq.arrangement.tick=95;seq.arrangement.clock_started=1;seq.clk_to_next=0;
    for(int t=0;t<NTRACKS;t++)seq.t[t].to_next=0xffffffffu;
    seq_advance_arrangement(&seq,0,&sink);
    assert(actual_retunes==1 && prob_rng==random && b303[0].env_y==env && b303[0].slew_y==slew && b303[0].accent_gain==acc && b303[0].amp_acc==amp_acc && !b303[0].trig && !b303[0].amp_trig);
    assert((seq.t[TRK_BASS0].held_note&127)==65 && seq.bass_notes_out[0]==65 && b303[0].gate);
    assert(fabsf(b303[0].osc_freq-b303[0].tuning*powf(2,(65-69)/12.0f))<0.01f);
    /* A same-sample new NOTE takes the new chord once, without a preliminary retune. */
    proj.pat[1].bass[0]=proj.pat[0].bass[0];proj.arr.song.bar[0].pat[TRK_BASS0]=1;
    seq.arrangement.tick=95;seq.clk_to_next=0;int retunes_before=actual_retunes,triggers_before=actual_triggers;
    seq_advance_arrangement(&seq,0,&sink);assert(actual_retunes==retunes_before && actual_triggers==triggers_before+1);
    prob[PART_303A][0]=0;seq.t[TRK_BASS0].to_next=0;seq.t[TRK_BASS0].started=0;trace_n=0;
    seq_advance_arrangement(&seq,0,&sink);assert(!seq.t[TRK_BASS0].sounding && !b303[0].gate && seq.bass_notes_out[0]==255);
    for(int i=0;i<trace_n;i++)assert(((trace[i]>>8)&0xf0)!=0x90); /* failed actual trigger cannot own an unheard MIDI note */
    /* Accepted load/reset clear owned original channel even with notes output disabled. */
    seq_stop(&seq);seq_advance_arrangement(&seq,0,&sink);prob[PART_303A][0]=100;assert(project_save()==0);
    for(int reset=0;reset<2;reset++) {
        seq.bass_notes_out[0]=0;seq.bass_channels_out[0]=7;seq.send_notes=0;trace_n=0;
        if(reset)project_defaults();else assert(project_load()==0);
        assert(trace_n==1 && ((trace[0]>>8)&255)==0x87 && ((trace[0]>>16)&127)==0 && seq.bass_notes_out[0]==255);
        assert(!seq.transport_hold && !seq.transport_stage);
    }
    puts("bass actual303: held-accent/slide boundary preserves envelopes and RNG, same-sample NOTE onlyonce, probability failure has no false MIDI ownership, acceptedload/reset cleanup PASS");
}
static int receiver[16][128];
static uint32_t received[4096];static int received_n;
static void receive_one(void) {
    assert(out_read!=out_write);uint32_t packet=out_ring[out_read++%64];
    if(received_n<4096)received[received_n++]=packet;
    unsigned st=(packet>>8)&255,n=(packet>>16)&127,v=(packet>>24)&127,ch=st&15;
    if((st&240)==144 && v) {receiver[ch][n]++;assert(receiver[ch][n]==1);} /* unbalanced repeated On forbidden */
    if((st&240)==128 || ((st&240)==144 && !v)) {assert(receiver[ch][n]>0);receiver[ch][n]--;}
}
static void receive_all(void) {while(out_read!=out_write)receive_one();}
static void fill_ring(void) {while(plat_midi_out(0x0000f80fu)){}assert(out_write-out_read==64);}
static void ring_fixture(int follow,int source,int own) {
    fresh();app_config();assert(command(13,0,0,0,follow)==VR_QUEUED);render_block();
    proj.pat[0].bass[0].len=2;proj.pat[0].bass[0].step[0]=(bstep_t){(uint8_t)source,G_NOTE|BS_SLIDE|BS_ACCENT};proj.pat[0].bass[0].step[1]=(bstep_t){99,G_TIE};
    proj.pat[0].bass[1].step[0].flags=G_REST;
    memset(receiver,0,sizeof receiver);out_write=out_read=0;received_n=trace_n=0;use_real_ring=1;
    seq.send_notes=1;seq.send_clock=0;
    if(!own)fill_ring();
    seq_start(&seq);seq_advance_arrangement(&seq,0,&SINK);
    assert(seq.t[TRK_BASS0].sounding);
    if(own){receive_all();assert(seq.bass_notes_out[0]!=255 && receiver[1][seq.bass_notes_out[0]]==1);}
    else assert(seq.bass_notes_out[0]==255);
}
static void fifo_feedback(void) {
    variant_snapshot_t view;
    /* Shared actual USB producer: wrap-around and all-or-none pair readback. */
    use_real_ring=1;midi_connected=1;out_write=out_read=0xfffffff0u;
    assert(plat_midi_pair(0x01020304u,0x05060708u));assert(out_write==0xfffffff2u);
    assert(out_ring[out_read++%64]==0x01020304u && out_ring[out_read++%64]==0x05060708u);
    out_write=out_read=0;fill_ring();uint32_t prior=out_write;
    assert(!plat_midi_pair(1,2) && out_write==prior);out_read++;
    assert(!plat_midi_pair(1,2) && out_write==prior);out_read++;
    assert(plat_midi_pair(1,2) && out_write==prior+2);out_write=out_read=0;
    /* Full Off retains ownership despite Stop/output-disable; one freed slot permits cleanup. */
    ring_fixture(0,0,1);fill_ring();seq.send_notes=0;seq_stop(&seq);seq_advance_arrangement(&seq,0,&SINK);
    assert(!seq.playing && !seq.t[TRK_BASS0].sounding && seq.bass_notes_out[0]==0 && seq.bass_channels_out[0]==1);
    assert(variant_snapshot(7,0,0,&view)==VR_OK && view.value[5]==255 && view.value[6]==0 && view.value[7]==1);
    receive_one();seq_midi_sync(&seq,1,&SINK);assert(seq.bass_notes_out[0]==255);receive_all();assert(receiver[1][0]==0);
    prior=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==prior); /* no stopped resurrection */
    /* Full initial On never falsely acquires ownership; retry acquires only upon enqueue. */
    ring_fixture(0,60,0);receive_one();seq_midi_sync(&seq,1,&SINK);
    assert(seq.bass_notes_out[0]==60);receive_all();assert(receiver[1][60]==1);
    seq_stop(&seq);seq_advance_arrangement(&seq,0,&SINK);receive_all();assert(!receiver[1][60]);
    /* Changed-pitch legato waits for TWO slots and retains old ownership when pair fails. */
    ring_fixture(0,60,1);fill_ring();receive_one();prior=out_write;
    proj.pat[0].bass[0].step[0]=(bstep_t){62,G_NOTE|BS_SLIDE};seq.t[TRK_BASS0].started=0;seq.t[TRK_BASS0].to_next=0;
    seq_advance_arrangement(&seq,0,&SINK);assert(out_write==prior && seq.bass_notes_out[0]==60 && (seq.t[TRK_BASS0].held_note&127)==62);
    receive_one();seq_midi_sync(&seq,1,&SINK);assert(out_write==prior+2 && seq.bass_notes_out[0]==62);
    assert(((out_ring[prior%64]>>8)&255)==0x91 && ((out_ring[(prior+1)%64]>>8)&255)==0x81);receive_all();
    assert(!receiver[1][60] && receiver[1][62]==1);seq_notes_clear(&seq,&SINK);receive_all();assert(!receiver[1][62]);
    /* Ordinary same-pitch NOTE is a balanced Off/On with NEW accent velocity, not a duplicate On. */
    ring_fixture(0,60,1);proj.pat[0].bass[0].step[0]=(bstep_t){60,G_NOTE};seq.t[TRK_BASS0].held_note=60;
    fill_ring();receive_one();prior=out_write;seq.t[TRK_BASS0].started=0;seq.t[TRK_BASS0].to_next=0;
    seq_advance_arrangement(&seq,0,&SINK);assert(out_write==prior && seq.bass_notes_out[0]==60);
    receive_one();seq_midi_sync(&seq,1,&SINK);assert(out_write==prior+2);
    assert(((out_ring[prior%64]>>8)&255)==0x81 && ((out_ring[(prior+1)%64]>>8)&255)==0x91 && (out_ring[(prior+1)%64]>>24)==90);
    receive_all();assert(receiver[1][60]==1);seq_notes_clear(&seq,&SINK);receive_all();assert(!receiver[1][60]);
    /* Channel/output changes retain the ORIGINAL channel until its Off has a slot. */
    ring_fixture(0,60,1);fill_ring();seq.ch[TRK_BASS0]=7;seq_midi_prepare(&seq,1,&SINK);
    assert(seq.bass_channels_out[0]==1);seq.send_notes=0;receive_one();seq_midi_sync(&seq,1,&SINK);receive_all();
    assert(seq.bass_channels_out[0]==255 && !receiver[1][60] && !receiver[7][60]);
    /* Disconnect retains owed Off; Stop during disconnection cancels future On. */
    ring_fixture(0,60,1);midi_connected=0;prior=out_write;seq_midi_prepare(&seq,0,&SINK);
    assert(seq.bass_notes_out[0]==60 && out_write==prior);seq_stop(&seq);seq_advance_arrangement(&seq,0,&SINK);
    assert(seq.bass_notes_out[0]==60 && !seq.playing);midi_connected=1;seq_midi_sync(&seq,1,&SINK);receive_all();
    assert(seq.bass_notes_out[0]==255 && !receiver[1][60]);prior=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==prior);
    /* Project reset/load and engine reinitialization must not erase a rejected Off. */
    for(int operation=0;operation<3;operation++) {
        ring_fixture(0,60,1);seq_stop(&seq);seq_advance_arrangement(&seq,0,&SINK);receive_all();assert(project_save()==0);
        seq.playing=1;seq.t[TRK_BASS0].sounding=1;seq.t[TRK_BASS0].held_note=60;s_bretune(0,0,60,0);
        assert(plat_midi_out(midi_packet(0x91,60,90)));seq.bass_notes_out[0]=60;seq.bass_channels_out[0]=1;receive_all();fill_ring();
        seq.playing=0;
        if(operation==0)project_defaults();else if(operation==1)assert(project_load()==0);else engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
        assert(seq.bass_notes_out[0]==60 && seq.bass_channels_out[0]==1 && !seq.t[TRK_BASS0].sounding && !seq.transport_hold);
        receive_one();seq_midi_sync(&seq,1,&SINK);receive_all();assert(seq.bass_notes_out[0]==255 && !receiver[1][60]);
        prior=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==prior);
    }
    use_real_ring=0;
    puts("bass actual USB ring: full/wrap/two-slot atomicity, rejected Off/On ownership, final pitch/velocity retry, balanced samepitch retrigger, original-channel cleanup and stopped disconnect/reset/load recovery readback PASS");
}
static void shared_channel(void) {
    ring_fixture(0,60,1);seq.ch[(TRK_BASS0+1)]=1;
    seq.t[(TRK_BASS0+1)].sounding=1;seq.t[(TRK_BASS0+1)].held_note=60;
    uint32_t prior=out_write;seq_midi_sync(&seq,1,&SINK);receive_all();
    assert(out_write==prior && seq.bass_notes_out[1]==60 && receiver[1][60]==1);
    /* One part changing pitch leaves the other reference sounding. */
    seq.t[TRK_BASS0].held_note=62;seq_midi_sync(&seq,1,&SINK);receive_all();
    assert(receiver[1][60]==1 && receiver[1][62]==1);
    seq.t[TRK_BASS0].held_note=60;seq_midi_sync(&seq,1,&SINK);receive_all();
    assert(receiver[1][60]==1 && !receiver[1][62]);
    seq.t[TRK_BASS0].sounding=0;prior=out_write;seq_midi_prepare(&seq,1,&SINK);
    assert(out_write==prior && seq.bass_notes_out[0]==255 && seq.bass_notes_out[1]==60);
    fill_ring();seq.t[(TRK_BASS0+1)].sounding=0;seq_midi_prepare(&seq,1,&SINK);
    assert(seq.bass_notes_out[1]==60);receive_one();seq_midi_sync(&seq,1,&SINK);receive_all();
    assert(!receiver[1][60] && seq.bass_notes_out[1]==255);
    /* Both shared references stop while disconnected: retain only final owed Off. */
    ring_fixture(0,60,1);seq.ch[(TRK_BASS0+1)]=1;seq.t[(TRK_BASS0+1)].sounding=1;seq.t[(TRK_BASS0+1)].held_note=60;
    seq_midi_sync(&seq,1,&SINK);receive_all();midi_connected=0;seq_midi_prepare(&seq,0,&SINK);
    assert(seq.bass_notes_out[0]==255 && seq.bass_notes_out[1]==60);
    seq.playing=0;seq_notes_clear(&seq,&SINK);midi_connected=1;seq_midi_sync(&seq,1,&SINK);receive_all();
    assert(!receiver[1][60] && seq.bass_notes_out[1]==255);use_real_ring=0;
    puts("bass shared channel: one receiver attack, independent pitch changes, final-reference Off and rejected/disconnected cleanup PASS");
}
static void boundary_recovery(void) {
    for(int due_note=0;due_note<2;due_note++) {
        ring_fixture(1,64,0); /* raw synth60, first MIDI On rejected */
        while(out_read!=out_write)receive_one();
        received_n=trace_n=0;
        seq.arrangement.tick=95;seq.arrangement.clock_started=1;seq.clk_to_next=0;
        for(int t=0;t<NTRACKS;t++)seq.t[t].to_next=0xffffffffu;
        if(due_note){proj.pat[0].bass[0].step[0]=(bstep_t){69,G_NOTE|BS_SLIDE};seq.t[TRK_BASS0].started=0;seq.t[TRK_BASS0].to_next=0;}
        seq_advance_arrangement(&seq,0,&SINK);receive_all();
        assert(received_n==1 && ((received[0]>>8)&255)==0x91 && ((received[0]>>16)&127)==65 && seq.bass_notes_out[0]==65);
        seq_notes_clear(&seq,&SINK);receive_all();assert(!receiver[1][65]);
    }
    /* Prior changed-pitch pair rejected, then recovery and new chord coincide. Never publish old chord64. */
    ring_fixture(2,60,1);fill_ring();proj.pat[0].bass[0].step[0]=(bstep_t){64,G_NOTE|BS_SLIDE};seq.t[TRK_BASS0].started=0;seq.t[TRK_BASS0].to_next=0;
    seq_advance_arrangement(&seq,0,&SINK);assert(seq.bass_notes_out[0]==60 && (seq.t[TRK_BASS0].held_note&127)==64);
    receive_all();received_n=trace_n=0;seq.arrangement.tick=95;seq.arrangement.clock_started=1;seq.clk_to_next=0;
    for(int t=0;t<NTRACKS;t++)seq.t[t].to_next=0xffffffffu;
    seq_advance_arrangement(&seq,0,&SINK);receive_all();assert(received_n==2 && ((received[0]>>16)&127)==65 && ((received[1]>>16)&127)==60);
    assert(!receiver[1][60] && receiver[1][65]);seq_notes_clear(&seq,&SINK);receive_all();
    use_real_ring=0;
    /* Reason reporting: strict range failed; next valid NOTE chance-rejected must not retain strictBlocked. */
    fresh();app_config();proj.pat[0].bass[0].step[0]=(bstep_t){64,G_NOTE|BS_SLIDE};
    proj.variant.arrangement.bass_low[0]=proj.variant.arrangement.bass_high[0]=61;
    seq.send_notes=1;seq.send_clock=0;seq_start(&seq);seq_advance_arrangement(&seq,0,&SINK);assert(seq.arrangement.reserved&1);
    seq.arrangement.key=1;seq.arrangement.degree=1;prob[PART_303A][0]=0;seq.t[TRK_BASS0].started=0;seq.t[TRK_BASS0].to_next=0;
    seq_advance_arrangement(&seq,0,&SINK); /* C-sharp root61 fits; probability rejection is a separate reason. */
    variant_snapshot_t v;assert(variant_snapshot(7,0,0,&v)==VR_OK && v.value[9]==0 && seq.t[TRK_BASS0].trigger_rejected && !seq.t[TRK_BASS0].sounding);
    puts("bass queue recovery at boundary: same-sample NOTE and held retune only retry current-chord pitch; range failure and chance rejection report distinct reasons PASS");
}
static pattern_t gen_pattern[2][NPAT];static seq_t gen_player[2];
static void generator_parity(void) {
    uint8_t flags=intent.flags;seq_sink_t sink=TEST_SINK;
    player_fresh(0,0,1,0,0,24,72,0);
    for(int b=0;b<2;b++){ptn[0].bass[b].len=5;ptn[0].bass[b].dir=DIR_RANDOM;ptn[0].bass[b].gen.mutate_bars=1;tb3po_generate(&ptn[0].bass[b]);}
    for(int k=0;k<2;k++){memcpy(gen_pattern[k],ptn,sizeof ptn);seq_init(&gen_player[k],gen_pattern[k]);gen_player[k].song=&song_fixture;gen_player[k].send_notes=gen_player[k].send_clock=0;seq_arrangement_apply(&gen_player[k],&intent);if(k)gen_player[k].arrangement.flags|=2|(2<<3);seq_start(&gen_player[k]);seq_advance_arrangement(&gen_player[k],0,&sink);}
    for(int block=0;block<8000;block++)for(int k=0;k<2;k++)seq_advance_arrangement(&gen_player[k],256,&sink);
    assert(!memcmp(gen_pattern[0],gen_pattern[1],sizeof gen_pattern[0]));
    for(int b=0;b<2;b++)assert(gen_player[0].mut_rng[b]==gen_player[1].mut_rng[b] && gen_player[0].t[TRK_BASS0+b].rng==gen_player[1].t[TRK_BASS0+b].rng && gen_player[0].t[TRK_BASS0+b].pos==gen_player[1].t[TRK_BASS0+b].pos);
    (void)flags;puts("bass generator: Off versus strict output projection leaves random direction, mutation RNG, positions and all pattern bytes identical over8000 stretches PASS");
}
#ifndef BASS_TEST_NO_MAIN
int main(void){storage_commands();matrix();tie_and_midi();actual_engine();fifo_feedback();shared_channel();boundary_recovery();generator_parity();printf("host layout seq_t=%u variant=%u arrangement=%u; target/resource costs not yet measured\n",(unsigned)sizeof(seq_t),(unsigned)sizeof(variant_settings_t),(unsigned)sizeof(arrangement_settings_t));return 0;}

#endif
