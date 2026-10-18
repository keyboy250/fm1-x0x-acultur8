/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X audio engine; see engine.h. */
#include "engine.h"
#include "../seq/rate_families.h"
#include "x0x.h" /* canonical project declaration for bounded MIDI intent commits */
#include "plat.h"
#include "../dsp/drum909.h"
#include "../dsp/drum808.h"
#include "../dsp/bass303.h"
#include "../dsp/breaks.h"
#include "../dsp/fxbus.h"
#include "../dsp/master.h"
#include "../dsp/fastmath.h"

#define POOL X0X_POOL
static POOL uint16_t rate_units[25];

/* time per stage of the render, in plat_cycles() units (the CPU's cycles, or the 24 MHz timer):
 * running totals (wrapping; the PERF screen takes differences) and the frames rendered. */
volatile uint32_t eng_prof[ENG_PROF_N], eng_prof_frames;
#define PROF_T(v) uint32_t v = plat_cycles()
#define PROF_ADD(k, t0) (eng_prof[k] += plat_cycles() - (t0))

/* ---------------------------------------------------------------- state --- */
seq_t seq;
volatile uint8_t eng_step[NTRACKS];
volatile uint16_t eng_peak[NPARTS];
volatile uint8_t eng_hit[NKIT][NDRUM];
int16_t eng_scope[SCOPE_N];
volatile uint32_t eng_scope_w;

static drum909_t d909 POOL;
static drum808_t d808 POOL;
static bass303_t b303[NBASS] POOL;
static breaks_t brk POOL;
static fxbus_t fx POOL;
static master_t mst;
#define DLY_LEN 88200
static int16_t dly_buf[DLY_LEN] POOL;
static volatile uint8_t brk_hold;      /* main loop is rewriting the break loops: ISR leaves it alone */
static motion_t mot;
static const sound_t *mot_base_snd;    /* the knobs' own values (the project's mirror) */
static uint8_t mot_playing;
static volatile uint8_t mot_on = 1;
static uint8_t mot_was_on = 1;
void engine_motion_enable(int on) { mot_on = (uint8_t)(on != 0); }   /* the ISR releases the knobs */
static volatile uint8_t mot_release_req;
void engine_motion_reset(void) { mot_release_req = 1; }   /* lanes changed under it (undo): knobs back */

/* overload guard: past the CPU (a block over 92%, or 85% on average) the two 303s drop their
 * oversampling (~12% of the CPU: a little aliasing on bright notes) and the 808's voices end at
 * -50 dB instead of -90 (~2%); after 3 s back under 65% both return. The platform reports each
 * block's load after rendering it (engine_load, same context as engine_render); the change is
 * made at the start of the next block. Off during the PERF test, which measures the full cost. */
#define GUARD_ON_LAST 92u
#define GUARD_ON_AVG 85u
#define GUARD_OFF 65u
#define GUARD_CALM_FRAMES (3u * 44100u)
extern float drum808_quiet;
static uint8_t guard_on, guard_applied, guard_enabled = 1;
static uint32_t guard_avg_q8, guard_calm;
volatile uint32_t eng_guard_count;
void engine_guard_enable(int on) { guard_enabled = (uint8_t)(on != 0); }
int engine_guard_active(void) { return guard_on; }
void engine_load(uint32_t pct, uint32_t frames)
{
    guard_avg_q8 = (guard_avg_q8 * 15u + pct * 256u) / 16u;
    if (!guard_enabled) {
        guard_on = 0;
        guard_calm = 0;
    } else if (!guard_on) {
        if (pct > GUARD_ON_LAST || guard_avg_q8 > GUARD_ON_AVG * 256u) {
            guard_on = 1;
            guard_calm = 0;
            eng_guard_count++;
        }
    } else if (pct >= GUARD_OFF || guard_avg_q8 >= GUARD_OFF * 256u) {
        guard_calm = 0;
    } else if ((guard_calm += frames) >= GUARD_CALM_FRAMES) {
        guard_on = 0;
    }
}
/* Rare state change stays outside the render caller; unchanged fast-path check
 * remains inline. Report this callee separately from engine_render's static gate. */
__attribute__((noinline)) static void guard_update(void)
{
    int k;
    guard_applied = guard_on;
    for (k = 0; k < NBASS; k++)
        bass303_set_lite(&b303[k],guard_on);
    drum808_quiet = guard_on ? 3.2e-3f : 3.2e-5f;
}
static inline void guard_apply(void)
{
    if (guard_on != guard_applied) guard_update();
}

/* mixer: per part Level, Rev send, Dly send (a drum machine's add to its voices' own sends) */
static const x0x_param_t MIX_P[MX_NPARAMS] = {{"LEVEL", 127, 100, 0}, {"REV", 127, 0, 0}, {"DLY", 127, 0, 0},
                                               {"PAN", 127, 64, 0}};
static const char *const PART_NAMES[NPARTS] = {"909", "808", "303 A", "303 B", "BREAK"};
static float mix_level[NPARTS], mix_rev[NPARTS], mix_dly[NPARTS];
/* pan: a balance law. Centre is full level on both sides (a centred mix is the mono one exactly);
 * turning toward a side fades the other out along a quarter cosine, the near side stays at 1 */
static float mix_pl[NPARTS] = {1, 1, 1, 1, 1}, mix_pr[NPARTS] = {1, 1, 1, 1, 1};

/* ------------------------------------------------------- command queue --- */
/* uint32 commands, single producer (main loop) / single consumer (audio ISR):
 * type:4 | a:8 | b:8 | c:12 */
enum { C_PARAM_T0 = 0, C_DRUM = 8, C_BON, C_BOFF, C_BRK, C_LANE, C_STRIP, C_ARR, C_ARR_QUEUE };
#define CQ 1024u
static uint32_t cq[CQ];
/* Same zeroed RAM and aligned publication semantics; small cursor metadata
 * uses 8 bytes of existing pool reserve, preserving its >=8KiB spare gate. */
static volatile uint32_t cq_w POOL, cq_r POOL;
static sound_strip_config_t strip_cfg[25];
static sound_strip_t mono_strip[3] POOL; /* 303 A/B and BREAK; existing mono buffers */
static sound_strip_meter_t mono_strip_meter[3] POOL;
/* Once per outer block, after prior core2 job has joined, before its next dispatch. */
__attribute__((noinline)) static void strip_block_begin(uint32_t dirty)
{
    int i;
    for (i = 0; i < 11; i++) {
        if (dirty & (1u << i)) drum909_strip_set(&d909,i,&strip_cfg[i]);
        if (dirty & (1u << (i+11))) drum808_strip_set(&d808,i,&strip_cfg[i+11]);
    }
    for (i = 0; i < 3; i++) {
        if (dirty & (1u << (i+22))) sound_strip_set(&mono_strip[i],&strip_cfg[i+22]);
        sound_strip_meter_clear(&mono_strip_meter[i]);
    }
    drum909_strip_meter_clear(&d909); drum808_strip_meter_clear(&d808);
}
__attribute__((noinline)) static void mono_strip_process(int strip, float *buf, uint32_t n)
{
    sound_strip_meter_t m;
    if (sound_strip_bypassed(&mono_strip[strip])) return;
    sound_strip_process(&mono_strip[strip],buf,buf,n,&m);
    sound_strip_meter_merge(&mono_strip_meter[strip],&m);
}
typedef char arrangement_block_has_six_bytes[(NTRACKS == 5 && sizeof(song_bar_t) == 6) ? 1 : -1];

int engine_clock_request(uint32_t protocol,unsigned role,unsigned source,unsigned policy,uint32_t *generation){
    uint32_t saved,w,g;
    if(protocol!=CLOCK_PROTOCOL || role>CLOCK_FOLLOW || source>2 || policy || (role!=CLOCK_FOLLOW && source))return -1;
    saved=plat_control_enter();w=cq_w;g=seq.clock.generation;
    if(w!=cq_r || seq.clock.pending || (seq.clock.flags&CLOCK_RETIRED) || g==UINT32_MAX){plat_control_leave(saved);return -2;}
    g++;
    cq[w%CQ]=(C_ARR<<28)|(3u<<20)|((role|(source<<2))<<12)|(g&4095u);
    cq[(w+1)%CQ]=(C_ARR<<28)|(4u<<20)|(((g>>12)&255u)<<12)|(g>>20);
    __asm__ volatile("" ::: "memory");cq_w=w+2;
    if(generation)*generation=g;
    plat_control_leave(saved);return 0;
}
int engine_clock_source_retire(uint32_t protocol,unsigned source,uint32_t *generation){
    uint32_t saved,w,g;
    if(protocol!=CLOCK_PROTOCOL || source>2)return -1;
    saved=plat_control_enter();w=cq_w;g=seq.clock.generation;
    if(w!=cq_r || seq.clock.pending || (seq.clock.flags&CLOCK_RETIRED) || g==UINT32_MAX){plat_control_leave(saved);return -2;}
    g++;
    cq[w%CQ]=(C_ARR<<28)|(5u<<20)|(source<<12)|(g&4095u);
    cq[(w+1)%CQ]=(C_ARR<<28)|(4u<<20)|(((g>>12)&255u)<<12)|(g>>20);
    __asm__ volatile("" ::: "memory");cq_w=w+2;
    if(generation)*generation=g;plat_control_leave(saved);return 0;
}
int engine_clock_status(uint32_t protocol,uint32_t words[10]){
    uint32_t saved;
    if(protocol!=CLOCK_PROTOCOL || !words)return -1;
    saved=plat_control_enter();
    words[0]=CLOCK_PROTOCOL;words[1]=seq.clock.generation;words[2]=seq.clock.role_source&3u;words[3]=seq.clock.role_source>>2;
    words[4]=seq.clock.pending?CLOCK_TRANSITION:seq.clock.state==CLOCK_LOST?CLOCK_LOST:seq.playing?CLOCK_RUNNING:seq.clock.state;
    words[5]=seq.clock.tick_lo;words[6]=seq.clock.tick_hi;words[7]=seq.clock.age;words[8]=seq.clock.flags;words[9]=seq.clock.pending;
    plat_control_leave(saved);return 0;
}
static int cq_put(uint32_t type, uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t w = cq_w;
    if (w - cq_r >= CQ)
        return 0;                      /* new control API checks capacity before mirror mutation */
    cq[w % CQ] = (type << 28) | ((a & 0xFFu) << 20) | ((b & 0xFFu) << 12) | (c & 0xFFFu);
    __asm__ volatile("" ::: "memory");
    cq_w = w + 1;
    return 1;
}

uint32_t engine_queue_space(void) { return CQ - (cq_w - cq_r); }
uint32_t engine_queue_written(void) { return cq_w; }
uint32_t engine_queue_read(void) { return cq_r; }
int engine_transport_reserve(void)
{
    uint32_t saved = plat_control_enter();
    if ((!seq.transport_hold && (seq.playing || seq.req_stop || seq.req_start || seq.req_cont)) ||
        seq.transport_hold == UINT8_MAX) {
        plat_control_leave(saved); return 0;
    }
    seq.transport_hold++;
    plat_control_leave(saved); return 1;
}
void engine_transport_release(void)
{
    uint32_t saved = plat_control_enter();
    if (seq.transport_hold) seq.transport_hold--;
    plat_control_leave(saved);
}
int engine_arrangement_block(song_bar_t *dst, uint8_t *degree, const song_bar_t *src, uint8_t chord)
{
    uint32_t saved;
    int i;
    if (!engine_queue_space()) return 0;
    saved = plat_control_enter();
    /* Exactly seven canonical bytes. The ISR cannot observe them before publication. */
    for (i = 0; i < NTRACKS; i++) dst->pat[i] = src->pat[i];
    dst->mute = src->mute; *degree = chord;
    cq_put(C_ARR, 0, 0, 0);
    plat_control_leave(saved);
    return 1;
}
int engine_arrangement_global(uint8_t *dst, uint8_t value, int stopped_only)
{
    uint32_t saved;
    if (!engine_queue_space()) return 0;
    saved = plat_control_enter();
    if (stopped_only && (seq.playing || seq.req_stop || seq.req_start || seq.req_cont)) {
        plat_control_leave(saved); return 0;
    }
    *dst = value; cq_put(C_ARR, 0, 0, 0); plat_control_leave(saved);
    return 1;
}
int engine_arrangement_length(uint16_t *dst, uint16_t value)
{
    uint32_t saved;
    if (!engine_queue_space()) return 0;
    saved = plat_control_enter();
    if (seq.playing || seq.req_stop || seq.req_start || seq.req_cont) {
        plat_control_leave(saved); return 0;
    }
    *dst = value; cq_put(C_ARR, 0, 0, 0); plat_control_leave(saved);
    return 1;
}
int engine_arrangement_refresh(const arrangement_settings_t *intent)
{
    uint32_t saved;
    if (!engine_queue_space()) return 0;
    saved = plat_control_enter(); seq.arrangement.intent = intent;
    cq_put(C_ARR, 0, 0, 0); plat_control_leave(saved);
    return 1;
}
int engine_arrangement_queue(uint8_t block) { return cq_put(C_ARR_QUEUE, block, 0, 0); }
POLY_COLD int engine_lane_set(int pattern,int lane,int index,int value)
{
    uint32_t saved;int len,rate;
    if(pattern<0 || pattern>=NPAT || lane<0 || lane>=POLY_LANES || index<0 || index>1 || value<(index?0:1) || value>(index?7:64) || !engine_queue_space())return 0;
    saved=plat_control_enter();
    if(!(proj.variant.arrangement.flags&POLY_FLAG)){plat_control_leave(saved);return 0;}
    len=poly_length(proj.pat,(const uint8_t *)&proj.sound,pattern,lane);
    rate=poly_rate(proj.pat,(const uint8_t *)&proj.sound,pattern,lane);
    poly_set(proj.pat,(uint8_t *)&proj.sound,pattern,lane,index?len:value,index?value:rate);
    cq_put(C_LANE,(uint32_t)pattern,(uint32_t)lane,(uint32_t)index);plat_control_leave(saved);return 1;
}
/* Structural conversion is stopped/reserved. Live policy/rate publication uses
 * the same exact ICFG guard and audio-owner CQ marker as legacy lane timing. */
POLY_COLD int engine_rate_set(int pattern,int lane,int field,int value)
{
 uint32_t saved;int i,rate,len;
 if(pattern<0||pattern>=NPAT||lane<0||lane>=25||field<0||field>4||value<0||value>(field==0?1:field==1?20:field==2?3:field==3?1:2)||!engine_queue_space())return 0;
 if(!(proj.variant.arrangement.flags&POLY_FLAG))return 0;
 if(!field){
  if(!value||!engine_transport_reserve())return 0;
  i=poly_expand(proj.pat,(uint8_t *)&proj.sound);
  if(i)cq_put(C_LANE,0,255,0);
  engine_transport_release();return i;
 }
 if(!poly_expanded((const uint8_t *)&proj.sound))return 0;
 saved=plat_control_enter();
 if(field==1){len=poly_length(proj.pat,(const uint8_t *)&proj.sound,pattern,lane);poly_set(proj.pat,(uint8_t *)&proj.sound,pattern,lane,len,value);}
 else {
  poly_rate_config_set(proj.pat,(uint8_t *)&proj.sound,lane,field-2,value);
  if(field==2)for(i=0;i<NPAT;i++){
   rate=poly_rate(proj.pat,(const uint8_t *)&proj.sound,i,lane);
   if(!rf_allowed((unsigned)value,(unsigned)rate))poly_set(proj.pat,(uint8_t *)&proj.sound,i,lane,poly_length(proj.pat,(const uint8_t *)&proj.sound,i,lane),(int)rf_nearest((unsigned)value,(unsigned)rate));
  }
 }
 cq_put(C_LANE,field==1?(uint32_t)pattern:255u,(uint32_t)lane,1);plat_control_leave(saved);return 1;
}

POLY_COLD int engine_lane_mode(int enabled)
{
    uint32_t saved;int reserved=0;
    if(enabled<0 || enabled>1 || !engine_queue_space())return 0;
    if(!(proj.variant.arrangement.flags&POLY_FLAG) && enabled){
        if(!engine_transport_reserve())return 0;
        reserved=1;
        poly_enable(proj.pat,(uint8_t *)&proj.sound);
    }
    saved=plat_control_enter();
    if(enabled || (proj.variant.arrangement.flags&POLY_FLAG)){
        proj.variant.arrangement.flags|=POLY_FLAG;poly_mode_set((uint8_t *)&proj.sound,enabled);
    }
    cq_put(C_LANE,0,255,0);plat_control_leave(saved);
    if(reserved)engine_transport_release();
    return 1;
}
POLY_COLD int engine_lane_break_step(int pattern,int step,int gate,int slice)
{
    uint32_t saved;
    if(pattern<0 || pattern>=NPAT || step<0 || step>63 || gate<0 || gate>1 || slice<0 || slice>8 || !engine_queue_space())return 0;
    saved=plat_control_enter();
    if(!(proj.variant.arrangement.flags&POLY_FLAG)){plat_control_leave(saved);return 0;}
    poly_break_set(proj.pat,(uint8_t *)&proj.sound,pattern,step,gate,slice);
    cq_put(C_LANE,(uint32_t)pattern,24,2);plat_control_leave(saved);return 1;
}
int engine_cc_refresh(int reconcile){return cq_put(C_ARR,(uint32_t)reconcile,0,0);}
int engine_strip_set(int strip, int index, int value)
{
    if (strip < 0 || strip >= 25 || index < 0 || index >= 6) return 0;
    if (index < 2 ? value < 20 || value > 20000 :
        index == 2 ? value < 0 || value > 24 :
        index == 3 ? value < 0 || value > 48 :
        index == 4 ? value < 10 || value > 250 : value < 0 || value > SOUND_STRIP_FLAGS) return 0;
    return cq_put(C_STRIP, (uint32_t)strip, (uint32_t)((index << 4) | (value >> 12)), (uint32_t)value);
}
const sound_strip_config_t *engine_strip_config(int strip) { return strip >= 0 && strip < 25 ? &strip_cfg[strip] : 0; }
uint32_t engine_strip_processing_mask(void) { return 0x01ffffffu; } /* all 25 production paths */
const sound_strip_meter_t *engine_strip_meter(int strip)
{
    if (strip >= 0 && strip < 11) return drum909_strip_meter(&d909,strip);
    if (strip >= 11 && strip < 22) return drum808_strip_meter(&d808,strip-11);
    return strip >= 22 && strip < 25 ? &mono_strip_meter[strip-22] : 0;
}

/* ------------------------------------------------------------ params --- */
int engine_nvoices(int t)
{
    switch (t) {
    case T_909: return DR_NUM + 1;
    case T_808: return D8_NUM + 1;
    case T_303: return NBASS;
    case T_MIX: return NPARTS;
    default: return 1;
    }
}

int engine_nparams(int t, int v)
{
    switch (t) {
    case T_909: return drum909_nparams(v);
    case T_808: return drum808_nparams(v);
    case T_303: return bass303_nparams();
    case T_BRK: return 2;                                /* Level, Pitch (sends: the mixer's; the rest: the pattern's) */
    case T_FX: return fxbus_nparams();
    case T_MIX: return MX_NPARAMS;                       /* every part: Level, Rev send, Dly send */
    case T_MST: return master_nparams();
    default: return 0;
    }
}

const x0x_param_t *engine_param(int t, int v, int i)
{
    switch (t) {
    case T_909: return drum909_param(v, i);
    case T_808: return drum808_param(v, i);
    case T_303: return bass303_param(i);
    case T_BRK: return breaks_param(BRK_NSET + i);
    case T_FX: return fxbus_param(i);
    case T_MIX: return (i >= 0 && i < MX_NPARAMS) ? &MIX_P[i] : 0;
    case T_MST: return master_param(i);
    default: return 0;
    }
}

static const char *const N909[DR_NUM + 1] = {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CH", "OH", "CR", "RD", "KIT"};
static const char *const N808[D8_NUM + 1] = {"BD", "SD", "LT", "MT", "HT", "RS", "CP", "CB", "CY", "OH", "CH", "KIT"};

const char *engine_voice_name(int t, int v)
{
    switch (t) {
    case T_909: return (v >= 0 && v <= DR_NUM) ? N909[v] : "";
    case T_808: return (v >= 0 && v <= D8_NUM) ? N808[v] : "";
    case T_303: return v ? "303 B" : "303 A";
    case T_MIX: return (v >= 0 && v < NPARTS) ? PART_NAMES[v] : "";
    case T_BRK: return "BREAK";
    case T_FX: return "FX";
    case T_MST: return "MASTER";
    default: return "";
    }
}

static void apply_param(int t, int v, int i, int val)    /* ISR */
{
    switch (t) {
    case T_909: drum909_set(&d909, v, i, val); break;
    case T_808: drum808_set(&d808, v, i, val); break;
    case T_303: if (v >= 0 && v < NBASS) bass303_set(&b303[v], i, val); break;
    case T_BRK: breaks_set(&brk, BRK_NSET + i, val); break;
    case T_FX: fxbus_set(&fx, i, val); break;
    case T_MIX:
        if (v < 0 || v >= NPARTS)
            break;
        if (i == MX_LEVEL)
            mix_level[v] = (float)val * (float)val / (100.0f * 100.0f);     /* 100 = unity, square law */
        else if (i == MX_REV)
            mix_rev[v] = (float)val / 127.0f;
        else if (i == MX_DLY)
            mix_dly[v] = (float)val / 127.0f;
        else if (i == MX_PAN)
            x0x_pan_gains(val, &mix_pl[v], &mix_pr[v]);
        break;
    case T_MST: master_set(&mst, i, val); break;
    default: break;
    }
}

void engine_set(int t, int v, int i, int val) { cq_put((uint32_t)t, (uint32_t)v, (uint32_t)i, (uint32_t)val); }

/* the output's scale: Felucca's -6 dBFS ceiling of the 24-bit codec (2^22), or its full scale (LOUD); the
 * clamp keeps an over from wrapping (normal: the codec's own full scale, as before) */
static volatile float out_scale = 4194303.0f, out_lim = 2.0f;
void engine_out_loud(int on)
{
    out_scale = on ? 8388607.0f : 4194303.0f;
    out_lim = on ? 1.0f : 2.0f;
}

/* USB AUDIO FULL (#12): the render at full level, for the USB tap, and the MASTER knob after it, for
 * the speaker, headphones and line out (with the master's ~10 ms glide). The limiter then works at
 * full level too: turned down, the FM-1 plays what it sends, quieter. */
static volatile int usb_full;
static float vol_after = -1.0f;
void engine_usb_full(int on) { usb_full = on; }

void engine_volume(int32_t *out_lr, uint32_t n)
{
    float vol = (float)plat_master() / 4096.0f, v = vol_after < 0.0f ? vol : vol_after;
    uint32_t i;
    if (!usb_full) {
        vol_after = -1.0f;
        return;
    }
    for (i = 0; i < n; i++) {
        if (v != vol) {
            v += (vol - v) * 0.0022651f;
            if (fm_fabsf(vol - v) < 1e-5f)
                v = vol;
        }
        out_lr[2u * i] = (int32_t)((float)out_lr[2u * i] * v);
        out_lr[2u * i + 1u] = (int32_t)((float)out_lr[2u * i + 1u] * v);
    }
    vol_after = v;
}

POLY_COLD void engine_sound_defaults(sound_t *s)
{
    int t, v, i;
    uint8_t *z = (uint8_t *)s;
    for (i = 0; i < (int)sizeof *s; i++)
        z[i] = 0;
    for (t = 0; t < NTARGETS; t++)
        for (v = 0; v < engine_nvoices(t) && v < NVOICES_MAX; v++)
            for (i = 0; i < engine_nparams(t, v) && i < NPARAMS_MAX; i++) {
                const x0x_param_t *p = engine_param(t, v, i);
                s->v[t][v][i] = p ? p->def : 0;
            }
    /* the factory mix: big beat (Chemical Brothers, Fatboy Slim), balanced by tools/mix_report.py
     * against a funk break, a 909 kick under it and a 303 line. The break leads, the kick sits
     * under it, the 303 screams through its RAT; the master squeezes and pumps. Each engine's
     * own voicing (its pot defaults) is left as its original; only these are X0X's. */
    s->v[T_MIX][PART_909][MX_LEVEL] = 75;            /* 9W9's kit is hot (it drives its own glue): -5 dB */
    s->v[T_909][DR_BD][3] = 61;                      /* its kick under the break, its clap and hats over */
    s->v[T_909][DR_BD][2] = 80;                      /* the kick: a little shorter, and driven. Its Attack */
    s->v[T_909][DR_BD][6] = 30;                      /* stays 9W9's 13: at 50 its tick was 12 dB louder, a click on every kick */
    s->v[T_909][DR_OH][1] = 60;
    s->v[T_MIX][PART_808][MX_LEVEL] = 112;
    s->v[T_808][D8_KIT][0] = 127;                    /* kit level */
    s->v[T_BRK][0][0] = 127;                         /* the break: full level, part a little over unity */
    s->v[T_MIX][PART_BRK][MX_LEVEL] = 120;
    s->v[T_MIX][PART_BRK][MX_REV] = 25;
    for (v = 0; v < NBASS; v++) {                    /* acid: low cutoff, high resonance, a long sweep, RAT */
        s->v[T_303][v][BASS303_CUTOFF] = 48;
        s->v[T_303][v][BASS303_RESO] = 100;
        s->v[T_303][v][BASS303_ENVMOD] = 85;
        s->v[T_303][v][BASS303_ACCENT] = 100;
        s->v[T_303][v][BASS303_VOLUME] = 127;
        s->v[T_303][v][BASS303_DRIVE] = 45;
        s->v[T_303][v][BASS303_DRVTYPE] = 2;
        s->v[T_MIX][PART_303A + v][MX_LEVEL] = 74;   /* the RAT makes it dense: -5 dB (square law) */
        s->v[T_MIX][PART_303A + v][MX_DLY] = v ? 60 : 80;
        s->v[T_MIX][PART_303A + v][MX_REV] = 12;
    }
    s->v[T_909][DR_CP][5] = 50;                      /* the clap in the room (90 drowned it) */
    s->v[T_909][DR_LT][1] = 92;                      /* the toms: shorter than 9W9's (its low tom rang */
    s->v[T_909][DR_MT][1] = 88;                      /* on for 1.7 s, which reads as reverb) */
    s->v[T_909][DR_HT][1] = 90;
    s->v[T_FX][0][FX_DL_TYPE] = 1;                   /* tape delay, dotted eighths */
    s->v[T_MST][0][MST_RATIO] = 4;                   /* 4:1 */
    s->v[T_MST][0][MST_THRESH] = 64;                 /* -24 dB */
    s->v[T_MST][0][MST_MAKEUP] = 85;                 /* +16 dB: into the limiter */
    s->v[T_MST][0][MST_ATTACK] = 85;                 /* 10 ms: faster grabbed each kick hard enough to click */
    s->v[T_MST][0][MST_RELEASE] = 58;                /* 100 ms: it breathes with the beat */
    s->v[T_MST][0][MST_PUMP] = 21;                   /* 4 dB of 909-keyed pump */
    s->v[T_MST][0][MST_COMP1] = 95;                  /* the one-knob COMP's nearest to the squash above */
    s->v[T_FX][1][0] = (uint8_t)(16 + s->v[T_FX][0][FX_DL_WEAR] * 111 / 127);   /* TAPE (ui.c R_TAPE): on, its wear */
}

POLY_COLD void engine_sound_centre_drum_pans(sound_t *s)
{
    int t, v;
    for (t = T_909; t <= T_808; t++)
        for (v = 0; v < NDRUM && v < NVOICES_MAX; v++) {
            int i = engine_nparams(t, v) - 1;           /* PAN is every voice's last pot */
            const x0x_param_t *p = engine_param(t, v, i);
            if (p && p->def == 64 && p->max == 127)
                s->v[t][v][i] = 64;
        }
}

POLY_COLD void engine_apply_sound(const sound_t *s)
{
    int t, v, i;
    engine_set(T_MST, 0, MST_COMP1, s->v[T_MST][0][MST_COMP1]);   /* the macros first: the pots they set */
    engine_set(T_MST, 0, MST_DJF, s->v[T_MST][0][MST_DJF]);       /* follow as saved (a project's own sound) */
    for (t = 0; t < NTARGETS; t++)
        for (v = 0; v < engine_nvoices(t) && v < NVOICES_MAX; v++)
            for (i = 0; i < engine_nparams(t, v) && i < NPARAMS_MAX; i++)
                if (t != T_MST || i < MST_COMP1)
                    engine_set(t, v, i, s->v[t][v][i]);
}

void engine_drum(int kit, int voice, float vel)
{
    cq_put(C_DRUM, (uint32_t)kit, (uint32_t)voice, (uint32_t)(vel * 4095.0f));
}
void engine_bass_on(int part, int note, int accent, int slide)
{
    cq_put(C_BON, (uint32_t)part, (uint32_t)note, (uint32_t)((accent ? 1 : 0) | (slide ? 2 : 0)));
}
void engine_bass_off(int part) { cq_put(C_BOFF, (uint32_t)part, 0, 0); }
void engine_brk_live(int key, int down) { cq_put(C_BRK, (uint32_t)key, (uint32_t)(down != 0), 0); }

/* ------------------------------------------------------- sequencer sink --- */
/* probability: each part's (and drum track's) chance on the step playing now, set by its T_PROB lane
 * before the step fires (motion_step runs first); 100 = always */
static uint8_t prob[NPARTS][NDRUM];
static uint32_t prob_rng = 0x2545F491u;
static int prob_skip(int part, int v)                 /* ISR: 1 = this hit does not play */
{
    uint32_t p = prob[part][v];
    if (p > 100u && p < PROB_COND + PROB_NCOND) {      /* A:B: the A-th of every B passes (seq_track_t.bars) */
        int a, b;
        prob_cond((int)p - PROB_COND, &a, &b);
        return (int)((seq.lane_sound ? seq_lane_cycles(&seq,part<NKIT?part*11+v:part+20) : seq.t[part].bars) % (uint32_t)b) != a - 1;
    }
    if (p >= 100u)
        return 0;
    prob_rng ^= prob_rng << 13;
    prob_rng ^= prob_rng >> 17;
    prob_rng ^= prob_rng << 5;
    return prob_rng % 100u >= p;
}
static void s_drum(void *x, int kit, int v, float vel);
static int s_drum_seq(void *x, int kit, int v, float vel)
{
    if (prob_skip(kit, v))
        return 0;
    s_drum(x, kit, v, vel);
    return 1;
}
static void s_drum(void *x, int kit, int v, float vel)
{
    (void)x;
    if (v == 0)                          /* BD (both kits' track 1): the master's PUMP key */
        master_key(&mst, kit, vel);
    if ((unsigned)kit < NKIT && (unsigned)v < NDRUM)   /* MIX's drum pages: the track's meter jumps */
        eng_hit[kit][v] = (uint8_t)(vel >= 1.0f ? 255 : (int)(vel * 255.0f));
    if (kit == 0)
        drum909_trigger(&d909, v, vel);
    else                                 /* the 808's unaccented hit sits at D8_VEL_NORMAL, not at our 909 level */
        drum808_trigger(&d808, v, vel >= 0.999f ? 1.0f : vel * (D8_VEL_NORMAL / (88.0f / 127.0f)));
}
/* Shared checked trigger: the void compatibility callback must not duplicate it. */
static __attribute__((noinline)) int s_bon_checked(void *x,int p,int note,int acc,int sl)
{
    (void)x;
    if(prob_skip(PART_303A+p,0)) {bass303_note_off(&b303[p]);return 0;}
    bass303_note_on(&b303[p],note,acc,sl);return 1;
}
static void s_bon(void *x,int p,int note,int acc,int sl) { (void)s_bon_checked(x,p,note,acc,sl); }
static void s_bretune(void *x,int p,int note,int acc) { (void)x;bass303_note_on(&b303[p],note,acc,1); }
static void s_boff(void *x, int p) { (void)x; bass303_note_off(&b303[p]); }
static uint8_t brk_applied[BRK_NSET];  /* the pattern settings the break was last given */
static void s_brk(void *x, int s16, int bar, float spb, int en)
{
    const brkpart_t *bp = &seq.pat[seq.ppat[TRK_BRK]].brk;
    int i;
    if (en && prob_skip(PART_BRK, 0))
        en = 0;
    (void)x;
    if (brk_hold)
        return;
    breaks_set_quiet(&brk, (!seq.lane_sound && bp->steps == 0) || (seq.mute & (1u << MUTE_BRK)));   /* unheard: stop decoding */
    for (i = 0; i < BRK_NSET; i++)       /* follow the playing pattern's settings (12 compares) */
        if (bp->set[i] != brk_applied[i]) {
            brk_applied[i] = bp->set[i];
            breaks_set(&brk, i, bp->set[i]);
        }
    breaks_pin(&brk, (seq.lane_sound?poly_break_slice(seq.pat,seq.lane_sound,seq.ppat[TRK_BRK],s16):(int)bp->slice[s16 & 15]) - 1);   /* the step's own slice, if it has one */
    breaks_step(&brk, s16 & 15, bar, spb, en);
}
static void s_brkstop(void *x) { (void)x; breaks_stop(&brk); }
static uint32_t midi_packet(uint8_t st,uint8_t d1,uint8_t d2)
{
    uint32_t cin=st>=0xF0?0x0Fu:(uint32_t)(st>>4);
    return cin|((uint32_t)st<<8)|((uint32_t)d1<<16)|((uint32_t)d2<<24);
}
static int s_midi_try(void *x,uint8_t st,uint8_t d1,uint8_t d2)
{ (void)x;return plat_midi_out(midi_packet(st,d1,d2)); }
static void s_midi(void *x,uint8_t st,uint8_t d1,uint8_t d2)
{ (void)s_midi_try(x,st,d1,d2); }
static int s_midi_pair(void *x,uint8_t st1,uint8_t n1,uint8_t v1,uint8_t st2,uint8_t n2,uint8_t v2)
{ (void)x;return plat_midi_pair(midi_packet(st1,n1,v1),midi_packet(st2,n2,v2)); }

static int s_midi_batch(void *x,const uint32_t *packets,uint32_t count)
{ (void)x;return plat_midi_batch(packets,count); }
static void s_midi_cancel(void *x) { (void)x;plat_midi_cancel(); }
static void s_clock_retire(void *x,uint8_t mask){(void)x;plat_midi_retire_clock(mask);}
static void s_midi_transport_cancel(void *x) { (void)x;plat_midi_transport_cancel(); }
#define MIDI_CANCEL_SINK s_midi_cancel
#define MIDI_TRANSPORT_CANCEL_SINK s_midi_transport_cancel

static void s_step(void *x, int t, int p)
{
    (void)x;
    eng_step[t] = (uint8_t)p;
    if (mot_on)
        motion_step(&mot, t, seq.ppat[t], p, seq_next_step(&seq, t, p), seq_part_len(&seq, t));
}

/* --------------------------------------------------------------- motion --- */
static int mot_base(void *x, int t, int v, int i)
{
    (void)x;
    if (t >= NTARGETS)
        return 100;                                   /* T_PROB: always */
    return mot_base_snd ? mot_base_snd->v[t][v][i] : 0;
}
static void mot_apply(void *x, int t, int v, int i, int val)
{
    (void)x;
    if (t == T_PROB) {
        if ((v >> 4) < NPARTS && (v & 15) < NDRUM)
            prob[v >> 4][v & 15] = (uint8_t)val;
    } else if (t < NTARGETS)
        apply_param(t, v, i, val);
}

int engine_motion_part(int t, int v)
{
    switch (t) {
    case T_909: return PART_909;
    case T_808: return PART_808;
    case T_303: return PART_303A + (v & 1);
    case T_BRK: return PART_BRK;
    case T_MIX: return v < NPARTS ? v : PART_909;
    case T_PROB: return (v >> 4) < NPARTS ? v >> 4 : PART_909;
    default: return PART_909;                     /* FX, master: the bar */
    }
}
int engine_motion_value(int k) { return motion_value(&mot, k); }
void engine_motion_rec(int k) { motion_req_rec(&mot, k); }
void engine_motion_hold(int k) { motion_req_hold(&mot, k); }
static const seq_sink_t SINK = {s_drum_seq, s_bon, s_boff, s_brk, s_brkstop, s_midi, s_step, 0, s_bretune, s_bon_checked, s_midi_try, s_midi_pair, s_midi_batch, MIDI_CANCEL_SINK, MIDI_TRANSPORT_CANCEL_SINK,s_clock_retire};

/* ---------------------------------------------------------------- MIDI in --- */
static uint8_t bass_held[NBASS];       /* note held from MIDI, for legato = slide */
int engine_midi_channel_allowed(int channel)
{ return channel==255 || proj.variant.arrangement.pad_channel==255 || channel!=proj.variant.arrangement.pad_channel; }
void engine_notes_clear(void)
{
    uint32_t saved=plat_control_enter();
    if(seq.pat)seq_notes_clear(&seq,&SINK);
    bass_held[0]=bass_held[1]=255;
    plat_control_leave(saved);
}

static void midi_in(uint32_t pkt)
{
    uint8_t st = (uint8_t)(pkt >> 8), d1 = (uint8_t)(pkt >> 16) & 0x7F, d2 = (uint8_t)(pkt >> 24) & 0x7F;
    uint8_t ch = st & 0x0F, ty = st & 0xF0;
    int k, v;
    if((st==248u||st==250u||st==251u||st==252u) && !seq_clock_admit(&seq,(pkt>>4)&15u,st))return;
    switch (st) {
    case 0xF8: seq_ext_clock(&seq); return;
    case 0xFA: seq_start(&seq); return;
    case 0xFB: seq_continue(&seq); return;
    case 0xFC: seq_stop(&seq); return;
    default: break;
    }
    if (ty == 0x90 && d2 == 0)
        ty = 0x80;
    for (k = 0; k < NKIT; k++)                          /* each part on its own channel (GLO): the drum */
        if (seq.ch[k] == ch && ty == 0x90) {             /* machines on GM drum notes */
            static const uint8_t GM[NDRUM] = {36, 38, 41, 45, 50, 37, 39, 42, 46, 49, 51};
            static const uint8_t GM8[NDRUM] = {36, 38, 41, 45, 50, 37, 39, 56, 49, 46, 42};
            for (v = 0; v < NDRUM; v++)
                if ((k ? GM8 : GM)[v] == d1)
                    s_drum(0, k, v, (float)d2 / 127.0f);
        }
    for (k = 0; k < NBASS; k++)                         /* the 303s: overlapping notes slide */
        if (seq.ch[TRK_BASS0 + k] == ch) {
            if (ty == 0x90) {
                bass303_note_on(&b303[k], d1, d2 >= 100, bass_held[k] != 255);
                bass_held[k] = d1;
            } else if (ty == 0x80 && bass_held[k] == d1) {
                bass303_note_off(&b303[k]);
                bass_held[k] = 255;
            }
        }
    if (seq.ch[TRK_BRK] == ch && (ty == 0x90 || ty == 0x80) && d1 >= 36 && d1 <= 43)   /* the break's slices */
        breaks_live(&brk, d1 - 36, ty == 0x90);
}

/* ---------------------------------------------------------------- render --- */
/* One outer-block transaction drain: keep queue/MIDI work in an explicit
 * measured callee, before render dispatch; no event-loop or ownership change. */
static __attribute__((noinline)) void drain(void)
{
    uint32_t r = cq_r, pkt;
    int guard = 64;
    uint32_t strip_dirty = 0;
    while (r != cq_w) {
        uint32_t c = cq[r % CQ], type = c >> 28, a = (c >> 20) & 0xFFu, b = (c >> 12) & 0xFFu, v = c & 0xFFFu;
        r++;
        if (type < 8u)
            apply_param((int)type, (int)a, (int)b, (int)v);
        else if (type == C_DRUM)
            s_drum(0, (int)a, (int)b, (float)v / 4095.0f);
        else if (type == C_BON && a < NBASS)
            bass303_note_on(&b303[a], (int)b, (int)(v & 1u), (int)((v >> 1) & 1u));
        else if (type == C_BOFF && a < NBASS)
            bass303_note_off(&b303[a]);
        else if (type == C_BRK && !brk_hold)
            breaks_live(&brk, (int)a, (int)b);
        else if (type == C_LANE) {if(b<25 && v<2 && seq.playing && (a==255u || seq.ppat[poly_part((int)b)]==a))seq_lane_mark_pending(&seq,(int)b,(int)v);seq.lane_sound=(proj.variant.arrangement.flags&POLY_FLAG)?(uint8_t *)&proj.sound:0;if(!seq.playing)seq.lane_enabled=(uint8_t)(seq.lane_sound && poly_mode(seq.lane_sound));}
        else if(type==C_ARR && (a==3 || a==5)){
            uint32_t next=cq[r%CQ],g=v|(((next>>12)&255u)<<12)|((next&4095u)<<20);
            if(r!=cq_w && (next>>28)==C_ARR && ((next>>20)&255u)==4u){
                r++;
                if(!(seq.clock.flags&CLOCK_RETIRED) && seq.clock.generation!=UINT32_MAX && g==seq.clock.generation+1u){
                    if(a==3)seq_clock_request(&seq,(uint8_t)b,&SINK);
                    else{
                        plat_midi_retire_clock((uint8_t)(1u<<b));
                        if((seq.clock.role_source&3u)==CLOCK_LEGACY || ((seq.clock.role_source&3u)==CLOCK_FOLLOW && (seq.clock.role_source>>2)==b)){
                            seq_transport_retire(&seq);seq.ext_pending=0;plat_midi_transport_cancel();
                        }else{seq.clock.generation=g;seq.clock.flags=(uint8_t)((seq.clock.flags&~CLOCK_CONFIG_APPLIED)|CLOCK_INVALIDATED);}
                    }
                }
            }
        }
        else if (type == C_ARR) {
            if(a==1){seq_notes_clear(&seq,&SINK);bass_held[0]=bass_held[1]=255;for(int key=0;key<8;key++)breaks_live(&brk,key,0);}
            seq.lane_sound=(proj.variant.arrangement.flags&POLY_FLAG)?(uint8_t *)&proj.sound:0;
            if(!seq.playing)seq.lane_enabled=(uint8_t)(seq.lane_sound && poly_mode(seq.lane_sound));
            seq_arrangement_apply_flags(&seq);seq.send_notes=(uint8_t)((proj.variant.arrangement.flags&ARR_NOTES_OUT)!=0);proj.set.notes_out=seq.send_notes;
            if(a==2)seq_cc_reconcile(&seq,&SINK);
        }
        else if (type == C_ARR_QUEUE) seq_arrangement_queue(&seq, (uint8_t)a);
        else if (type == C_STRIP && a < 25u) {
            uint32_t val = v | ((b & 15u) << 12);
            switch (b >> 4) {
            case 0: strip_cfg[a].highpass_hz = (uint16_t)val; break;
            case 1: strip_cfg[a].lowpass_hz = (uint16_t)val; break;
            case 2: strip_cfg[a].boost_db = (uint8_t)val; break;
            case 3: strip_cfg[a].ceiling_db = (uint8_t)val; break;
            case 4: strip_cfg[a].release_ms = (uint8_t)val; break;
            case 5: strip_cfg[a].flags = (uint8_t)val; break;
            }
            strip_dirty |= 1u << a;
        }
    }
    cq_r = r;
    strip_block_begin(strip_dirty);
    seq_midi_prepare(&seq,plat_midi_connected(),&SINK);
    while (guard-- && plat_midi_in(&pkt))
        midi_in(pkt);
}

static float part_peak[NPARTS];
__attribute__((noinline)) static void meter(int p, const float *x, float g, uint32_t n)
{
    uint32_t i;
    float pk = part_peak[p];
    for (i = 0; i < n; i++) {
        float a = fm_fabsf(x[i]) * g;
        if (a > pk)
            pk = a;
    }
    part_peak[p] = pk;
}

/* dry into L and R by the part's pan; the sends stay mono (the reverb and delay make the width) */
/* a mix gain that glides to its target (one pole, ~10 ms) instead of stepping each block, which
 * zippered on quick turns of LEVEL, PAN or a send. cur < 0: never used, takes the target at once
 * (no fade-in at boot); settled, cur IS the target, so the product is the unsmoothed one exactly */
typedef struct { float cur, tgt; } glide_t;
#define GLIDE_A 0.0022651f                                /* 1 - exp(-1 / (10 ms * 44.1 kHz)) */
static void glide_set(glide_t *g, float t)
{
    g->tgt = t;
    if (g->cur < 0.0f)
        g->cur = t;
}
__attribute__((noinline)) static void add_glide(float *dst, const float *src, glide_t *g, uint32_t n)
{
    uint32_t i;
    float c = g->cur, t = g->tgt;
    if (c == t) {
        if (c != 0.0f)
            for (i = 0; i < n; i++)
                dst[i] += src[i] * c;
        return;
    }
    for (i = 0; i < n; i++) {
        c += (t - c) * GLIDE_A;
        dst[i] += src[i] * c;
    }
    if (fm_fabsf(t - c) < 1e-5f)
        c = t;
    g->cur = c;
}
/* each part's gains: dry left and right, its reverb and delay sends, and (drums) the voices' own sends */
enum { GL_L, GL_R, GL_REV, GL_DLY, GL_VREV, GL_VDLY, GL_N };
static glide_t mix_glide[NPARTS][GL_N] = {
    {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}, {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}},
    {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}, {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}},
    {{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}};
static void mix_targets(int p, float g)                     /* g: the part level */
{
    glide_set(&mix_glide[p][GL_L], g * mix_pl[p]);
    glide_set(&mix_glide[p][GL_R], g * mix_pr[p]);
    glide_set(&mix_glide[p][GL_REV], g * mix_rev[p]);
    glide_set(&mix_glide[p][GL_DLY], g * mix_dly[p]);
    glide_set(&mix_glide[p][GL_VREV], g);
    glide_set(&mix_glide[p][GL_VDLY], g);
}

#if X0X_DEBUG
/* development: what one block cost, for the worst-block record (main_fm1.c render_block) */
volatile uint32_t eng_dbg_wait, eng_dbg_nsub;  /* 24 MHz ticks waiting for core 1, stretches */
#endif

/* the mix of one stretch between two events: dry left and right, the reverb and delay sends;
 * t0..tr the 808's scratch, kbuf the break's, mono the 909's kit send */
static float dry_l[256], dry_r[256], rev[256], dly[256], t0[256], t1[256], t2[256], tr[256], kbuf[256], mono[256];

/* a glide applied in place to a buffer that started at zero: buf * c is 0 + buf * c exactly, so this
 * is add_glide(zeros, buf) to the bit (the same steps of c) */
__attribute__((noinline)) static void scale_glide(float *buf, glide_t *g, uint32_t n)
{
    uint32_t i;
    float c = g->cur, t = g->tgt;
    if (c == t) {
        for (i = 0; i < n; i++)
            buf[i] *= c;
        return;
    }
    for (i = 0; i < n; i++) {
        c += (t - c) * GLIDE_A;
        buf[i] *= c;
    }
    if (fm_fabsf(t - c) < 1e-5f)
        c = t;
    g->cur = c;
}

static int brk_silent_now = 1;
/* New strip work stays in explicit measured callees, replacing the existing raw
 * renderer call at each legacy site. No new loop callsites in engine_render. */
__attribute__((noinline)) static int bass_strip_render(int b, float *buf, uint32_t n)
{
    int idle = b303[b].idle;
    bass303_render(&b303[b],buf,(int)n);
    if (!idle || sound_strip_active(&mono_strip[b])) {
        mono_strip_process(b,buf,n);
        idle = 0;
    }
    return idle;
}
__attribute__((noinline)) static void break_strip_render(uint32_t n)
{
    brk_silent_now = breaks_silent(&brk);
    breaks_render(&brk,kbuf,(int)n);
    if (!brk_silent_now || sound_strip_active(&mono_strip[2])) {
        mono_strip_process(2,kbuf,n);
        brk_silent_now = 0;
    }
}

/* the second core's job (or the first's, after its own part, with one core): the 808 into t0..tr
 * and the break into kbuf; arg bit 8: not timed (the cycle counter is the first core's). Nothing
 * here is mixed: the first core does that afterwards, in one core's order */

static uint8_t d808_was_active;
static void core2_job(uint32_t arg)
{
    uint32_t n = arg & 0xFFu ? arg & 0xFFu : 256u, i, timed = !(arg & 0x100u), pt = timed ? plat_cycles() : 0;
    d808_was_active = (uint8_t)drum808_active(&d808);
    if (d808_was_active) {                                /* silent, it does nothing (and is not mixed) */
        for (i = 0; i < n; i++)
            t0[i] = t1[i] = t2[i] = tr[i] = 0.0f;
        drum808_render_st(&d808, t0, tr, t1, t2, (int)n);
    } else
        drum808_pan_settle(&d808);
    if (timed) {
        PROF_ADD(PR_808, pt);
        pt = plat_cycles();
    }
    if (!brk_hold) {
        break_strip_render(n);
    }
    if (timed)
        PROF_ADD(PR_BRK, pt);
}

/* a drum machine's mix, from its render (L, R, voice sends; mono, its kit send) */
__attribute__((noinline)) static void kit_sends(int b, const float *m, uint32_t n)
{
    add_glide(rev, m, &mix_glide[b][GL_REV], n);
    add_glide(dly, m, &mix_glide[b][GL_DLY], n);
    meter(b, m, mix_level[b], n);
}

/* one stretch: the 909 (into the mix, in place) and the 303s here while the second core renders the
 * 808 and the break; then those are mixed in, in the order one core always used (909, 808, 303 A,
 * 303 B, break): the same sums either way */
static void render_sub(float *out_l, float *out_r, uint32_t n)
{
    float *bb[NBASS] = {out_l, out_r};
    int idle[NBASS], b;
    uint32_t arg = n & 0xFFu, i, pt;
    int on2 = plat_cpu2_run(core2_job, arg | 0x100u);
    for (i = 0; i < n; i++)
        dry_l[i] = dry_r[i] = rev[i] = dly[i] = 0.0f;
    {   /* the 909, straight into the mix: L, R and its voices' sends, then scaled in place */
        PROF_T(p9);
        if (!drum909_active(&d909)) {
            drum909_render(&d909, mono, mono, mono, (int)n);   /* writes nothing; advances the noise */
            drum909_pan_settle(&d909);                    /* silent: pans move at once, no glide */
        } else {
            drum909_render_st(&d909, dry_l, dry_r, rev, dly, (int)n);
            mix_targets(PART_909, mix_level[PART_909]);
            for (i = 0; i < n; i++)                       /* the kit send: the whole machine, both sides */
                mono[i] = (dry_l[i] + dry_r[i]) * 0.5f;   /* (centred: L exactly) */
            scale_glide(dry_l, &mix_glide[PART_909][GL_L], n);   /* then the part's PAN over the kit */
            scale_glide(dry_r, &mix_glide[PART_909][GL_R], n);
            scale_glide(rev, &mix_glide[PART_909][GL_VREV], n);
            scale_glide(dly, &mix_glide[PART_909][GL_VDLY], n);
            kit_sends(PART_909, mono, n);
        }
        PROF_ADD(PR_909, p9);
    }
    for (b = 0; b < NBASS; b++) {
        PROF_T(pb);
        idle[b] = bass_strip_render(b,bb[b],n);
        PROF_ADD(2 + b, pb);
    }
    pt = plat_cycles();
#if X0X_DEBUG
    eng_dbg_nsub++;
    {
        uint32_t w0 = plat_ticks24();
        int failed = on2 && plat_cpu2_wait();
        eng_dbg_wait += plat_ticks24() - w0;
        if (failed)
            on2 = 0;
    }
    if (!on2)
#else
    if (!on2 || plat_cpu2_wait())
#endif
        core2_job(arg);                                   /* one core (or the second failed): here */
    else
        PROF_ADD(PR_808, pt);                             /* what the first core waited for the second */
    if (d808_was_active) {
        mix_targets(PART_808, mix_level[PART_808]);
        add_glide(dry_l, t0, &mix_glide[PART_808][GL_L], n);
        add_glide(dry_r, tr, &mix_glide[PART_808][GL_R], n);
        add_glide(rev, t1, &mix_glide[PART_808][GL_VREV], n);
        add_glide(dly, t2, &mix_glide[PART_808][GL_VDLY], n);
        for (i = 0; i < n; i++)
            t0[i] = (t0[i] + tr[i]) * 0.5f;
        kit_sends(PART_808, t0, n);
    }
    for (b = 0; b < NBASS; b++) {
        int p = PART_303A + b;
        if (idle[b])
            continue;
        mix_targets(p, mix_level[p]);
        add_glide(dry_l, bb[b], &mix_glide[p][GL_L], n);
        add_glide(dry_r, bb[b], &mix_glide[p][GL_R], n);
        add_glide(rev, bb[b], &mix_glide[p][GL_REV], n);
        add_glide(dly, bb[b], &mix_glide[p][GL_DLY], n);
        meter(p, bb[b], mix_level[p], n);
    }
    if (!brk_hold && !brk_silent_now) {
        float g = mix_level[PART_BRK];
        mix_targets(PART_BRK, g);
        add_glide(dry_l, kbuf, &mix_glide[PART_BRK][GL_L], n);
        add_glide(dry_r, kbuf, &mix_glide[PART_BRK][GL_R], n);
        add_glide(rev, kbuf, &mix_glide[PART_BRK][GL_REV], n);
        add_glide(dly, kbuf, &mix_glide[PART_BRK][GL_DLY], n);
        meter(PART_BRK, kbuf, g, n);
    }
    {
        PROF_T(pf);
        fxbus_process_st(&fx, dry_l, dry_r, rev, dly, out_l, out_r, (int)n);
        PROF_ADD(5, pf);
    }
}

#ifdef X0X_WEB
extern void web_slice_audio(uint32_t n);
#endif

void engine_render(int32_t *out_lr, uint32_t n)
{
    static float out_l[256], out_r[256];
    uint32_t done = 0, i;
    float vol = usb_full ? 1.0f : (float)plat_master() / 4096.0f;   /* (FULL: engine_volume) */
    int p;
    uint32_t (*until_event)(seq_t *,uint32_t);
    void (*advance)(seq_t *,uint32_t,const seq_sink_t *);
    if (n > 256u)
        n = 256u;
    eng_prof_frames += n;
    if ((mot_was_on && !mot_on) || mot_release_req) {   /* motion off, or its lanes replaced: knobs back */
        mot_release_req = 0;
        motion_release(&mot);
    }
    mot_was_on = mot_on;
    guard_apply();
    {
        PROF_T(pt);
        drain();
        /* Only the applied audio-owned flag chooses the mode; one indirect call
         * at each existing callsite, never a desired-project read in this loop. */
        if(seq.lane_sound){until_event=seq_until_event_poly;advance=seq_advance_poly;}
        else if (seq.arrangement.flags & ARR_ENABLED) {
            until_event = seq_until_event_arrangement; advance = seq_advance_arrangement;
        } else {
            until_event = seq_until_event; advance = seq_advance;
        }
        if (mot_on)
            motion_tick(&mot, n);
        fxbus_set_bpm(&fx, seq_tempo(&seq));
        advance(&seq, 0, &SINK);
        PROF_ADD(7, pt);
    }
    while (done < n) {
        uint32_t k = until_event(&seq, n - done);
        render_sub(out_l + done, out_r + done, k);
#ifdef X0X_WEB
        web_slice_audio(k);
#endif
        {
            PROF_T(pt);
            advance(&seq, k, &SINK);
            PROF_ADD(7, pt);
        }
        done += k;
    }
    if (mot_playing && !seq.playing)                     /* stopped: every knob back to its own value */
        motion_release(&mot);
    mot_playing = seq.playing;
    {
        PROF_T(pt);
        master_process_st(&mst, out_l, out_r, (int)n, vol);
        PROF_ADD(6, pt);
    }
    for (i = 0; i < n; i++) {
        float l = out_l[i], r = out_r[i];            /* (out_scale: NORMAL 2^22, Felucca's; LOUD 2^23) */
        l = l > out_lim ? out_lim : l < -out_lim ? -out_lim : l;
        r = r > out_lim ? out_lim : r < -out_lim ? -out_lim : r;
        out_lr[2u * i] = (int32_t)(l * out_scale);
        out_lr[2u * i + 1u] = (int32_t)(r * out_scale);
        if (i & 1u)
            eng_scope[eng_scope_w++ & (SCOPE_N - 1u)] = (int16_t)((out_l[i] + out_r[i]) * 16383.5f);
    }
    for (p = 0; p < NPARTS; p++) {                      /* meters: peak hold, ~300 ms decay at the UI */
        uint32_t q = (uint32_t)(fm_minf(part_peak[p], 1.0f) * 32767.0f);
        if (q > eng_peak[p])
            eng_peak[p] = (uint16_t)q;
        part_peak[p] = 0.0f;
    }
}

/* -------------------------------------------------------------- loops --- */
/* a user loop's length in bars: 1, 2, 4 or 8, whichever puts its tempo nearest 120 BPM
 * inside 80..180 (breaks are recorded at their own tempo; the player stretches to ours) */
static uint8_t loop_bars(uint32_t ns, uint32_t rate)
{
    float sec = (float)ns / (float)(rate ? rate : 22050u), best = 1e9f;
    uint8_t bars = 1, b;
    for (b = 1; b <= 8; b = (uint8_t)(b * 2)) {
        float bpm = (float)b * 240.0f / sec, d = bpm > 120.0f ? bpm - 120.0f : 120.0f - bpm;
        if (bpm >= 80.0f && bpm <= 180.0f && d < best) {
            best = d;
            bars = b;
        }
    }
    return bars;
}

#include "x0x_break_bank.h"              /* tools/gen_break_bank.py: the built-in loops */

static const brk_loop_t *brk_builtin(int k)
{
    static brk_loop_t L[2];
    static int which;
    brk_loop_t *l = &L[which ^= 1];      /* two: A and B are set one after the other */
    if (k < 0 || k >= X0X_NBREAKS)
        return 0;
    l->adpcm = X0X_BREAKS[k].adpcm;
    l->nsamples = X0X_BREAKS[k].n;
    l->rate = X0X_BREAK_RATE;
    l->bars = X0X_BREAKS[k].bars;
    return l;
}

/* the LOOP choices: the built-in bank, then every loop (zone) of the user slots, in order.
 * The list follows uploads: engine_brk_loops() rescans it. */
#define NLOOPS_MAX (X0X_NBREAKS + PLAT_NSLOTS * PLAT_SLOT_ZONES)
static const char *loop_names[NLOOPS_MAX];
static char user_names[PLAT_NSLOTS * PLAT_SLOT_ZONES][8];
static uint8_t loop_slot[NLOOPS_MAX], loop_zone[NLOOPS_MAX];
static int nloops;

static void scan_loops(void)
{
    int i, k, z, u = 0;
    nloops = 0;
    for (i = 0; i < X0X_NBREAKS; i++)
        loop_names[nloops++] = X0X_BREAKS[i].name;
    for (k = 0; k < PLAT_NSLOTS; k++) {                 /* yours: BR1, BR2 ... in order, whatever slot */
        int nz = plat_slot_zones(k);                    /* holds them (the break page packs the list) */
        for (z = 0; z < nz && z < PLAT_SLOT_ZONES; z++) {
            char *n = user_names[k * PLAT_SLOT_ZONES + z];
            int j = 2;
            u++;
            n[0] = 'B';
            n[1] = 'R';
            if (u >= 10)
                n[j++] = (char)('0' + u / 10);
            n[j++] = (char)('0' + u % 10);
            n[j] = 0;
            loop_slot[nloops] = (uint8_t)k;
            loop_zone[nloops] = (uint8_t)z;
            loop_names[nloops++] = n;
        }
    }
}

int engine_brk_nslots(void)
{
    if (!nloops)
        scan_loops();
    return nloops;
}
const char *const *engine_brk_slot_names(void)
{
    if (!nloops)
        scan_loops();
    return loop_names;
}

int engine_brk_slice(void) { return brk.running ? brk.st_slice : -1; }

void engine_brk_state(int *slice, int *bank, int *div, int *running)
{
    *slice = brk.st_slice;
    *bank = brk.st_bank;
    *div = brk.st_div;
    *running = brk.running;
}

/* the loop's outline for the screen: n columns of peak level 0..255 (main loop: decodes the
 * whole loop once, ~20k IMA steps; called when the loop changes, never per frame) */
static const int16_t ISTEP[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
    4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767};
static const int8_t IIDX[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
int engine_brk_outline(int which, uint8_t *peaks, int n)
{
    const brk_bank_t *bk = &brk.bank[which & 1];
    uint32_t i, ns = bk->n;
    int32_t pred = 0, idx = 0, col = 0, pk = 0;
    uint32_t per;
    if (!bk->adpcm || !ns || n <= 0)
        return 0;
    per = (ns + (uint32_t)n - 1u) / (uint32_t)n;       /* samples per column (32-bit: no __udivdi3) */
    for (i = 0; i < ns; i++) {
        uint32_t code = (bk->adpcm[i >> 1] >> ((i & 1u) * 4u)) & 15u;
        int32_t st = ISTEP[idx], vd = st >> 3, c;
        if (code & 4u) vd += st;
        if (code & 2u) vd += st >> 1;
        if (code & 1u) vd += st >> 2;
        pred += (code & 8u) ? -vd : vd;
        pred = pred < -32768 ? -32768 : pred > 32767 ? 32767 : pred;
        idx += IIDX[code & 7u];
        idx = idx < 0 ? 0 : idx > 88 ? 88 : idx;
        c = (int32_t)(i / per);
        if (c != col) {
            peaks[col] = (uint8_t)(pk >> 7);
            col = c;
            pk = 0;
        }
        if ((pred < 0 ? -pred : pred) > pk)
            pk = pred < 0 ? -pred : pred;
    }
    peaks[col] = (uint8_t)(pk >> 7);
    for (col++; col < n; col++)
        peaks[col] = 0;
    return 1;
}
float engine_gr_db(void) { return mst.gr_view; }
void engine_master_format(int i, char *buf) { master_format(&mst, i, buf); }

void engine_brk_loops(void)
{
    const brkpart_t *bp = &seq.pat[seq.ppat[TRK_BRK]].brk;
    brk_loop_t L[2];
    int w;
    scan_loops();                                       /* an upload may have changed the user loops */
    brk_hold = 1;                                       /* the ISR skips the break while we rewrite it */
    for (w = 0; w < 2; w++) {
        int slot = w ? bp->slot_b : bp->slot_a;
        uint32_t ns = 0, rate = 0;
        char name[9];
        const uint8_t *d = (slot >= X0X_NBREAKS && slot < nloops)
                               ? plat_slot(loop_slot[slot], loop_zone[slot], &ns, &rate, name) : 0;
        if (d) {
            L[w].adpcm = d;
            L[w].nsamples = ns;
            L[w].rate = rate;
            L[w].bars = loop_bars(ns, rate);
            breaks_set_loop(&brk, w, &L[w]);
        } else {
            breaks_set_loop(&brk, w, brk_builtin(slot < X0X_NBREAKS ? slot : 0));   /* built-in (or an empty user slot: the first) */
        }
    }
    brk_hold = 0;
}

POLY_COLD void engine_init(pattern_t *patterns, song_t *song, lane_t *lanes, const sound_t *base)
{
    clock_runtime_t previous_clock=seq.clock;
    int p;uint8_t owned_note[NBASS],owned_channel[NBASS],pad_note[3],pad_channel;
    if(seq.pat)engine_notes_clear();
    for(p=0;p<NBASS;p++) {owned_note[p]=seq.pat?seq.bass_notes_out[p]:255;owned_channel[p]=seq.pat?seq.bass_channels_out[p]:255;}
    pad_channel=seq.pat?seq.pad_channel_out:255;
    for(p=0;p<3;p++)pad_note[p]=seq.pat?seq.pad_notes_out[p]:255;
    cq_w = cq_r = 0;
    for (p = 0; p < 25; p++) sound_strip_config_default(&strip_cfg[p]);
    for (p = 0; p < 3; p++) { sound_strip_init(&mono_strip[p]); sound_strip_meter_clear(&mono_strip_meter[p]); }
    for (p = 0; p < NPARTS * NDRUM; p++)
        prob[p / NDRUM][p % NDRUM] = 100;             /* every step plays, until a lane says */
    drum909_init(&d909);
    drum808_init(&d808);
    bass303_init(&b303[0]);
    bass303_init(&b303[1]);
    breaks_init(&brk);
    fxbus_init(&fx, dly_buf, DLY_LEN);
    master_init(&mst);
    seq_init(&seq, patterns);
    seq.rate_units=rate_units;for(unsigned i=0;i<25;i++)rate_units[i]=0;
    seq.clock.tick_lo=previous_clock.tick_lo;seq.clock.tick_hi=previous_clock.tick_hi;
    seq.clock.generation=previous_clock.generation==UINT32_MAX?UINT32_MAX:previous_clock.generation+1u;
    seq.clock.flags=CLOCK_INVALIDATED;plat_midi_retire_clock(7);
    seq.midi_ready=(uint8_t)plat_midi_connected();
    for(p=0;p<NBASS;p++){seq.bass_notes_out[p]=owned_note[p];seq.bass_channels_out[p]=owned_channel[p];}
    seq.pad_channel_out=pad_channel;for(p=0;p<3;p++)seq.pad_notes_out[p]=pad_note[p];
    if(pad_channel<16)seq.pad_status=64;
    bass_held[0]=bass_held[1]=255;
    seq.song = song;
    motion_init(&mot, lanes);
    mot.base = mot_base;
    mot.apply = mot_apply;
    mot_base_snd = base;
    for (p = 0; p < NPARTS; p++) {
        mix_level[p] = 1.0f;
        mix_rev[p] = mix_dly[p] = 0.0f;
    }
}

/* Check all refusal conditions before a caller changes its undo journal. The
 * existing transport hold excludes Run; only this main thread produces CQ
 * writes, while the audio consumer can only make more room. */
POLY_COLD int engine_strip_restore_prepare(unsigned strip,const sound_strip_config_t *config)
{
    uint32_t saved;int ready;
    if(strip>=VARIANT_STRIPS||!config||!sound_strip_config_valid(config))return 0;
    saved=plat_control_enter();
    ready=!seq.playing&&!seq.req_stop&&seq.transport_hold&&CQ-(cq_w-cq_r)>=VP_COUNT;
    plat_control_leave(saved);return ready;
}
/* Main-only after successful prepare, with the same reservation and config.
 * A later Stop is ordered after acceptance and remains pending. It must not
 * turn an already journaled operation into a refusal. No I/O or journal scan
 * runs with interrupts masked. */
POLY_COLD void engine_strip_restore_commit(unsigned strip,const sound_strip_config_t *config)
{
    uint32_t saved=plat_control_enter(),w=cq_w;unsigned i;
    for(i=0;i<VP_COUNT;i++){
        unsigned value=(unsigned)variant_field_get(config,(int)i);
        cq[(w+i)%CQ]=(C_STRIP<<28)|(strip<<20)|(((i<<4)|(value>>12))<<12)|(value&4095u);
    }
    proj.variant.strip[strip]=*config;
    __asm__ volatile("" ::: "memory");cq_w=w+VP_COUNT;
    plat_control_leave(saved);
}
POLY_COLD int engine_strip_restore(unsigned strip,const sound_strip_config_t *config)
{
    if(!engine_strip_restore_prepare(strip,config))return 0;
    engine_strip_restore_commit(strip,config);return 1;
}
