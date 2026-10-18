/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X sequencer: runs in the audio ISR, sample accurate.
 *
 * The engine renders a block like this:
 *     while (n) {
 *         k = seq_until_event(&seq, n);       // samples until the next due event (>= 1)
 *         render k samples;
 *         seq_advance(&seq, k, &sink);        // moves time, fires the events now due
 *         n -= k;
 *     }
 * so a step lands on its exact sample, and engine code never sees time.
 *
 * Five parts, each with its own step counter (lengths and rates may differ):
 * 909, 808, 303 A, 303 B, BREAK (always 16ths, 16 to the bar). Each part plays its own
 * pattern (ppat[part]); track t is part t. The 909 part's length defines the bar: cued
 * patterns (or the next one of a chain, or the song's next bar) start when it wraps, and a
 * part whose pattern changed restarts on step 1 while the others keep running.
 *
 * Song: one entry per bar, each part's pattern and the part mutes. song_on plays it (and
 * loops); song_on + song_rec writes the live state (cues, mutes) into it from song_start on.
 *
 * Clock: internal (BPM) or external (24 ppqn MIDI clock, fed with seq_ext_clock()).
 * Swing delays every second 16th (909 shuffle: 0 = straight, 100 = 75 %).
 *
 * 303 semantics (as the TB-303 and Open303's sequencer):
 *   NOTE  plays; the gate falls at half the step, unless the step slides (BS_SLIDE)
 *         or the next step is a TIE, in which case it is held through.
 *   TIE   continues the sounding note (no retrigger); same gate rule.
 *   slide a NOTE following a step with BS_SLIDE glides from the held note
 *         (note_on with slide = 1, envelopes not retriggered).
 *   REST  releases.
 */
#pragma once
#include <stdint.h>
#include "pattern.h"
#include "harmony.h"
#include "lanes.h"

#define FS 44100u
#define TRK_DRUM 0                    /* + kit: 0 = 909, 1 = 808 */
#define TRK_BASS0 NKIT
#define TRK_BRK (NKIT + NBASS)
#define NTRACKS (NKIT + NBASS + 1)
/* mute bits: kit k voice v = k * NDRUM + v; then 303 A, 303 B, BREAK */
#define MUTE_BASS0 (NKIT * NDRUM)
#define MUTE_BRK (MUTE_BASS0 + NBASS)

/* the mute bits of part p (a drum machine: all its voices) */
static inline uint32_t seq_part_mask(int p)
{
    return p < NKIT ? ((1u << NDRUM) - 1u) << (p * NDRUM) : 1u << (MUTE_BASS0 + p - NKIT);
}

#define NSONG 192
#define ARR_ENABLED 1u
#define ARR_NOTES_OUT 32u /* schema4 tracked mirror; settings_t is outside undo journal */
#define ARR_FLAGS (63u | POLY_FLAG)
enum { BASS_FOLLOW_OFF, BASS_FOLLOW_ROOT, BASS_FOLLOW_CHORD };
static inline unsigned arrangement_bass_mode(unsigned flags,int b) { return (flags >> (1+2*b)) & 3u; }
typedef struct {
    uint8_t flags, key, mode, parent;
    uint8_t bass_low[2], bass_high[2];
    uint8_t pad_channel, pad_root, pad_low, pad_high;
    uint8_t degree[NSONG];
} arrangement_settings_t;
typedef char arrangement_settings_size_204[(sizeof(arrangement_settings_t) == 204) ? 1 : -1];
/* Current block/global intent is latched on musical boundaries; never a second song table. */
typedef struct {
    const arrangement_settings_t *intent;
    uint8_t flags, key, mode, parent;
    uint8_t pat[NTRACKS], mute, degree, tick;
    uint8_t queued, editing, clock_started, reserved;
} arrangement_runtime_t;
#define SEQ_CH_OFF 0xFFu
/* a step's chance (the T_PROB lanes): 1-100 = that percentage, PROB_COND + k = the cycle k in the order
 * 1:2 2:2 1:3 2:3 3:3 1:4 ... 8:8 (A:B: on the A-th of every B passes of the part's pattern) */
#define PROB_COND 101
#define PROB_NCOND 35                    /* 2 + 3 + ... + 8 */
static inline void prob_cond(int k, int *a, int *b)
{
    int n = 2;
    while (k >= n) {
        k -= n;
        n++;
    }
    *a = k + 1;
    *b = n;
}
#define SONG_REP(b) (((b)->mute >> 5) + 1)    /* a bar plays 1-8 times: mute's bits 5-7 (the parts use 0-4) */
typedef struct {
    uint8_t pat[NTRACKS];             /* each part's pattern */
    uint8_t mute;                     /* bit p = part p muted */
} song_bar_t;
typedef struct {
    uint16_t len;                     /* bars; 0 = empty */
    uint8_t rsv[2];
    song_bar_t bar[NSONG];
} song_t;

typedef struct {
    int (*drum)(void *ctx, int kit, int voice, float vel);      /* 0: the hit did not play (its chance) */
    void (*bass_on)(void *ctx, int part, int note, int accent, int slide);
    void (*bass_off)(void *ctx, int part);
    void (*brk)(void *ctx, int step16, int bar, float samples_per_16th, int enabled);
    void (*brk_stop)(void *ctx);
    void (*midi)(void *ctx, uint8_t status, uint8_t d1, uint8_t d2);   /* clock / transport / notes out */
    void (*step)(void *ctx, int part, int step);                        /* UI playhead (optional) */
    void *ctx;
    void (*bass_retune)(void *ctx,int part,int note,int accent); /* slide, no probability draw */
    int (*bass_trigger)(void *ctx,int part,int note,int accent,int slide); /* optional actual trigger status */
    int (*midi_try)(void *ctx,uint8_t status,uint8_t d1,uint8_t d2);
    int (*midi_pair_try)(void *ctx,uint8_t st1,uint8_t n1,uint8_t v1,uint8_t st2,uint8_t n2,uint8_t v2);
    int (*midi_batch_try)(void *ctx,const uint32_t *packets,uint32_t count); /* atomic <=6 USB-MIDI packets */
    void (*midi_cancel)(void *ctx); /* browser-only stale queued attacks; physical sink leaves null */
    void (*midi_transport_cancel)(void *ctx); /* explicit Stop transport epoch */
    void (*clock_retire)(void *ctx,uint8_t sources); /* bounded audio-owned ingress cutoff */
} seq_sink_t;

typedef struct {                      /* one part's runtime */
    uint8_t pos;                      /* current step */
    int8_t pp_dir;                    /* ping-pong direction */
    uint8_t started;                  /* the first step has fired since start */
    uint8_t sounding;                 /* 303: a note is held */
    uint8_t held_note;
    uint8_t source_note; /* accepted untransposed NOTE for live CC reprojection; runtime only */
    uint8_t trigger_rejected; /* uses existing alignment gap; chance failure is not strict failure */
    uint32_t to_next;                 /* samples to the next step (Q8 fraction) */
    uint32_t to_off;                  /* 303: samples to the gate falling, Q8; 0 = none */
    uint32_t rng;                     /* DIR_RANDOM and auto-mutate */
    uint16_t bars;                    /* wraps since the pattern started (auto-mutate) */
    uint8_t ext_ticks;                /* external clock: ticks into this step */
} seq_track_t;

typedef struct { uint32_t to_next; uint16_t bars; uint8_t pos,ext_ticks; } seq_lane_clock_t;
typedef char lane_clock_size_8[(sizeof(seq_lane_clock_t)==8)?1:-1];
#define CLOCK_PROTOCOL ((1u << 16) | 1u) /* version1 / basic capability1, not a register address */
enum { CLOCK_LEGACY, CLOCK_STANDALONE, CLOCK_LEAD, CLOCK_FOLLOW };
enum { CLOCK_STOPPED, CLOCK_WAITING, CLOCK_RUNNING, CLOCK_LOST, CLOCK_TRANSITION };
#define CLOCK_RETIRED 1u
#define CLOCK_TICK_VALID 2u
#define CLOCK_CONFIG_APPLIED 4u
#define CLOCK_INVALIDATED 8u
/* Runtime only; no persistent settings ABI change. Explicit 20 bytes, no spare-byte assumption. */
typedef struct {
    uint32_t tick_lo,tick_hi,generation,age;
    uint8_t role_source,state,pending,flags;
} clock_runtime_t;
typedef char clock_runtime_size_20[(sizeof(clock_runtime_t)==20)?1:-1];
typedef struct {
    volatile clock_runtime_t clock; /* audio state; main snapshots/retirement under guard */
    uint8_t *lane_sound; /* audio-owned published metadata view; null retains old algorithm */
    seq_lane_clock_t lane[20]; /* voices1..10; voice0 and pitched lanes reuse t[] */
    uint32_t lane_pending; /* applied intent awaits a local step, one bit per lane */
    uint8_t lane_enabled; /* effective musical-boundary clock mode; uses alignment padding */
    uint16_t *rate_units; /* LAN2: engine-owned .pool, BPM tenths per in-flight interval; 0 means external Q10 */
    uint8_t lane_rate[10]; /* 25 x 3 bits, latched through current interval */
    pattern_t *pat;                   /* NPAT patterns (the project's) */
    song_t *song;                     /* the project's song (may be 0) */
    volatile uint8_t playing;         /* 1 = running */
    volatile uint8_t ppat[NTRACKS];   /* the pattern each part plays */
    volatile uint8_t pcue[NTRACKS];   /* each part's cued pattern (0xFF = none) */
    volatile uint8_t chain_a, chain_b;/* chain range (whole patterns); a == b = no chain */
    volatile uint8_t song_on;         /* 1 = the song decides the patterns, bar by bar */
    volatile uint8_t song_rec;        /* with song_on: write the live state into the song */
    volatile uint16_t song_pos;       /* bar playing */
    volatile uint16_t song_start;     /* bar PLAY starts at */
    volatile uint8_t ext;             /* 1 = following an external clock */
    volatile uint8_t send_clock;      /* 1 = emit MIDI clock + transport when internal */
    volatile uint8_t send_notes;      /* 1 = echo the sequence on MIDI, on each part's channel */
    volatile uint8_t ch[NTRACKS];     /* each part's MIDI channel, 0-15 (in and out); SEQ_CH_OFF = none */
    uint8_t song_rep;                 /* the song bar playing: its passes still to play (SONG_REP) */
    volatile uint32_t mute;           /* MUTE_* bits */
    volatile uint32_t vmute;          /* drum tracks muted on their own (bit k * NDRUM + v): kept apart from
                                       * the part mutes, which the song and HOME set as a whole */
    volatile uint8_t accent_q7;       /* drum velocity of a non-accented hit, of 127 */
    float bpm;                        /* internal tempo (UI writes; read per step) */
    float ext_bpm;                    /* tempo measured from the external clock */
    seq_track_t t[NTRACKS];           /* TRK_DRUM, TRK_BASS0.., TRK_BRK */
    uint32_t clk_to_next;             /* internal: samples to the next MIDI clock, Q8 */
    uint32_t ext_since;               /* samples since the last external clock (tempo estimate) */
    uint32_t ext_period;              /* smoothed samples per external clock, Q8 */
    volatile uint32_t ext_pending;    /* external clocks received, not yet consumed */
    volatile uint32_t transport_epoch, stop_epoch; /* preserve qualified action witnesses */
    volatile uint8_t req_start, req_stop, req_cont; /* transport requests from the UI / MIDI */
    uint8_t out_stop_pending;          /* bit0 retry FC; bit1 retired local-only Stop; target not qualified */
    uint32_t mut_rng[NBASS];          /* auto-mutate random streams */
    uint8_t bass_notes_out[NBASS], bass_channels_out[NBASS]; /* actually emitted ownership;255 inactive */
    uint8_t pad_notes_out[3], pad_channel_out, pad_status; /* five runtime bytes;255 inactive */
    uint8_t midi_ready;                /* platform connection, not receiver-delivery proof */
    volatile uint8_t transport_hold, transport_stage; /* main-only nested count; audio defers requests while nonzero */
    volatile arrangement_runtime_t arrangement; /* 20 bytes on target 32-bit ABI; ISR/main snapshots */
} seq_t;
typedef char seq_transport_epochs_aligned[((__builtin_offsetof(seq_t,transport_epoch)|__builtin_offsetof(seq_t,stop_epoch))&3u)==0?1:-1];

/* Audio owner: a single effective flag proves pointer/nonempty-song prerequisites. */
static inline __attribute__((always_inline)) void seq_arrangement_apply_flags(seq_t *s)
{
    const arrangement_settings_t *intent = s->arrangement.intent;
    s->arrangement.flags = intent && s->song && s->song->len ? intent->flags : 0;
    if (!s->playing && intent) {
        if (s->song && s->song_start >= s->song->len) s->song_start = 0;
        s->arrangement.queued = 255;
    }
}
static inline __attribute__((always_inline)) void seq_arrangement_apply(seq_t *s, const arrangement_settings_t *intent)
{
    s->arrangement.intent = intent; seq_arrangement_apply_flags(s);
}
static inline __attribute__((always_inline)) void seq_arrangement_queue(seq_t *s, uint8_t block)
{
    if (!(s->arrangement.flags & ARR_ENABLED) || block >= s->song->len) return;
    if (s->playing) s->arrangement.queued = block;
    else { s->song_start = block; s->arrangement.queued = 255; }
}

void seq_init(seq_t *s, pattern_t *patterns);
uint32_t seq_until_event(seq_t *s, uint32_t n);
void seq_advance(seq_t *s, uint32_t k, const seq_sink_t *o);
/* Legacy functions above retain their measured gate; engine selects these feature
 * specializations once per block from the applied audio-owned flag after drain. */
uint32_t seq_until_event_arrangement(seq_t *s, uint32_t n);
void seq_advance_arrangement(seq_t *s, uint32_t k, const seq_sink_t *o);
uint32_t seq_until_event_poly(seq_t *s,uint32_t n);
void seq_advance_poly(seq_t *s,uint32_t k,const seq_sink_t *o);
int seq_lane_length(const seq_t *s,int lane);
int seq_lane_rate(const seq_t *s,int lane);
unsigned seq_lane_cycles(const seq_t *s,int lane);
int seq_lane_position(const seq_t *s,int lane);
int seq_lane_latched_rate(const seq_t *s,int lane);

/* main loop / MIDI side (all are flag writes picked up at the next block) */
void seq_start(seq_t *s);             /* from step 1 of the current (or cued) pattern */
void seq_transport_retire(seq_t *s); /* explicit route/load invalidation, not note reconciliation */
void seq_stop(seq_t *s);
void seq_notes_clear(seq_t *s,const seq_sink_t *sink);
void seq_midi_prepare(seq_t *s,int connected,const seq_sink_t *sink); /* cleanup only; never retries On */
void seq_midi_sync(seq_t *s,int connected,const seq_sink_t *sink);
void seq_cc_reconcile(seq_t *s,const seq_sink_t *sink);
enum { PAD_NONE, PAD_NO_CHORD, PAD_RANGE, PAD_COLLISION, PAD_DISCONNECTED, PAD_OUTPUT_DISABLED };
int seq_pad_channel_available(const seq_t *s,int channel);
int seq_pad_desired(const seq_t *s,uint8_t notes[3]);
int seq_pad_pending(const seq_t *s);
void seq_continue(seq_t *s);
void seq_ext_clock(seq_t *s);
int seq_clock_admit(seq_t *s,unsigned source,unsigned status);
void seq_clock_request(seq_t *s,uint8_t packed,const seq_sink_t *o);         /* one 24-ppqn tick (USB MIDI 0xF8) */
void seq_cue(seq_t *s, int pattern);  /* every part, at the end of the bar (now, if stopped) */
void seq_cue_part(seq_t *s, int part, int pattern);
void seq_chain(seq_t *s, int a, int b);
int seq_cue_of(const seq_t *s, int part);   /* the part's cued pattern, or -1 */
int seq_part_muted(const seq_t *s, int part);

/* steps of one part's rate, in samples Q8 (no swing) at the current tempo */
uint32_t seq_step_q8(const seq_t *s, int rate);
float seq_tempo(const seq_t *s);      /* the tempo in effect (internal or measured) */
int seq_part_len(const seq_t *s, int t);         /* steps in part t's pattern */
int seq_next_step(seq_t *s, int t, int p);       /* the step after p (direction; no commit) */

void seq_lane_mark_pending(seq_t *s,int lane,int rate);
