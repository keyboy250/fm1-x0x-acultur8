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
static uint32_t chunk_start,event_frame,clock_frame[20000],clock_n;
static void event_position(uint32_t frame){assert(frame<=256);event_frame=chunk_start+frame;}
static int timed_out(uint32_t packet){
    if(((packet>>8)&255u)==248u){assert(clock_n<20000);clock_frame[clock_n++]=event_frame;}
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
static void run_case(float bpm,unsigned pulses)
{
    project_defaults();boot();seq.bpm=bpm;seq.send_clock=1;seq.send_notes=0;seq.ext=0;
    seq_midi_prepare(&seq,1,&SINK);seq_start(&seq);int32_t audio[512];clock_n=0;
    for(chunk_start=0;clock_n<pulses;chunk_start+=256)engine_render(audio,256);
    double expected=(double)X0X_TIMING_RATE_HZ*60.0/(seq_tempo(&seq)*24.0),worst=0;
    for(uint32_t i=1;i<clock_n;i++){
        double error=fabs((double)(clock_frame[i]-clock_frame[i-1])-expected);
        if(error>worst)worst=error;assert(error<1.1);
    }
    double elapsed=(double)(clock_frame[clock_n-1]-clock_frame[0]);
    double ideal=(clock_n-1)*expected, measured=(clock_n-1)*(double)X0X_TIMING_RATE_HZ*60.0/(24.0*elapsed);
    assert(fabs(elapsed-ideal)<clock_n/200.0+2.0);
    assert(fabs(measured-bpm)/bpm<.00001);
    printf("PASS profile=%u tempo=%.1f clocks=%u frames=%.0f physical_BPM=%.9f accumulated_error_samples=%.6f period_error_samples=%.6f\n",X0X_TIMING_RATE_HZ,bpm,clock_n,elapsed,measured,elapsed-ideal,worst);
}
int main(void)
{
    float tempos[]={20,60,120,134,240,300};
    for(unsigned i=0;i<6;i++)run_case(tempos[i],(unsigned)(tempos[i]*24));
    run_case(134,16000);
    return 0;
}
