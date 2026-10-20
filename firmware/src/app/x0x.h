/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X app: the project (everything that is saved) and the app-level entry points.
 * Included by the unity build after gfx.c, so drawing helpers are in scope. */
#pragma once
#include <stdint.h>
#include "plat.h"
#include "engine.h"
#include "variant.h"
#include "../seq/pattern.h"
#include "../seq/sequencer.h"
#include "../seq/tb3po.h"
#include "../seq/motion.h"
#include "../dsp/breaks.h"
#include "../dsp/fxbus.h"            /* FX_* param indices (the send pages) */

#ifndef X0X_VERSION                     /* a release build passes its own (tools/build.py --release) */
#define X0X_VERSION "DEV"
#endif
#define PROJ_MAGIC 0x50305830u           /* "0X0P" */
#define PROJ_FORMAT 1u
#define PROJ_STEREO 2u
#define PROJ_SND_REV 4u                  /* 1: the 909 SD has a DECAY pot (index 3; LEVEL.. moved up one);
                                          * 2: the master's one-knob COMP and FILTER, the delay's TAPE;
                                          * 3: the 909 BD has sends (REV, DLY at 8, 9; PAN moved to 10);
                                          * 4: the 808 BD too (REV, DLY at 7, 8; PAN moved to 9) */

typedef struct {
    uint32_t magic, format;
    uint8_t palette, keyled, clk_out, notes_out;   /* keyled: 0 off, 1 keys and the buttons' glow, 2 keys only */
    uint16_t bpm_x10;
    uint8_t accent_q7, keysound;         /* black drum keys sound: 0 = only when stopped or recording, 1 = always */
    uint8_t autosave_off;                /* 0: save by itself while stopped and idle (ui.c autosave) */
    uint8_t rsv_bright;                  /* was the screen's brightness (removed: always full); unused */
    uint8_t stereo;                      /* the pans the sound has: 0 none (saved mono), 1 the parts', 2 the
                                          * drum voices' too (PROJ_STEREO); a load centres what is missing */
    uint8_t snd_rev;                     /* the sound's layout (PROJ_SND_REV): 0 = before the 909 SD's DECAY */
    uint8_t midi_ch[5];                  /* each part's MIDI channel (in and out): 0 = its default (an older
                                          * project), 1-16, 17 = OFF */
    uint8_t out_loud;                    /* 1: the output 6 dB hotter (the codec's full scale) */
    uint8_t usb_full;                    /* 1: USB audio at full level, the MASTER knob for the FM-1's own outputs */
    uint8_t one_core;                    /* 1: the second core stays off (GLO > 2ND CORE, or after it crashed
                                          * beside it); applies at the next power-on. Older projects: 0, two */
} settings_t;

/* the song and the motion lanes: one blob, split across OBJ_SONG and OBJ_MOTION */
typedef struct {
    song_t song;
    lane_t lane[NLANE];
} arrange_t;

typedef struct {
    settings_t set;
    sound_t sound;
    pattern_t pat[NPAT];
    arrange_t arr;
    variant_settings_t variant;       /* appended: legacy project prefix offsets stay fixed */
} project_t;

extern project_t proj;

void project_defaults(void);
int project_load(void);                  /* 0 loaded; -8 rejected packed save leaves live project intact */
int project_save(void);                  /* 0 verified; -7 full; -8 protected; -9 transport pending/running */
void project_apply_io(void);             /* the settings into the sequencer and the engine: MIDI, the output */
int project_mem_pct(void);               /* how full the room for patterns, song and motion is, % */
int project_write_protected(void);
void project_discard_rejected(void);     /* explicit factory reset only */
int project_save_settings(void);
void project_flash_changed(void);

int ui_delete_block(int block); /* stopped, arrangement row only; preserves patterns/sounds */
int ui_duplicate_block(int block); /* stopped, independent patterns; bank or VR_* */
void ui_init(void);
void ui_frame(void);
void ui_input_only(void);
int ui_dirty(void);
void ui_external_edit(int begin);        /* 1 starts a gesture; 0 commits it */
void ui_external_changed(void);
void ui_external_sound(int target, int voice, int index, int value);
int ui_external_undo(int redo);          /* VR_OK or shared preflight/reservation rejection */
int ui_cc_undo(int redo);                /* stopped MIDI history; never closes an owned gesture */
int ui_undo_preflight(int redo);        /* shared command/physical path; no history mutation */
void ui_external_saved(void);
uint8_t *ui_project_stage(uint32_t bytes);
void ui_project_stage_finish(int loaded);
void ui_say(const char *a, const char *b);

/* Isolated explicit-context native actions; main producer only. */
enum { CA_OK,CA_STOP,CA_OWNER,CA_CONTEXT,CA_CONFIRM,CA_PROTECTED,CA_QUEUE,CA_MOTION,CA_JOURNAL,CA_CHECKPOINT,CA_IO,CA_BENCHMARK,CA_SCRATCH,CA_INVALID };
int ui_cc_action_ready(int act);
typedef struct { uint32_t revision, transport_epoch, fingerprint; } cc_action_guard_t;
typedef char cc_action_guard_size[(sizeof(cc_action_guard_t)==12)?1:-1];
int ui_cc_action_arm(int act,cc_action_guard_t *guard);
int ui_cc_action(int act,int pattern,int part,int destination,int block,const cc_action_guard_t *guard);
uint32_t project_fingerprint(void);
uint32_t variant_current_revision(void);
void cc_actions_reset(void);
void project_defaults_into(project_t *destination);
int project_checkpoint_matches(void);

int project_strip_default_save(unsigned strip);
int project_strip_default_get(unsigned strip,sound_strip_config_t *config);
int ui_strip_default_action(unsigned strip,int apply);
void ui_warn(const char *a, const char *b);   /* ui_say for what did not happen: a screen reader says it whole */
/* backup and restore over USB (backup.c) */
void ui_flush(void);                     /* stopped: the unsaved changes saved now (a backup takes the flash's) */
int ui_saves_held(void);
void ui_hold_saves(int on);              /* 1: no save until 0 (a backup streams the flash) */
void ui_redraw(void);                    /* the whole screen drawn again at the next frame */
int app_restored(void);                 /* a restore wrote the project: loaded again, as at power-on */
