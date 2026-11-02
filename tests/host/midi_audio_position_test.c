/* SPDX-License-Identifier: GPL-3.0-only */
/* Real engine render + sequencer: verify the platform receives each event's
 * sample position, including the second chunk of a512-frame DMA half. */
#define _POSIX_C_SOURCE 200809L
#define X0X_WEB 1
#include <stdint.h>
static void web_audio(const int32_t *p,uint32_t n){(void)p;(void)n;}
#include "../../host/x0x_host.c"
#include <assert.h>
#include <math.h>
void web_seq_tick(uint32_t n){(void)n;}
void web_seq_transport(int start,int top){(void)start;(void)top;}
void web_slice_audio(uint32_t n){(void)n;}
static uint32_t chunk_start,event_frame,clock_frame[256],clock_n;
static void event_position(uint32_t frame){assert(frame<=256);event_frame=chunk_start+frame;}
static int timed_out(uint32_t packet){
    if(((packet>>8)&255u)==248u){assert(clock_n<256);clock_frame[clock_n++]=event_frame;}
    return 1;
}
static int timed_pair(uint32_t a,uint32_t b){timed_out(a);timed_out(b);return 1;}
static int timed_batch(const uint32_t *p,uint32_t n){for(uint32_t i=0;i<n;i++)timed_out(p[i]);return 1;}
#undef plat_midi_audio_position
#define plat_midi_audio_position(frame) event_position(frame)
#define plat_midi_out timed_out
#define plat_midi_pair timed_pair
#define plat_midi_batch timed_batch
#include "../../firmware/src/app/engine.c"
int main(void)
{
    project_defaults();boot();seq.bpm=134.0f;seq.send_clock=1;seq.send_notes=0;seq.ext=0;
    seq_midi_prepare(&seq,1,&SINK);seq_start(&seq);
    int32_t audio[512];clock_n=0;
    for(chunk_start=0;clock_n<100;chunk_start+=256)engine_render(audio,256);
    double expected=44100.0*60.0/(seq_tempo(&seq)*24.0),worst=0;
    for(uint32_t i=1;i<clock_n;i++){
        double delta=(double)(clock_frame[i]-clock_frame[i-1]);
        double error=fabs(delta-expected);if(error>worst)worst=error;
        assert(error<1.1);
    }
    printf("PASS real engine: %u clock events carry sample offsets across256-frame chunks; max period error %.4f samples\n",clock_n,worst);
    return 0;
}
