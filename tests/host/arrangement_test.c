/* SPDX-License-Identifier: GPL-3.0-only */
/* Real shared sequencer/application/storage. Independent wall-clock/event oracle. */
#define _POSIX_C_SOURCE 200809L
#define PLAT_CONTROL_TEST
static int store_probe(int writing);
#define HOST_STORE_PROBE store_probe
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static int locked, publish_probe, start_before_lock, start_during_hold, store_interrupt, store_failure;
static uint32_t written_before;
static song_bar_t expected_block;
static uint8_t expected_degree;
uint32_t plat_control_enter(void)
{
    assert(!locked);
    if (start_before_lock) {
        int32_t block[512];
        seq_start(&seq);engine_render(block,256);assert(seq.playing);
    }
    locked = 1; written_before = engine_queue_written(); return 17;
}
void plat_control_leave(uint32_t saved)
{
    assert(locked && saved == 17);
    if (publish_probe) assert(engine_queue_written() == written_before + 1u);
    if (publish_probe) {
        int32_t block[512];
        assert(!memcmp(&proj.arr.song.bar[2],&expected_block,sizeof expected_block));
        assert(proj.variant.arrangement.degree[2] == expected_degree);
        locked = 0;
        engine_render(block,256); /* ISR immediately after restore sees published marker + all seven bytes */
        assert(engine_queue_read() == engine_queue_written());
    } else locked = 0;
    if (start_during_hold && seq.transport_hold) {
        int32_t b[512]; start_during_hold=0;
        seq_continue(&seq); engine_render(b,256);
        assert(!seq.playing && seq.req_cont && seq.transport_hold);
    }
}
static int store_probe(int writing)
{
    if (store_interrupt) {
        int32_t b[512]; store_interrupt=0;
        assert(seq.transport_hold && !locked);
        seq_start(&seq);engine_render(b,256);
        assert(!seq.playing && seq.req_start && seq.transport_hold);
    }
    return writing && store_failure;
}
static variant_reply_t reply;
static int cmd(int op,int target,int voice,int index,int value)
{
    variant_command_t c={1,(uint16_t)op,77,(uint8_t)target,(uint8_t)voice,(uint8_t)index,0,value};
    int rc=variant_command(&c,&reply); assert(reply.request==77 && reply.result==rc); return rc;
}
static void drain(void) {int32_t b[512];engine_render(b,256);}
static void fresh(void)
{
    int i;
    for(i=0;i<OBJ_NOBJ;i++)store_len[i]=-1;
    memset(stored_known,0,sizeof stored_known);project_discard_rejected();boot();drain();ui.dirty=0;
}
static void command_storage(void)
{
    project_t before;
    uint32_t writes,packed;
    variant_snapshot_t view;
    fresh();
    assert(sizeof(arrangement_settings_t)==204 && sizeof(variant_settings_t)==412);
    assert(sizeof(arrangement_runtime_t)<=24); /* host pointer ABI may add four bytes */
    assert(cmd(VC_SET_ARR_LENGTH,0,0,0,192)==VR_QUEUED);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_QUEUED);
    expected_block=(song_bar_t){{1,2,3,4,5},(uint8_t)(21|(7<<5))};expected_degree=6;
    publish_probe=1;
    assert(cmd(VC_SET_BLOCK,2,7|(6<<3),21,1|(2<<5)|(3<<10)|(4<<15)|(5<<20))==VR_QUEUED);
    publish_probe=0;
    assert(cmd(VC_EDIT_BLOCK,191,0,0,0)==VR_OK);
    assert(cmd(VC_SET_STRIP,24,0,VP_BOOST,9)==VR_QUEUED);
    assert(variant_snapshot(VS_BLOCK,2,0,&view)==VR_OK && view.count==8 && view.value[6]==8 && view.value[7]==6);
    before=proj;
    assert(cmd(VC_SET_BLOCK,2,64,0,0)==VR_INVALID);
    assert(cmd(VC_SET_BLOCK,2,0,32,0)==VR_INVALID);
    assert(cmd(VC_SET_BLOCK,2,0,0,1<<25)==VR_INVALID);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,2,HARMONY_CHROMATIC)==VR_INVALID);
    assert(!memcmp(&before,&proj,sizeof proj));
    drain();assert(cmd(VC_SAVE,0,0,0,0)==VR_OK);
    assert(get32(store[OBJ_PAT0])==4 && get32(store[OBJ_PAT0]+4)==RAW_LEN);
    project_defaults();assert(project_load()==0);drain();
    assert(proj.arr.song.len==192 && proj.variant.arrangement.degree[2]==6 && proj.arr.song.bar[2].mute==expected_block.mute);
    /* Old format-3 fixture is generated with its independent schema-1 byte length/header. */
    proj.variant.schema=1;proj.variant.bytes=208;
    assert(stream_measure(SCHEMA1_RAW_LEN,&packed)==0 && save_stream(SCHEMA1_RAW_LEN,3,packed)==0);
    {
        FILE *fixture = fopen("build/host/arrangement_schema1_store.bin","wb");
        int obj;
        assert(fixture);
        for (obj=0;obj<OBJ_NOBJ;obj++) {
            int32_t n=store_len[obj];
            assert(fwrite(&n,4,1,fixture)==1);
            assert(fwrite(store[obj],1,PLAT_STORE_MAX,fixture)==PLAT_STORE_MAX);
        }
        fclose(fixture);
    }
    assert(project_load()==0);
    assert(proj.variant.schema==4 && proj.variant.bytes==412 && proj.variant.arrangement.flags==0 && proj.variant.arrangement.degree[2]==0);
    assert(!memcmp(proj.variant.strip,before.variant.strip,sizeof proj.variant.strip));
    /* Valid CRC but invalid arrangement data must preserve project, journal and stored bytes. */
    proj.variant.arrangement.degree[191]=8;
    assert(stream_measure(RAW_LEN,&packed)==0 && save_stream(RAW_LEN,4,packed)==0);
    arrangement_defaults(&proj.variant.arrangement);before=proj;writes=store_writes;
    assert(project_load()==-8 && project_write_protected());
    assert(!memcmp(&before,&proj,sizeof proj) && store_writes==writes);
    assert(cmd(VC_SAVE,0,0,0,0)==VR_PROTECTED);
    fresh();assert(project_save()==0 && get32(store[OBJ_PAT0])==2);
    before=proj;writes=store_writes;
    while(engine_strip_set(24,VP_BOOST,9)) {}
    assert(project_load()==-9 && !memcmp(&before,&proj,sizeof proj) && store_writes==writes);
    drain();assert(project_load()==0);drain();
    puts("arrangement: atomic seven-byte publication, bounds, 192 blocks, format4 roundtrip, schema1 migration, protected CRC-valid invalid degree and legacy default format2 PASS");
}

static void reject_format4(void)
{
    int kind;
    for(kind=0;kind<9;kind++) {
        project_t before;
        undo_t journal;
        uint8_t shadow[sizeof undo_shadow];
        uint32_t packed,writes;
        fresh();assert(cmd(VC_SET_ARR_LENGTH,0,0,0,2)==VR_QUEUED);
        assert(cmd(VC_SET_BLOCK,0,1<<3,0,0)==VR_QUEUED);drain();
        before=proj;
        if(kind==0)proj.variant.arrangement.flags=64;
        if(kind==1)proj.variant.arrangement.key=12;
        if(kind==2)proj.variant.arrangement.mode=HARMONY_CHROMATIC;
        if(kind==3)proj.variant.arrangement.parent=7;
        if(kind==4)proj.variant.arrangement.degree[191]=8;
        if(kind==5)proj.arr.song.len=193;
        if(kind==6)proj.arr.song.bar[191].pat[4]=32;
        if(kind==7)proj.variant.schema=5;
        if(kind==8)proj.variant.bytes--;
        assert(stream_measure(RAW_LEN,&packed)==0 && save_stream(RAW_LEN,4,packed)==0);
        proj=before;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);writes=store_writes;
        assert(project_load()==-8 && project_write_protected());
        assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo));
        assert(!memcmp(shadow,undo_shadow,sizeof shadow) && writes==store_writes);
        assert(cmd(VC_SET_BLOCK,0,0,0,0)==VR_PROTECTED && project_save()==-8);
    }
    puts("arrangement: nine CRC-valid format4 flag/key/mode/parent/degree/song-length/pattern/schema/extension-length rejections preserve project and full journal PASS");
}

static uint32_t ui_revision(void) { variant_snapshot_t v; assert(variant_snapshot(VS_INFO,0,0,&v)==VR_OK); return v.revision; }

static void undo_and_panel_safety(void)
{
    project_t before;
    undo_t journal;
    uint8_t shadow[sizeof undo_shadow];
    uint8_t edit;
    uint32_t revision,writes;
    fresh();
    assert(cmd(VC_SET_ARR_LENGTH,0,0,0,1)==VR_QUEUED);
    assert(cmd(VC_END_EDIT,0,0,0,0)==VR_OK);drain();
    assert(cmd(VC_BEGIN_EDIT,0,0,0,0)==VR_OK);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_QUEUED);
    assert(cmd(VC_END_EDIT,0,0,0,0)==VR_OK);drain();
    do_undo(0);drain(); /* actual physical undo path must refresh flags */
    assert(!proj.variant.arrangement.flags && !seq.arrangement.flags && undo.cur+1==undo.n);
    seq_start(&seq);drain();assert(seq.playing);
    before=proj;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);edit=external_edit;revision=ui_revision();
    assert(ui_undo_preflight(1)==VR_BUSY && cmd(VC_REDO,0,0,0,0)==VR_BUSY);
    do_undo(1);
    assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo));
    assert(!memcmp(shadow,undo_shadow,sizeof shadow) && external_edit==edit && ui_revision()==revision);
    assert(strstr(ui.msg,"STOP FOR ARRANGEMENT UNDO"));
    seq_stop(&seq);drain();do_undo(1);drain();
    assert(proj.variant.arrangement.flags==1 && seq.arrangement.flags==1);
    /* An uncommitted disable has a shadow flag of one: pending undo would re-enable. */
    assert(cmd(VC_BEGIN_EDIT,0,0,0,0)==VR_OK);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,0)==VR_QUEUED);drain();
    seq_start(&seq);drain();before=proj;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);
    assert(external_edit && ui_undo_preflight(0)==VR_BUSY && cmd(VC_UNDO,0,0,0,0)==VR_BUSY);
    do_undo(0);
    assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo));
    assert(!memcmp(shadow,undo_shadow,sizeof shadow) && external_edit);
    seq_stop(&seq);drain();assert(cmd(VC_END_EDIT,0,0,0,0)==VR_OK);
    /* Legacy live sound undo/redo remains supported when no prospective flag enables arrangement. */
    fresh();assert(cmd(VC_BEGIN_EDIT,0,0,0,0)==VR_OK);
    assert(cmd(VC_SET_STRIP,24,0,VP_BOOST,7)==VR_QUEUED);assert(cmd(VC_END_EDIT,0,0,0,0)==VR_OK);drain();
    seq_start(&seq);drain();assert(cmd(VC_UNDO,0,0,0,0)==VR_OK);drain();
    assert(proj.variant.strip[24].boost_db==0);
    assert(cmd(VC_REDO,0,0,0,0)==VR_OK);drain();assert(proj.variant.strip[24].boost_db==7);
    seq_stop(&seq);drain();
    /* Protected physical undo must retain a pending gesture and complete journal. */
    assert(project_save()==0);assert(cmd(VC_BEGIN_EDIT,0,0,0,0)==VR_OK);
    assert(cmd(VC_SET_STRIP,24,0,VP_BOOST,9)==VR_QUEUED);drain();
    store[OBJ_PAT0][8]^=1;assert(project_load()==-8);
    before=proj;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);writes=store_writes;
    assert(ui_undo_preflight(0)==VR_PROTECTED);do_undo(0);
    assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo));
    assert(!memcmp(shadow,undo_shadow,sizeof shadow) && external_edit && writes==store_writes);
    assert(strstr(ui.msg,"PROJECT PROTECTED"));
    /* Exercise every legacy SONG mutation entry point against the real UI. */
    fresh();assert(cmd(VC_SET_ARR_LENGTH,0,0,0,3)==VR_QUEUED);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_QUEUED);drain();
    ui.song_sel=1;ui.view=V_SONG;ui.overlay=O_NONE;ui.btn=0;before=proj;revision=ui_revision();
    pref_set(PR(R_SPAT,0,0,0),3);pref_set(PR(R_SREP,0,0,0),7);pref_set(PR(R_SLEN,0,0,0),10);
    key_event(0,1);key_event(1,1);song_fill(10);
    run_action(ACT_SONG_INS,0);run_action(ACT_SONG_DEL,0);run_action(ACT_SONG_CLR,0);
    button_tap(B_REC);
    assert(!ui.rec && !memcmp(&before,&proj,sizeof proj) && ui_revision()==revision);
    assert(strstr(ui.msg,"ARRANGEMENT: EDIT IN LAB"));
    seq.playing=1;seq.song_on=seq.song_rec=1;seq.song_pos=1;mute_part(0);
    run_action(ACT_RESET,0);assert(strstr(ui.msg,"STOP FOR ARRANGEMENT RESET"));
    assert(!memcmp(&before,&proj,sizeof proj));seq.playing=0;
    /* Pending start/continue are not stopped: reject before gesture commit or storage writes. */
    assert(cmd(VC_BEGIN_EDIT,0,0,0,0)==VR_OK);
    before=proj;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);writes=store_writes;
    seq.req_start=1;
    assert(cmd(VC_SAVE,0,0,0,0)==VR_BUSY && cmd(VC_SET_ARR_LENGTH,0,0,0,10)==VR_BUSY);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,0)==VR_BUSY && project_save()==-9 && project_load()==-9);
    seq.req_start=0;seq.req_cont=1;
    assert(cmd(VC_SAVE,0,0,0,0)==VR_BUSY && project_load()==-9);
    assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo));
    assert(!memcmp(shadow,undo_shadow,sizeof shadow) && external_edit && writes==store_writes);
    seq.req_cont=0;
    /* Interrupt starts playback between the public stopped check and lock acquisition. */
    fresh();before=proj;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);revision=ui_revision();
    start_before_lock=1;
    assert(cmd(VC_SET_ARR_LENGTH,0,0,0,4)==VR_BUSY);
    start_before_lock=0;
    assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo));
    assert(!memcmp(shadow,undo_shadow,sizeof shadow) && ui_revision()==revision);
    seq_stop(&seq);drain();
    start_before_lock=1;
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_BUSY);
    start_before_lock=0;
    assert(!memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo));
    assert(!memcmp(shadow,undo_shadow,sizeof shadow) && ui_revision()==revision);
    seq_stop(&seq);drain();
    /* All old SONG editing paths remain usable when arrangement is disabled. */
    fresh();ui.song_sel=0;ui.view=V_SONG;ui.overlay=O_NONE;ui.btn=0;
    pref_set(PR(R_SLEN,0,0,0),2);pref_set(PR(R_SPAT,0,0,0),3);pref_set(PR(R_SREP,0,0,0),4);
    assert(song()->len==2 && song()->bar[0].pat[0]==3 && SONG_REP(&song()->bar[0])==5);
    key_event(1,1);assert(song()->bar[0].mute & 1u);
    run_action(ACT_SONG_INS,0);assert(song()->len==3);
    run_action(ACT_SONG_DEL,0);assert(song()->len==2);
    button_tap(B_REC);assert(ui.rec);
    run_action(ACT_SONG_CLR,0);assert(!song()->len);
    puts("arrangement: prospective live redo-enable/pending-shadow undo blocked with exact journal; legacy live undo retained; protected physical undo, SONG panel gates and pending transport no-writes PASS");
}

static void transport_reservations(void)
{
    project_t before;
    undo_t journal;
    uint8_t shadow[sizeof undo_shadow];
    uint32_t writes, revision;
    int op;
    /* Each public path must reject an IRQ start before acquisition, before even gesture commit. */
    for (op=0;op<4;op++) {
        fresh();assert(cmd(VC_SET_ARR_LENGTH,0,0,0,2)==VR_QUEUED);
        assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_QUEUED);drain();
        assert(cmd(VC_BEGIN_EDIT,0,0,0,0)==VR_OK);
        assert(cmd(VC_SET_STRIP,24,0,VP_BOOST,8)==VR_QUEUED);drain();
        before=proj;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);writes=store_writes;revision=ui_revision();
        start_before_lock=1;
        if(op==0) assert(cmd(VC_SAVE,0,0,0,0)==VR_BUSY);
        if(op==1) assert(project_load()==-9);
        if(op==2) assert(cmd(VC_UNDO,0,0,0,0)==VR_BUSY);
        if(op==3) run_action(ACT_RESET,0);
        start_before_lock=0;
        assert(!seq.transport_hold && !memcmp(&before,&proj,sizeof proj));
        assert(!memcmp(&journal,&undo,sizeof undo) && !memcmp(shadow,undo_shadow,sizeof shadow));
        assert(external_edit && writes==store_writes && revision==ui_revision());
        seq_stop(&seq);drain();
    }
    fresh();assert(cmd(VC_SET_ARR_LENGTH,0,0,0,2)==VR_QUEUED);
    assert(cmd(VC_SET_BLOCK,0,1|(6<<3),0,3)==VR_QUEUED);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_QUEUED);drain();
    store_interrupt=1;assert(cmd(VC_SAVE,0,0,0,0)==VR_OK);
    assert(!seq.transport_hold && seq.req_start && !seq.playing);drain();
    assert(seq.playing && seq.ppat[0]==3 && seq.arrangement.degree==6);
    seq_stop(&seq);drain();
    project_defaults();store_interrupt=1;assert(project_load()==0);
    assert(!seq.transport_hold && !seq.transport_stage && seq.req_start && !seq.playing);drain();
    assert(seq.playing && seq.ppat[0]==3 && seq.arrangement.degree==6);
    seq_stop(&seq);drain();
    /* Undo and reset defer a Continue immediately after acquisition until the full swap is published. */
    assert(cmd(VC_BEGIN_EDIT,0,0,0,0)==VR_OK);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,0)==VR_QUEUED);
    assert(cmd(VC_END_EDIT,0,0,0,0)==VR_OK);drain();
    start_during_hold=1;assert(cmd(VC_UNDO,0,0,0,0)==VR_OK);
    assert(!seq.transport_hold && seq.req_cont && !seq.playing);drain();
    assert(seq.playing && seq.arrangement.flags==1 && seq.ppat[0]==3);
    seq_stop(&seq);drain();start_during_hold=1;run_action(ACT_RESET,0);
    assert(!seq.transport_hold && seq.req_cont && !seq.playing);drain();
    assert(seq.playing && !seq.arrangement.flags && !proj.variant.arrangement.flags);
    seq_stop(&seq);drain();
    /* Failures release all nesting, including protected, full, I/O, malformed load, staging and no-op undo. */
    proj.set.autosave_off ^= 1u;
    store_failure=1;store_interrupt=1;assert(project_save()==-1);store_failure=0;
    assert(!seq.transport_hold && seq.req_start);drain();seq_stop(&seq);drain();
    assert(project_save()==0);store[OBJ_PAT0][8]^=1;store_interrupt=1;assert(project_load()==-8);
    assert(!seq.transport_hold && !seq.transport_stage && seq.req_start);drain();seq_stop(&seq);drain();
    assert(project_save()==-8 && !seq.transport_hold);
    fresh();assert(engine_transport_reserve());
    writes=seq.ext_since;seq_start(&seq);seq_ext_clock(&seq);drain();
    assert(!seq.playing && seq.req_start && !seq.ext_pending && seq.ext_since==writes+256);
    assert(engine_transport_reserve() && seq.transport_hold==2); /* nested caller accepts deferred request */
    engine_transport_release();drain();assert(seq.transport_hold==1 && !seq.playing && seq.req_start);
    engine_transport_release();drain();assert(!seq.transport_hold && seq.playing);
    seq_stop(&seq);drain();
    fresh();assert(!ui_project_stage(UINT32_MAX) && !seq.transport_hold && !seq.transport_stage);
    assert(ui_project_stage(128));seq_start(&seq);drain();assert(!seq.playing && seq.req_start);
    ui_project_stage_finish(0);assert(!seq.transport_hold && !seq.transport_stage);drain();
    seq_stop(&seq);drain();
    fresh();assert(cmd(VC_SET_ARR_LENGTH,0,0,0,1)==VR_QUEUED);
    assert(cmd(VC_SET_ARR_GLOBAL,0,0,0,1)==VR_QUEUED);drain();undo_reset(&undo);
    assert(cmd(VC_UNDO,0,0,0,0)==VR_OK && !seq.transport_hold);
    puts("arrangement: reservation IRQ-before-acquire exact rejection, during-save/load/undo/reset deferral, coherent released start and failure/nested-stage cleanup PASS");
}

static pattern_t patterns[NPAT];
static song_t timeline_song;
static arrangement_settings_t settings;
static seq_t player;
static uint32_t sample_now, transition_time[16], transitions;
static int step_order_ok, boundary_expected;
static void step_event(void *ctx,int part,int pos)
{
    (void)ctx;(void)pos;
    if(boundary_expected && part==0) {assert(player.song_pos==1 && player.ppat[0]==1);step_order_ok=1;}
}
static int drum_event(void *ctx,int kit,int voice,float vel) {(void)ctx;(void)kit;(void)voice;(void)vel;return 1;}
static void bass_event(void *ctx,int part,int note,int accent,int slide) {(void)ctx;(void)part;(void)note;(void)accent;(void)slide;}
static void bass_off(void *ctx,int part) {(void)ctx;(void)part;}
static const seq_sink_t sink={drum_event,bass_event,bass_off,0,0,0,step_event,0,0,0,0,0,0,0};
static void advance(uint32_t n)
{
    while(n) {
        uint32_t k=(player.arrangement.flags & ARR_ENABLED) ? seq_until_event_arrangement(&player,n) : seq_until_event(&player,n);
        uint32_t old=player.song_pos;
        sample_now+=k;
        if(player.arrangement.flags & ARR_ENABLED)seq_advance_arrangement(&player,k,&sink);
        else seq_advance(&player,k,&sink);
        n-=k;
        if(player.song_pos!=old && transitions<16)transition_time[transitions++]=sample_now;
    }
}
static void init_player_params(float bpm,int length,int duration,int rate,int swing)
{
    int i;
    memset(&timeline_song,0,sizeof timeline_song);
    for(i=0;i<NPAT;i++)pattern_init(&patterns[i],1,2);
    patterns[0].drum[0].len=(uint8_t)length;
    patterns[0].drum[0].rate=(uint8_t)rate;patterns[0].swing=(uint8_t)swing;
    patterns[0].bass[0].len=7;patterns[0].bass[1].len=11;
    timeline_song.len=2;timeline_song.bar[0].mute=(uint8_t)((duration-1)<<5);timeline_song.bar[1].pat[0]=1;
    arrangement_defaults(&settings);settings.flags=1;settings.degree[0]=1;settings.degree[1]=6;
    seq_init(&player,patterns);player.song=&timeline_song;player.bpm=bpm;player.send_clock=0;
    seq_arrangement_apply(&player,&settings);
    sample_now=transitions=0;step_order_ok=boundary_expected=0;
    seq_start(&player);advance(1);
}
static void init_player(float bpm,int length,int duration) {init_player_params(bpm,length,duration,RATE_16,0);}
static void clocks(void)
{
    const int bpms[]={90,120,180},lengths[]={5,16,31};
    int b,l,d,rate;
    for(b=0;b<3;b++)for(l=0;l<3;l++)for(d=1;d<=8;d++)for(rate=0;rate<NRATES;rate++) {
        uint32_t expected;
        init_player_params((float)bpms[b],lengths[l],d,rate,(rate & 1)?100:0);
        expected=(uint32_t)(44100.0*240.0/bpms[b]*d);
        advance(expected-4);
        boundary_expected=1;
        advance(8);
        assert(transitions==1 && transition_time[0]>=expected-1 && transition_time[0]<=expected+2 && step_order_ok);
    }
    /* Queue overrides at the next musical bar, not the eight-bar block end or 909 wrap. */
    init_player(120.0f,5,8);advance(22050);seq_arrangement_queue(&player,1);
    assert(player.arrangement.queued==1 && player.song_pos==0);
    advance(66152);assert(player.song_pos==1 && player.arrangement.queued==255);
    assert(transition_time[0]>=88199 && transition_time[0]<=88202);
    /* Same pattern in an unchanged odd-length part continues rather than restarting. */
    init_player(120.0f,5,1);advance(88190);{
        advance(20);assert(player.song_pos==1 && player.t[TRK_BASS0].pos==2);
        assert(player.t[TRK_BASS0].bars==2 && player.t[TRK_BASS0].started);
        assert(player.ppat[0]==1 && player.t[0].pos==0);
    }
    puts("arrangement: 288 tempo/duration/odd-length/rate/swing combinations, clock output off, same-sample restart, unchanged-part continuation and next-bar queue PASS");
}
static void external_clock(void)
{
    int i;
    init_player(120.0f,5,1);
    /* Begin external transport afresh: pulse zero is its downbeat. */
    seq_stop(&player);advance(1);seq_start(&player);seq_ext_clock(&player);advance(1);
    assert(player.arrangement.tick==0);
    for(i=0;i<95;i++){seq_ext_clock(&player);advance(919);}
    assert(player.song_pos==0 && player.arrangement.tick==95);
    seq_ext_clock(&player);advance(919);assert(player.song_pos==1 && player.arrangement.tick==0);
    /* Loss freezes external phase until timeout; internal resumes after a full new tick. */
    for(i=0;i<10;i++){seq_ext_clock(&player);advance(919);}
    assert(player.arrangement.tick==10);
    advance(44100/2+2);assert(!player.ext && player.arrangement.tick==10);
    assert(player.song_pos==1);
    advance(1000);assert(player.arrangement.tick>=11 && player.arrangement.tick<=12);
    puts("arrangement: external 96-tick boundary and clock-loss internal fallback without duplicated boundary PASS");
}
int main(void) {command_storage();reject_format4();undo_and_panel_safety();transport_reservations();clocks();external_clock();return 0;}
