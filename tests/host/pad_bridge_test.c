/* SPDX-License-Identifier: GPL-3.0-only */
/* Native compile of the real browser-only FIFO and engine cancellation hooks. */
#define _POSIX_C_SOURCE 200809L
#define X0X_WEB 1
#include <stdint.h>
static void web_audio(const int32_t *p,uint32_t n) {(void)p;(void)n;}
#include "../../host/x0x_host.c"
#include <assert.h>
void web_seq_tick(uint32_t n) {(void)n;}
void web_seq_transport(int start,int top) {(void)start;(void)top;}
void web_slice_audio(uint32_t n) {(void)n;}
#include "../../firmware/src/app/engine.c"
static uint32_t packet(int status,int note,int velocity) {return (uint32_t)(status>>4)|((uint32_t)status<<8)|((uint32_t)note<<16)|((uint32_t)velocity<<24);}
int main(void) {
    assert(!plat_midi_connected() && !plat_midi_out(packet(0x90,60,90)));
    assert(host_web_midi_ready(2)==-1 && host_web_midi_batch(0)==-1 && host_web_midi_batch(65)==-1);
    assert(host_web_midi_ready(1)==0);web_midi_w=web_midi_r=0xfffffffdu;
    uint32_t p[]={packet(0x90,60,90),packet(0x90,61,0),packet(0x80,60,0),0x0000f80fu};
    for(int i=0;i<4;i++)assert(plat_midi_out(p[i]));
    assert(host_web_midi_batch(16)==4);
    uint32_t posted[4];for(int i=0;i<4;i++)posted[i]=host_web_midi_packet(i);
    uint32_t epoch=host_web_midi_epoch();assert(host_web_midi_ready(0)==0 && host_web_midi_epoch()==epoch+1);
    assert(host_web_midi_packet(0)==(posted[0]|0x80000000u));for(int i=1;i<4;i++)assert(host_web_midi_packet(i)==posted[i]);
    /* Router had a posted in-flight batch: dispositions cancel its unsent On,
       then accepted-prefix ACK shifts remaining Off/clock without replay. */
    assert(host_web_midi_ack(5)==-1 && host_web_midi_ack(-1)==-1 && host_web_midi_pending()==4);
    assert(host_web_midi_ack(1)==0 && host_web_midi_packet(0)==p[1] && host_web_midi_batch(16)==-1);
    assert(host_web_midi_ready(1)==0);uint32_t fresh=packet(0x90,62,90);assert(plat_midi_out(fresh));
    assert(host_web_midi_packet(2)==p[3] && host_web_midi_packet(3)==0);assert(host_web_midi_ack(3)==0);
    assert(host_web_midi_batch(16)==1 && host_web_midi_packet(0)==fresh);assert(host_web_midi_ack(1)==0);
    /* Complete batch publication across wrap rejects at free5, accepts at free6. */
    uint32_t tx[6];for(int i=0;i<6;i++)tx[i]=packet(0x98,40+i,90);
    for(int i=0;i<59;i++)assert(plat_midi_out(0x0000f80fu));
    uint32_t w=web_midi_w;
    assert(!plat_midi_batch(tx,6) && web_midi_w==w);assert(host_web_midi_batch(16)==16 && host_web_midi_ack(1)==0);
    assert(plat_midi_batch(tx,6) && web_midi_w==w+6);assert(!plat_midi_out(fresh));assert(host_web_midi_ack(15)==0);
    while(host_web_midi_pending()){int n=host_web_midi_batch(16);assert(n>0 && n<=16);assert(host_web_midi_ack(n)==0);}
    /* Actual Stop and engine cleanup cancel queued old attacks, not later audition. */
    boot();seq.send_clock=0;seq.send_notes=1;seq.playing=1;assert(plat_midi_out(packet(0x90,70,90)));
    epoch=host_web_midi_epoch();seq_stop(&seq);seq_advance(&seq,0,&SINK);assert(host_web_midi_epoch()>epoch);
    assert(host_web_midi_batch(16)>=1 && (host_web_midi_packet(0)&0x80000000u));assert(host_web_midi_ack((int)web_midi_latched)==0);
    assert(plat_midi_out(packet(0x90,71,90)));assert(host_web_midi_batch(16)==1 && !(host_web_midi_packet(0)&0x80000000u));
    engine_notes_clear();assert(host_web_midi_packet(0)&0x80000000u);assert(host_web_midi_ack(1)==0);
    assert(plat_midi_out(packet(0x90,72,0)));epoch=host_web_midi_epoch();project_defaults();assert(host_web_midi_epoch()>epoch);
    assert(host_web_midi_batch(16)==1 && !(host_web_midi_packet(0)&0x80000000u));assert(host_web_midi_ack(1)==0);
    puts("browser FIFO native: explicit detached default, prefix ACK/in-flight cancellation, new epoch audition, velocity0/Off/clock preservation, wrap/full/6atomic and actual Stop/reset hooks PASS");return 0;
}
