/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X sequencer; see sequencer.h for the model. */
#include "sequencer.h"
#include "tb3po.h"

#ifdef X0X_WEB
extern void web_seq_transport(int playing, int from_top);
extern void web_seq_tick(uint32_t period_q8);
#define WEB_CLOCK 1
#else
#define WEB_CLOCK 0
#endif

#define NEVER 0xFFFFFFFFu

static const uint8_t TICKS_PER_STEP[NRATES] = {6, 4, 3, 8};     /* 24 ppqn: 1/16, 1/16T, 1/32, 1/8T */
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
    for(i=0;i<NBASS;i++)s->bass_notes_out[i]=s->bass_channels_out[i]=255;
}

static int arrangement_active(const seq_t *s)
{
    return (s->arrangement.flags & ARR_ENABLED) != 0;
}
float seq_tempo(const seq_t *s) { return s->ext ? s->ext_bpm : s->bpm; }

uint32_t seq_step_q8(const seq_t *s, int rate)
{
    float bpm = seq_tempo(s), q;
    if (bpm < 20.0f)
        bpm = 20.0f;
    if (bpm > 300.0f)
        bpm = 300.0f;
    q = (float)FS * 60.0f / bpm;                    /* samples per quarter */
    return (uint32_t)(q * 256.0f * (float)TICKS_PER_STEP[rate & 3] / 24.0f);
}

static const dpart_t *dpart(const seq_t *s, int k) { return &s->pat[s->ppat[k]].drum[k]; }
static int is_drum(int t) { return t < NKIT; }
static int is_bass(int t) { return t >= TRK_BASS0 && t < TRK_BASS0 + NBASS; }
static bpart_t *bpart(const seq_t *s, int b) { return &s->pat[s->ppat[TRK_BASS0 + b]].bass[b]; }

int seq_part_len(const seq_t *s, int t)
{
    int n = is_drum(t) ? dpart(s, t)->len : t == TRK_BRK ? 16 : bpart(s, t - TRK_BASS0)->len;
    return n < 1 ? 1 : n > NSTEPS ? NSTEPS : n;
}

static int part_rate(const seq_t *s, int t)
{
    return (is_drum(t) ? dpart(s, t)->rate : t == TRK_BRK ? RATE_16 : bpart(s, t - TRK_BASS0)->rate) & 3;
}

/* swing offset of a step, Q8: odd 16ths / 32nds come late; triplets are never swung */
static uint32_t swing_q8(const seq_t *s, int rate, uint32_t step_q8)
{
    uint32_t sw = s->pat[s->ppat[TRK_DRUM]].swing;   /* the 909 is the bar: its pattern's groove */
    if (rate == RATE_16T || rate == RATE_8T || !sw)
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
static int next_pos(seq_t *s, int t, int p, int commit)
{
    seq_track_t *tr = &s->t[t];
    int len = seq_part_len(s, t), dir = is_bass(t) ? bpart(s, t - TRK_BASS0)->dir : DIR_FWD, n;
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
    int b;s->midi_ready=(uint8_t)(connected!=0);
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

static void fire_drum(seq_t *s, int k, int p, const seq_sink_t *o)
{
    const dpart_t *d = dpart(s, k);
    uint32_t bit = 1u << (p & 31);
    int w = p >> 5;
    float vel = (d->accent[w] & bit) ? 1.0f : (float)s->accent_q7 / 127.0f;
    int v;
    for (v = 0; v < NDRUM; v++) {
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

static void arrangement_latch(seq_t *s, uint16_t block)
{
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
            s->ppat[t] = next[t]; tr->started = 0; tr->bars = 0; tr->pp_dir = 1;
            tr->to_next = s->ext ? NEVER : 0;
            tr->ext_ticks = (uint8_t)(TICKS_PER_STEP[part_rate(s,t)] - 1);
        }
        s->pcue[t] = 255;
    }
}

static void arrangement_tick(seq_t *s)
{
    volatile arrangement_runtime_t *a = &s->arrangement;
    if (!arrangement_active(s) || !s->playing) return;
    if (!a->clock_started) { a->clock_started = 1; return; }
    if (++a->tick < 96) return;
    a->tick = 0;
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
static uint32_t fire_step(seq_t *s, int t, const seq_sink_t *o, int arrangement)
{
    seq_track_t *tr = &s->t[t];
    int rate = part_rate(s, t), p, wrapped;
    uint32_t changed = 0;
    uint32_t L = seq_step_q8(s, rate);
    uint32_t carry = tr->to_next < 256u ? tr->to_next : 0;   /* the step was due this far into the sample: keep it */
    int bass = is_bass(t);
    if (!tr->started) {
        p = (bass && bpart(s, t - TRK_BASS0)->dir == DIR_REV) ? seq_part_len(s, t) - 1 : 0;
        tr->started = 1;
        wrapped = 0;
    } else {
        p = next_pos(s, t, tr->pos, 1);
        wrapped = !bass ? p == 0 : p == 0 || (bpart(s, t - TRK_BASS0)->dir == DIR_REV && p == seq_part_len(s, t) - 1);
    }
    if (wrapped && t == TRK_DRUM && !arrangement && (changed = bar_end(s, 0)) != 0) {
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
        fire_drum(s, t, p, o);
    else if (t == TRK_BRK) {
        if (o->brk)
            o->brk(o->ctx, p, tr->bars, (float)L / 256.0f,
                   (s->pat[s->ppat[TRK_BRK]].brk.steps >> p & 1u) && !(s->mute & (1u << MUTE_BRK)));
    } else
        fire_bass(s, t - TRK_BASS0, p, L, carry, o);
    if (s->ext) {
        tr->to_next = NEVER;                         /* the clock arms the next one */
    } else {
        /* the step that follows p: swung if it is an odd one */
        int np = next_pos(s, t, p, 0);
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
        s->t[t].ext_ticks = (uint8_t)(TICKS_PER_STEP[part_rate(s, t)] - 1);
        s->t[t].bars = 0;
        s->t[t].pp_dir = 1;
    }
}

static void do_start(seq_t *s, int from_top, const seq_sink_t *o, int arrangement)
{
    int t;
    if (from_top) {
        s->arrangement.reserved=0;
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
        tr->to_off = 0;
        tr->to_next = s->ext ? NEVER : 0;
        tr->ext_ticks = (uint8_t)(TICKS_PER_STEP[part_rate(s, t)] - 1);   /* the first clock is the downbeat */
    }
    s->clk_to_next = 0;
    s->playing = 1;
#ifdef X0X_WEB
    web_seq_transport(1, from_top);
#endif
    if (!s->ext && s->send_clock)
        midi(o, from_top ? 0xFA : 0xFB, 0, 0);
}

static void do_stop(seq_t *s, const seq_sink_t *o, int arrangement)
{
    int b, t;
    s->playing = 0;
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
    if (!s->ext && s->send_clock)
        midi(o, 0xFC, 0, 0);
}

/* external clock ticks: arm the steps they complete */
static void ext_ticks(seq_t *s, int arrangement)
{
    int t;
    uint32_t n = s->ext_pending;
    s->ext_pending = 0;
    for (; n; n--) {
        if (s->ext_since > 0 && s->ext_since < FS) {  /* tempo: smoothed tick period */
            uint32_t q = s->ext_since << 8;
            s->ext_period = s->ext_period ? s->ext_period - (s->ext_period >> 3) + (q >> 3) : q;
            s->ext_bpm = (float)FS * 60.0f * 256.0f / (24.0f * (float)s->ext_period);
        }
        s->ext_since = 0;
        if (!s->playing)
            continue;
        /* Musical block changes precede track arming on the same incoming clock. */
        if (arrangement) arrangement_tick(s);
        for (t = 0; t < NTRACKS; t++) {
            seq_track_t *tr = &s->t[t];
            int rate = part_rate(s, t), tps = TICKS_PER_STEP[rate];
            if (++tr->ext_ticks < tps)
                continue;
            tr->ext_ticks = 0;
            {
                int np = tr->started ? next_pos(s, t, tr->pos, 0) : 0;
                uint32_t L = s->ext_period ? s->ext_period * (uint32_t)tps : seq_step_q8(s, rate);
                tr->to_next = (np & 1) ? swing_q8(s, rate, L) : 0;
            }
        }
    }
}

static inline __attribute__((always_inline)) uint32_t until_event_common(seq_t *s, uint32_t n, int arrangement)
{
    uint32_t m = n, e;
    int t;
    if (!s->playing)
        return n;
    if (s->ext && arrangement) {
        e = s->ext_since > FS / 2u ? 1u : FS / 2u + 1u - s->ext_since;
        if (e < m) m = e;
    }
    for (t = 0; t < NTRACKS; t++) {
        if (s->t[t].to_next != NEVER && (e = s->t[t].to_next >> 8) < m)
            m = e;
        if (s->t[t].to_off && (e = s->t[t].to_off >> 8) < m)
            m = e;
    }
    if (!s->ext && (s->send_clock || WEB_CLOCK || arrangement) && (e = s->clk_to_next >> 8) < m)
        m = e;
    return m ? m : 1;
}

static inline __attribute__((always_inline)) void advance_common(seq_t *s, uint32_t k, const seq_sink_t *o, int arrangement)
{
    int t, guard, clock_rebased = 0;
    uint32_t dq = k << 8;
    s->ext_since += k;
    if (s->transport_hold) { /* stopped: retain elapsed clock time, discard pre-start clock ticks */
        s->ext_pending = 0; /* Start/Continue/Stop requests remain pending until release */
        return;
    }
    if (s->ext_pending)
        s->ext = 1;
    else if (s->ext && s->ext_since > FS / 2u) {    /* clock gone for 0.5 s: back to internal */
        s->ext = 0;
        if (arrangement) {
            s->clk_to_next = seq_step_q8(s,RATE_16) / 6u; /* resume next tick, without duplicating the last one */
            clock_rebased = 1;
        }
        for (t = 0; t < NTRACKS; t++)
            if (s->playing && s->t[t].to_next == NEVER)
                s->t[t].to_next = 0;
    }
    if (s->req_stop) {
        s->req_stop = 0;
        s->req_start = s->req_cont = 0;
        if (s->playing)
            do_stop(s, o, arrangement);
    }
    if (s->req_start) {
        s->req_start = 0;
        do_start(s, 1, o, arrangement);
    }
    if (s->req_cont) {
        s->req_cont = 0;
        do_start(s, 0, o, arrangement);
    }
    if (s->ext_pending)
        ext_ticks(s, arrangement);
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
        if (tr->to_next != NEVER)
            tr->to_next = tr->to_next > dq ? tr->to_next - dq : 0;
        if (tr->to_off)
            tr->to_off = tr->to_off > dq ? tr->to_off - dq : 1;   /* 1: due, still "set" */
    }
    if (!clock_rebased && !s->ext && (s->send_clock || WEB_CLOCK || arrangement))
        s->clk_to_next = s->clk_to_next > dq ? s->clk_to_next - dq : 0;
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
            if (tr->to_next < 256u) {
                uint32_t ch = fire_step(s, t, o, arrangement);
                if (ch)
                    restart(s, ch);                   /* a part's new pattern starts on step 1 */
                fired = 1;
            }
        }
        if (!fired)
            break;
    }
    if (arrangement) for(t=0;t<NBASS;t++)bass_retune_held(s,t,o);
    seq_midi_sync(s,s->midi_ready,o); /* retry only FINAL played pitches, after boundary/NOTE/TIE */
    if (!s->ext && (s->send_clock || WEB_CLOCK || arrangement) && s->clk_to_next < 256u) {
        uint32_t period = seq_step_q8(s, RATE_16) / 6u;
        if (s->send_clock) midi(o, 0xF8, 0, 0);
#ifdef X0X_WEB
        web_seq_tick(period);
#endif
        s->clk_to_next += period;           /* 24 ppqn */
    }
}

uint32_t seq_until_event(seq_t *s, uint32_t n) { return until_event_common(s,n,0); }
uint32_t seq_until_event_arrangement(seq_t *s, uint32_t n) { return until_event_common(s,n,1); }
void seq_advance(seq_t *s, uint32_t k, const seq_sink_t *o) { advance_common(s,k,o,0); }
void seq_advance_arrangement(seq_t *s, uint32_t k, const seq_sink_t *o) { advance_common(s,k,o,1); }

void seq_start(seq_t *s) { s->req_start = 1; }
void seq_stop(seq_t *s) { s->req_stop = 1; }
void seq_continue(seq_t *s) { s->req_cont = 1; }
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
