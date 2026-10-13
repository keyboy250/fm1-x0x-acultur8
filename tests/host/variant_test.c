/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise actual application/storage/undo paths against disposable simulated flash. */
#define _POSIX_C_SOURCE 200809L
/* Exercise the production scratch guard with a simulated outstanding DMA. */
static unsigned canvas_dma_timeouts, canvas_sync_calls, canvas_timeout_on_sync;
static void canvas_stage_sync(void);
#define CV_STAGE_DMA_TIMEOUTS canvas_dma_timeouts
#define CV_STAGE_SYNC canvas_stage_sync
#define main host_scenario_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static void canvas_stage_sync(void)
{
    canvas_sync_calls++;
    lcd_sync();
    if (canvas_timeout_on_sync) canvas_dma_timeouts++;
}

static variant_reply_t reply;
static int command(int op, int target, int index, int value)
{
    variant_command_t c = {VARIANT_ABI, (uint16_t)op, 123, (uint8_t)target, 0, (uint8_t)index, 0, value};
    int rc = variant_command(&c, &reply);
    assert(reply.request == 123 && reply.result == rc);
    return rc;
}
static void drain(void) { int32_t block[512]; engine_render(block, 256); }
static void fresh(void)
{
    int i;
    for (i = 0; i < OBJ_NOBJ; i++) store_len[i] = -1;
    memset(stored_known, 0, sizeof stored_known);
    project_discard_rejected();
    boot(); drain();
    ui.dirty = 0;
}
static void legacy(void)
{
    uint8_t image[OBJ_NOBJ][PLAT_STORE_MAX];
    int lens[OBJ_NOBJ], i;
    fresh();
    proj.pat[31].drum[0].hit[0][1] = 0x80000001u;
    assert(project_save() == 0);
    assert(get32(store[OBJ_PAT0]) == 2 && get32(store[OBJ_PAT0] + 4) == LEGACY_RAW_LEN);
    memcpy(image, store, sizeof image); memcpy(lens, store_len, sizeof lens);
    proj.pat[31].drum[0].hit[0][1] = 0;
    proj.variant.strip[0].boost_db = 10;
    assert(project_load() == 0);
    assert(proj.pat[31].drum[0].hit[0][1] == 0x80000001u && variant_is_default(&proj.variant));
    assert(project_save() == 0);
    for (i = 0; i < OBJ_NOBJ; i++) {
        assert(lens[i] == store_len[i]);
        if (lens[i] >= 0) assert(!memcmp(image[i], store[i], (size_t)lens[i]));
    }
    /* Independent format-1 object fixture: original 32-step pattern structures. */
    {
        pattern1_t old[8];
        memset(old, 0, sizeof old);
        old[0].drum[0].hit[0] = 0x80000001u;
        old[0].drum[0].len = 32;
        old[0].bass[0].len = 16;
        old[0].bass[0].step[2].note = 42;
        put32(store[OBJ_PAT0], 1); put32(store[OBJ_PAT1], 1);
        memcpy(store[OBJ_PAT0] + 4, old, sizeof old);
        memcpy(store[OBJ_PAT1] + 4, old, sizeof old);
        store_len[OBJ_PAT0] = store_len[OBJ_PAT1] = sizeof old + 4;
        store_len[OBJ_SONG] = store_len[OBJ_MOTION] = -1;
        assert(project_load() == 0);
        assert(proj.pat[0].drum[0].hit[0][0] == 0x80000001u);
        assert(proj.pat[0].drum[0].hit[0][1] == 0);
        assert(proj.pat[0].bass[0].step[2].note == 42 && variant_is_default(&proj.variant));
    }
}
static void roundtrip(void)
{
    fresh();
    assert(command(VC_SET_STRIP, 24, VP_HIGHPASS, 17500) == VR_QUEUED);
    assert(command(VC_SET_STRIP, 24, VP_FLAGS, 15) == VR_QUEUED);
    assert(command(VC_SET_STRIP, 1, VP_BOOST, 24) == VR_QUEUED);
    drain();
    assert(engine_strip_config(24)->highpass_hz == 17500);
    assert(command(VC_SAVE, 0, 0, 0) == VR_OK);
    assert(get32(store[OBJ_PAT0]) == 4 && get32(store[OBJ_PAT0] + 4) == RAW_LEN);
    project_defaults();
    assert(project_load() == 0);
    assert(proj.variant.strip[24].highpass_hz == 17500 && proj.variant.strip[24].flags == 15);
    assert(proj.variant.strip[1].boost_db == 24 && !project_write_protected());
}
static void rejected(int kind)
{
    project_t before;
    uint32_t writes;
    fresh();
    assert(command(VC_SET_STRIP, 0, VP_BOOST, 8) == VR_QUEUED);
    assert(project_save() == 0);
    if (kind == 0) store[OBJ_PAT0][8] ^= 1;              /* CRC */
    if (kind == 1) put32(store[OBJ_PAT0], 5);             /* future format */
    if (kind == 2) store_len[OBJ_PAT0] = 15;              /* truncated header */
    if (kind == 3) store_len[OBJ_PAT0]--;                 /* truncated compressed stream */
    if (kind == 4) put32(store[OBJ_PAT0] + 4, RAW_LEN - 1);/* malformed length */
    if (kind >= 5 && kind <= 7) {
        uint32_t packed = 0;
        if (kind == 5) proj.variant.schema = 3;
        if (kind == 6) proj.variant.bytes--;
        if (kind == 7) proj.variant.strip[0].boost_db = 25;
        assert(stream_measure(RAW_LEN, &packed) == 0);
        assert(save_stream(RAW_LEN, 4, packed) == 0);    /* valid CRC, invalid extension */
        variant_defaults(&proj.variant);
    }
    if (kind == 8) store_len[OBJ_PAT0] = -1;           /* missing first sector, remaining v3 sectors */
    /* Pending unsaved edit must remain undoable after a rejected candidate. */
    assert(command(VC_SET_STRIP, 0, VP_BOOST, 9) == VR_QUEUED);
    before = proj; writes = store_writes;
    assert(project_load() == -8);
    assert(!memcmp(&before, &proj, sizeof proj));
    assert(project_write_protected() && project_save() == -8 && store_writes == writes);
    project_defaults();                                /* boot fallback cannot unprotect flash */
    assert(project_write_protected() && project_save() == -8);
    proj = before;
    project_discard_rejected(); drain();
    assert(command(VC_UNDO, 0, 0, 0) == VR_OK);
    assert(proj.variant.strip[0].boost_db == 0);
    assert(command(VC_REDO, 0, 0, 0) == VR_OK);
    assert(proj.variant.strip[0].boost_db == 9);
    drain(); assert(engine_strip_config(0)->boost_db == 9);
}
static void capacity(void)
{
    uint8_t saved[OBJ_NOBJ][PLAT_STORE_MAX];
    int lens[OBJ_NOBJ]; uint32_t writes, x = 17, i;
    fresh(); assert(project_save() == 0);
    memcpy(saved, store, sizeof saved); memcpy(lens, store_len, sizeof lens); writes = store_writes;
    for (i = 0; i < LEGACY_RAW_LEN; i++) {
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        ((uint8_t *)&proj)[RAW_OFF + i] = (uint8_t)x;
    }
    proj.set.palette = 3; proj.sound.v[0][0][0] = 1;
    assert(project_save() == -7 && !seq.transport_hold && !seq.transport_stage);
    assert(writes == store_writes && !memcmp(saved, store, sizeof saved) && !memcmp(lens, store_len, sizeof lens));
}
static void rejection_keeps_full_history(void)
{
    undo_t journal;
    uint8_t shadow[sizeof undo_shadow];
    project_t before, pending_base;
    uint32_t i, changed = 0;
    fresh();
    assert(command(VC_BEGIN_EDIT, 0, 0, 0) == VR_OK);
    assert(command(VC_SET_STRIP, 0, VP_BOOST, 9) == VR_QUEUED);
    assert(command(VC_END_EDIT, 0, 0, 0) == VR_OK);
    assert(command(VC_SAVE, 0, 0, 0) == VR_OK);
    assert(command(VC_UNDO, 0, 0, 0) == VR_OK);
    drain();
    assert(undo.cur == 0 && undo.n == 1);             /* preserve an existing redo entry */
    pending_base = proj;
    assert(command(VC_BEGIN_EDIT, 0, 0, 0) == VR_OK);
    for (i = 0; i < LEGACY_RAW_LEN; i++) {
        ((uint8_t *)&proj)[RAW_OFF + i] ^= 0x5au;   /* >20 KiB pending diff would clear history */
        changed++;
    }
    assert(changed > UNDO_POOL);
    before = proj; journal = undo;
    memcpy(shadow, undo_shadow, sizeof shadow);
    store[OBJ_PAT0][8] ^= 1;
    assert(project_load() == -8);
    assert(!memcmp(&before, &proj, sizeof proj));
    assert(!memcmp(&journal, &undo, sizeof undo));
    assert(!memcmp(shadow, undo_shadow, sizeof shadow) && external_edit);
    for (i = 0; i < 4; i++) assert(blit_hash[i] == 0);
    assert(cv_w == 0 && cv_h == 0);
    assert(ui_project_stage(sizeof cv_px + 1u) == 0);
    /* Discard just the synthetic pending diff: the retained redo and undo still work. */
    proj = pending_base;
    project_discard_rejected();
    assert(command(VC_END_EDIT, 0, 0, 0) == VR_OK);
    assert(command(VC_REDO, 0, 0, 0) == VR_OK && proj.variant.strip[0].boost_db == 9);
    assert(command(VC_UNDO, 0, 0, 0) == VR_OK && proj.variant.strip[0].boost_db == 0);
    drain();
    /* A valid supported load intentionally discards the previous journal and gesture. */
    store[OBJ_PAT0][8] ^= 1;
    assert(command(VC_BEGIN_EDIT, 0, 0, 0) == VR_OK);
    assert(project_load() == 0);
    assert(undo.n == 0 && undo.cur == 0 && !external_edit);
    assert(proj.variant.strip[0].boost_db == 9);
    assert(!memcmp(undo_shadow, (uint8_t *)&proj + UNDO_OFF, sizeof undo_shadow));
    ui_frame();                                      /* redraw reconstructs canvas after borrowing */
    assert(cv_w == 240 && cv_h == KNOB_H && blit_hash[1] != 0);
}
static void canvas_borrow_guards(void)
{
    unsigned calls;
    fresh();
    calls = canvas_sync_calls;
    assert(ui_project_stage(sizeof cv_px + 1u) == 0 && canvas_sync_calls == calls);
    seq.playing = 1;
    assert(ui_project_stage(RAW_LEN) == 0 && canvas_sync_calls == calls);
    seq.playing = 0; seq.req_stop = 1;
    assert(ui_project_stage(RAW_LEN) == 0 && canvas_sync_calls == calls);
    seq.req_stop = 0;
    canvas_dma_timeouts = 1;
    assert(ui_project_stage(RAW_LEN) == 0 && canvas_sync_calls == calls);
    assert(!seq.transport_hold && !seq.transport_stage);
    canvas_dma_timeouts = 0; canvas_timeout_on_sync = 1;
    assert(ui_project_stage(RAW_LEN) == 0 && canvas_sync_calls == calls + 1u);
    assert(!seq.transport_hold && !seq.transport_stage);
    canvas_dma_timeouts = canvas_timeout_on_sync = 0;
    assert(ui_project_stage(RAW_LEN) == (uint8_t *)cv_px && canvas_sync_calls == calls + 2u);
    ui_project_stage_finish(0);
    assert(!seq.transport_hold && !seq.transport_stage);
    ui_frame();
    assert(cv_w == 240 && cv_h == KNOB_H);
}
static void controls(void)
{
    variant_snapshot_t snapshot;
    variant_command_t invalid = {VARIANT_ABI + 1, VC_SET_STRIP, 77, 0, 0, VP_BOOST, 0, 2};
    uint32_t rev, free, accepted = 0;
    int t, v, bound = VARIANT_STRIPS * VP_COUNT;
    fresh();
    for (t = 0; t < NTARGETS; t++) for (v = 0; v < engine_nvoices(t); v++) bound += engine_nparams(t,v);
    assert(bound + 1u <= 768);                         /* includes arrangement refresh after undo */
    assert(variant_snapshot(VS_INFO, 0, 0, &snapshot) == VR_OK && snapshot.count == 8);
    assert(snapshot.value[0] == (VCAP_SOUND | VCAP_STRIP_CONFIG | VCAP_UNDO | VCAP_SAVE | VCAP_STRIP_AUDIO | VCAP_ARRANGEMENT | VCAP_HARMONY_METADATA));
    rev = snapshot.revision;
    assert(variant_command(&invalid, &reply) == VR_UNSUPPORTED && reply.request == 77);
    assert(command(VC_SET_STRIP, 25, VP_BOOST, 0) == VR_INVALID);
    assert(command(VC_SET_STRIP, 0, VP_BOOST, 25) == VR_INVALID);
    assert(reply.revision == rev && !ui_dirty());
    assert(command(VC_BEGIN_EDIT, 0, 0, 0) == VR_OK);
    assert(command(VC_SET_STRIP, 0, VP_BOOST, 3) == VR_QUEUED);
    {
        uint32_t writes = store_writes;
        run_ms(5000);                                  /* explicit gesture outlives idle undo/autosave */
        assert(external_edit && undo.n == 0 && store_writes == writes);
        assert(command(VC_BEGIN_EDIT, 0, 0, 0) == VR_OK); /* repeated begin is idempotent */
    }
    assert(command(VC_SET_STRIP, 0, VP_BOOST, 8) == VR_QUEUED);
    assert(command(VC_END_EDIT, 0, 0, 0) == VR_OK);
    assert(command(VC_UNDO, 0, 0, 0) == VR_OK && proj.variant.strip[0].boost_db == 0);
    assert(command(VC_REDO, 0, 0, 0) == VR_OK && proj.variant.strip[0].boost_db == 8);
    drain(); assert(engine_strip_config(0)->boost_db == 8);
    free = engine_queue_space(); assert(free == 1024);
    while (accepted < free) {
        assert(command(VC_SET_STRIP, 0, VP_BOOST, accepted % 25) == VR_QUEUED); accepted++;
    }
    rev = reply.revision;
    assert(command(VC_SET_STRIP, 0, VP_BOOST, 24) == VR_BUSY && reply.revision == rev);
    assert(proj.variant.strip[0].boost_db == (free - 1) % 25);
    assert(command(VC_SET_SOUND, 0, 0, 1) == VR_BUSY && reply.revision == rev);
    assert(command(VC_UNDO, 0, 0, 0) == VR_BUSY && reply.revision == rev);
    drain(); assert(engine_queue_read() == engine_queue_written());
    assert(engine_strip_config(0)->boost_db == (free - 1) % 25);
    seq.playing = 1;
    assert(command(VC_SAVE, 0, 0, 0) == VR_BUSY && project_load() == -9);
    seq.playing = 0;
    drain(); assert(command(VC_SAVE, 0, 0, 0) == VR_OK);
    assert(command(VC_BEGIN_EDIT, 0, 0, 0) == VR_OK);
    assert(project_load() == 0 && !external_edit);
    assert(command(VC_BEGIN_EDIT, 0, 0, 0) == VR_OK);
    run_action(ACT_RESET, 0);
    assert(!external_edit && variant_is_default(&proj.variant));
}
int main(void)
{
    int i;
    legacy(); roundtrip(); for (i = 0; i < 9; i++) rejected(i); capacity(); rejection_keeps_full_history(); canvas_borrow_guards(); controls();
    puts("variant: legacy v1/v2, v4 roundtrip, 9 rejection cases, exact rejected-load journal, canvas DMA guards/redraw, capacity preflight, undo and queue pressure PASS (v3 migration covered by arrangement_test)");
    return 0;
}
