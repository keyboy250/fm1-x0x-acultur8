/* SPDX-License-Identifier: GPL-3.0-only */
/* Real production enqueue/USB consumer with simulated TIMER4 and endpoint. */
#define CLOCK_USB_NO_MAIN
#include "clock_usb_consumer_test.c"
#include <math.h>

static void deadline_cases(void)
{
    reset();midi_audio_begin(1000);plat_midi_audio_position(100);
    uint32_t due=midi_deadline_value(&midi_deadline);
    assert(midi_out_event(pk(250,0,0)));
    assert(midi_out_pair(pk(144,60,90),pk(128,61,0)));
    assert(midi_out_event(pk(248,0,0)));
    test_ticks=due-1;next();assert(nw==0&&mo_r==0);
    test_ticks=due;next();assert(nw==4&&status(wire[0])==250&&status(wire[3])==248);
    /* A stopped epoch immediately retires future deadlines and attacks,
     * keeping its owned Off and the newly published Stop in order. */
    reset();midi_audio_begin(1000);plat_midi_audio_position(200);
    assert(midi_out_event(pk(144,60,90)));assert(midi_out_event(pk(128,61,0)));
    assert(midi_out_event(pk(248,0,0)));midi_out_cancel(1);midi_audio_end();
    assert(midi_out_event(pk(252,0,0)));next();
    assert(nw==2&&status(wire[0])==128&&status(wire[1])==252);
    /* Counter wrap and bounded enqueue retain atomic pair/batch publication. */
    reset();midi_audio_begin(0xffff0000u);plat_midi_audio_position(511);
    due=midi_deadline_value(&midi_deadline);assert(due<0xffff0000u);
    assert(midi_out_event(pk(248,0,0)));test_ticks=due-1;next();assert(!nw);
    test_ticks=due;next();assert(nw==1);
    midi_deadline.base=0;midi_deadline.frame=midi_deadline.chunk=0;
    assert(midi_deadline_value(&midi_deadline)==1u);
    assert(!midi_deadline_ready(1u,0u)&&midi_deadline_ready(1u,1u));
    assert(midi_deadline_ready(100u,0x80000100u)); /* long endpoint blockage */
    reset();midi_audio_begin(1000);
    for(int i=0;i<63;i++)assert(midi_out_event(pk(248,0,0)));
    uint32_t w=mo_w,deadlines[64];memcpy(deadlines,midi_deadline.due,sizeof deadlines);
    assert(!midi_out_pair(pk(144,60,90),pk(128,61,0))&&mo_w==w);
    assert(!memcmp(deadlines,midi_deadline.due,sizeof deadlines));
    midi_audio_end();midi_usb_state(0,0,0);assert(!midi_out_event(pk(248,0,0)));
    for(int i=0;i<64;i++)assert(!midi_deadline.due[i]);
}

static void timing_case(double bpm)
{
    reset();
    const double sample_ticks=24000000.0/X0X_TIMING_RATE_HZ;
    const double clock_samples=(double)X0X_TIMING_RATE_HZ*60.0/(bpm*24.0);
    double pulse=0, next_block=0, worst=0;
    unsigned published=0, received=0, half=0;
    for(uint32_t now=0;received<180;now+=12000u) { /* actual 2kHz TX service */
        while(next_block<=now) {
            uint32_t start=half*512u;
            midi_audio_begin((uint32_t)llround(next_block));
            while(pulse<start+512 && published<180) {
                plat_midi_audio_position((uint32_t)floor(pulse)-start);
                assert(midi_out_event(pk(248,0,0)));
                pulse+=clock_samples;published++;
            }
            midi_audio_end();half++;next_block=half*512.0*sample_ticks;
        }
        test_ticks=now;unsigned old=nw;next();
        while(old<nw) {
            assert(status(wire[old++])==248);
            double expected=(512.0+received*clock_samples)*sample_ticks;
            double error=(now-expected)/24000.0;
            assert(error>-.03&&error<.53);
            if(error>worst)worst=error;
            received++;
        }
    }
    printf("PASS %.1fBPM:180 production USB clock handoffs, max scheduling lateness %.4fms (host/USB bus timing not simulated)\n",bpm,worst);
}

int main(void)
{
    deadline_cases();
    double bpm[]={20,60,120,134,240,300};
    for(unsigned i=0;i<sizeof bpm/sizeof bpm[0];i++)timing_case(bpm[i]);
    puts("PASS deadline future/ready/wrap, Start/notes/clock ordering, atomic full queue refusal, Stop cleanup, disconnect retirement");
    return 0;
}
