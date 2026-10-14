/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual browser C FIFO failure/retry acceptance; no browser/receiver claim. */
#define CC_REFERENCE_TABLES 1
#define _POSIX_C_SOURCE 200809L
#define X0X_WEB 1
#include <stdint.h>
static void web_audio(const int32_t *p,uint32_t n){(void)p;(void)n;}
#include "../../host/x0x_host.c"
#include <assert.h>
void web_seq_tick(uint32_t n){(void)n;}
void web_seq_transport(int start,int top){(void)start;(void)top;}
void web_slice_audio(uint32_t n){(void)n;}
#include "../../firmware/src/app/engine.c"
static void render(void){int32_t out[512];engine_render(out,256);}
static void consume(void){while(host_web_midi_pending()){int n=host_web_midi_batch(16);assert(n>0);assert(!host_web_midi_ack(n));}}
static void direct(int id,int value){assert(cc_parameter((uint16_t)id,(uint32_t)value,0)==0);render();}
static uint32_t control(int number,int value){int c=number%88;return 11u|((uint32_t)(0xb0+number/88)<<8)|((uint32_t)(c<32?c:c+32)<<16)|((uint32_t)value<<24);}
static void enqueue(uint32_t p){assert(cc_ingress_put(min_q,&mi_w,&mi_r,MQ,p,0));}
static uint32_t note(int channel){return 9u|((uint32_t)(0x90+channel)<<8)|(60u<<16)|(100u<<24);}
static void ordering(void){
    /* Both directions use the real audio dispatcher, not direct packet reads. */
    seq.ch[TRK_BASS0]=2;bass_held[0]=255;
    enqueue(note(2));enqueue(control(954,24)); /* external channel3, internal2 */
    plat_midi_service();assert(seq.ch[TRK_BASS0]==2);render();assert(bass_held[0]==60);
    plat_midi_service();render();assert(bass_held[0]==255);
    seq.ch[TRK_BASS0]=2;
    enqueue(control(954,32));enqueue(note(3));plat_midi_service();render();assert(seq.ch[TRK_BASS0]==3 && bass_held[0]==60);
    /* Dense non-CC bursts publish 32 events per service, not one per block. */
    for(int i=0;i<64;i++)enqueue((note(3)&~(127u<<16))|((uint32_t)(40+i%40)<<16));
    enqueue(control(954,24));enqueue(note(2));uint32_t begin=mi_r;
    plat_midi_service();assert(mi_ready==begin+32 && seq.ch[TRK_BASS0]==3);render();assert(bass_held[0]==71);
    plat_midi_service();assert(mi_ready==begin+64 && seq.ch[TRK_BASS0]==3);render();assert(bass_held[0]==63);
    plat_midi_service();render();assert(seq.ch[TRK_BASS0]==2 && bass_held[0]==60 && mi_r==mi_w);
    /* Stop then CC Play must restart; opposite order must stop. */
    enqueue(15u|(0xfcu<<8));enqueue(control(1119,127));plat_midi_service();render();assert(!seq.playing);
    plat_midi_service();render();assert(seq.playing);
    enqueue(control(1119,0));plat_midi_service();render();
    enqueue(control(1119,127));enqueue(15u|(0xfcu<<8));plat_midi_service();render();assert(!seq.playing);
    /* Clock is a boundary even while stopped; later CC cannot cross it. */
    enqueue(15u|(0xf8u<<8));enqueue(control(0,83));int old=proj.sound.v[T_909][0][0];
    plat_midi_service();assert(proj.sound.v[T_909][0][0]==old);render();plat_midi_service();render();assert(proj.sound.v[T_909][0][0]==83);
    /* Every framing target is rejected without recursive execution or musical
     * mutation; the next valid musical transaction still reaches its setter. */
    project_t unchanged=proj;
    for(unsigned i=0;i<CC_BINDINGS_COUNT;i++)if((cc_binding_words[i]&31u)==CCK_EXACT || (cc_binding_words[i]&31u)==CCK_SELECTOR){
        direct(1010,0);direct(1071,1);
        direct(1072,cc_numbers[i]&127);direct(1073,cc_numbers[i]>>7);
        direct(1074,1);direct(1075,0);direct(1130,0);
        assert(cc_parameter(1077,1,0)==CC_REJECTED && cc_sources[0].exact_parts==0);
        assert(!memcmp(&proj,&unchanged,sizeof proj));
    }
    /* A complete exact value retains all chunks when the real CQ is full. */
    direct(1010,0);direct(1071,1);direct(1072,0);direct(1073,0);
    direct(1074,33);direct(1075,0);direct(1130,0);
    while(engine_queue_space())assert(cq_put(C_STRIP,255,0,0));
    enqueue(control(1077,127));plat_midi_service();assert(cc_sources[0].exact_parts==7 && mi_ready!=mi_w);
    render();plat_midi_service();assert(cc_sources[0].exact_parts==0 && proj.sound.v[T_909][0][0]==33);render();assert(mi_r==mi_w);
}
int main(void){
    for(int i=0;i<OBJ_NOBJ;i++)store_len[i]=-1;project_discard_rejected();boot();render();host_web_midi_ready(1);
    assert(host_web_midi_ingest(11u|(0xb0u<<8)|(127u<<24),2)==0);render();assert(proj.sound.v[T_909][0][0]==127);
    assert(host_web_midi_ingest(11u|(0xb0u<<8)|(128u<<24),2)==CC_REJECTED);
    assert(host_web_midi_ingest(11u|(0xb0u<<8),3)==CC_REJECTED);
    direct(1010,0);direct(1013,0);direct(1071,1);
    direct(1052,2);direct(1056,1);direct(1051,1);direct(910,1);direct(1064,8);
    proj.pat[0].bass[0].step[0]=(bstep_t){64,G_NOTE|BS_ACCENT|BS_SLIDE};
    for(int i=1;i<NSTEPS;i++)proj.pat[0].bass[0].step[i]=(bstep_t){64,G_TIE|BS_SLIDE};
    seq.send_clock=0;seq_start(&seq);render();assert(seq.t[TRK_BASS0].sounding);
    assert(seq.t[TRK_BASS0].source_note==64 && (seq.t[TRK_BASS0].held_note&127u)==64);
    assert(seq.pad_channel_out==8 && seq.pad_notes_out[0]==60);consume();
    /* Changing follow while held uses the accepted original NOTE; Off restores it. */
    direct(1061,BASS_FOLLOW_ROOT);assert((seq.t[TRK_BASS0].held_note&127u)==60);consume();
    direct(1061,BASS_FOLLOW_OFF);assert((seq.t[TRK_BASS0].held_note&127u)==64);consume();
    direct(1061,BASS_FOLLOW_ROOT);consume();
    /* Follow changes cannot overtake a preceding actual external-clock event. */
    enqueue(15u|(0xf8u<<8));enqueue(control(1061,0));
    plat_midi_service();assert(((proj.variant.arrangement.flags>>1)&3u)==BASS_FOLLOW_ROOT);render();
    assert((seq.t[TRK_BASS0].held_note&127u)==60);
    plat_midi_service();render();assert((seq.t[TRK_BASS0].held_note&127u)==64);consume();
    enqueue(control(1061,64));enqueue(15u|(0xf8u<<8));
    plat_midi_service();render();assert(((proj.variant.arrangement.flags>>1)&3u)==BASS_FOLLOW_ROOT);
    assert((seq.t[TRK_BASS0].held_note&127u)==60);consume();
    /* Saturated old-route Off cleanup cannot vanish on pad-channel change. */
    while(host_web_midi_pending()<64)assert(plat_midi_out(0x0000f80fu));
    direct(1064,7);assert(seq.pad_channel_out==8);assert(seq_pad_pending(&seq));
    /* Exactly6 spaces allow old-channel three Offs before new-channel triad. */
    assert(host_web_midi_batch(16)==16);assert(host_web_midi_ack(6)==0);assert(host_web_midi_ack(10)==0);render();
    assert(seq.pad_channel_out==7);consume();
    /* Narrow strict range rejects held C-chord note and no TIE/CC restores it. */
    direct(1062,61);direct(1063,61);assert(!seq.t[TRK_BASS0].sounding);consume();
    direct(1063,84);direct(1062,48);assert(!seq.t[TRK_BASS0].sounding);
    /* Pad/part channel collision leaves canonical configuration unchanged. */
    int old=proj.variant.arrangement.pad_channel;assert(cc_parameter(1064,seq.ch[0],0)==CC_REJECTED);assert(proj.variant.arrangement.pad_channel==old);
    /* Validated CC edits have the same stopped save/reopen and undo owner. */
    seq_stop(&seq);render();consume();ui_external_edit(1);direct(0,10);ui_external_edit(0);
    assert(ui_external_undo(0)==VR_OK);render();assert(proj.sound.v[T_909][0][0]!=10);
    assert(ui_external_undo(1)==VR_OK);render();assert(proj.sound.v[T_909][0][0]==10);
    assert(project_save()==0);proj.sound.v[T_909][0][0]=99;assert(project_load()==0);assert(proj.sound.v[T_909][0][0]==10);
    ordering();
    puts("Live CC native debt: running follow reprojection/Off restoration, saturated original-pad-channel cleanup, strict range/no TIE revival, collision rejection, canonical undo/redo/save/reopen PASS");return 0;
}
