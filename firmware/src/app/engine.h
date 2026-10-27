/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X audio engine: the five parts, the mixer, the send FX and the master, driven by
 * the sequencer. engine_render() runs in the audio ISR; everything else is called
 * from the main loop and reaches the ISR only through the command queue, so the
 * engines' state is only ever touched by one context.
 *
 * Parameters are addressed as (target, voice, index) -> pot value, the same triple
 * the UI pages, the project file and the command queue use:
 *   T_909 / T_808   voice = track 0..10, or DR_KIT / D8_KIT (= 11) for the kit
 *   T_303           voice = 0 / 1 (303 A / B)
 *   T_BRK           voice 0: breaks.h params (the per-pattern ones live in the pattern)
 *   T_FX            voice 0: fxbus.h params (reverb, delay, master drive / comp)
 *   T_MIX           voice = part 0..4: Level, Rev send, Dly send (drums: on top of the per-voice sends)
 *   T_MST           voice 0: master filter + limiter
 */
#pragma once
#include <stdint.h>
#include "../dsp/x0x_param.h"
#include "../seq/sequencer.h"
#include "../seq/motion.h"
#include "../dsp/sound_strip.h"
/* Validated producer commands; canonical bytes are published before the queue marker. */
int engine_arrangement_block(song_bar_t *dst, uint8_t *degree, const song_bar_t *src, uint8_t chord);
int engine_arrangement_global(uint8_t *dst, uint8_t value, int stopped_only);
int engine_arrangement_length(uint16_t *dst, uint16_t value);
int engine_arrangement_refresh(const arrangement_settings_t *intent);
int engine_arrangement_queue(uint8_t block);
int engine_lane_set(int pattern,int lane,int index,int value);
int engine_lane_mode(int enabled);
int engine_sync_mode(int lane,int value);
int engine_rate_set(int pattern,int lane,int field,int value);
int engine_lane_break_step(int pattern,int step,int gate,int slice);
int engine_cc_refresh(int reconcile);
/* Main context only. Nested holds permit helper calls after incoming requests defer. */
int engine_transport_reserve(void);
void engine_notes_clear(void);
void engine_transport_release(void);

enum { T_909, T_808, T_303, T_BRK, T_FX, T_MIX, T_MST, NTARGETS };
#define T_PROB NTARGETS                /* a lane's pseudo-target: a step's chance, 5..100 % (not in sound_t);
                                        * voice = part << 4 | drum track (0 on the 303s and the break) */
enum { PART_909, PART_808, PART_303A, PART_303B, PART_BRK, NPARTS };
/* T_MIX, per part: level, reverb send, delay send, pan (64 = centre) */
enum { MX_LEVEL, MX_REV, MX_DLY, MX_PAN, MX_NPARAMS };
#define NVOICES_MAX 12                 /* a drum machine: 11 tracks + the kit */
#define NPARAMS_MAX 16                 /* params per (target, voice) */

/* the sound of a project: every pot of every engine, mirrored here by the main loop */
typedef struct {
    uint8_t v[NTARGETS][NVOICES_MAX][NPARAMS_MAX];
} sound_t;

extern seq_t seq;                      /* the sequencer (its pattern pointer is the project's) */
/* render stages (the PERF screen): 909, 808, 303 A, 303 B, BREAK, FX, MASTER, SEQ */
enum { PR_909, PR_808, PR_303A, PR_303B, PR_BRK, PR_FX, PR_MST, PR_SEQ, ENG_PROF_N };
extern volatile uint32_t eng_prof[ENG_PROF_N], eng_prof_frames;
void engine_motion_enable(int on);                          /* off: lanes do not play (the PERF test) */
void engine_motion_reset(void);                             /* lanes replaced (undo): knobs to their own values */
/* overload guard (engine.c): the platform reports each block's load after rendering it */
void engine_load(uint32_t pct, uint32_t frames);           /* ISR, after engine_render */
void engine_guard_enable(int on);                           /* off: full quality always (the PERF test) */
int engine_guard_active(void);
extern volatile uint32_t eng_guard_count;                   /* times it engaged (PERF screen) */
extern volatile uint8_t eng_step[NTRACKS];   /* playheads for the UI */
extern volatile uint16_t eng_peak[NPARTS];   /* part meters, Q15, decaying (UI) */
extern volatile uint8_t eng_hit[NKIT][NDRUM]; /* each drum track's last hit, 255 = full velocity (the UI lets it fall) */
#define SCOPE_N 256u
extern int16_t eng_scope[SCOPE_N];     /* the last output samples, for the screen */
extern volatile uint32_t eng_scope_w;

void engine_init(pattern_t *patterns, song_t *song, lane_t *lanes, const sound_t *base);
void engine_render(int32_t *out_lr, uint32_t n);   /* ISR: n stereo frames, 24-bit in int32 */

/* parameter descriptors (UI) */
int engine_nvoices(int target);
int engine_nparams(int target, int voice);
const x0x_param_t *engine_param(int target, int voice, int i);
const char *engine_voice_name(int target, int voice);

/* main loop -> ISR */
void engine_set(int target, int voice, int i, int value);   /* queued; the sound_t mirror is the caller's */
uint32_t engine_queue_space(void);
uint32_t engine_queue_written(void);
uint32_t engine_queue_read(void);
int engine_strip_set(int strip, int index, int value);     /* 1 queued, 0 full/invalid */
uint32_t engine_strip_processing_mask(void);
const sound_strip_meter_t *engine_strip_meter(int strip); /* audio context only */
const sound_strip_config_t *engine_strip_config(int strip); /* audio context only */
void engine_out_loud(int on);          /* 1: the output at the codec's full scale (6 dB hotter than Felucca's) */
void engine_usb_full(int on);          /* 1: engine_render at full level, engine_volume the MASTER knob after */
void engine_volume(int32_t *out_lr, uint32_t n);   /* ISR, after engine_render (and the USB tap) */
void engine_apply_sound(const sound_t *s);                  /* queue every value (load, init) */
void engine_sound_defaults(sound_t *s);                     /* each engine's power-on values */
void engine_sound_centre_drum_pans(sound_t *s);             /* every drum voice's PAN (its last pot) to centre */
void engine_drum(int kit, int voice, float vel);            /* play now (keys) */
void engine_bass_on(int part, int note, int accent, int slide);
void engine_bass_off(int part);
void engine_brk_live(int key, int down);
void engine_brk_loops(void);                                /* (re)read the A / B loops of the current pattern */
int engine_brk_slice(void);                                 /* slice sounding now, -1 none (UI) */
void engine_brk_state(int *slice, int *bank, int *div, int *running);
int engine_brk_outline(int which, uint8_t *peaks, int n);   /* the loop's peaks for the screen (main loop) */
/* motion (seq/motion.h): lanes play into the engine, never into the sound_t mirror */
int engine_motion_part(int target, int voice);              /* the part a parameter's lane follows */
int engine_motion_value(int lane);                          /* what the lane plays now, -1 none */
void engine_motion_rec(int lane);                           /* record a pass from the next step */
void engine_probability_clear(int lane); /* main, stopped transport reservation only */
void engine_motion_hold(int lane);                          /* the knob wins for a pass */
float engine_gr_db(void);                                   /* master compressor gain reduction (UI meter) */
void engine_master_format(int i, char *buf);                /* a master pot in its unit */
int engine_brk_nslots(void);                                /* built-in loops + user slots */
const char *const *engine_brk_slot_names(void);

int engine_midi_channel_allowed(int channel); /* saved pad channel remains dedicated */

/* Prototype clock API: accepted generation != applied role/status; policy0 is freeze only. */
int engine_clock_request(uint32_t protocol,unsigned role,unsigned source,unsigned policy,uint32_t *generation);
int engine_clock_status(uint32_t protocol,uint32_t words[10]); /* snapshot: protocol,generation,role,source,state,tickLo,tickHi,age,flags,pending */
int engine_clock_source_retire(uint32_t protocol,unsigned source,uint32_t *generation);

int engine_strip_restore(unsigned strip,const sound_strip_config_t *config);
/* Main-only, under an existing transport reservation. Prepare is the acceptance
 * boundary; after it succeeds, finish the journal then commit before releasing
 * the reservation. No other main producer may run between these calls. */
int engine_strip_restore_prepare(unsigned strip,const sound_strip_config_t *config);
void engine_strip_restore_commit(unsigned strip,const sound_strip_config_t *config);

#include "runtime_diagnostics.h"

void engine_timing_init(void);
void engine_timing_status(uint32_t words[10]);
int engine_timing_update(uint32_t rate,uint32_t expected_rate,uint32_t generation,uint32_t old_unit,uint32_t new_unit);
