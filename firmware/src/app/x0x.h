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
    uint8_t rsv[1];                      /* room to grow: an older, shorter object loads (zeros here) */
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

void ui_init(void);
void ui_frame(void);
void ui_input_only(void);
int ui_dirty(void);
void ui_external_edit(int begin);        /* 1 starts a gesture; 0 commits it */
void ui_external_changed(void);
void ui_external_sound(int target, int voice, int index, int value);
int ui_external_undo(int redo);          /* VR_OK or shared preflight/reservation rejection */
int ui_undo_preflight(int redo);        /* shared command/physical path; no history mutation */
void ui_external_saved(void);
uint8_t *ui_project_stage(uint32_t bytes);
void ui_project_stage_finish(int loaded);
void ui_say(const char *a, const char *b);
