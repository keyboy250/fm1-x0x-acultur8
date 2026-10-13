/* SPDX-License-Identifier: GPL-3.0-only */
#include "x0x.h"

static uint32_t variant_revision;
void variant_changed(void) { variant_revision++; }

void arrangement_defaults(arrangement_settings_t *a)
{
    int i;
    a->flags = a->key = a->mode = 0; a->parent = 255;
    for (i = 0; i < 2; i++) { a->bass_low[i] = 24; a->bass_high[i] = 72; }
    a->pad_channel = 255; a->pad_root = 60; a->pad_low = 48; a->pad_high = 84;
    for (i = 0; i < NSONG; i++) a->degree[i] = 0;
}
static int arrangement_degree_valid(const arrangement_settings_t *a, uint8_t degree)
{
    harmony_spec_t spec = {a->key,a->mode,degree,a->parent};
    harmony_triad_t chord;
    return degree == 0 || (degree <= 7 && harmony_resolve(&spec,60,0,127,&chord));
}
int arrangement_valid(const arrangement_settings_t *a)
{
    int i;
    if (a->flags & ~ARR_ENABLED || a->key > 11 || a->mode > HARMONY_CHROMATIC ||
        (a->parent > HARMONY_LOCRIAN && a->parent != 255) ||
        (a->pad_channel > 15 && a->pad_channel != 255) || a->pad_root > 127 ||
        a->pad_low > a->pad_high || a->pad_high > 127) return 0;
    for (i = 0; i < 2; i++) if (a->bass_low[i] > a->bass_high[i] || a->bass_high[i] > 127) return 0;
    for (i = 0; i < NSONG; i++) if (!arrangement_degree_valid(a,a->degree[i])) return 0;
    return 1;
}

void variant_defaults(variant_settings_t *v)
{
    uint32_t i;
    v->magic = VARIANT_MAGIC; v->schema = VARIANT_SCHEMA; v->bytes = sizeof *v;
    for (i = 0; i < VARIANT_STRIPS; i++) sound_strip_config_default(&v->strip[i]);
    arrangement_defaults(&v->arrangement);
}

int variant_valid(const variant_settings_t *v)
{
    uint32_t i;
    if (v->magic != VARIANT_MAGIC || v->schema != VARIANT_SCHEMA || v->bytes != sizeof *v) return 0;
    for (i = 0; i < VARIANT_STRIPS; i++) if (!sound_strip_config_valid(&v->strip[i])) return 0;
    return arrangement_valid(&v->arrangement);
}

int variant_schema1_valid(const variant_settings_t *v)
{
    uint32_t i;
    if (v->magic != VARIANT_MAGIC || v->schema != 1 || v->bytes != 208) return 0;
    for (i = 0; i < VARIANT_STRIPS; i++) if (!sound_strip_config_valid(&v->strip[i])) return 0;
    return 1;
}

int variant_is_default(const variant_settings_t *v)
{
    sound_strip_config_t d;
    arrangement_settings_t a;
    uint32_t i, b;
    if (!variant_valid(v)) return 0;
    sound_strip_config_default(&d);
    for (i = 0; i < VARIANT_STRIPS; i++)
        for (b = 0; b < sizeof d; b++)
            if (((const uint8_t *)&v->strip[i])[b] != ((const uint8_t *)&d)[b]) return 0;
    arrangement_defaults(&a);
    for (b = 0; b < sizeof a; b++)
        if (((const uint8_t *)&v->arrangement)[b] != ((const uint8_t *)&a)[b]) return 0;
    return 1;
}

int variant_field_get(const sound_strip_config_t *s, int i)
{
    switch (i) {
    case VP_HIGHPASS: return s->highpass_hz;
    case VP_LOWPASS: return s->lowpass_hz;
    case VP_BOOST: return s->boost_db;
    case VP_CEILING: return s->ceiling_db;
    case VP_RELEASE: return s->release_ms;
    case VP_FLAGS: return s->flags;
    default: return -1;
    }
}

int variant_field_set(sound_strip_config_t *s, int i, int value)
{
    sound_strip_config_t candidate = *s;
    if (i < 0 || i >= VP_COUNT || value < 0 || value > 20000) return 0;
    if (i >= VP_BOOST && value > 255) return 0;
    switch (i) {
    case VP_HIGHPASS: candidate.highpass_hz = (uint16_t)value; break;
    case VP_LOWPASS: candidate.lowpass_hz = (uint16_t)value; break;
    case VP_BOOST: candidate.boost_db = (uint8_t)value; break;
    case VP_CEILING: candidate.ceiling_db = (uint8_t)value; break;
    case VP_RELEASE: candidate.release_ms = (uint8_t)value; break;
    case VP_FLAGS: candidate.flags = (uint8_t)value; break;
    }
    if (!sound_strip_config_valid(&candidate)) return 0;
    *s = candidate;
    return 1;
}

void variant_apply(void)
{
    uint32_t s; int i;
    for (s = 0; s < VARIANT_STRIPS; s++)
        for (i = 0; i < VP_COUNT; i++)
            engine_strip_set((int)s, i, variant_field_get(&proj.variant.strip[s], i));
    engine_arrangement_refresh(&proj.variant.arrangement);
}

int variant_command(const variant_command_t *c, variant_reply_t *r)
{
    int result = VR_INVALID;
    if (!r) return VR_INVALID;
    r->request = c ? c->request : 0;
    if (!c) goto done;
    if (c->abi != VARIANT_ABI) { result = VR_UNSUPPORTED; goto done; }
    if (c->reserved) goto done;
    if (project_write_protected()) { result = VR_PROTECTED; goto done; }
    switch (c->op) {
    case VC_SET_SOUND: {
        const x0x_param_t *p;
        if (c->target >= NTARGETS || c->voice >= engine_nvoices(c->target) || c->index >= engine_nparams(c->target,c->voice)) break;
        p = engine_param(c->target, c->voice, c->index);
        if (!p || c->value < 0 || c->value > p->max) break;
        if (!engine_queue_space()) { result = VR_BUSY; break; }
        ui_external_sound(c->target, c->voice, c->index, c->value);
        result = VR_QUEUED;
        break;
    }
    case VC_SET_STRIP: {
        sound_strip_config_t candidate;
        if (c->target >= VARIANT_STRIPS || c->voice || c->index >= VP_COUNT) break;
        candidate = proj.variant.strip[c->target];
        if (!variant_field_set(&candidate, c->index, c->value)) break;
        if (!engine_strip_set(c->target, c->index, c->value)) { result = VR_BUSY; break; }
        proj.variant.strip[c->target] = candidate;
        ui_external_changed(); result = VR_QUEUED;
        break;
    }
    case VC_BEGIN_EDIT: ui_external_edit(1); result = VR_OK; break;
    case VC_END_EDIT: ui_external_edit(0); result = VR_OK; break;
    case VC_UNDO: case VC_REDO:
        result = ui_undo_preflight(c->op == VC_REDO);
        if (result != VR_OK) break;
        result = ui_external_undo(c->op == VC_REDO); break;
    case VC_SET_BLOCK: {
        song_bar_t b;
        int i;
        uint8_t degree = c->voice >> 3;
        if (c->target >= NSONG || c->voice & 192u || c->index > 31 ||
            c->value < 0 || (uint32_t)c->value > 0x1ffffffu ||
            !arrangement_degree_valid(&proj.variant.arrangement,degree)) break;
        for (i = 0; i < NTRACKS; i++) b.pat[i] = (uint8_t)(((uint32_t)c->value >> (i * 5)) & 31u);
        b.mute = (uint8_t)(c->index | ((c->voice & 7u) << 5));
        if (!engine_arrangement_block(&proj.arr.song.bar[c->target],&proj.variant.arrangement.degree[c->target],&b,degree)) {
            result = VR_BUSY; break;
        }
        ui_external_changed(); result = VR_QUEUED; break;
    }
    case VC_SET_ARR_GLOBAL: {
        arrangement_settings_t candidate = proj.variant.arrangement;
        uint8_t *field = (uint8_t *)&candidate;
        if (c->target || c->voice || c->index > 3 || c->value < 0 || c->value > 255) break;
        field[c->index] = (uint8_t)c->value;
        if (!arrangement_valid(&candidate)) break;
        if (c->index == 0 && (seq.playing || seq.req_stop || seq.req_start || seq.req_cont)) { result = VR_BUSY; break; }
        if (!engine_arrangement_global((uint8_t *)&proj.variant.arrangement + c->index,(uint8_t)c->value,c->index == 0)) {
            result = VR_BUSY; break;
        }
        ui_external_changed(); result = VR_QUEUED; break;
    }
    case VC_SET_ARR_LENGTH:
        if (c->target || c->voice || c->index || c->value < 0 || c->value > NSONG) break;
        if (seq.playing || seq.req_stop || seq.req_start || seq.req_cont) { result = VR_BUSY; break; }
        if (!engine_arrangement_length(&proj.arr.song.len,(uint16_t)c->value)) { result = VR_BUSY; break; }
        ui_external_changed(); result = VR_QUEUED; break;
    case VC_EDIT_BLOCK:
        if (c->target >= NSONG || c->voice || c->index || c->value) break;
        seq.arrangement.editing = c->target; result = VR_OK; break;
    case VC_QUEUE_BLOCK:
        if (c->target >= proj.arr.song.len || c->voice || c->index || c->value ||
            !(proj.variant.arrangement.flags & ARR_ENABLED)) break;
        result = engine_arrangement_queue(c->target) ? VR_QUEUED : VR_BUSY; break;
    case VC_SAVE:
        if (!engine_transport_reserve()) { result = VR_BUSY; break; }
        ui_external_edit(0);
        result = project_save();
        engine_transport_release();
        if (!result) { ui_external_saved(); result = VR_OK; }
        else result = result == -7 ? VR_FULL : result == -8 ? VR_PROTECTED : result == -9 ? VR_BUSY : VR_IO;
        break;
    default: result = VR_UNSUPPORTED; break;
    }
done:
    r->result = result; r->revision = variant_revision; r->ticket = engine_queue_written();
    return result;
}

int variant_snapshot(int section, int target, int voice, variant_snapshot_t *s)
{
    int i, n;
    if (!s) return VR_INVALID;
    s->count = 0;
    for (i = 0; i < 16; i++) s->value[i] = 0;
    s->abi = VARIANT_ABI; s->section = (uint32_t)section; s->revision = variant_revision;
    if (section == VS_INFO) {
        s->count = 8;
        s->value[0] = VCAP_SOUND | VCAP_STRIP_CONFIG | VCAP_UNDO | VCAP_SAVE | VCAP_STRIP_AUDIO |
            VCAP_ARRANGEMENT | VCAP_HARMONY_METADATA;
        s->value[1] = (uint32_t)ui_dirty(); s->value[2] = (uint32_t)project_write_protected();
        s->value[3] = engine_queue_written(); s->value[4] = engine_queue_read();
        s->value[5] = VARIANT_SCHEMA; s->value[6] = VARIANT_STRIPS;
        s->value[7] = engine_strip_processing_mask();
    } else if (section == VS_SOUND) {
        if (target < 0 || target >= NTARGETS || voice < 0 || voice >= engine_nvoices(target)) return VR_INVALID;
        n = engine_nparams(target, voice);
        if (n > 16) return VR_INVALID;
        s->count = (uint32_t)n;
        for (i = 0; i < n; i++) s->value[i] = proj.sound.v[target][voice][i];
    } else if (section == VS_STRIP) {
        if (target < 0 || target >= (int)VARIANT_STRIPS || voice) return VR_INVALID;
        s->count = VP_COUNT;
        for (i = 0; i < VP_COUNT; i++) s->value[i] = (uint32_t)variant_field_get(&proj.variant.strip[target],i);
    } else if (section == VS_METERS) {
        const sound_strip_meter_t *m;
        if (voice || target < 0 || target >= (int)VARIANT_STRIPS) return VR_INVALID;
        m = engine_strip_meter(target);
        if (!m) return VR_UNSUPPORTED;
        s->count = 3;
        s->value[0] = (uint32_t)(m->input_peak * 1000000.0f);
        s->value[1] = (uint32_t)(m->output_peak * 1000000.0f);
        s->value[2] = (uint32_t)(m->gain_min * 1000000.0f);
    } else if (section == VS_BLOCK) {
        if (voice || target < 0 || target >= NSONG) return VR_INVALID;
        s->count = 8;
        for (i = 0; i < NTRACKS; i++) s->value[i] = proj.arr.song.bar[target].pat[i];
        s->value[5] = proj.arr.song.bar[target].mute & 31u;
        s->value[6] = (uint32_t)SONG_REP(&proj.arr.song.bar[target]);
        s->value[7] = proj.variant.arrangement.degree[target];
    } else if (section == VS_ARRANGEMENT) {
        const arrangement_settings_t *a = &proj.variant.arrangement;
        const volatile arrangement_runtime_t *r = &seq.arrangement;
        if (target || voice) return VR_INVALID;
        s->count = 16;
        s->value[0] = a->flags; s->value[1] = proj.arr.song.len; s->value[2] = r->editing;
        s->value[3] = seq.playing && (r->flags & ARR_ENABLED) && proj.arr.song.len ? seq.song_pos : 255;
        s->value[4] = r->queued; s->value[5] = proj.arr.song.len ? seq.song_start : 255;
        s->value[6] = r->tick; s->value[7] = seq.song_rep;
        s->value[8] = a->key; s->value[9] = a->mode; s->value[10] = a->parent;
        s->value[11] = r->degree; s->value[12] = r->key; s->value[13] = r->mode; s->value[14] = r->parent;
        s->value[15] = ((r->mute >> 5) + 1u);
    } else if (section == VS_CHORD) {
        const volatile arrangement_runtime_t *a = &seq.arrangement;
        harmony_spec_t spec = {a->key,a->mode,a->degree,a->parent};
        harmony_triad_t chord;
        if (target || voice) return VR_INVALID;
        s->count = 7;
        if (seq.playing && (a->flags & ARR_ENABLED) && a->degree && harmony_resolve(&spec,60,0,127,&chord)) {
            s->value[0] = 1;
            for (i = 0; i < 3; i++) { s->value[1+i] = chord.pitch_class[i]; s->value[4+i] = chord.midi[i]; }
        }
    } else return VR_UNSUPPORTED;
    return VR_OK;
}
