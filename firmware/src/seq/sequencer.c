/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X sequencer; see sequencer.h for the model. */
#include "sequencer.h"
#include "tb3po.h"
#include "rate_families.h"
#include "../app/plat.h"

#ifdef X0X_WEB
extern void web_seq_transport(int playing, int from_top);
extern void web_seq_tick(uint32_t period_q8);
#define WEB_CLOCK 1
#else
#define WEB_CLOCK 0
#endif

#define NEVER 0xFFFFFFFFu
#ifdef CLOCK_RUN_REQUEST_TEST_HOOK
extern void clock_run_request_boundary(seq_t *s);
#define RUN_REQUEST_BOUNDARY(s) clock_run_request_boundary(s)
#else
#define RUN_REQUEST_BOUNDARY(s) ((void)0)
#endif

/* Shared step/clock helpers are outlined to prevent playback paths from
 * cloning their bodies. This trades local
 * instruction duplication for explicit calls; target/physical cost is measured
 * separately and no legacy limit is weakened. */
static const uint8_t TICKS_PER_STEP[NRATES] = {6, 4, 3, 8, 24, 12, 16, 2};     /* 24 ppqn: 1/16, 1/16T, 1/32, 1/8T */
static const uint8_t GM_DRUM[NKIT][NDRUM] = {
    {36, 38, 41, 45, 50, 37, 39, 42, 46, 49, 51},   /* 909: BD SD LT MT HT RS CP CH OH CR RD */
    {36, 38, 41, 45, 50, 37, 39, 56, 49, 46, 42},   /* 808: BD SD LT MT HT RS CP CB CY OH CH */
};

void seq_init(seq_t *s, pattern_t *patterns)
{
    uint32_t i;
    uint8_t *p = (uint8_t *)s;
    for (i = 0; i < sizeof *s; i++)
        p[i] = 0;
    s->pat = patterns;
    s->clock.age=UINT32_MAX;
    for (i = 0; i < NTRACKS; i++)
        s->pcue[i] = 0xFF;
    s->bpm = 125.0f;
    s->ext_bpm = 125.0f;
    s->accent_q7 = 88;                /* a 909 non-accented hit sits ~3 dB under an accent */
    s->send_clock = 1;
    {
        static const uint8_t CH[NTRACKS] = {9, 10, 1, 2, 3};   /* 909 10, 808 11, 303s 2 / 3, break 4 */
        for (i = 0; i < NTRACKS; i++)
            s->ch[i] = CH[i];
    }
    for (i = 0; i < NTRACKS; i++) {
        s->t[i].to_next = NEVER;
        s->t[i].pp_dir = 1;
        s->t[i].rng = 0x1234567u * (i + 1);
    }
    s->clk_to_next = NEVER;
    s->arrangement.queued = 255;
    s->midi_ready=1;
    s->pad_channel_out=255;for(i=0;i<3;i++)s->pad_notes_out[i]=255;
    for(i=0;i<NBASS;i++)s->bass_notes_out[i]=s->bass_channels_out[i]=255;
}

static int arrangement_active(const seq_t *s)
{
    return (s->arrangement.flags & ARR_ENABLED) != 0;
}
float seq_tempo(const seq_t *s) { return s->ext ? s->ext_bpm : s->bpm; }

/* LAN2 countdowns retain exact rational units in the existing uint32 timers.
 * Internal: numerator FS*150*pulses4, denominator BPM-tenths*24.
 * External: quarter-Q8 sample units, preserving fractional MIDI-pulse steps.
 * Legacy LAN1 timers and IDs keep their existing Q8 path unchanged. */
static __attribute__((noinline)) int expanded(const seq_t *s){return s->lane_sound && poly_expanded(s->lane_sound);}
static uint8_t *rate_clock(seq_t *s,int lane){return lane<22 && lane%11?&s->lane[(lane/11)*10+lane%11-1].ext_ticks:&s->t[poly_part(lane)].ext_ticks;}
/* The denominator belongs to each interval. Tempo/source changes never
 * reinterpret or rescale the remaining countdown. Only a local step may adopt
 * current timing, so legal slow-to-fast changes cannot overflow uint32. */
static inline uint32_t rate_unit(const seq_t *s,int lane){return s->rate_units[lane]?(uint32_t)s->rate_units[lane]*24u:1024u;}
static uint32_t adopt_rate_unit(seq_t *s,int lane,uint32_t carry){unsigned bpm=(unsigned)(s->bpm*10.0f+0.5f);uint32_t old=rate_unit(s,lane),next=s->ext?0u:bpm<200?200u:bpm>3000?3000u:bpm;
 s->rate_units[lane]=(uint16_t)next;
 /* carry is less than one sample, never a long remaining interval. */
 return old==rate_unit(s,lane)?carry:(uint32_t)((float)carry*(float)rate_unit(s,lane)/(float)old);
}
static __attribute__((noinline)) uint32_t rate_interval(const seq_t *s,int rate){if(!expanded(s))return seq_step_q8(s,rate);return s->ext?(s->ext_period?s->ext_period:seq_step_q8(s,RATE_16)/6u)*rf_divisions[rate].pulses4:(uint32_t)FS*150u*rf_divisions[rate].pulses4;}
void seq_lane_mark_pending(seq_t *s,int lane,int rate){s->lane_pending|=1u<<lane;if(rate && expanded(s))*rate_clock(s,lane)=(uint8_t)((*rate_clock(s,lane)&31u)|32u);}
static void rate_bar(seq_t *s){int lane;if(expanded(s))for(lane=0;lane<25;lane++)if(*rate_clock(s,lane)&32u)*rate_clock(s,lane)|=64u;}
static __attribute__((noinline)) int rate_boundary(seq_t *s,int lane){int rate=seq_lane_rate(s,lane);
 if(expanded(s) && (*rate_clock(s,lane)&32u) && poly_rate_config(s->pat,s->lane_sound,lane,1) && !(*rate_clock(s,lane)&64u))return *rate_clock(s,lane)&31u;
 s->lane_pending&=~(1u<<lane);if(expanded(s))*rate_clock(s,lane)&=31u;return rate;
}

uint32_t seq_step_q8(const seq_t *s, int rate)
{
    float bpm = seq_tempo(s), q;
    if (bpm < 20.0f)
        bpm = 20.0f;
    if (bpm > 300.0f)
        bpm = 300.0f;
    q = (float)FS * 60.0f / bpm;                    /* samples per quarter */
    return (uint32_t)(q * 256.0f * (expanded(s)?(float)rf_divisions[rate<21?rate:0].pulses4/96.0f:(float)TICKS_PER_STEP[rate & 7]/24.0f));
}

static const dpart_t *dpart(const seq_t *s, int k) { return &s->pat[s->ppat[k]].drum[k]; }
static int is_drum(int t) { return t < NKIT; }
static int is_bass(int t) { return t >= TRK_BASS0 && t < TRK_BASS0 + NBASS; }
static bpart_t *bpart(const seq_t *s, int b) { return &s->pat[s->ppat[TRK_BASS0 + b]].bass[b]; }

int seq_part_len(const seq_t *s, int t)
{
    int n = s->lane_sound ? seq_lane_length(s,t<NKIT?t*11:t+20) : is_drum(t) ? dpart(s, t)->len : t == TRK_BRK ? 16 : bpart(s, t - TRK_BASS0)->len;
    return n < 1 ? 1 : n > NSTEPS ? NSTEPS : n;
}

static __attribute__((noinline)) int part_rate(const seq_t *s, int t)
{
    return s->lane_sound ? seq_lane_rate(s,t<NKIT?t*11:t+20) : (is_drum(t) ? dpart(s, t)->rate : t == TRK_BRK ? RATE_16 : bpart(s, t - TRK_BASS0)->rate) & 3;
}

/* swing offset of a step, Q8: odd 16ths / 32nds come late; triplets are never swung */
static __attribute__((noinline)) uint32_t swing_q8(const seq_t *s, int rate, uint32_t step_q8)
{
    uint32_t sw = s->pat[s->ppat[TRK_DRUM]].swing;   /* the 909 is the bar: its pattern's groove */
    if ((expanded(s) && rf_divisions[rate].kind!=1) || rate == RATE_16T || rate == RATE_8T || rate == RATE_4T || rate == RATE_32T || !sw)
        return 0;
    if (sw > 100u)
        sw = 100u;
    return step_q8 / 200u * sw;                     /* 100 -> half a step late = 75 % (32-bit: no __udivdi3) */
}

static uint32_t rng_next(uint32_t *r)
{
    uint32_t x = *r ? *r : 1;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *r = x;
}

/* the step after p in this part's direction (also used to look ahead for TIEs) */
static __attribute__((noinline)) int next_pos(seq_t *s, int t, int p, int commit)
{
    seq_track_t *tr = &s->t[t];
    int len = seq_part_len(s, t), dir = is_bass(t) ? bpart(s, t - TRK_BASS0)->dir : DIR_FWD, n;
    if(s->lane_sound && p>=len)p%=len;
    switch (dir & 3) {
    case DIR_REV:
        return (p - 1 + len) % len;
    case DIR_PINGPONG:
        if (len == 1)
            return 0;
        n = p + tr->pp_dir;
        if (n >= len || n < 0) {
            n = p - tr->pp_dir;
            if (commit)
                tr->pp_dir = (int8_t)-tr->pp_dir;
        }
        return n;
    case DIR_RANDOM:
        if (!commit)
            return (p + 1) % len;                    /* a TIE look-ahead cannot know: assume forward */
        return (int)(rng_next(&tr->rng) % (uint32_t)len);
    default:
        return (p + 1) % len;
    }
}

static void midi(const seq_sink_t *o, uint8_t st, uint8_t d1, uint8_t d2)
{
    if (o->midi)
        o->midi(o->ctx, st, d1, d2);
}

enum { PAD_PENDING=64, PAD_ARMED=128 };
int seq_pad_channel_available(const seq_t *s,int channel)
{
    int i;if(channel==255)return 1;
    for(i=0;i<NTRACKS;i++)if(s->ch[i]==channel)return 0;
    return channel>=0 && channel<16;
}
int seq_pad_desired(const seq_t *s,uint8_t notes[3])
{
    const arrangement_settings_t *a=s->arrangement.intent;harmony_triad_t chord;int i;
    harmony_spec_t spec={s->arrangement.key,s->arrangement.mode,s->arrangement.degree,s->arrangement.parent};
    for(i=0;i<3;i++)notes[i]=255;
    if(!s->playing || !(s->arrangement.flags&ARR_ENABLED) || !a || a->pad_channel==255 || !spec.degree)return PAD_NO_CHORD;
    if(!harmony_resolve(&spec,a->pad_root,a->pad_low,a->pad_high,&chord))return PAD_RANGE;
    for(i=0;i<3;i++)notes[i]=chord.midi[i];
    if(!seq_pad_channel_available(s,a->pad_channel))return PAD_COLLISION;
    if(!s->send_notes)return PAD_OUTPUT_DISABLED;
    if(!s->midi_ready)return PAD_DISCONNECTED;
    return PAD_NONE;
}
int seq_pad_pending(const seq_t *s) {return (s->pad_status&PAD_PENDING)!=0;}
static __attribute__((noinline)) int pad_publish(seq_t *s,const seq_sink_t *o,const uint8_t *notes,int channel)
{
    uint32_t packets[6],n=0;int i;
    if(s->pad_channel_out<16)for(i=0;i<3;i++)
        packets[n++]=8u|((uint32_t)(0x80|s->pad_channel_out)<<8)|((uint32_t)s->pad_notes_out[i]<<16);
    if(notes)for(i=0;i<3;i++)
        packets[n++]=9u|((uint32_t)(0x90|channel)<<8)|((uint32_t)notes[i]<<16)|(90u<<24);
    if(n && (!o->midi_batch_try || !o->midi_batch_try(o->ctx,packets,n))) {s->pad_status|=PAD_PENDING;return 0;}
    s->pad_channel_out=notes?(uint8_t)channel:255;
    for(i=0;i<3;i++)s->pad_notes_out[i]=notes?notes[i]:255;
    s->pad_status&=(uint8_t)~PAD_PENDING;return 1;
}
static void pad_release(seq_t *s,const seq_sink_t *o)
{ s->pad_status&=(uint8_t)~PAD_ARMED;(void)pad_publish(s,o,0,255); }
static __attribute__((noinline)) void pad_sync(seq_t *s,const seq_sink_t *o)
{
    uint8_t notes[3];int i,reason=seq_pad_desired(s,notes),same;
    s->pad_status=(uint8_t)((s->pad_status&(PAD_PENDING|PAD_ARMED))|reason);
    if(reason || !(s->pad_status&PAD_ARMED)) {pad_release(s,o);return;}
    same=s->pad_channel_out==s->arrangement.intent->pad_channel;
    for(i=0;i<3;i++)same=same && s->pad_notes_out[i]==notes[i];
    if(same){s->pad_status&=(uint8_t)~PAD_PENDING;return;}
    (void)pad_publish(s,o,notes,s->arrangement.intent->pad_channel);
}

enum { FOLLOW_BLOCKED_A=1, FOLLOW_ACCENT_A=4, FOLLOW_RETUNE_A=16, FOLLOW_MIDI_REFRESH_A=64 };
static __attribute__((noinline)) int midi_try(const seq_sink_t *o,uint8_t st,uint8_t n,uint8_t v)
{
    if(o->midi_try)return o->midi_try(o->ctx,st,n,v);
    if(!o->midi)return 0;
    midi(o,st,n,v);return 1;
}
static __attribute__((noinline)) int bass_midi_shared(const seq_t *s,int b)
{
    int peer=b^1;
    return s->bass_notes_out[b]!=255 && s->bass_notes_out[b]==s->bass_notes_out[peer] &&
        s->bass_channels_out[b]==s->bass_channels_out[peer];
}
/* These shared bass paths must remain single copies: both constant-specialized
 * advance loops use them, and MIDI cleanup also runs outside the musical loop. */
static __attribute__((noinline)) int bass_midi_off(seq_t *s,int b,const seq_sink_t *o)
{
    unsigned bit=FOLLOW_MIDI_REFRESH_A<<b;
    if(!bass_midi_shared(s,b) && s->bass_notes_out[b]!=255 && s->bass_channels_out[b]<16 &&
        !midi_try(o,(uint8_t)(0x80|s->bass_channels_out[b]),s->bass_notes_out[b],0)) {
        s->arrangement.reserved|=(uint8_t)bit;return 0; /* keep actual ownership until accepted */
    }
    s->bass_notes_out[b]=s->bass_channels_out[b]=255;
    s->arrangement.reserved&=(uint8_t)~bit;return 1;
}
static __attribute__((noinline)) void bass_release(seq_t *s,int b,const seq_sink_t *o)
{
    seq_track_t *tr=&s->t[TRK_BASS0+b];
    if(tr->sounding) {if(o->bass_off)o->bass_off(o->ctx,b);tr->sounding=0;}
    (void)bass_midi_off(s,b,o);tr->to_off=0;
}
void seq_notes_clear(seq_t *s,const seq_sink_t *o)
{
    int b;
    if(o->midi_cancel)o->midi_cancel(o->ctx);
    pad_release(s,o);
    for(b=0;b<NBASS;b++) {
        int sounding=s->t[TRK_BASS0+b].sounding;
        bass_release(s,b,o);if(!sounding && o->bass_off)o->bass_off(o->ctx,b);
        s->t[TRK_BASS0+b].held_note=0;s->t[TRK_BASS0+b].trigger_rejected=0;
    }
    s->arrangement.reserved&=(uint8_t)(FOLLOW_MIDI_REFRESH_A|(FOLLOW_MIDI_REFRESH_A<<1));
}
static void bass_midi_on(seq_t *s,int b,int note,int accent,int retrigger,const seq_sink_t *o)
{
    unsigned bit=FOLLOW_MIDI_REFRESH_A<<b;
    uint8_t channel=s->ch[TRK_BASS0+b],old=s->bass_notes_out[b],old_channel=s->bass_channels_out[b];
    if(retrigger)s->arrangement.reserved|=(uint8_t)bit;
    if(!s->send_notes || !s->midi_ready || channel>=16) {(void)bass_midi_off(s,b,o);return;}
    /* Two basses may share one receiver tone. Existing ownership pairs are references;
       the final reference alone emits Off. A shared tone has one MIDI attack. */
    {
        int peer=b^1;unsigned peer_bit=FOLLOW_MIDI_REFRESH_A<<peer;
        if(s->bass_notes_out[peer]==(uint8_t)note && s->bass_channels_out[peer]==channel) {
            if(s->arrangement.reserved&peer_bit) {
                if(!bass_midi_off(s,peer,o))return;
            } else {
                if(old!=(uint8_t)note || old_channel!=channel) {
                    if(!bass_midi_off(s,b,o))return;
                }
                s->bass_notes_out[b]=(uint8_t)note;s->bass_channels_out[b]=channel;
                s->arrangement.reserved&=(uint8_t)~bit;return;
            }
        }
        if(bass_midi_shared(s,b)) {
            if(!midi_try(o,(uint8_t)(0x90|channel),(uint8_t)note,accent?127:90))return;
            s->bass_notes_out[b]=(uint8_t)note;s->bass_channels_out[b]=channel;
            s->arrangement.reserved&=(uint8_t)~bit;return;
        }
    }
    if(old==(uint8_t)note && old_channel==channel && !(s->arrangement.reserved&bit))return;
    if(old!=255 && old_channel<16) {
        uint8_t on=(uint8_t)(0x90|channel),off=(uint8_t)(0x80|old_channel),velocity=accent?127:90;
        int off_first=(s->arrangement.reserved&bit) || old_channel!=channel;
        if(o->midi_pair_try) {
            if(!(off_first?o->midi_pair_try(o->ctx,off,old,0,on,(uint8_t)note,velocity):
                 o->midi_pair_try(o->ctx,on,(uint8_t)note,velocity,off,old,0)))return;
        } else if(o->midi_try) {
            /* A feedback sink lacking pair support must accept Off before any new On. */
            if(!bass_midi_off(s,b,o))return;
            if(!midi_try(o,on,(uint8_t)note,velocity))return;
        } else {
            if(!o->midi)return;
            if(off_first){midi(o,off,old,0);midi(o,on,(uint8_t)note,velocity);}
            else {midi(o,on,(uint8_t)note,velocity);midi(o,off,old,0);}
        }
    } else if(!midi_try(o,(uint8_t)(0x90|channel),(uint8_t)note,accent?127:90))return;
    s->bass_notes_out[b]=(uint8_t)note;s->bass_channels_out[b]=channel;s->arrangement.reserved&=(uint8_t)~bit;
}
void seq_midi_prepare(seq_t *s,int connected,const seq_sink_t *o)
{
    int b;uint8_t notes[3];
    if(s->midi_ready && !connected && o->midi_cancel)o->midi_cancel(o->ctx);
    if(!connected)s->out_stop_pending=0;
    s->midi_ready=(uint8_t)(connected!=0);
    if(seq_pad_desired(s,notes))pad_release(s,o);
    for(b=0;b<NBASS;b++) {
        seq_track_t *tr=&s->t[TRK_BASS0+b];
        if(s->mute & (1u<<(MUTE_BASS0+b))) {bass_release(s,b,o);continue;}
        if(!tr->sounding || !s->playing || !s->send_notes || !s->midi_ready || s->ch[TRK_BASS0+b]>=16 ||
            (s->bass_notes_out[b]!=255 && s->ch[TRK_BASS0+b]!=s->bass_channels_out[b]))(void)bass_midi_off(s,b,o);
    }
}
void seq_midi_sync(seq_t *s,int connected,const seq_sink_t *o)
{
    int b;seq_midi_prepare(s,connected,o);
    if(!s->playing)return;
    for(b=0;b<NBASS;b++) {
        seq_track_t *tr=&s->t[TRK_BASS0+b];
        if(tr->sounding && !(s->mute&(1u<<(MUTE_BASS0+b))))
            bass_midi_on(s,b,tr->held_note&127u,(s->arrangement.reserved&(FOLLOW_ACCENT_A<<b))!=0,0,o);
    }
    pad_sync(s,o);
}
static int bass_follow_note(seq_t *s,int b,int source,int transpose,uint8_t *note)
{
    unsigned mode=arrangement_bass_mode(s->arrangement.flags,b);
    if(!(s->arrangement.flags&ARR_ENABLED) || !mode || !s->arrangement.degree) {
        int n=source+transpose;*note=(uint8_t)(n<0?0:n>127?127:n);return 1;
    }
    {
        harmony_spec_t spec={s->arrangement.key,s->arrangement.mode,s->arrangement.degree,s->arrangement.parent};
        const arrangement_settings_t *intent=s->arrangement.intent;
        return intent && harmony_resolve_note(&spec,source,transpose,intent->bass_low[b],intent->bass_high[b],
            mode==BASS_FOLLOW_ROOT?HARMONY_ROOT_ONLY:HARMONY_CHORD_TONES,note);
    }
}
static void bass_retune_held(seq_t *s,int b,const seq_sink_t *o)
{
    seq_track_t *tr=&s->t[TRK_BASS0+b];uint8_t note;
    unsigned bit=FOLLOW_RETUNE_A<<b;
    if(!(s->arrangement.reserved&bit))return;
    s->arrangement.reserved &= (uint8_t)~bit;
    if(s->mute&(1u<<(MUTE_BASS0+b))) {bass_release(s,b,o);return;}
    if(!tr->sounding || !(s->arrangement.flags&ARR_ENABLED) ||
        !arrangement_bass_mode(s->arrangement.flags,b) || !s->arrangement.degree)return;
    if(!bass_follow_note(s,b,tr->held_note&127u,0,&note)) {
        bass_release(s,b,o);tr->held_note=0;s->arrangement.reserved|=(uint8_t)(FOLLOW_BLOCKED_A<<b);return;
    }
    if(note!=(tr->held_note&127u)) {
        int accent=(s->arrangement.reserved&(FOLLOW_ACCENT_A<<b))!=0;
        if(o->bass_retune)o->bass_retune(o->ctx,b,note,accent);
        else if(o->bass_on)o->bass_on(o->ctx,b,note,accent,1);
        tr->held_note=(uint8_t)(note|(tr->held_note&128u));
        bass_midi_on(s,b,note,accent,0,o);
    }
}
void seq_cc_reconcile(seq_t *s,const seq_sink_t *o){
    const arrangement_settings_t *a=s->arrangement.intent;
    if(a){s->arrangement.key=a->key;s->arrangement.mode=a->mode;s->arrangement.parent=a->parent;
        if(s->song && s->song_pos<s->song->len)s->arrangement.degree=a->degree[s->song_pos];}
    for(int b=0;b<NBASS;b++){
        seq_track_t *tr=&s->t[TRK_BASS0+b];uint8_t note;
        if(!tr->sounding)continue; /* never revive a probability/range-rejected NOTE via TIE */
        if(!bass_follow_note(s,b,tr->source_note,(int)bpart(s,b)->transpose-24,&note)){
            bass_release(s,b,o);tr->held_note=0;s->arrangement.reserved|=(uint8_t)(FOLLOW_BLOCKED_A<<b);continue;
        }
        if(note!=(tr->held_note&127u)){
            int accent=(s->arrangement.reserved&(FOLLOW_ACCENT_A<<b))!=0;
            if(o->bass_retune)o->bass_retune(o->ctx,b,note,accent);
            else if(o->bass_on)o->bass_on(o->ctx,b,note,accent,1);
            tr->held_note=(uint8_t)(note|(tr->held_note&128u));bass_midi_on(s,b,note,accent,0,o);
        }
    }
    seq_midi_sync(s,s->midi_ready,o);
}

static void fire_drum_range(seq_t *s, int k, int p, const seq_sink_t *o,int first,int end)
{
    const dpart_t *d = dpart(s, k);
    uint32_t bit = 1u << (p & 31);
    int w = p >> 5;
    float vel = (d->accent[w] & bit) ? 1.0f : (float)s->accent_q7 / 127.0f;
    int v;
    for (v = first; v < end; v++) {
        if (!(d->hit[v][w] & bit) || ((s->mute | s->vmute) & (1u << (k * NDRUM + v))))
            continue;
        if (!o->drum(o->ctx, k, v, vel))
            continue;                                /* its chance said no: no MIDI either */
        if (s->send_notes && s->ch[k] != SEQ_CH_OFF) {
            uint8_t mv = (uint8_t)(vel * 127.0f);
            midi(o, (uint8_t)(0x90 | s->ch[k]), GM_DRUM[k][v], mv);
            midi(o, (uint8_t)(0x80 | s->ch[k]), GM_DRUM[k][v], 0);
        }
    }
}

static void fire_drum(seq_t *s,int k,int p,const seq_sink_t *o) { fire_drum_range(s,k,p,o,0,NDRUM); }

static __attribute__((noinline)) void fire_bass(seq_t *s,int b,int p,uint32_t step_q8,uint32_t carry,const seq_sink_t *o)
{
    seq_track_t *tr=&s->t[TRK_BASS0+b];bpart_t *bp=bpart(s,b);const bstep_t *st=&bp->step[p];
    int gate=bstep_gate(st),hold,nx;uint8_t note;
    uint8_t prev_slide=tr->held_note&128u;
    unsigned blocked=FOLLOW_BLOCKED_A<<b,accentbit=FOLLOW_ACCENT_A<<b,retunebit=FOLLOW_RETUNE_A<<b;
    if(s->mute&(1u<<(MUTE_BASS0+b)))gate=G_REST;
    if(gate==G_REST) {bass_release(s,b,o);tr->held_note=0;s->arrangement.reserved&=(uint8_t)~retunebit;return;}
    if(gate==G_NOTE)s->arrangement.reserved&=(uint8_t)~retunebit; /* NOTE handles final projection once */
    else bass_retune_held(s,b,o);
    if(gate==G_TIE && (tr->trigger_rejected || (s->arrangement.reserved&blocked) ||
        (!tr->sounding && (s->arrangement.flags&ARR_ENABLED) && s->arrangement.degree && arrangement_bass_mode(s->arrangement.flags,b))))return;
    if(gate==G_TIE && tr->sounding)note=tr->held_note&127u; /* TIE's stored note is not played */
    else {
        if(!bass_follow_note(s,b,st->note,(int)bp->transpose-24,&note)) {
            bass_release(s,b,o);tr->held_note=0;s->arrangement.reserved|=(uint8_t)blocked;return;
        }
        s->arrangement.reserved&=(uint8_t)~blocked; /* successful projection; chance rejection is separate */
        {
            int accent=(st->flags&BS_ACCENT)!=0,slide=tr->sounding && prev_slide;
            int triggered=o->bass_trigger?o->bass_trigger(o->ctx,b,note,accent,slide):1;
            if(!o->bass_trigger && o->bass_on)o->bass_on(o->ctx,b,note,accent,slide);
            if(!triggered) {bass_release(s,b,o);tr->held_note=0;tr->trigger_rejected=1;return;}
            tr->source_note=st->note;
            bass_midi_on(s,b,note,accent,!slide,o);tr->sounding=1;tr->trigger_rejected=0;
            s->arrangement.reserved=(uint8_t)((s->arrangement.reserved&~(blocked|accentbit))|(accent?accentbit:0));
        }
    }
    nx=next_pos(s,TRK_BASS0+b,p,0);
    hold=(st->flags&BS_SLIDE) || bstep_gate(&bp->step[nx])==G_TIE;
    tr->held_note=(uint8_t)(note|((st->flags&BS_SLIDE)?128u:0));
    tr->to_off=hold?0:carry+step_q8/2u;
}

int seq_part_muted(const seq_t *s, int p)
{
    uint32_t m = seq_part_mask(p);
    return (s->mute & m) == m;
}

/* the song's bar k into the parts and their mutes (into next[]: the caller switches) */
static void song_read(seq_t *s, int k, uint8_t *next)
{
    const song_bar_t *b = &s->song->bar[k];
    uint32_t mute = 0;                                  /* the bar decides every part's mute (built up, */
    int p;                                              /* not masked: ~mask reads as a ROM address) */
    for (p = 0; p < NTRACKS; p++) {
        next[p] = b->pat[p] < NPAT ? b->pat[p] : 0;
        if (b->mute >> p & 1u)
            mute |= seq_part_mask(p);
    }
    s->mute = mute;
}

static __attribute__((noinline)) void row_reset(seq_t *s,int kit,int from_top);
static int latched_rate(const seq_t *s,int lane);
static void latch_rate(seq_t *s,int lane,int rate);

/* Compact per-row clocks. Existing five tracks retain gate/RNG/motion state. */
static seq_lane_clock_t *row_clock(seq_t *s,int lane)
{return &s->lane[(lane/11)*10+lane%11-1];}
int seq_lane_length(const seq_t *s,int lane)
{int part=poly_part(lane);return poly_length(s->pat,s->lane_sound,s->ppat[part],lane);}
int seq_lane_rate(const seq_t *s,int lane)
{int part=poly_part(lane);return poly_rate(s->pat,s->lane_sound,s->ppat[part],lane);}
unsigned seq_lane_cycles(const seq_t *s,int lane)
{return s->lane_enabled && lane<22 && lane%11 ? row_clock((seq_t *)s,lane)->bars : s->t[poly_part(lane)].bars;}
int seq_lane_position(const seq_t *s,int lane)
{return s->lane_enabled && lane<22 && lane%11 ? row_clock((seq_t *)s,lane)->pos : s->t[poly_part(lane)].started?s->t[poly_part(lane)].pos:255;}
int seq_lane_latched_rate(const seq_t *s,int lane);
static int latched_rate(const seq_t *s,int lane) {return seq_lane_latched_rate(s,lane);}
int seq_lane_latched_rate(const seq_t *s,int lane)
{
 if(expanded(s))return !s->playing?seq_lane_rate(s,lane):(int)(*rate_clock((seq_t *)s,lane)&31u);
 unsigned bit=(unsigned)lane*3,n=bit/8,sh=bit%8,x=s->lane_rate[n];
 if(n+1<sizeof s->lane_rate)x|=(unsigned)s->lane_rate[n+1]<<8;
 return (x>>sh)&7;
}
static void latch_rate(seq_t *s,int lane,int rate)
{
 if(expanded(s)){uint8_t *c=rate_clock(s,lane);*c=(uint8_t)((*c&96u)|(unsigned)rate);return;}
 unsigned bit=(unsigned)lane*3,n=bit/8,sh=bit%8,x=s->lane_rate[n],mask=7u<<sh;
 if(n+1<sizeof s->lane_rate)x|=(unsigned)s->lane_rate[n+1]<<8;
 x=(x&~mask)|((unsigned)rate<<sh);s->lane_rate[n]=(uint8_t)x;
 if(n+1<sizeof s->lane_rate)s->lane_rate[n+1]=(uint8_t)(x>>8);
}
static __attribute__((noinline)) void row_reset(seq_t *s,int kit,int from_top)
{
 int v;for(v=1;v<NDRUM;v++){
  int lane=kit*11+v,rate=seq_lane_rate(s,lane);seq_lane_clock_t *c=row_clock(s,lane);
  if(from_top){c->pos=255;c->bars=0;}
  latch_rate(s,lane,rate);if(!expanded(s))c->ext_ticks=(uint8_t)(TICKS_PER_STEP[rate]-1);
  c->to_next=s->ext?NEVER:0;
 }
}
static __attribute__((noinline)) void row_fire(seq_t *s,int lane,const seq_sink_t *o)
{
 seq_lane_clock_t *c=row_clock(s,lane);int rate=rate_boundary(s,lane),len=seq_lane_length(s,lane);
 int p=c->pos==255?0:(c->pos+1>=len?0:c->pos+1),np=p+1>=len?0:p+1;
 uint32_t unit=expanded(s)?rate_unit(s,lane):256u,carry=c->to_next<unit?c->to_next:0;
 if(expanded(s))carry=adopt_rate_unit(s,lane,carry);
 uint32_t L=rate_interval(s,rate),sw=swing_q8(s,rate,L);
 if(c->pos!=255 && !p)c->bars++;
 c->pos=(uint8_t)p;latch_rate(s,lane,rate);
 fire_drum_range(s,lane/11,p,o,lane%11,lane%11+1);
 c->to_next=s->ext && !expanded(s)?NEVER:carry+((np&1)?L+sw:L-sw);
 if((!s->ext || expanded(s)) && !(np&1) && !(p&1))c->to_next=carry+L;
}

static void arrangement_latch(seq_t *s, uint16_t block)
{
    s->pad_status|=PAD_ARMED;
    volatile arrangement_runtime_t *a = &s->arrangement;
    uint8_t next[NTRACKS];
    int t;
    if (block >= s->song->len) block = 0;
    s->song_pos = block;
    song_read(s, block, next);
    a->mute = s->song->bar[block].mute;
    a->degree = a->intent->degree[block];
    a->key = a->intent->key; a->mode = a->intent->mode; a->parent = a->intent->parent;
    a->reserved |= FOLLOW_RETUNE_A | (FOLLOW_RETUNE_A<<1);
    s->song_rep = (uint8_t)SONG_REP(&s->song->bar[block]);
    for (t = 0; t < NTRACKS; t++) {
        a->pat[t] = next[t];
        if (next[t] != s->ppat[t]) {
            seq_track_t *tr = &s->t[t];
            s->ppat[t] = next[t]; if(s->lane_sound){latch_rate(s,t<NKIT?t*11:t+20,part_rate(s,t));if(t<NKIT)row_reset(s,t,1);} tr->started = 0; tr->bars = 0; tr->pp_dir = 1;
            tr->to_next = s->ext ? NEVER : 0;
            if(!expanded(s))tr->ext_ticks = (uint8_t)(TICKS_PER_STEP[part_rate(s,t)] - 1);
        }
        s->pcue[t] = 255;
    }
}

static __attribute__((noinline)) void poly_mode_boundary(seq_t *s)
{
 if(s->lane_sound){int enabled=poly_mode(s->lane_sound);if(enabled!=s->lane_enabled){s->lane_enabled=(uint8_t)enabled;if(enabled){row_reset(s,0,1);row_reset(s,1,1);}}}
}

static __attribute__((noinline)) void poly_musical_tick(seq_t *s);

static void arrangement_tick(seq_t *s)
{
    volatile arrangement_runtime_t *a = &s->arrangement;
    if(!arrangement_active(s)){if(s->lane_sound)poly_musical_tick(s);return;}
    if(!s->playing)return;
    if (!a->clock_started) { a->clock_started = 1; return; }
    if (++a->tick < 96) return;
    a->tick = 0;
    poly_mode_boundary(s);rate_bar(s);
    if (a->queued < s->song->len) {
        uint8_t block = a->queued;
        a->queued = 255; arrangement_latch(s,block);
    } else if (s->song_rep <= 1) arrangement_latch(s,(uint16_t)(s->song_pos + 1));
    else s->song_rep--;
}

/* the live state (next[], the mutes) into the song's bar k, growing it */
static void song_write(seq_t *s, int k, const uint8_t *next)
{
    song_bar_t *b = &s->song->bar[k];
    int p;
    b->mute = 0;
    for (p = 0; p < NTRACKS; p++) {
        b->pat[p] = next[p];
        if (seq_part_muted(s, p))
            b->mute |= (uint8_t)(1u << p);
    }
    if (s->song->len < k + 1)
        s->song->len = (uint16_t)(k + 1);
}

static int song_plays(const seq_t *s) { return s->song_on && s->song && (s->song_rec || s->song->len); }

/* the bar's end (the 909 wrapping), or the start: cues, the chain or the song decide each
 * part's next pattern. Returns the parts whose pattern changed (bit t). */
static uint32_t bar_end(seq_t *s, int start)
{
    uint8_t next[NTRACKS];
    uint32_t changed = 0;
    int t, cued = 0;
    for (t = 0; t < NTRACKS; t++)
        next[t] = s->ppat[t];
    if (song_plays(s) && !s->song_rec) {               /* the song: its next bar (looping), or this one again */
        int k = start ? s->song_start : s->song_pos + 1;
        if (!start && s->song_rep > 1 && s->song_pos < s->song->len) {
            k = s->song_pos;
            s->song_rep--;
        } else {
            if (k >= s->song->len)
                k = 0;
            s->song_rep = (uint8_t)SONG_REP(&s->song->bar[k]);
        }
        s->song_pos = (uint16_t)k;
        song_read(s, k, next);
        for (t = 0; t < NTRACKS; t++)
            s->pcue[t] = 0xFF;
    } else {
        for (t = 0; t < NTRACKS; t++)
            if (s->pcue[t] < NPAT) {
                next[t] = s->pcue[t];
                s->pcue[t] = 0xFF;
                cued = 1;
            }
        if (!cued && !start && s->chain_a != s->chain_b) {
            int a = s->chain_a < s->chain_b ? s->chain_a : s->chain_b;
            int b = s->chain_a < s->chain_b ? s->chain_b : s->chain_a, c = s->ppat[TRK_DRUM];
            c = (c < a || c >= b) ? a : c + 1;
            for (t = 0; t < NTRACKS; t++)
                next[t] = (uint8_t)c;
        }
        if (song_plays(s)) {                           /* recording: what plays is the song */
            int k = start ? s->song_start : s->song_pos + 1;
            if (k >= NSONG) {
                s->song_rec = 0;                       /* full */
            } else {
                if (start && k < s->song->len)         /* overdub from where the song is */
                    song_read(s, k, next);
                s->song_pos = (uint16_t)k;
                song_write(s, k, next);
            }
        }
    }
    for (t = 0; t < NTRACKS; t++)
        if (next[t] != s->ppat[t]) {
            s->ppat[t] = next[t];
            changed |= 1u << t;
        }
    return changed;
}

static void auto_mutate(seq_t *s, int b)
{
    bpart_t *bp = bpart(s, b);
    seq_track_t *tr = &s->t[TRK_BASS0 + b];
    tr->bars++;
    if (bp->gen.mutate_bars && tr->bars % bp->gen.mutate_bars == 0)
        tb3po_mutate(bp, &s->mut_rng[b]);
}

/* fire track t's next step now; returns the parts whose pattern changed (the 909 wrapping) */
static __attribute__((noinline)) uint32_t fire_step(seq_t *s, int t, const seq_sink_t *o, int arrangement)
{
    seq_track_t *tr = &s->t[t];
    int lane=t<NKIT?t*11:t+20,rate=s->lane_sound?rate_boundary(s,lane):part_rate(s,t),p,wrapped;
    if(s->lane_sound)latch_rate(s,lane,rate);
    uint32_t changed = 0;
    uint32_t L = seq_step_q8(s, rate);
    uint32_t unit=expanded(s)?rate_unit(s,lane):256u;
    uint32_t carry = tr->to_next < unit ? tr->to_next : 0;
    if(expanded(s))carry=adopt_rate_unit(s,lane,carry);   /* the step was due this far into the sample: keep it */
    int bass = is_bass(t);
    if (!tr->started) {
        p = (bass && bpart(s, t - TRK_BASS0)->dir == DIR_REV) ? seq_part_len(s, t) - 1 : 0;
        tr->started = 1;
        wrapped = 0;
    } else {
        p = next_pos(s, t, tr->pos, 1);
        wrapped = !bass ? p == 0 : p == 0 || (bpart(s, t - TRK_BASS0)->dir == DIR_REV && p == seq_part_len(s, t) - 1);
    }
    if (wrapped && t == TRK_DRUM && !arrangement && !s->lane_sound && (changed = bar_end(s, 0)) != 0) {
        rate = part_rate(s, t);                       /* the 909's new pattern */
        L = seq_step_q8(s, rate);
        p = 0;
    }
    if (wrapped && bass)
        auto_mutate(s, t - TRK_BASS0);
    if (wrapped && (t == TRK_BRK || is_drum(t)))   /* (the 303s count theirs in auto_mutate) */
        tr->bars++;
    if (changed & 1u)                                /* the 909's new pattern: its passes from 1 again, as */
        tr->bars = 0;                                /* restart() does for the others (probability cycles) */
    tr->pos = (uint8_t)p;
    if (o->step)                                     /* first: the step's motion and p-locks, which the hit */
        o->step(o->ctx, t, p);                       /* below must hear (a drum takes its tune as it fires) */
    if (is_drum(t))
        { if(s->lane_enabled)fire_drum_range(s,t,p,o,0,1);else fire_drum(s,t,p,o); }
    else if (t == TRK_BRK) {
        if (o->brk)
            o->brk(o->ctx, p, tr->bars, (float)L / 256.0f,
                   (s->lane_sound ? poly_break_gate(s->pat,s->lane_sound,s->ppat[TRK_BRK],p) : (int)(s->pat[s->ppat[TRK_BRK]].brk.steps >> p & 1u)) && !(s->mute & (1u << MUTE_BRK)));
    } else
        fire_bass(s, t - TRK_BASS0, p, L, expanded(s)?carry*256u/rate_unit(s,lane):carry, o);
    if (s->ext && !expanded(s)) {
        tr->to_next = NEVER;                         /* the clock arms the next one */
    } else {
        /* the step that follows p: swung if it is an odd one */
        int np = next_pos(s, t, p, 0);
        if(expanded(s))L=rate_interval(s,rate);
        uint32_t sw = swing_q8(s, rate, L);
        tr->to_next = carry + ((np & 1) ? L + sw : L - sw);
        if (!(np & 1) && (p & 1) == 0)               /* even -> even (odd lengths, wraps): no swing */
            tr->to_next = carry + L;
    }
    return changed;
}

static void restart(seq_t *s, uint32_t parts)  /* parts whose pattern switched: from step 1 (not the 909's) */
{
    int t;
    for (t = 1; t < NTRACKS; t++) {
        if (!(parts >> t & 1u))
            continue;
        s->t[t].started = 0;
        s->t[t].to_next = s->ext ? NEVER : 0;
        if(!expanded(s))s->t[t].ext_ticks = (uint8_t)(TICKS_PER_STEP[part_rate(s, t)] - 1);
        s->t[t].bars = 0;
        s->t[t].pp_dir = 1;
        if(s->lane_sound && t<NKIT)row_reset(s,t,1);
    }
}

static __attribute__((noinline)) void do_start(seq_t *s, int from_top, const seq_sink_t *o, int arrangement)
{
    int t;
    if (from_top) {
        s->arrangement.reserved=0;
        if(s->lane_sound)s->arrangement.tick=s->arrangement.clock_started=0;
        if (arrangement) {
            s->arrangement.tick = s->arrangement.clock_started = 0;
            s->arrangement.queued = 255;
            arrangement_latch(s,s->song_start);
        } else bar_end(s, 1);                         /* legacy 909 cycle semantics */
    }
    for (t = 0; t < NTRACKS; t++) {
        seq_track_t *tr = &s->t[t];
        if (from_top) {
            tr->started = 0;
            tr->trigger_rejected=0;
            tr->bars = 0;
            tr->pp_dir = 1;
        }
        if(s->lane_sound){latch_rate(s,t<NKIT?t*11:t+20,part_rate(s,t));if(t<NKIT)row_reset(s,t,from_top);}
        tr->to_off = 0;
        tr->to_next = s->ext ? NEVER : 0;
        if(!expanded(s))tr->ext_ticks = (uint8_t)(TICKS_PER_STEP[part_rate(s, t)] - 1);   /* the first clock is the downbeat */
    }
    s->clk_to_next = 0;
    s->playing = 1;
    if((s->clock.role_source&3u)==CLOCK_FOLLOW){s->clock.state=CLOCK_RUNNING;s->ext_since=0;s->clk_to_next=(FS/2u+1u)<<8;}
    s->pad_status|=PAD_ARMED;
#ifdef X0X_WEB
    web_seq_transport(1, from_top);
#endif
    /* Start/Continue packet was accepted before local transport is released. */
}

static unsigned clock_role(const seq_t *s){return s->clock.role_source&3u;}
static int clock_output(const seq_t *s){return clock_role(s)==CLOCK_LEAD || (clock_role(s)==CLOCK_LEGACY && !s->ext && s->send_clock);}
static int clock_schedule(const seq_t *s){unsigned role=clock_role(s);return (role-1u)<2u || s->send_clock;}
static __attribute__((noinline)) void clock_tick(seq_t *s){if(s->clock.tick_lo!=UINT32_MAX || s->clock.tick_hi!=UINT32_MAX){if(++s->clock.tick_lo==0)s->clock.tick_hi++;}s->clock.age=0;s->clock.flags|=CLOCK_TICK_VALID;}
static void clock_generation(seq_t *s){if(s->clock.generation!=UINT32_MAX)s->clock.generation++;}
static void clock_cutoff(seq_t *s,const seq_sink_t *o){s->ext_pending=0;if(o->clock_retire)o->clock_retire(o->ctx,7);}
void seq_clock_request(seq_t *s,uint8_t packed,const seq_sink_t *o){
    s->clock.flags&=(uint8_t)~(CLOCK_CONFIG_APPLIED|CLOCK_INVALIDATED);
    s->clock.pending=(uint8_t)(packed+1u);s->clock.state=CLOCK_TRANSITION;
    clock_cutoff(s,o);seq_stop(s);
}
int seq_clock_admit(seq_t *s,unsigned source,unsigned status){
    unsigned role=clock_role(s);
    if(s->clock.pending || (s->clock.flags&CLOCK_RETIRED) || s->clock.generation==UINT32_MAX)return 0;
    if(role==CLOCK_LEGACY)return 1;
    if(role!=CLOCK_FOLLOW || source!=s->clock.role_source>>2)return 0;
    /* Reacquired ticks measure age only. An explicit owner Start/Continue is required. */
    return 1;
}
static void do_stop(seq_t *s, const seq_sink_t *o, int arrangement)
{
    int retired=(s->out_stop_pending&2u)!=0;
    if(o->midi_transport_cancel)o->midi_transport_cancel(o->ctx);
    else if(o->midi_cancel)o->midi_cancel(o->ctx);
    pad_release(s,o);
    int b, t;
    s->playing = 0;
    if(clock_role(s)==CLOCK_FOLLOW && s->clock.state!=CLOCK_LOST && s->clock.state!=CLOCK_TRANSITION)s->clock.state=CLOCK_STOPPED;
    if (arrangement) s->arrangement.queued = 255;
#ifdef X0X_WEB
    web_seq_transport(0, 0);
#endif
    for (b = 0; b < NBASS; b++)
        bass_release(s, b, o);
    if (o->brk_stop)
        o->brk_stop(o->ctx);
    for (t = 0; t < NTRACKS; t++)
        s->t[t].to_next = NEVER;
    s->out_stop_pending=(uint8_t)(!retired && clock_output(s) && s->midi_ready);
}

static __attribute__((noinline)) void poly_musical_tick(seq_t *s)
{
    volatile arrangement_runtime_t *a=&s->arrangement;uint32_t changed;int t;
    if(!s->playing)return;
    if(!a->clock_started){a->clock_started=1;return;}
    if(++a->tick<96)return;
    a->tick=0;poly_mode_boundary(s);rate_bar(s);changed=bar_end(s,0);
    for(t=0;t<NTRACKS;t++)if(changed&(1u<<t)){
        seq_track_t *tr=&s->t[t];tr->started=0;tr->bars=0;tr->pp_dir=1;
        latch_rate(s,t<NKIT?t*11:t+20,part_rate(s,t));if(!expanded(s))tr->ext_ticks=(uint8_t)(TICKS_PER_STEP[part_rate(s,t)]-1);
        tr->to_next=s->ext?NEVER:0;if(t<NKIT)row_reset(s,t,1);
    }
}

/* external clock ticks: arm the steps they complete */
static __attribute__((noinline)) void ext_ticks(seq_t *s, int arrangement,const seq_sink_t *o)
{
    int t;
    uint32_t n = s->ext_pending;
    int late=s->lane_sound && n>1;
    s->ext_pending = 0;
    if(expanded(s)&&n>1){clock_cutoff(s,o);do_stop(s,o,arrangement && arrangement_active(s));seq_transport_retire(s);s->clock.state=CLOCK_LOST;s->ext_period=0;s->ext_since=0;return;}
    for (; n; n--) {
        if (s->ext_since > 0 && s->ext_since < FS) {  /* tempo: smoothed tick period */
            uint32_t q = s->ext_since << 8;
            s->ext_period = s->ext_period ? s->ext_period - (s->ext_period >> 3) + (q >> 3) : q;
            s->ext_bpm = (float)FS * 60.0f * 256.0f / (24.0f * (float)s->ext_period);
        }
        /* Expanded interpolation supports the explicit20..300 BPM domain.
         * A single late/too-fast measured period must not wrap a long interval. */
        if(expanded(s) && s->ext_period && (s->ext_period<(FS*60u*256u)/(300u*24u)-256u || s->ext_period>(FS*60u*256u)/(20u*24u)+256u)){clock_cutoff(s,o);do_stop(s,o,arrangement && arrangement_active(s));seq_transport_retire(s);s->clock.state=CLOCK_LOST;s->ext_period=0;s->ext_since=0;return;}
        s->ext_since = 0;
        if(clock_role(s)==CLOCK_FOLLOW)s->clk_to_next=(FS/2u+1u)<<8;
        clock_tick(s);
        if (!s->playing)
            continue;
        /* Musical block changes precede track arming on the same incoming clock. */
        if (arrangement) arrangement_tick(s);else if(s->lane_sound)poly_musical_tick(s);
        if(expanded(s)){
            for(t=0;t<NTRACKS;t++)if(s->t[t].to_next==NEVER)s->t[t].to_next=0;
            if(s->lane_enabled)for(t=0;t<20;t++)if(s->lane[t].to_next==NEVER)s->lane[t].to_next=0;
            continue;
        }
        if(s->lane_enabled) { int lane;
            for(lane=0;lane<22;lane++)if(lane%11) {
                seq_lane_clock_t *c=&s->lane[(lane/11)*10+lane%11-1];
                int rate=latched_rate(s,lane),tps=TICKS_PER_STEP[rate];
                if(++c->ext_ticks>=tps){int len=seq_lane_length(s,lane),np=c->pos==255?0:c->pos+1>=len?0:c->pos+1;
                    uint32_t L=s->ext_period?s->ext_period*(uint32_t)tps:seq_step_q8(s,rate);
                    c->ext_ticks=0;c->to_next=late?0:(np&1)?swing_q8(s,rate,L):0;}
            }
        }
        for (t = 0; t < NTRACKS; t++) {
            seq_track_t *tr = &s->t[t];
            int rate = s->lane_sound?latched_rate(s,t<NKIT?t*11:t+20):part_rate(s,t), tps = TICKS_PER_STEP[rate];
            if (++tr->ext_ticks < tps)
                continue;
            tr->ext_ticks = 0;
            {
                int np = tr->started ? next_pos(s, t, tr->pos, 0) : 0;
                uint32_t L = s->ext_period ? s->ext_period * (uint32_t)tps : seq_step_q8(s, rate);
                tr->to_next = late?0:(np & 1) ? swing_q8(s, rate, L) : 0;
            }
        }
        /* Queued clock ticks have no sample timestamps. Preserve every logical
         * event in clock order; overdue swing is collapsed rather than replayed
         * as fabricated historical audio. A single fresh tick retains swing. */
        if(s->lane_sound)for(t=0;t<NTRACKS;t++){
            if(s->t[t].to_next<256u)(void)fire_step(s,t,o,arrangement);
            if(s->lane_enabled && t<NKIT){int v;for(v=1;v<NDRUM;v++)if(s->lane[t*10+v-1].to_next<256u)row_fire(s,t*11+v,o);}
        }
    }
}

static __attribute__((noinline)) int clock_prepare(seq_t *s,uint32_t k,const seq_sink_t *o)
{
    s->ext_since = UINT32_MAX-s->ext_since<k?UINT32_MAX:s->ext_since+k;
    if(s->clock.flags&CLOCK_TICK_VALID)s->clock.age=UINT32_MAX-s->clock.age<k?UINT32_MAX:s->clock.age+k;
    if(s->clock.flags&CLOCK_RETIRED){
        s->clock.flags&=(uint8_t)~CLOCK_RETIRED;s->clock.pending=0;clock_generation(s);s->clock.flags=(uint8_t)((s->clock.flags&~CLOCK_CONFIG_APPLIED)|CLOCK_INVALIDATED);
        clock_cutoff(s,o);
        if(clock_role(s)!=CLOCK_LEGACY){s->req_stop=1;s->out_stop_pending=2;s->clock.state=CLOCK_LOST;}
    }
    if (s->transport_hold) { /* stopped: retain elapsed clock time, discard pre-start clock ticks */
        s->ext_pending = 0; /* Start/Continue/Stop requests remain pending until release */
        return 0;
    }
    if (s->ext_pending)
        s->ext = 1;
    else if(clock_role(s)==CLOCK_FOLLOW && s->clock.state==CLOCK_RUNNING && s->ext_since>FS/2u){
        s->clock.state=CLOCK_LOST;s->clock.flags=(uint8_t)((s->clock.flags&~CLOCK_CONFIG_APPLIED)|CLOCK_INVALIDATED);clock_generation(s);clock_cutoff(s,o);seq_stop(s);
    }
    /* External Follow never uses the internal pulse countdown. Cache its
     * exact loss event here without reducing the independent tick/age status. */
    if(clock_role(s)==CLOCK_FOLLOW)s->clk_to_next=(s->ext_since>FS/2u?1u:FS/2u+1u-s->ext_since)<<8;
    return 1;
}

/* One shared transport/mode cleanup path. Outlining prevents both legacy and
 * arrangement advance entrypoints from cloning the same ownership protocol. */
static __attribute__((noinline)) void clock_transport(seq_t *s,const seq_sink_t *o,int arrangement)
{
    if (s->req_stop) {
        if(s->transport_epoch!=UINT32_MAX)s->transport_epoch++;
        if(s->stop_epoch!=UINT32_MAX)s->stop_epoch++;
        __asm__ volatile("" ::: "memory");
        s->req_stop = 0;
        /* Request-time Stop clears older run intents. A subsequent run survives. */
        do_stop(s, o, arrangement && arrangement_active(s));
    }
    if(s->out_stop_pending){
        if(!clock_output(s) || !s->midi_ready)s->out_stop_pending=0;
        else if(midi_try(o,0xFC,0,0))s->out_stop_pending=0;
    }
    /* Retain stopped position under backpressure. Cleanup ownership must be
     * accepted before a new FA/FB; otherwise a later Off could kill a new On. */
    /* A mode transition owns this stopped boundary. Discard late main run
     * publication before the old role can enqueue FA/FB, including a request
     * whose producer checked pending before the audio IRQ staged the mode. */
    if(s->clock.pending)s->req_start=s->req_cont=0;
    int transport_ready=!s->out_stop_pending;
    if(transport_ready && !s->clock.pending && (s->req_start || s->req_cont)){
        int b;seq_midi_prepare(s,s->midi_ready,o);
        if(s->pad_channel_out!=255)transport_ready=0;
        for(b=0;b<NBASS;b++)if(s->bass_notes_out[b]!=255)transport_ready=0;
        if(transport_ready && clock_output(s) && s->midi_ready)transport_ready=midi_try(o,s->req_start?0xFA:0xFB,0,0);
    }
    if(s->clock.pending && transport_ready){
        int b;seq_midi_prepare(s,s->midi_ready,o);
        if(s->pad_channel_out!=255)transport_ready=0;
        for(b=0;b<NBASS;b++)if(s->bass_notes_out[b]!=255)transport_ready=0;
        if(transport_ready){
            s->clock.role_source=(uint8_t)(s->clock.pending-1u);clock_generation(s);
            s->clock.flags=(uint8_t)((s->clock.flags&~(CLOCK_TICK_VALID|CLOCK_INVALIDATED))|CLOCK_CONFIG_APPLIED);s->clock.age=UINT32_MAX;
            s->clock.state=clock_role(s)==CLOCK_FOLLOW?CLOCK_WAITING:CLOCK_STOPPED;
            s->ext=(uint8_t)(clock_role(s)==CLOCK_FOLLOW);s->ext_since=0;s->ext_period=0;
            /* The exact cutoff is the new role's ingress generation boundary.
             * Retire RT received while cleanup was blocked, including packets
             * main has not reviewed yet; new producer packets after this cutoff
             * belong to the already-published role/generation. */
            clock_cutoff(s,o);s->clock.pending=0;
            s->req_start=s->req_cont=0;
        }
    }
    if (s->req_start && transport_ready && !s->clock.pending) {
        if(s->transport_epoch!=UINT32_MAX)s->transport_epoch++;
        s->req_start = 0;
        do_start(s, 1, o, arrangement && arrangement_active(s));
    }
    if (s->req_cont && transport_ready && !s->clock.pending) {
        if(s->transport_epoch!=UINT32_MAX)s->transport_epoch++;
        s->req_cont = 0;
        do_start(s, 0, o, arrangement && arrangement_active(s));
    }
}

static inline __attribute__((always_inline)) uint32_t until_event_common(seq_t *s, uint32_t n, int arrangement,int poly)
{
    uint32_t m = n, e, role;
    int rf=poly && expanded(s);
    int t;
    if (!s->playing)
        return n;
    role=clock_role(s);
    if (s->ext && (arrangement || poly) && role!=CLOCK_FOLLOW) {
        e = s->ext_since > FS / 2u ? 1u : FS / 2u + 1u - s->ext_since;
        if (e < m) m = e;
    }
    for (t = 0; t < NTRACKS; t++) {
        uint32_t unit=rf?rate_unit(s,t<NKIT?t*11:t+20):256u;
        if (s->t[t].to_next != NEVER && (e = s->t[t].to_next / unit) < m)
            m = e;
        if (s->t[t].to_off && (e = s->t[t].to_off >> 8) < m)
            m = e;
    }
    if(poly && s->lane_enabled)for(t=0;t<20;t++)if(s->lane[t].to_next!=NEVER && (e=s->lane[t].to_next/(rf?rate_unit(s,(t/10)*11+t%10+1):256u))<m)m=e;
    if ((!s->ext || role==CLOCK_FOLLOW) && (role || s->send_clock || WEB_CLOCK || arrangement || poly) && (e = s->clk_to_next >> 8) < m)
        m = e;
    return m ? m : 1;
}

static __attribute__((noinline)) void advance_common(seq_t *s, uint32_t k, const seq_sink_t *o, int arrangement,int poly)
{
    int t, guard, clock_rebased = 0,rf=poly && expanded(s);
    uint32_t dq = k << 8;
    if(!clock_prepare(s,k,o))return;
    if(!s->ext_pending && clock_role(s)==CLOCK_LEGACY && s->ext && s->ext_since > FS / 2u) {    /* clock gone for 0.5 s: back to internal */
        s->ext = 0;
        if (arrangement || poly) {
            s->clk_to_next = seq_step_q8(s,RATE_16) / 6u; /* resume next tick, without duplicating the last one */
            clock_rebased = 1;
        }
        for (t = 0; t < NTRACKS; t++)
            if (s->playing && s->t[t].to_next == NEVER)
                s->t[t].to_next = poly && !rf ? seq_step_q8(s,latched_rate(s,t<NKIT?t*11:t+20))/TICKS_PER_STEP[latched_rate(s,t<NKIT?t*11:t+20)] * (TICKS_PER_STEP[latched_rate(s,t<NKIT?t*11:t+20)]-s->t[t].ext_ticks) : 0;
        if(poly && !rf && s->lane_enabled)for(t=0;t<20;t++)if(s->playing && s->lane[t].to_next==NEVER){int lane=(t/10)*11+t%10+1,rate=latched_rate(s,lane);s->lane[t].to_next=seq_step_q8(s,rate)/TICKS_PER_STEP[rate]*(TICKS_PER_STEP[rate]-s->lane[t].ext_ticks);}
    }
    clock_transport(s,o,arrangement);
    if (s->ext_pending)
        ext_ticks(s, arrangement,o);
    if (!s->playing) {
        seq_midi_prepare(s,s->midi_ready,o);
        if (!s->ext)
            s->clk_to_next = 0;
        for (t = 0; t < NTRACKS; t++)                /* stopped: a cue takes effect at once */
            if (s->pcue[t] < NPAT) {
                s->ppat[t] = s->pcue[t];
                s->pcue[t] = 0xFF;
            }
        return;
    }
    for (t = 0; t < NTRACKS; t++) {                 /* move time */
        seq_track_t *tr = &s->t[t];
        uint32_t unit=rf?rate_unit(s,t<NKIT?t*11:t+20):256u;
        if (tr->to_next != NEVER)
            tr->to_next = tr->to_next > k*unit ? tr->to_next - k*unit : 0;
        if (tr->to_off)
            tr->to_off = tr->to_off > dq ? tr->to_off - dq : 1;   /* 1: due, still "set" */
    }
    if(poly && s->lane_enabled)for(t=0;t<20;t++){seq_lane_clock_t *c=&s->lane[t];uint32_t unit=rf?rate_unit(s,(t/10)*11+t%10+1):256u;if(c->to_next!=NEVER)c->to_next=c->to_next>k*unit?c->to_next-k*unit:0;}
    if (!clock_rebased && !s->ext && (clock_schedule(s) || WEB_CLOCK || arrangement || poly))
        s->clk_to_next = s->clk_to_next > dq ? s->clk_to_next - dq : 0;
    if(!s->ext && poly && !arrangement && s->clk_to_next<256u)poly_musical_tick(s);
    if (!s->ext && arrangement && s->clk_to_next < 256u)
        arrangement_tick(s); /* before any same-sample notes/steps */
    /* fire what is due; the drum part first, since its wrap may switch the pattern */
    for (guard = 0; guard < 8; guard++) {
        int fired = 0;
        for (t = 0; t < NTRACKS; t++) {
            seq_track_t *tr = &s->t[t];
            if (tr->to_off && tr->to_off < 256u) {    /* gate falls (before a new step at the same time) */
                bass_release(s, t - TRK_BASS0, o);
                fired = 1;
            }
            if (tr->to_next < (rf?rate_unit(s,t<NKIT?t*11:t+20):256u)) {
                uint32_t ch = fire_step(s, t, o, arrangement);
                if (ch)
                    restart(s, ch);                   /* a part's new pattern starts on step 1 */
                fired = 1;
            }
            if(poly && s->lane_enabled && t<NKIT){int v;for(v=1;v<NDRUM;v++)if(s->lane[t*10+v-1].to_next<(rf?rate_unit(s,t*11+v):256u)){row_fire(s,t*11+v,o);fired=1;}}
        }
        if (!fired)
            break;
    }
    if (arrangement) for(t=0;t<NBASS;t++)bass_retune_held(s,t,o);
    seq_midi_sync(s,s->midi_ready,o); /* retry only FINAL played pitches, after boundary/NOTE/TIE */
    if (!s->ext && (clock_schedule(s) || WEB_CLOCK || arrangement || poly) && s->clk_to_next < 256u) {
        uint32_t period = seq_step_q8(s, RATE_16) / 6u;
        clock_tick(s);
        if (clock_output(s)) midi(o, 0xF8, 0, 0);
#ifdef X0X_WEB
        web_seq_tick(period);
#endif
        s->clk_to_next += period;           /* 24 ppqn */
    }
}

uint32_t seq_until_event(seq_t *s, uint32_t n) { return until_event_common(s,n,0,0); }
uint32_t seq_until_event_arrangement(seq_t *s, uint32_t n) { return until_event_common(s,n,1,s->lane_sound!=0); }
void seq_advance(seq_t *s, uint32_t k, const seq_sink_t *o) { advance_common(s,k,o,0,0); }
void seq_advance_arrangement(seq_t *s, uint32_t k, const seq_sink_t *o) { advance_common(s,k,o,1,s->lane_sound!=0); }

/* A main producer may be interrupted after capturing its intent. Retirement
 * and completed role changes advance generation even when pending is clear.
 * Validate that capture and publish all request flags under the same guard. */
static __attribute__((noinline)) void run_request(seq_t *s,int from_top)
{
    uint32_t generation,saved=plat_control_enter();
    unsigned retired=s->clock.flags&CLOCK_RETIRED;
    generation=s->clock.generation;
    plat_control_leave(saved);
    RUN_REQUEST_BOUNDARY(s);
    saved=plat_control_enter();
    if(!s->clock.pending && retired==(s->clock.flags&CLOCK_RETIRED) &&
       generation!=UINT32_MAX && generation==s->clock.generation){
        if(from_top){if(s->playing)s->req_stop=1;s->req_cont=0;s->req_start=1;}
        else if(!s->playing || s->req_stop){s->req_start=0;s->req_cont=1;}
    }
    plat_control_leave(saved);
}
void seq_start(seq_t *s) {run_request(s,1);}
void seq_transport_retire(seq_t *s) {s->clock.flags|=CLOCK_RETIRED;s->out_stop_pending=s->req_stop?2:0;s->req_start=s->req_cont=0;}
void seq_stop(seq_t *s) {s->out_stop_pending&=1u;s->req_start=s->req_cont=0;s->req_stop = 1;}
void seq_continue(seq_t *s) {run_request(s,0);}
void seq_ext_clock(seq_t *s) { s->ext_pending++; }
void seq_cue(seq_t *s, int p)
{
    int t;
    if (p >= 0 && p < NPAT)
        for (t = 0; t < NTRACKS; t++)
            s->pcue[t] = (uint8_t)p;
}
void seq_cue_part(seq_t *s, int t, int p)
{
    if (t >= 0 && t < NTRACKS && p >= 0 && p < NPAT)
        s->pcue[t] = (uint8_t)p;
}
int seq_cue_of(const seq_t *s, int t) { return s->pcue[t] < NPAT ? s->pcue[t] : -1; }
int seq_next_step(seq_t *s, int t, int p) { return next_pos(s, t, p, 0); }
void seq_chain(seq_t *s, int a, int b)
{
    s->chain_a = (uint8_t)a;
    s->chain_b = (uint8_t)b;
}

/* Initialized lane storage shares the arrangement-capable transport loop.
 * The raw legacy specialization remains constant0 and unchanged in behavior;
 * arrangement_tick selects chord arrangement versus plain96-tick progression. */
uint32_t seq_until_event_poly(seq_t *s,uint32_t n) {return seq_until_event_arrangement(s,n);}
void seq_advance_poly(seq_t *s,uint32_t k,const seq_sink_t *o) {seq_advance_arrangement(s,k,o);}
