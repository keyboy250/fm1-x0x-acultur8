/* SPDX-License-Identifier: GPL-3.0-only */
#include "x0x.h"

static uint32_t variant_revision;
void variant_changed(void) { variant_revision++; }

void variant_defaults(variant_settings_t *v)
{
    uint32_t i;
    v->magic = VARIANT_MAGIC; v->schema = VARIANT_SCHEMA; v->bytes = sizeof *v;
    for (i = 0; i < VARIANT_STRIPS; i++) sound_strip_config_default(&v->strip[i]);
}

int variant_valid(const variant_settings_t *v)
{
    uint32_t i;
    if (v->magic != VARIANT_MAGIC || v->schema != VARIANT_SCHEMA || v->bytes != sizeof *v) return 0;
    for (i = 0; i < VARIANT_STRIPS; i++) if (!sound_strip_config_valid(&v->strip[i])) return 0;
    return 1;
}

int variant_is_default(const variant_settings_t *v)
{
    sound_strip_config_t d;
    uint32_t i, b;
    if (!variant_valid(v)) return 0;
    sound_strip_config_default(&d);
    for (i = 0; i < VARIANT_STRIPS; i++)
        for (b = 0; b < sizeof d; b++)
            if (((const uint8_t *)&v->strip[i])[b] != ((const uint8_t *)&d)[b]) return 0;
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
        /* An undo may reapply every sound and strip value; reserve the whole burst. */
        if (engine_queue_space() < 768u) { result = VR_BUSY; break; }
        ui_external_undo(c->op == VC_REDO); result = VR_OK; break;
    case VC_SAVE:
        if (seq.playing || seq.req_stop) { result = VR_BUSY; break; }
        ui_external_edit(0);
        result = project_save();
        if (!result) { ui_external_saved(); result = VR_OK; }
        else result = result == -7 ? VR_FULL : result == -8 ? VR_PROTECTED : VR_IO;
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
        s->value[0] = VCAP_SOUND | VCAP_STRIP_CONFIG | VCAP_UNDO | VCAP_SAVE | VCAP_STRIP_AUDIO;
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
    } else return VR_UNSUPPORTED;
    return VR_OK;
}
