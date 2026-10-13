/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual application/sequencer/engine plus shared production USB producer. */
#define BASS_TEST_NO_MAIN
#include "bass_follow_test.c"
#undef main
static uint32_t triad_packets[6];static uint32_t triad_n;
static int collect(void*x,const uint32_t*p,uint32_t n) {(void)x;assert(n<=6);memcpy(triad_packets,p,n*4);triad_n=n;return 1;}
static void theory(void) {
    const int ranges[][2]={{0,127},{48,84},{60,67},{61,61},{0,12},{120,127}};
    const int hints[]={0,60,127};unsigned cases=0;seq_sink_t sink={.midi_batch_try=collect};
    for(int mode=0;mode<9;mode++)for(int key=0;key<12;key++)for(int degree=1;degree<=7;degree++)for(int r=0;r<6;r++)for(int h=0;h<3;h++) {
        player_fresh(mode,key,degree,0,0,0,127,0);intent.pad_channel=8;intent.pad_root=(uint8_t)hints[h];intent.pad_low=(uint8_t)ranges[r][0];intent.pad_high=(uint8_t)ranges[r][1];
        for(int b=0;b<2;b++)ptn[0].bass[b].step[0].flags=G_REST;
        int parent=mode==7?0:mode==8?5:mode,d=degree-1,best=-1,dist=256,expect[3];
        int rootpc=(key+scales[parent][d])%12,third=scales[parent][(d+2)%7]+12*((d+2)/7)-scales[parent][d],fifth=scales[parent][(d+4)%7]+12*((d+4)/7)-scales[parent][d];
        for(int n=ranges[r][0];n<=ranges[r][1];n++)if(n%12==rootpc && n+fifth<=ranges[r][1]) {int delta=abs(n-hints[h]);if(delta<dist){dist=delta;best=n;}}
        expect[0]=best;expect[1]=best+third;expect[2]=best+fifth;triad_n=0;
        seq_start(&player);seq_advance_arrangement(&player,0,&sink);
        if(best<0)assert(triad_n==0 && player.pad_channel_out==255);
        else {assert(triad_n==3 && player.pad_channel_out==8);for(int i=0;i<3;i++)assert(player.pad_notes_out[i]==expect[i] && ((triad_packets[i]>>16)&127)==(unsigned)expect[i] && (triad_packets[i]>>24)==90);}
        cases++;
    }
    printf("pad theory: %u independent root-position voicing/key/mode/degree/range/hint cases PASS\n",cases);
}
static void pad_fixture(int hint,int low,int high) {
    fresh();app_config();assert(command(14,0,0,0,8)==VR_QUEUED);assert(command(14,0,0,1,hint)==VR_QUEUED);
    if(high<48){assert(command(14,0,0,2,low)==VR_QUEUED);assert(command(14,0,0,3,high)==VR_QUEUED);}else {assert(command(14,0,0,3,high)==VR_QUEUED);assert(command(14,0,0,2,low)==VR_QUEUED);}
    assert(command(15,0,0,0,1)==VR_QUEUED);render_block();
    for(int b=0;b<2;b++)proj.pat[0].bass[b].step[0].flags=G_REST;
    memset(receiver,0,sizeof receiver);out_write=out_read=0;received_n=trace_n=0;use_real_ring=1;
    seq.send_clock=0;seq_start(&seq);seq_advance_arrangement(&seq,0,&SINK);receive_all();assert(seq.pad_channel_out==8);
}
static void boundary(int degree) {
    proj.variant.arrangement.degree[1]=(uint8_t)degree;seq.arrangement.queued=1;seq.arrangement.tick=95;seq.arrangement.clock_started=1;seq.clk_to_next=0;
    for(int t=0;t<NTRACKS;t++)seq.t[t].to_next=0xffffffffu;
    seq_advance_arrangement(&seq,0,&SINK);
}
static void atomic_cleanup(void) {
    pad_fixture(60,48,84);assert(receiver[8][60] && receiver[8][64] && receiver[8][67]);uint32_t before=out_write;
    boundary(1);assert(out_write==before); /* unchanged triad sustains */
    fill_ring();for(int i=0;i<5;i++)receive_one();before=out_write;boundary(4);
    assert(out_write==before && seq.pad_notes_out[0]==60 && seq_pad_pending(&seq));
    /* A newer Am latch replaces the rejected F desire; no obsolete attack. */
    seq.arrangement.degree=6;receive_one();seq_midi_sync(&seq,1,&SINK);assert(out_write==before+6);
    for(int i=0;i<3;i++)assert(((out_ring[(before+i)%64]>>8)&255)==0x88);
    for(int i=3;i<6;i++)assert(((out_ring[(before+i)%64]>>8)&255)==0x98);
    receive_all();assert(seq.pad_notes_out[0]==57 && receiver[8][57] && receiver[8][60] && receiver[8][64] && !receiver[8][67]);
    fill_ring();receive_one();receive_one();before=out_write;seq_stop(&seq);seq_advance_arrangement(&seq,0,&SINK);
    assert(out_write==before && seq.pad_channel_out==8 && !seq.playing);receive_one();seq_midi_sync(&seq,1,&SINK);receive_all();
    assert(seq.pad_channel_out==255 && !receiver[8][57] && !receiver[8][60] && !receiver[8][64]);before=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==before);
    /* Note0 valid, disconnected cleanup retained; recovery does not start until next boundary. */
    pad_fixture(0,0,12);assert(seq.pad_notes_out[0]==0 && receiver[8][0]);midi_connected=0;seq_midi_prepare(&seq,0,&SINK);assert(seq.pad_channel_out==8);
    midi_connected=1;seq_midi_sync(&seq,1,&SINK);receive_all();assert(seq.pad_channel_out==255 && !receiver[8][0]);before=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==before);
    boundary(1);receive_all();assert(seq.pad_channel_out==8 && receiver[8][0]);
    seq.ch[0]=8;seq_midi_prepare(&seq,1,&SINK);receive_all();assert(seq.pad_channel_out==255 && !receiver[8][0]);
    seq.ch[0]=9;before=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==before);boundary(1);receive_all();assert(receiver[8][0]);
    boundary(0);receive_all();assert(seq.pad_channel_out==255);
    boundary(1);receive_all();assert(seq.pad_channel_out==8 && receiver[8][0]);
    proj.variant.arrangement.pad_low=proj.variant.arrangement.pad_high=1;boundary(1);receive_all();assert(seq.pad_channel_out==255);
    proj.variant.arrangement.pad_low=0;proj.variant.arrangement.pad_high=12;boundary(1);receive_all();assert(seq.pad_channel_out==8 && receiver[8][0]);
    seq_notes_clear(&seq,&SINK);receive_all();use_real_ring=0;
    /* Full initial On is unowned; retry current triad after exactly three slots. */
    fresh();app_config();assert(command(14,0,0,0,8)==VR_QUEUED);assert(command(15,0,0,0,1)==VR_QUEUED);render_block();
    for(int b=0;b<2;b++)proj.pat[0].bass[b].step[0].flags=G_REST;
    memset(receiver,0,sizeof receiver);out_write=out_read=0;use_real_ring=1;fill_ring();seq.send_clock=0;
    seq_start(&seq);seq_advance_arrangement(&seq,0,&SINK);assert(seq.pad_channel_out==255 && seq_pad_pending(&seq));
    receive_one();receive_one();before=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==before);
    receive_one();seq_midi_sync(&seq,1,&SINK);receive_all();assert(seq.pad_channel_out==8);
    /* Reset/load/reinit retain owed old triad under a full queue; stopped retry Off only. */
    for(int operation=0;operation<3;operation++) {
        pad_fixture(60,48,84);seq_stop(&seq);seq_advance_arrangement(&seq,0,&SINK);receive_all();assert(project_save()==0);
        seq.playing=1;seq.pad_status=128;seq_midi_sync(&seq,1,&SINK);receive_all();assert(seq.pad_channel_out==8);fill_ring();seq.playing=0;
        if(operation==0)project_defaults();else if(operation==1)assert(project_load()==0);else engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
        assert(seq.pad_channel_out==8 && !seq.playing);receive_one();receive_one();before=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==before);
        receive_one();seq_midi_sync(&seq,1,&SINK);receive_all();assert(seq.pad_channel_out==255 && !receiver[8][60] && !receiver[8][64] && !receiver[8][67]);
        before=out_write;seq_midi_sync(&seq,1,&SINK);assert(out_write==before);
    }
    use_real_ring=0;
    puts("pad atomic: six-packet changed chord, newest retry, 3-packet retained Stop/disconnect cleanup, note0, collision and degree0 PASS");
}
static void persistence_commands(void) {
    fresh();app_config();undo_t journal;variant_snapshot_t v;assert(variant_snapshot(8,0,0,&v)==VR_OK && v.count==16 && v.value[0]==255);
    project_t before=proj;uint32_t rev=ui_revision(),q=engine_queue_written();
    for(int bad=0;bad<8;bad++) {
        int rc=bad==0?command(14,1,0,0,8):bad==1?command(14,0,1,0,8):bad==2?command(14,0,0,4,8):bad==3?command(14,0,0,0,16):bad==4?command(14,0,0,0,seq.ch[0]):bad==5?command(14,0,0,1,128):bad==6?command(14,0,0,2,85):command(15,0,0,0,2);
        assert(rc==VR_INVALID && !memcmp(&before,&proj,sizeof proj) && rev==ui_revision() && q==engine_queue_written());
    }
    for(int pending=0;pending<3;pending++) {seq.req_start=pending==0;seq.req_cont=pending==1;seq.req_stop=pending==2;assert(command(14,0,0,0,8)==VR_BUSY && command(15,0,0,0,1)==VR_BUSY && !memcmp(&before,&proj,sizeof proj));seq.req_start=seq.req_cont=seq.req_stop=0;}
    irq_start=1;assert(command(14,0,0,0,8)==VR_BUSY && !memcmp(&before,&proj,sizeof proj));seq.req_start=0;seq.playing=0;
    assert(command(14,0,0,0,8)==VR_QUEUED);assert(command(15,0,0,0,1)==VR_QUEUED);render_block();assert(proj.set.notes_out && seq.send_notes);assert(project_save()==0);assert(project_load()==0 && proj.variant.schema==4 && proj.variant.arrangement.pad_channel==8);
    for(int old=2;old<=3;old++) {proj.variant.schema=(uint16_t)old;proj.variant.arrangement.flags=old==2?1:19;proj.variant.arrangement.pad_channel=8;proj.variant.arrangement.pad_root=72;
        uint32_t packed;assert(stream_measure(RAW_LEN,&packed)==0 && save_stream(RAW_LEN,4,packed)==0);assert(project_load()==0 && proj.variant.schema==4 && proj.variant.arrangement.pad_channel==255 && proj.variant.arrangement.pad_root==72 && proj.variant.arrangement.flags==((old==2?1:19)|32));}
    for(int bad=0;bad<5;bad++) {
        fresh();app_config();if(bad==0)proj.variant.arrangement.pad_channel=16;else if(bad==1)proj.variant.arrangement.pad_root=128;else if(bad==2)proj.variant.arrangement.pad_low=85;else if(bad==3)proj.variant.schema=5;else proj.variant.bytes=411;
        uint32_t packed;assert(stream_measure(RAW_LEN,&packed)==0 && save_stream(RAW_LEN,4,packed)==0);variant_defaults(&proj.variant);before=proj;undo_t journal=undo;uint32_t writes=store_writes;seq.pad_channel_out=8;seq.pad_notes_out[0]=0;seq.pad_notes_out[1]=4;seq.pad_notes_out[2]=7;
        assert(project_load()==-8 && !memcmp(&before,&proj,sizeof proj) && !memcmp(&journal,&undo,sizeof undo) && store_writes==writes && seq.pad_channel_out==8 && seq.pad_notes_out[0]==0 && project_write_protected());
        seq.pad_channel_out=255;for(int i=0;i<3;i++)seq.pad_notes_out[i]=255;
    }
    fresh();app_config();assert(command(14,0,0,0,8)==VR_QUEUED);render_block();int ch=seq.ch[0];pref_set((pref_t){R_MIDICH,0},9);assert(seq.ch[0]==ch); /* physical collision rejected */
    /* Command15 is undo-tracked by schema4 bit5 although settings precede UNDO_OFF. */
    fresh();assert(!seq.send_notes && !proj.set.notes_out);assert(command(VC_BEGIN_EDIT,0,0,0,0)==VR_OK);
    assert(command(15,0,0,0,1)==VR_QUEUED && !seq.send_notes && !proj.set.notes_out && (proj.variant.arrangement.flags&32));
    assert(command(VC_END_EDIT,0,0,0,0)==VR_OK);render_block();assert(seq.send_notes && proj.set.notes_out);
    assert(command(VC_UNDO,0,0,0,0)==VR_OK);render_block();assert(!seq.send_notes && !proj.set.notes_out);
    assert(command(VC_REDO,0,0,0,0)==VR_OK);render_block();assert(seq.send_notes && proj.set.notes_out);
    seq.playing=1;journal=undo;assert(command(VC_UNDO,0,0,0,0)==VR_BUSY && !memcmp(&journal,&undo,sizeof undo));seq.playing=0;
    assert(project_save()==0 && get32(store[OBJ_PAT0])==2);assert(project_load()==0 && seq.send_notes && proj.set.notes_out && (proj.variant.arrangement.flags&32));
    assert(command(15,0,0,0,0)==VR_QUEUED);assert(command(VC_SAVE,0,0,0,0)==VR_OK);assert(project_load()==0);render_block();assert(!seq.send_notes && !proj.set.notes_out); /* save accepted intent before render */
    pref_set((pref_t){R_NOTEOUT,0},1);assert(seq.send_notes && proj.set.notes_out && (proj.variant.arrangement.flags&32));
    assert(engine_arrangement_refresh(&proj.variant.arrangement));render_block();assert(seq.send_notes && proj.set.notes_out);
    pref_set((pref_t){R_NOTEOUT,0},0);assert(engine_arrangement_refresh(&proj.variant.arrangement));render_block();assert(!seq.send_notes && !proj.set.notes_out);
    before=proj;rev=ui_revision();while(engine_strip_set(0,VP_BOOST,0)){};q=engine_queue_written();
    assert(command(15,0,0,0,1)==VR_BUSY && !memcmp(&before,&proj,sizeof proj) && ui_revision()==rev && engine_queue_written()==q && !seq.send_notes);
    render_block();
    puts("pad command/storage: schema4 same412B, migration2/3 disables old channel, bounded validation/IRQ/pending transport, protected malformed saves and physical channel collision PASS");
}
int main(void) {theory();atomic_cleanup();persistence_commands();printf("pad runtime host seq_t=%u savedvariant=%u pool allocations=0; target measurements pending\n",(unsigned)sizeof(seq_t),(unsigned)sizeof(variant_settings_t));return 0;}
