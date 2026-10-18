/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#define PLAT_CONTROL_TEST
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../firmware/src/app/engine.c"
static int locked,irq_start;
uint32_t plat_control_enter(void){assert(!locked);if(irq_start){irq_start=0;seq_start(&seq);seq_advance(&seq,0,&SINK);}locked=1;return 19;}
void plat_control_leave(uint32_t saved){assert(locked && saved==19);locked=0;}
static void render_block(void){int32_t b[512];engine_render(b,256);}
static void fresh(void){int i;for(i=0;i<OBJ_NOBJ;i++)store_prev_len[i]=store_len[i]=-1;memset(stored_known,0,sizeof stored_known);project_discard_rejected();midi_connected=1;boot();render_block();}
static int command(int op,int pattern,int lane,int index,int value){variant_command_t c={1,(uint16_t)op,91,(uint8_t)pattern,(uint8_t)lane,(uint8_t)index,0,value};variant_reply_t r;int rc=variant_command(&c,&r);assert(rc==r.result);return rc;}
static void storage(void)
{
    project_t before;variant_snapshot_t v;int p,t,i;uint32_t q;
    fresh();before=proj;
    assert(project_save()==0 && get32(store[OBJ_PAT0])==2);
    assert(!memcmp(&before.sound,&proj.sound,sizeof(sound_t)));before=proj;
    for(t=0;t<NTARGETS;t++)for(p=0;p<engine_nvoices(t);p++)for(i=0;i<engine_nparams(t,p);i++)assert(!poly_cell_owned((t*12+p)*16+i));
    assert(!poly_cell_owned((T_FX*12+1)*16));
    q=engine_queue_written();assert(command(18,0,0,0,4)==VR_INVALID && q==engine_queue_written());
    irq_start=1;assert(command(19,0,0,0,1)==VR_BUSY && !memcmp(&before,&proj,sizeof before));seq_stop(&seq);render_block();
    assert(command(19,0,0,0,1)==VR_QUEUED);render_block();assert(seq.lane_sound && poly_valid(proj.pat,(uint8_t *)&proj.sound));
    assert(command(3,0,0,0,0)==VR_OK);
    for(p=0;p<NPAT;p++)for(i=0;i<25;i++){assert(command(18,p,i,0,1+(p+i)%64)==VR_QUEUED);assert(command(18,p,i,1,(p+i)%8)==VR_QUEUED);render_block();}
    assert(command(18,31,24,2,63|64|(8<<7))==VR_QUEUED);
    assert(command(4,0,0,0,0)==VR_OK);render_block();
    assert(variant_snapshot(12,31,1,&v)==VR_OK && v.count==5 && v.value[0]==0x80000000u && v.value[4]==0x80000000u);
    before=proj;assert(project_save()==0 && get32(store[OBJ_PAT0])==5);assert(project_load()==0 && !memcmp(&before.sound,&proj.sound,sizeof(sound_t)) && !memcmp(before.pat,proj.pat,sizeof proj.pat));render_block();
    assert(command(18,1,7,0,64)==VR_QUEUED);render_block();assert(command(4,0,0,0,0)==VR_OK);assert(command(5,0,0,0,0)==VR_OK);render_block();assert(poly_length(proj.pat,(uint8_t *)&proj.sound,1,7)==9);
    assert(command(6,0,0,0,0)==VR_OK);render_block();assert(poly_length(proj.pat,(uint8_t *)&proj.sound,1,7)==64);
    before=proj;while(engine_strip_set(0,VP_BOOST,0)){};q=engine_queue_written();assert(command(18,1,7,0,2)==VR_BUSY && !memcmp(&before,&proj,sizeof before) && q==engine_queue_written());render_block();
    seq_start(&seq);assert(command(19,0,0,0,0)==VR_QUEUED);render_block();assert(command(18,1,7,0,5)==VR_QUEUED);seq_stop(&seq);render_block();
    before=proj;store[OBJ_SOUND][4+poly_sound_offset(0)]^=1;assert(project_load()==-8 && !memcmp(&before,&proj,sizeof before));
    puts("lane actual app: descriptor/TAPE isolation, pending-start, save/load/undo/full queue/live edits/protected cross-object mismatch PASS");
}
static void migrations(void)
{
    project_t before;undo_t journal;uint8_t shadow[sizeof undo_shadow];uint32_t measured=0,writes;int schema;
    for(schema=2;schema<=4;schema++){
        fresh();proj.variant.schema=(uint16_t)schema;proj.variant.arrangement.flags=(uint8_t)(schema==2?0:18);
        if(schema<4)proj.variant.arrangement.pad_channel=8;
        proj.sound.v[T_FX][7][7]=0x96; /* old inactive bytes must not be interpreted or erased */
        assert(stream_measure(RAW_LEN,RAW_OFF,&measured)==0);
        assert(save_obj(OBJ_SET,&proj.set,sizeof proj.set)==0 && save_obj(OBJ_SOUND,&proj.sound,sizeof proj.sound)==0 && save_stream(RAW_LEN,RAW_OFF,4,measured)==0);
        {int rc=project_load();if(rc)fprintf(stderr,"migration schema%d rc%d protected%d\n",schema,rc,project_write_protected());assert(rc==0 && proj.variant.schema==5 && !(proj.variant.arrangement.flags&POLY_FLAG));}
        assert(proj.sound.v[T_FX][7][7]==0x96 && (schema==4 || proj.variant.arrangement.pad_channel==255));render_block();
        proj.variant.schema=(uint16_t)schema;proj.variant.arrangement.flags=(uint8_t)(schema==2?2:schema==3?32:128);
        assert(stream_measure(RAW_LEN,RAW_OFF,&measured)==0 && save_stream(RAW_LEN,RAW_OFF,4,measured)==0);
        before=proj;journal=undo;memcpy(shadow,undo_shadow,sizeof shadow);writes=store_writes;
        assert(project_load()==-8 && !memcmp(&before,&proj,sizeof before) && !memcmp(&journal,&undo,sizeof journal) && !memcmp(shadow,undo_shadow,sizeof shadow));
        assert(writes==store_writes && !seq.transport_hold && !seq.transport_stage);
    }
    fresh();assert(command(19,0,0,0,1)==VR_QUEUED);render_block();
    ((uint8_t *)&proj.sound)[poly_sound_offset(4)]=2; /* CRC-valid unknown typed version */
    assert(stream_measure(LANE_RAW_LEN,LANE_RAW_OFF,&measured)==0 && save_obj(OBJ_SOUND,&proj.sound,sizeof proj.sound)==0 && save_stream(LANE_RAW_LEN,LANE_RAW_OFF,5,measured)==0);
    before=proj;writes=store_writes;assert(project_load()==-8 && !memcmp(&before,&proj,sizeof before) && writes==store_writes);
    assert(!seq.transport_hold && !seq.transport_stage);
    puts("lane storage: full old-schema2/3/4 migrations preserve inactive bytes; CRC-valid reserved flags/typed version reject before journal/live/storage mutation PASS");
}
static void preserved_modes_and_copies(void)
{
    uint8_t slab[POLY_BYTES];bpart_t bass;int i,p,l;variant_snapshot_t view;
    fresh();assert(command(19,0,0,0,1)==VR_QUEUED);render_block();
    for(p=0;p<2;p++)for(l=0;l<25;l++){assert(command(18,p,l,0,1+(p*7+l*3)%64)==VR_QUEUED);assert(command(18,p,l,1,(p+l)%8)==VR_QUEUED);}
    render_block();
    for(i=0;i<64;i++)assert(command(18,0,24,2,i|((i&1)<<6)|((i%9)<<7))==VR_QUEUED);
    render_block();bass=proj.pat[0].bass[0];
    for(i=0;i<(int)POLY_BYTES;i++)slab[i]=((uint8_t *)&proj.sound)[poly_sound_offset((unsigned)i)];
    /* Actual UI macro side-effects must never touch slab-owned cells. */
    pref_set(PR(R_TAPE,0,0,0),100);pref_set(PR(R_ENG,T_MST,0,MST_COMP1),90);pref_set(PR(R_ENG,T_MST,0,MST_DJF),44);
    for(i=0;i<(int)POLY_BYTES;i++)assert(slab[i]==((uint8_t *)&proj.sound)[poly_sound_offset((unsigned)i)]);
    assert(command(3,0,0,0,0)==VR_OK && command(19,0,0,0,0)==VR_QUEUED && command(4,0,0,0,0)==VR_OK);render_block();assert(seq.lane_sound && !seq.lane_enabled);
    for(p=0;p<2;p++)for(l=0;l<25;l++){assert(poly_length(proj.pat,(uint8_t *)&proj.sound,p,l)==1+(p*7+l*3)%64);assert(poly_rate(proj.pat,(uint8_t *)&proj.sound,p,l)==(p+l)%8);}
    assert(command(5,0,0,0,0)==VR_OK);render_block();assert(seq.lane_enabled);assert(command(6,0,0,0,0)==VR_OK);render_block();assert(!seq.lane_enabled);
    assert(project_save()==0 && project_load()==0);render_block();assert(seq.lane_sound && !seq.lane_enabled);
    assert(command(19,0,0,0,1)==VR_QUEUED);render_block();assert(seq.lane_enabled && !memcmp(&bass,&proj.pat[0].bass[0],sizeof bass));
    /* Copy one kit: other kit, bass and BREAK packed timing must survive. */
    assert(engine_transport_reserve());assert(copy_part(1,0,0)==0);engine_transport_release();
    for(l=0;l<25;l++)assert(poly_length(proj.pat,(uint8_t *)&proj.sound,1,l)==(l<11?1+l*3%64:1+(7+l*3)%64));
    assert(engine_transport_reserve());assert(copy_part(1,0,4)==0);engine_transport_release();
    for(i=0;i<64;i++){assert(poly_break_gate(proj.pat,(uint8_t *)&proj.sound,1,i)==(i&1));assert(poly_break_slice(proj.pat,(uint8_t *)&proj.sound,1,i)==i%9);}
    edit_clear_part(&proj,&proj.pat[1],4);for(i=0;i<64;i++){assert(!poly_break_gate(proj.pat,(uint8_t *)&proj.sound,1,i));assert(!poly_break_slice(proj.pat,(uint8_t *)&proj.sound,1,i));}
    seq_start(&seq);render_block();assert(command(19,0,0,0,0)==VR_QUEUED);render_block();assert(seq.lane_enabled);
    assert(variant_snapshot(11,0,0,&view)==VR_OK && view.value[1]==0 && view.value[2]==1 && view.value[3]==1);
    seq.arrangement.tick=95;seq.arrangement.clock_started=1;seq.clk_to_next=0;seq_advance_poly(&seq,0,&SINK);assert(!seq.lane_enabled);
    assert(command(19,0,0,0,1)==VR_QUEUED);render_block();assert(!seq.lane_enabled);seq.arrangement.tick=95;seq.clk_to_next=0;seq_advance_poly(&seq,0,&SINK);assert(seq.lane_enabled);
    seq_stop(&seq);render_block();assert(poly_valid(proj.pat,(uint8_t *)&proj.sound));
    puts("lane preserving modes: Off/On/reopen/undo, real macro spans, whole-part copy/clear isolation, live next-bar mode latch PASS");
}
static pattern_t patterns[NPAT];static uint8_t sound[sizeof(sound_t)];static seq_t lane_seq;
static unsigned counts[25],last[25];static unsigned now;
static int hit(void *x,int kit,int voice,float vel){int lane=kit*11+voice;(void)x;(void)vel;counts[lane]++;last[lane]=now;return 1;}
static void bon(void *x,int part,int note,int accent,int slide){(void)x;(void)note;(void)accent;(void)slide;counts[22+part]++;last[22+part]=now;}
static void brk_event(void *x,int step,int bar,float samples,int enabled){(void)x;(void)step;(void)bar;(void)samples;if(enabled){counts[24]++;last[24]=now;}}
static const seq_sink_t sink={.drum=hit,.bass_on=bon,.brk=brk_event};
static void fixture(void)
{
    int p,v,step;memset(sound,0,sizeof sound);memset(counts,0,sizeof counts);memset(last,0,sizeof last);now=0;
    for(p=0;p<32;p++)pattern_init(&patterns[p],1,2);
    poly_enable(patterns,sound);seq_init(&lane_seq,patterns);lane_seq.lane_sound=sound;lane_seq.lane_enabled=1;lane_seq.bpm=120;lane_seq.send_clock=0;
    for(v=0;v<22;v++)for(step=0;step<64;step++)sm_set(patterns[0].drum[v/11].hit[v%11],step);
    for(v=0;v<2;v++)for(step=0;step<64;step++){patterns[0].bass[v].step[step].flags=G_NOTE;patterns[0].bass[v].step[step].note=48+step%12;}
    for(step=0;step<64;step++)poly_break_set(patterns,sound,0,step,1,step%9);
}
static void run(unsigned samples)
{unsigned end=now+samples;while(now<end){unsigned k=seq_until_event_poly(&lane_seq,end-now);now+=k;seq_advance_poly(&lane_seq,k,&sink);}}
static void scheduling(void)
{
    static const unsigned ticks[8]={6,4,3,8,24,12,16,2};int lane,rate;unsigned duration=44100;
    fixture();for(lane=0;lane<25;lane++)poly_set(patterns,sound,0,lane,3+lane%13,lane%8);
    seq_start(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);run(duration);
    for(lane=0;lane<25;lane++){unsigned expected=1+(duration*24u)/(22050u*ticks[lane%8]);assert(counts[lane]==expected);}
    /* Every length wraps independently; each period is separately observable. */
    for(rate=0;rate<8;rate++){fixture();poly_set(patterns,sound,0,10,3,rate);seq_start(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);run(44100);assert(seq_lane_cycles(&lane_seq,10)==(counts[10]-1)/3);}
    /* Current interval retains the old rate, then next event latches new rate. */
    fixture();seq_start(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);run(1000);poly_set(patterns,sound,0,1,5,RATE_4);
    run(4511);assert(counts[1]==1);run(1);assert(counts[1]==2);run(22049);assert(counts[1]==2);run(1);assert(counts[1]==3);
    /* External old-rate phase must not move when a new timing value arrives. */
    fixture();lane_seq.ext=1;lane_seq.ext_period=235200;seq_start(&lane_seq);seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);assert(counts[1]==1);
    for(lane=0;lane<2;lane++){seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);}poly_set(patterns,sound,0,1,7,RATE_4);
    for(lane=0;lane<3;lane++){seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);assert(counts[1]==1);}seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);assert(counts[1]==2);
    for(lane=0;lane<23;lane++){seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);assert(counts[1]==2);}seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);assert(counts[1]==3);
    fixture();lane_seq.ext=1;lane_seq.ext_period=235200;
    for(lane=0;lane<25;lane++)poly_set(patterns,sound,0,lane,64,lane%8);
    seq_start(&lane_seq);for(lane=0;lane<49;lane++)seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);
    for(lane=0;lane<25;lane++)assert(counts[lane]==1+48/ticks[lane%8]);
    puts("lane schedule: all25 independent lengths/eight periods, cycle counts, next-local-step live changes, external latched phase PASS");
}
static void transitions(void)
{
    song_t song;arrangement_settings_t a;int lane,p,before;
    fixture();memset(&song,0,sizeof song);arrangement_defaults(&a);a.flags=ARR_ENABLED;song.len=2;song.bar[1].pat[1]=1;
    lane_seq.song=&song;seq_arrangement_apply(&lane_seq,&a);a.degree[1]=4;poly_set(patterns,sound,0,1,7,RATE_16T);poly_set(patterns,sound,0,12,5,RATE_32);
    seq_start(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);run(3000);
    before=seq_lane_position(&lane_seq,1);lane_seq.arrangement.clock_started=1;lane_seq.arrangement.tick=95;lane_seq.arrangement.queued=1;lane_seq.clk_to_next=0;
    for(p=0;p<NTRACKS;p++)lane_seq.t[p].to_next=0xffffffffu;
    for(p=0;p<20;p++)lane_seq.lane[p].to_next=0xffffffffu;
    lane_seq.lane[0].to_next=0;seq_advance_poly(&lane_seq,0,&sink);
    assert(lane_seq.song_pos==1 && lane_seq.arrangement.degree==4 && seq_lane_position(&lane_seq,1)==(before+1)%7);
    for(lane=11;lane<22;lane++)assert(seq_lane_position(&lane_seq,lane)==0); /* changed kit only */
    /* Continue keeps positions/cycles and advances, rather than restarting. */
    before=seq_lane_position(&lane_seq,1);seq_stop(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);seq_continue(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);assert(seq_lane_position(&lane_seq,1)==(before+1)%7);
    /* Pitched ties keep the actual held pitch through a live timing change. */
    fixture();patterns[0].bass[0].len=4;patterns[0].bass[0].step[0].flags=G_NOTE|BS_SLIDE;patterns[0].bass[0].step[1].flags=G_TIE;patterns[0].bass[0].step[1].note=100;
    seq_start(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);run(1000);poly_set(patterns,sound,0,22,4,RATE_4);run(4512);
    assert(counts[22]==1 && lane_seq.t[TRK_BASS0].sounding && (lane_seq.t[TRK_BASS0].held_note&127)==48);
    /* Lost-clock fallback retains the remaining external local phase. */
    fixture();lane_seq.ext=1;lane_seq.ext_period=235200;seq_start(&lane_seq);seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);
    for(p=0;p<2;p++){seq_ext_clock(&lane_seq);seq_advance_poly(&lane_seq,0,&sink);}lane_seq.ext_since=FS/2;seq_advance_poly(&lane_seq,1,&sink);
    assert(!lane_seq.ext && counts[1]==1);run(3600);assert(counts[1]==1);run(100);assert(counts[1]==2);
    puts("lane transitions: musical boundary before events; unchanged rows continue/changed kit restarts; Continue/tie/fallback phase PASS");
}
int main(void){storage();migrations();preserved_modes_and_copies();scheduling();transitions();puts("individual lane focused native PASS");return 0;}
