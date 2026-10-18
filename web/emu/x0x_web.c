/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X in the browser: the host simulator (host/x0x_host.c: the whole app, UI, sequencer, engines,
 * against a simulated FM-1) compiled to WebAssembly and run in an AudioWorklet. The worklet asks
 * for audio; producing it advances the device's clock one millisecond at a time, which runs the
 * engine every 256 samples (the I2S half buffer) and the UI every 16 ms, as on the FM-1. The page
 * sends input, and reads the screen, the lights and the saved objects back through these exports.
 *
 *   web/emu/build.sh   ->  build/emu/x0x.wasm (+ the page) */
#include <stdint.h>

static void web_audio(const int32_t *blk, uint32_t n);
#define X0X_WEB_SYSEX 1
#include "../../host/x0x_host.c"
#include "../../firmware/src/app/variant.h"

#undef __attribute__

/* Bounded browser MIDI FIFO. Host strips attributes; exports live here after undef. */
__attribute__((used,visibility("default"))) int web_midi_ready(int ready) {return host_web_midi_ready(ready);}
__attribute__((used,visibility("default"))) uint32_t web_midi_epoch(void) {return host_web_midi_epoch();}
__attribute__((used,visibility("default"))) uint32_t web_midi_transport_epoch(void) {return host_web_midi_transport_epoch();}
__attribute__((used,visibility("default"))) uint32_t web_midi_output_version(void) {return host_web_midi_output_version();}
__attribute__((used,visibility("default"))) uint32_t web_midi_cancel(void) {return host_web_midi_cancel();}
__attribute__((used,visibility("default"))) uint32_t web_midi_pending(void) {return host_web_midi_pending();}
__attribute__((used,visibility("default"))) int web_midi_batch(int max) {return host_web_midi_batch(max);}
__attribute__((used,visibility("default"))) uint32_t web_midi_packet(int index) {return host_web_midi_packet(index);}
__attribute__((used,visibility("default"))) int web_midi_ack(int prefix) {return host_web_midi_ack(prefix);}
/* Main/worklet message context only: explicit selected input is source2.
 * Return0 is bounded input-ring acceptance, not audio application/delivery. */
__attribute__((used,visibility("default"))) int web_midi_ingest(uint32_t packet,int source){return host_web_midi_ingest(packet,source);}
__attribute__((used,visibility("default"))) void web_cc_reset_source(int source){if(source>=0&&source<CC_SOURCES)cc_reset_source((uint8_t)source);}
__attribute__((used,visibility("default"))) uint32_t web_cc_source(int source,int field){return source<0||source>=CC_SOURCES||field<0?0xffffffffu:cc_source_field((uint8_t)source,(unsigned)field);}
__attribute__((used,visibility("default"))) uint32_t web_cc_stat(int index){return index<0||index>=6?0xffffffffu:((uint32_t *)&cc_stats)[index];}
__attribute__((used,visibility("default"))) uint32_t web_cc_pending(void){return mi_w-mi_r;}
__attribute__((used,visibility("default"))) uint32_t web_cc_version(void){return 0x00010001u;} /* ABI1 / map1; partial candidate, not complete coverage */
/* Browser slice CC ABI1. Host-only: no firmware capability or MIDI-source0/1 claim.
 * Exact take returns0 success,-1 invalid21-bit domain,-2 mismatch/context.
 * Route admission belongs to the correlated worklet request, not this C ABI. */
__attribute__((used,visibility("default"))) uint32_t web_slice_cc_version(void){return 0x00010001u;}
__attribute__((used,visibility("default"))) int web_cc_take_exact(uint32_t parameter,uint32_t value){
    cc_source_t *s=&cc_sources[2];
    if(parameter<1104u||parameter>1116u||value>0x1fffffu)return -1;
    if(!s->ready||s->exact_parts!=7||s->parameter!=parameter||s->exact!=value)return -2;
    s->exact_parts=0;return 0; /* selectors/native arm/edge/destination untouched */
}
/* Bit0 running,1 pendingtransport,2 reservation/stage,3 C ownership,
 * 4 benchmark/save job,5 queued audio commands or MIDI ingress. */
__attribute__((used,visibility("default"))) uint32_t web_slice_control_state(void){
    return (seq.playing?1u:0u)|((seq.req_start||seq.req_stop||seq.req_cont)?2u:0u)|
      ((seq.transport_hold||seq.transport_stage)?4u:0u)|
      ((external_edit||ui.rec||ui.btn||ui.keys||ui.overlay==O_ASK)?8u:0u)|
      ((perf_testing()||save_wait)?16u:0u)|
      ((engine_queue_read()!=engine_queue_written()||mi_w!=mi_r)?32u:0u);
}

static uint32_t web_clock_words[10],web_clock_generation;
__attribute__((used,visibility("default"))) uint32_t web_clock_version(void){return CLOCK_PROTOCOL;}
__attribute__((used,visibility("default"))) int web_clock_command(uint32_t protocol,unsigned role,unsigned source,unsigned policy){return engine_clock_request(protocol,role,source,policy,&web_clock_generation);}
__attribute__((used,visibility("default"))) int web_clock_retire(uint32_t protocol,unsigned source){return engine_clock_source_retire(protocol,source,&web_clock_generation);}
__attribute__((used,visibility("default"))) uint32_t web_clock_expected(void){return web_clock_generation;}
__attribute__((used,visibility("default"))) uint32_t *web_clock_snapshot(uint32_t protocol){return engine_clock_status(protocol,web_clock_words)?0:web_clock_words;}

/* Fixed-size, shared-engine editor bridge. Never expose live project memory. */
static variant_reply_t web_variant_reply;
static variant_snapshot_t web_variant_view;
__attribute__((used, visibility("default"))) variant_reply_t *web_variant_command(
    int abi, int op, uint32_t request, int target, int voice, int index, int32_t value)
{
    web_variant_reply = (variant_reply_t){request, 0, 0, VR_INVALID};
    /* Rejected transport fields still acknowledge current progress. */
    if (variant_snapshot(VS_INFO, 0, 0, &web_variant_view) == VR_OK) {
        web_variant_reply.revision = web_variant_view.revision;
        web_variant_reply.ticket = web_variant_view.value[3];
    }
    if (abi < 0 || abi > 65535 || op < 0 || op > 65535 ||
        target < 0 || target > 255 || voice < 0 || voice > 255 || index < 0 || index > 255)
        return &web_variant_reply;
    variant_command_t command = {(uint16_t)abi, (uint16_t)op, request,
        (uint8_t)target, (uint8_t)voice, (uint8_t)index, 0, value};
    variant_command(&command, &web_variant_reply);
    return &web_variant_reply;
}
__attribute__((used, visibility("default"))) variant_snapshot_t *web_variant_snapshot(int section, int target, int voice)
{
    return variant_snapshot(section, target, voice, &web_variant_view) == VR_OK ? &web_variant_view : 0;
}

/* output ring, stereo float, filled by run_ms, drained by web_render */
#define RING 4096u
static float ring[RING * 2];
static uint32_t ring_w, ring_r;
static void web_audio(const int32_t *blk, uint32_t n)
{
    uint32_t k;
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_w++ % RING) * 2u;
        ring[i] = (float)blk[2 * k] / 8388608.0f;
        ring[i + 1] = (float)blk[2 * k + 1] / 8388608.0f;
    }
}

static float out_l[1024], out_r[1024], sync_l[1024], sync_r[1024];
static float sync_ring[RING * 2];
static uint32_t sync_w;
__attribute__((used, visibility("default"))) float *web_out_l(void) { return out_l; }
__attribute__((used, visibility("default"))) float *web_out_r(void) { return out_r; }

/* n frames (<= 1024) into out_l / out_r: the device runs until they exist */
__attribute__((used, visibility("default"))) void web_render(uint32_t n)
{
    uint32_t k;
    if (n > 1024u)
        n = 1024u;
    while (ring_w - ring_r < n)
        run_ms(1);
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_r++ % RING) * 2u;
        out_l[k] = ring[i];
        out_r[k] = ring[i + 1];
        sync_l[k] = sync_ring[i]; sync_r[k] = sync_ring[i + 1];
    }
}

__attribute__((used, visibility("default"))) void web_boot(void) { boot(); }

/* the screen reader (announce.c): the page hands the device a reader's SysEx (between F0 and F7) and takes
 * what it says back, whole F0..F7 frames, from web_said_buf (web_said_len bytes; web_said_clear when read) */
static uint8_t said_buf[4096], sx_in[64];
static uint32_t said_n;
static void web_said(const uint8_t *p, uint32_t n)
{
    if (said_n + n <= sizeof said_buf) {
        memcpy(said_buf + said_n, p, n);
        said_n += n;
    }
}
__attribute__((used, visibility("default"))) uint8_t *web_said_buf(void) { return said_buf; }
__attribute__((used, visibility("default"))) uint32_t web_said_len(void) { return said_n; }
__attribute__((used, visibility("default"))) void web_said_clear(void) { said_n = 0; }
__attribute__((used, visibility("default"))) uint8_t *web_sysex_in(void) { return sx_in; }
__attribute__((used, visibility("default"))) void web_sysex(uint32_t n) { ann_frame(sx_in, n > sizeof sx_in ? sizeof sx_in : n, now_ms); }
__attribute__((used, visibility("default"))) void web_buttons(uint32_t m) { held_btn = m; }
__attribute__((used, visibility("default"))) void web_buttons_tagged(uint32_t m,uint32_t ticket) {if(ticket){held_btn=m;web_input_pending=ticket;}}
__attribute__((used, visibility("default"))) uint32_t web_input_ticket(void) {return web_input_applied;}
__attribute__((used, visibility("default"))) void web_keys(uint32_t m) { held_keys = m; }
__attribute__((used, visibility("default"))) void web_enc(int role, int32_t n)
{
    if (role >= 0 && role < NE)
        enc_acc[role] += n;
}
__attribute__((used, visibility("default"))) void web_master(uint32_t v) { (void)master_control_set(&master_control,v>4096u?4096u:v,MC_POINTER); }
__attribute__((used, visibility("default"))) uint32_t web_master_state(void) { return master_control_state(&master_control); }
__attribute__((used, visibility("default"))) void web_master_step(int32_t delta) {
    if(!delta || delta < -4096 || delta > 4096)return;
    int32_t v=(int32_t)plat_master()+delta;
    (void)master_control_set(&master_control,(uint32_t)(v<0?0:v>4096?4096:v),MC_POINTER);
}
__attribute__((used, visibility("default"))) uint32_t web_lit_buttons(void) { return lit_btn; }
__attribute__((used, visibility("default"))) uint32_t web_lit_keys(void) { return lit_keys; }
__attribute__((used, visibility("default"))) uint32_t web_dim_keys(void) { return dim_keys & ~lit_keys; }
__attribute__((used, visibility("default"))) uint16_t *web_fb(void) { return fb; }
__attribute__((used, visibility("default"))) uint32_t web_blits(void) { return blits; }
__attribute__((used, visibility("default"))) uint32_t web_now(void) { return now_ms; }
__attribute__((used, visibility("default"))) uint32_t web_cpu(void) { return cpu_pct; }

/* saved objects: the page keeps them (localStorage) and gives them back before boot */
__attribute__((used, visibility("default"))) uint8_t *web_store(uint32_t obj) { return obj < OBJ_NOBJ ? store[obj] : 0; }
__attribute__((used, visibility("default"))) int web_store_len(uint32_t obj) { return obj < OBJ_NOBJ ? store_len[obj] : -1; }
__attribute__((used, visibility("default"))) void web_store_set_len(uint32_t obj, int n)
{
    if (obj < OBJ_NOBJ)
        store_len[obj] = n < 0 ? -1 : n > (int)PLAT_STORE_MAX ? (int)PLAT_STORE_MAX : n;
}
__attribute__((used, visibility("default"))) uint32_t web_store_writes(void) { return store_writes; }
__attribute__((used, visibility("default"))) uint32_t web_nobj(void) { return OBJ_NOBJ; }
__attribute__((used, visibility("default"))) uint32_t web_store_max(void) { return PLAT_STORE_MAX; }

/* user break slots (the BREAK part's own loops, as tools/upload_breaks.py puts them on the FM-1):
 * the page decodes a file to mono 22050 Hz 16-bit into web_slot_pcm and hands it over one loop
 * (zone) at a time; it is IMA-encoded here exactly as the simulator's "slot" command does. A slot
 * holds what 80 KiB of flash holds on the device, all its loops together. */
#define WEB_SLOT_MAX (2u * (0x14000u - 512u))
static int16_t web_pcm[WEB_SLOT_MAX];
static char web_name[9];
__attribute__((used, visibility("default"))) int16_t *web_slot_pcm(void) { return web_pcm; }
__attribute__((used, visibility("default"))) uint32_t web_slot_max(void) { return WEB_SLOT_MAX; }
__attribute__((used, visibility("default"))) char *web_slot_namebuf(void) { return web_name; }
__attribute__((used, visibility("default"))) void web_slot_clear(int k)
{
    int z;
    if (k < 0 || k >= PLAT_NSLOTS)
        return;
    for (z = 0; z < PLAT_SLOT_ZONES; z++) {
        free(slot_data[k][z]);
        slot_data[k][z] = 0;
        slot_n[k][z] = 0;
    }
    slot_nz[k] = 0;
    slot_is_img[k] = 0;
    web_name[8] = 0;
    memcpy(slot_name[k], web_name, 9);                /* the name the page put in web_slot_namebuf */
}
/* loop z of slot k from the first n samples of web_slot_pcm: 0 = kept */
__attribute__((used, visibility("default"))) int web_slot_zone(int k, int z, uint32_t n, uint32_t rate)
{
    if (k < 0 || k >= PLAT_NSLOTS || z < 0 || z >= PLAT_SLOT_ZONES || !n)
        return -1;
    if (n > WEB_SLOT_MAX)
        n = WEB_SLOT_MAX;
    free(slot_data[k][z]);
    if (!(slot_data[k][z] = calloc(1, (n + 1) / 2 + 1)))
        return -1;
    ima_encode(web_pcm, n, slot_data[k][z]);
    slot_n[k][z] = n;
    slot_rate[k][z] = rate;
    if (z + 1 > slot_nz[k])
        slot_nz[k] = z + 1;
    return 0;
}
/* the slots changed: the break part reloads its loops (and the LOOPS page's names) */
__attribute__((used, visibility("default"))) void web_slots_changed(void) { engine_brk_loops(); }

/* Marker lane is independent of the legacy 8-slice BREAK bank and physical flash layout. */
#include "../../firmware/src/dsp/marked_player.c"
static marked_player_t marked;
static float marked_pcm[MARKED_MAX_FRAMES * 2u], marked_l[1024], marked_r[1024];
static marked_slice_t marked_meta[MARKED_MAX_SLICES];
__attribute__((used, visibility("default"))) float *web_marked_pcm(void) { return marked_pcm; }
__attribute__((used, visibility("default"))) marked_slice_t *web_marked_meta(void) { return marked_meta; }
__attribute__((used, visibility("default"))) float *web_marked_l(void) { return marked_l; }
__attribute__((used, visibility("default"))) float *web_marked_r(void) { return marked_r; }
__attribute__((used, visibility("default"))) int web_marked_load(uint32_t frames, uint32_t rate, int channels, int count, uint32_t length)
{ return marked_load(&marked, marked_pcm, frames, rate, channels, marked_meta, count, length); }
__attribute__((used, visibility("default"))) void web_marked_play(int repeat) { marked_play(&marked, repeat); }
/* Current cursor and sounding tails survive; flag is used at next loop end.
 * This controls original audition only. FM1-sync repeats until explicit Stop. */
__attribute__((used,visibility("default"))) int web_marked_repeat(int repeat){
    if(repeat<0||repeat>1)return -1;
    if(!marked.count)return -2;
    marked.repeat=(uint8_t)repeat;return 0;
}

__attribute__((used, visibility("default"))) void web_marked_stop(void) { marked_stop(&marked); }
__attribute__((used, visibility("default"))) int web_marked_pad(int i) { marked_stop(&marked); return marked_trigger(&marked, i); }
__attribute__((used, visibility("default"))) void web_marked_render(uint32_t n) { marked_render(&marked, marked_l, marked_r, n > 1024 ? 1024 : n); }
__attribute__((used, visibility("default"))) uint32_t web_marked_cursor(void) { return marked.cursor; }
__attribute__((used, visibility("default"))) uint32_t web_marked_triggers(void) { return marked.triggers; }
__attribute__((used, visibility("default"))) int web_marked_last(void) { return marked.last_slice; }
__attribute__((used, visibility("default"))) int web_marked_playing(void) { return marked.playing; }

/* Browser audition follows the sequencer clock at its exact audio event boundaries.
 * Only trigger intervals change: PCM playback increments remain rate/44100. */
static int sync_enabled, sync_waiting, sync_error;
static uint64_t sync_ticks;
static double sync_offset, sync_period = 44100.0 * 60.0 / (125.0 * 24.0);
static double sync_origin, sync_source_bpm, sync_cycle = -1;
static uint32_t sync_tick_seen;
static double sync_beat(void) { return (double)sync_ticks / 24.0 + sync_offset / sync_period / 24.0; }
void web_seq_transport(int playing, int from_top) {
    if(from_top) { sync_ticks=0;sync_tick_seen=0;sync_offset=0; }
    if(sync_enabled) {
        marked_stop(&marked);
        if(playing) sync_waiting=1;
    }
}
void web_seq_tick(uint32_t period_q8) {
    if(sync_tick_seen) sync_ticks++; else sync_tick_seen=1;
    sync_offset=0;sync_period=(double)period_q8/256.0;
    if(sync_enabled && sync_waiting && sync_ticks%96u==0) {
        sync_waiting=0;sync_origin=sync_beat();sync_cycle=-1;
        marked.triggers=0;marked.next=0;marked.last_slice=255;
    }
}
void web_slice_audio(uint32_t n) {
    uint32_t i;
    for(i=0;i<n;i++) {
        float l=0,r=0;
        if(sync_enabled && seq.playing && !sync_waiting) {
            double loop_beats=(double)marked.length/44100.0*sync_source_bpm/60.0;
            double beat=sync_beat()-sync_origin;
            double cycle=floor((beat+1e-10)/loop_beats);
            double phase=beat-cycle*loop_beats;
            if(phase<0)phase=0;
            if(cycle!=sync_cycle) { sync_cycle=cycle;marked.next=0; }
            marked.cursor=(uint32_t)(phase*60.0/sync_source_bpm*44100.0);
            while(marked.next<marked.count && (double)marked.slices[marked.next].time/44100.0*sync_source_bpm/60.0<=phase+1e-10) {
                if(marked_trigger(&marked,marked.next)) {
                    sync_error=1;sync_enabled=0;marked_stop(&marked);break;
                }
                marked.next++;
            }
            marked.playing=0; /* voices advance normally; musical scheduling is above */
            marked_render(&marked,&l,&r,1);
        }
        sync_ring[(sync_w%RING)*2]=l;sync_ring[(sync_w%RING)*2+1]=r;sync_w++;
        if(seq.playing) sync_offset++;
    }
}
__attribute__((used, visibility("default"))) int web_sync_arm(float source_bpm) {
    if(!marked.count || !isfinite(source_bpm) || source_bpm<20 || source_bpm>300 || seq.ext)return -1;
    marked_stop(&marked);sync_source_bpm=source_bpm;sync_enabled=sync_waiting=1;sync_error=0;
    if(!seq.playing)seq_start(&seq);
    return 0;
}
__attribute__((used, visibility("default"))) void web_sync_off(void) { sync_enabled=sync_waiting=sync_error=0;marked_stop(&marked); }
__attribute__((used, visibility("default"))) float *web_sync_l(void) { return sync_l; }
__attribute__((used, visibility("default"))) float *web_sync_r(void) { return sync_r; }
__attribute__((used, visibility("default"))) int web_sync_state(void) { return sync_error?-1:sync_enabled?(sync_waiting?1:2):0; }
__attribute__((used, visibility("default"))) float web_seq_bpm(void) { return seq_tempo(&seq); }
__attribute__((used, visibility("default"))) int web_seq_playing(void) { return seq.playing; }
__attribute__((used, visibility("default"))) float web_seq_beat(void) { return (float)sync_beat(); }
__attribute__((used, visibility("default"))) void web_seq_set_bpm(float bpm) { if(isfinite(bpm)&&bpm>=20&&bpm<=275)seq.bpm=bpm; }
__attribute__((used, visibility("default"))) void web_seq_play(int on) { if(on)seq_start(&seq);else seq_stop(&seq); }
