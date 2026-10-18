/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X project: the settings, the sound, the 32 patterns, the song and the motion lanes.
 *
 * The settings and the sound are one storage object each, as they are. The patterns, the song and
 * the lanes (~30 KB, mostly empty steps) are one stream packed with lz.c across the four objects
 * OBJ_PAT0, OBJ_PAT1, OBJ_SONG, OBJ_MOTION (PROJ_STREAM). The first carries the raw length and a
 * CRC of the raw bytes, so a stream torn by a power cut between two objects, or one that does not
 * unpack, is never loaded as garbage: the objects' previous copies (storage.c keeps two) are tried
 * instead, which give the save before; only when no mix of copies checks out, the defaults. A save packs once to measure (nothing is
 * written if it would not fit: "MEMORY FULL") and once to write; an object whose bytes did not
 * change is not written again.
 *
 * Every object carries its format number. A project saved before the stream (format 1: 16
 * patterns of 32 steps in OBJ_PAT0/1, the song and 32-step lanes in OBJ_SONG/OBJ_MOTION) is
 * converted as it loads: its steps land on steps 1-32, its patterns on 1-16. */
#include "x0x.h"
#include "../dsp/drum909.h"            /* DR_SD (the sound's revisions) */
#include "../dsp/master.h"             /* MST_* (the sound's revisions) */
#include "lz.c"

project_t proj;

#define PROJ_STREAM 2u                  /* the format word of the packed objects */
#define PROJ_VARIANT_STREAM 3u
#define PROJ_ARRANGEMENT_STREAM 4u
#define PROJ_LANE_STREAM 5u
#define PROJ_RATE_STREAM 6u
#define LANE_FORMAT(f) ((f)==PROJ_LANE_STREAM || (f)==PROJ_RATE_STREAM)
#define LANE_RAW_OFF __builtin_offsetof(project_t,sound)
#define LANE_RAW_LEN (sizeof(project_t)-LANE_RAW_OFF)
static const uint8_t STREAM_OBJ[] = {OBJ_PAT0, OBJ_PAT1, OBJ_SONG, OBJ_MOTION};
#define NSTREAM ((int)sizeof STREAM_OBJ)
#define STREAM_HDR 16u                  /* in the first object: format, raw length, raw CRC, packed length */
#define STREAM_CAP (NSTREAM * (PLAT_STORE_MAX - 4u) - (STREAM_HDR - 4u))
#define RAW_OFF __builtin_offsetof(project_t, pat)
#define RAW_LEN (sizeof(project_t) - RAW_OFF)   /* the patterns and the arrangement, contiguous */
#define LEGACY_RAW_LEN (__builtin_offsetof(project_t, variant) - RAW_OFF)
#define SCHEMA1_RAW_LEN (LEGACY_RAW_LEN + 208u)
typedef char raw_fits[(RAW_LEN < 0xFFFFu) ? 1 : -1];
typedef char raw_contiguous[(__builtin_offsetof(project_t, arr) == RAW_OFF + sizeof proj.pat) ? 1 : -1];
typedef char legacy_prefix_fixed[(__builtin_offsetof(project_t, variant) == 30944u && RAW_OFF == 1372u) ? 1 : -1];

static void arrange_defaults(void);
static uint32_t packed_len;             /* the last save's (or load's) packed stream, bytes */
static uint8_t rejected_save;
static int arrangement_song_valid(const song_t *song)
{
    int b, p;
    if (song->len > NSONG) return 0;
    for (b = 0; b < NSONG; b++) for (p = 0; p < NTRACKS; p++)
        if (song->bar[b].pat[p] >= NPAT) return 0;
    return 1;
}
int project_write_protected(void) { return rejected_save != 0; }
void project_discard_rejected(void) { rejected_save = 0; }

void project_defaults_into(project_t *destination)
{
    int i;
    destination->set.magic = PROJ_MAGIC;
    destination->set.format = PROJ_FORMAT;
    destination->set.palette = 1;                /* amber */
    destination->set.keyled = 1;
    destination->set.keysound = 0;
    destination->set.autosave_off = 0;
    destination->set.clk_out = 1;
    destination->set.notes_out = 0;
    destination->set.bpm_x10 = 1250;
    destination->set.accent_q7 = 88;
    destination->set.stereo = PROJ_STEREO;
    destination->set.snd_rev = PROJ_SND_REV;
    engine_sound_defaults(&destination->sound);
    variant_defaults(&destination->variant);
    memset(&destination->arr,0,sizeof destination->arr);
    /* the 303 lines start empty, like the drums: OCT+ on TB-3PO (or keyboard mode) writes one */
    for (i = 0; i < NPAT; i++)
        pattern_init(&destination->pat[i], 0x3B0u + (uint32_t)i * 977u, 0x5A1u + (uint32_t)i * 613u);
}

void project_defaults(void)
{
    cc_actions_reset();
    /* A reserved project swap retains explicit Start/Continue received during
     * its hold. Unreserved defaults retire the old session and run intent. */
    if(!seq.transport_hold)seq_transport_retire(&seq);
    engine_notes_clear();
    project_defaults_into(&proj);
}

/* how full the stream's room is, in % (GLO > ABOUT) */
int project_mem_pct(void) { return (int)((packed_len * 100u + STREAM_CAP - 1u) / STREAM_CAP); }

static uint32_t checkpoint_hash;
static uint8_t checkpoint_verified;
uint32_t project_fingerprint(void)
{
    const uint8_t *p=(const uint8_t *)&proj;uint32_t h=2166136261u,i;
    for(i=0;i<sizeof proj;i++)h=(h^p[i])*16777619u;
    return h;
}
int project_checkpoint_matches(void){return checkpoint_verified && checkpoint_hash==project_fingerprint();}

/* object payload: format word + body */
static uint8_t io_buf[PLAT_STORE_MAX] __attribute__((aligned(4)));

/* what each object last held on the flash (a hash of its payload), so a save writes only the
 * objects that changed: an autosave usually erases one or two sectors, not six */
static uint32_t stored_hash[OBJ_MOTION+1];
static uint8_t stored_known[OBJ_MOTION+1];
static POLY_COLD uint32_t hash_of(const uint8_t *p, uint32_t n)        /* FNV-1a */
{
    uint32_t h = 2166136261u;
    while (n--)
        h = (h ^ *p++) * 16777619u;
    return h;
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static POLY_COLD uint32_t crc32_of(const uint8_t *p, uint32_t n)       /* zlib CRC-32, 4 bits per step */
{
    static const uint32_t T[16] = {
        0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu, 0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
        0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu, 0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu};
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        c = (c >> 4) ^ T[c & 15u];
        c = (c >> 4) ^ T[c & 15u];
    }
    return ~c;
}

/* the object is the n bytes of io_buf, format word included; written only if they changed */
static POLY_COLD int save_buf(uint32_t obj, uint32_t n)
{
    uint32_t h = hash_of(io_buf, n);
    int rc;
    if (stored_known[obj] && stored_hash[obj] == h)
        return 0;                                    /* the flash already holds exactly this */
    rc = plat_store_save(obj, io_buf, n);
    stored_known[obj] = rc == 0;
    stored_hash[obj] = h;
    return rc;
}

static POLY_COLD int load_obj(uint32_t obj, void *dst, uint32_t len)
{
    int n = plat_store_load(obj, io_buf, sizeof io_buf);
    uint32_t i;
    if (n < 0 || (uint32_t)n != len + 4u || get32(io_buf) != PROJ_FORMAT)
        return -1;
    for (i = 0; i < len; i++)
        ((uint8_t *)dst)[i] = io_buf[4u + i];
    stored_known[obj] = 1;
    stored_hash[obj] = hash_of(io_buf, len + 4u);
    return 0;
}

/* the settings may have grown since they were saved: a shorter payload of this format loads into
 * the front of dst, and the rest keeps what dst held (zeros: the new settings' defaults) */
static POLY_COLD int load_settings(settings_t *dst)
{
    int n = plat_store_load(OBJ_SET, io_buf, sizeof io_buf);
    uint32_t i;
    if (n < 20 || (uint32_t)n > sizeof *dst + 4u || io_buf[0] != (uint8_t)PROJ_FORMAT || io_buf[1] || io_buf[2] || io_buf[3])
        return -1;
    for (i = 0; i + 4u < (uint32_t)n; i++)
        ((uint8_t *)dst)[i] = io_buf[4u + i];
    if ((uint32_t)n == sizeof *dst + 4u) {           /* exactly what a save of these settings writes */
        stored_known[OBJ_SET] = 1;
        stored_hash[OBJ_SET] = hash_of(io_buf, (uint32_t)n);
    }
    return 0;
}

static POLY_COLD int save_obj(uint32_t obj, const void *src, uint32_t len)
{
    uint32_t i;
    if (len + 4u > sizeof io_buf)
        return -1;
    put32(io_buf, PROJ_FORMAT);
    for (i = 0; i < len; i++)
        io_buf[4u + i] = ((const uint8_t *)src)[i];
    return save_buf(obj, len + 4u);
}

typedef char sound_fits[(sizeof(sound_t) + 4u <= PLAT_STORE_MAX) ? 1 : -1];

static void arrange_defaults(void)
{
    uint32_t i;
    uint8_t *z = (uint8_t *)&proj.arr;
    for (i = 0; i < sizeof proj.arr; i++)
        z[i] = 0;
}

/* ---- the packed stream ---------------------------------------------------------------------- */

/* the writer: fills io_buf after the object's header and writes it when full (measure: counts only) */
typedef struct {
    int measure, k, err;
    uint32_t fill, total, format;
} st_out_t;

static int st_flush(st_out_t *o)
{
    int rc = save_buf(STREAM_OBJ[o->k], o->fill);
    o->k++;
    o->fill = 4u;
    put32(io_buf, o->format);
    return rc;
}

static POLY_COLD int st_put(void *ctx, const uint8_t *p, uint32_t n)
{
    st_out_t *o = ctx;
    o->total += n;
    if (o->measure)
        return o->total > STREAM_CAP ? -1 : 0;
    while (n) {
        if (o->fill == PLAT_STORE_MAX) {
            if (o->k + 1 >= NSTREAM || st_flush(o)) {
                o->err = 1;
                return -1;
            }
        }
        io_buf[o->fill++] = *p++;
        n--;
    }
    return 0;
}

static POLY_COLD int stream_measure(uint32_t raw_len, uint32_t raw_off,uint32_t *packed)
{
    st_out_t o = {1, 0, 0, 0, 0, 0};
    lz_sink_t sink = {st_put, &o};
    if (lz_compress((const uint8_t *)&proj + raw_off, raw_len, &sink)) return -7;
    *packed = o.total;
    return 0;
}

static POLY_COLD int save_stream(uint32_t raw_len, uint32_t raw_off,uint32_t format, uint32_t measured)
{
    const uint8_t *raw = (const uint8_t *)&proj + raw_off;
    uint32_t crc = crc32_of(raw, raw_len);
    st_out_t o = {0, 0, 0, STREAM_HDR, 0, format};
    lz_sink_t sink = {st_put, &o};
    packed_len = measured;                            /* preflight already ran before any object write */
    put32(io_buf, format);
    put32(io_buf + 4, raw_len);
    put32(io_buf + 8, crc);
    put32(io_buf + 12, packed_len);
    if (lz_compress(raw, raw_len, &sink) || o.err || st_flush(&o))
        return -3;
    while (o.k < NSTREAM)                             /* the objects past the stream: empty */
        if (st_flush(&o))
            return -3;
    return 0;
}

/* the reader: the objects in turn, from io_buf */
typedef struct {
    int k, n;
    uint32_t pos, left, format, prev;
} st_in_t;

static POLY_COLD int stream_obj_load(int k, uint32_t prev)      /* object k of the stream into io_buf, its copy per prev */
{
    return (prev >> k) & 1u ? plat_store_load_prev(STREAM_OBJ[k], io_buf, sizeof io_buf)
                            : plat_store_load(STREAM_OBJ[k], io_buf, sizeof io_buf);
}

static POLY_COLD int st_get(void *ctx)
{
    st_in_t *s = ctx;
    if (!s->left)
        return -1;
    while (s->pos >= (uint32_t)s->n) {
        if (++s->k >= NSTREAM)
            return -1;
        s->n = stream_obj_load(s->k, s->prev);
        if (s->n < 4 || get32(io_buf) != s->format)
            return -1;
        stored_known[STREAM_OBJ[s->k]] = !((s->prev >> s->k) & 1u);   /* a previous copy: written again next save */
        stored_hash[STREAM_OBJ[s->k]] = hash_of(io_buf, (uint32_t)s->n);
        s->pos = 4u;
    }
    s->left--;
    return io_buf[s->pos++];
}

/* Decode into borrowed staging memory: live project bytes are untouched until fully validated. */
static POLY_COLD int load_stream(int n, uint32_t format, uint8_t *raw, uint32_t raw_len, uint32_t prev)
{
    uint32_t crc = get32(io_buf + 8), plen = get32(io_buf + 12);
    st_in_t s = {0, n, STREAM_HDR, plen, format, prev};
    lz_src_t src = {st_get, &s};
    if (n < (int)STREAM_HDR || get32(io_buf + 4) != raw_len || !plen || plen > STREAM_CAP)
        return -1;
    stored_known[STREAM_OBJ[0]] = !(prev & 1u);
    stored_hash[STREAM_OBJ[0]] = hash_of(io_buf, (uint32_t)n);
    if (lz_decompress(&src, raw, raw_len) || s.left || crc32_of(raw, raw_len) != crc)
        return -1;
    packed_len = plen;
    return 0;
}

/* Recover only streams of the recognized format into scratch; live bytes stay untouched.
 * For lane formats the sound object must match the CRC-protected sound in the stream. */
static POLY_COLD int load_stream_recover(uint32_t format, uint8_t *raw, uint32_t raw_len)
{
    uint32_t prev, count;
    int n;
    for (count=0; count<=NSTREAM; count++) for(prev=0;prev<(1u<<NSTREAM);prev++) {
        if((uint32_t)__builtin_popcount(prev)!=count)continue;
        n=stream_obj_load(0,prev);
        if(n<4 || get32(io_buf)!=format || load_stream(n,format,raw,raw_len,prev))continue;
        if(LANE_FORMAT(format)) {
            uint32_t copy,k; int matched=0;
            for(copy=0;copy<2 && !matched;copy++) {
                n=copy ? plat_store_load_prev(OBJ_SOUND,io_buf,sizeof io_buf) : plat_store_load(OBJ_SOUND,io_buf,sizeof io_buf);
                if(n!=(int)sizeof(sound_t)+4 || get32(io_buf)!=PROJ_FORMAT)continue;
                for(k=0;k<sizeof(sound_t) && io_buf[k+4]==raw[k];k++);
                if(k==sizeof(sound_t)){matched=1;if(copy)project_flash_changed();}
            }
            if(!matched)continue;
        }
        if(prev)project_flash_changed();
        return 0;
    }
    return -1;
}

/* ---- format 1: 16 patterns of 32 steps, 32-step lanes ---------------------------------------- */

#define NSTEPS1 32
#define NPAT1 16
typedef struct {
    uint32_t hit[NDRUM];
    uint32_t accent;
    uint8_t len, rate, rsv[2];
} dpart1_t;
typedef struct {
    bstep_t step[NSTEPS1];
    uint8_t len, rate, dir, transpose;
    tb3po_cfg_t gen;
} bpart1_t;
typedef struct {
    uint32_t steps;
    uint8_t set[BRK_NSET];
    uint8_t slot_a, slot_b, rsv[2];
} brkpart1_t;
typedef struct {
    dpart1_t drum[NKIT];
    bpart1_t bass[NBASS];
    brkpart1_t brk;
    uint8_t swing, rsv[3];
} pattern1_t;
typedef struct {
    uint8_t used, gen, pat, part, t, v, i, rsv;
    uint8_t val[NSTEPS1];
} lane1_t;
#define ARR1_LEN (sizeof(song_t) + NLANE * sizeof(lane1_t))
#define ARR1_A (PLAT_STORE_MAX - 4u)                 /* format 1's arrangement bytes in OBJ_SONG; the rest in OBJ_MOTION */
typedef char arr1_room[(ARR1_LEN <= sizeof(arrange_t)) ? 1 : -1];   /* it unpacks in proj.arr */

static POLY_COLD void pattern_from1(pattern_t *p, const pattern1_t *o)
{
    int k, s, v;
    for (k = 0; k < NKIT; k++) {
        for (v = 0; v < NDRUM; v++)
            p->drum[k].hit[v][0] = o->drum[k].hit[v];
        p->drum[k].accent[0] = o->drum[k].accent;
        p->drum[k].len = o->drum[k].len;
        p->drum[k].rate = o->drum[k].rate;
    }
    for (k = 0; k < NBASS; k++) {
        for (s = 0; s < NSTEPS1; s++)
            p->bass[k].step[s] = o->bass[k].step[s];
        p->bass[k].len = o->bass[k].len;
        p->bass[k].rate = o->bass[k].rate;
        p->bass[k].dir = o->bass[k].dir;
        p->bass[k].transpose = o->bass[k].transpose;
        p->bass[k].gen = o->bass[k].gen;
    }
    p->brk.steps = o->brk.steps;
    for (k = 0; k < BRK_NSET; k++)
        p->brk.set[k] = o->brk.set[k];
    p->brk.slot_a = o->brk.slot_a;
    p->brk.slot_b = o->brk.slot_b;
    p->swing = o->swing;
}

/* OBJ_PAT0 + OBJ_PAT1, format 1 (eight patterns each) onto patterns 1-16 (the defaults are in place) */
static POLY_COLD int load_patterns1(void)
{
    int h, i;
    for (h = 0; h < 2; h++) {
        int n = plat_store_load(h ? OBJ_PAT1 : OBJ_PAT0, io_buf, sizeof io_buf);
        if (n != (int)(8u * sizeof(pattern1_t) + 4u) || get32(io_buf) != PROJ_FORMAT)
            return -1;
        for (i = 0; i < 8; i++) {
            pattern1_t o;
            uint32_t b;
            for (b = 0; b < sizeof o; b++)
                ((uint8_t *)&o)[b] = io_buf[4u + (uint32_t)i * sizeof o + b];
            pattern_from1(&proj.pat[h * 8 + i], &o);
        }
    }
    return 0;
}

/* OBJ_SONG + OBJ_MOTION, format 1: the song as it is, the lanes widened to 64 steps (in place, last
 * first: lane k's new place never reaches below its old one) */
static POLY_COLD int load_arrange1(void)
{
    uint8_t *a = (uint8_t *)&proj.arr;
    int k, i;
    if (load_obj(OBJ_SONG, a, ARR1_A) || load_obj(OBJ_MOTION, a + ARR1_A, ARR1_LEN - ARR1_A))
        return -1;
    for (k = NLANE - 1; k >= 0; k--) {
        lane1_t o;
        lane_t *l = &proj.arr.lane[k];
        uint32_t b;
        for (b = 0; b < sizeof o; b++)
            ((uint8_t *)&o)[b] = a[sizeof(song_t) + (uint32_t)k * sizeof o + b];
        l->used = o.used;
        l->gen = o.gen;
        l->pat = o.pat;
        l->part = o.part;
        l->t = o.t;
        l->v = o.v;
        l->i = o.i;
        l->rsv = 0;
        for (i = 0; i < NSTEPS; i++)
            l->val[i] = i < NSTEPS1 ? o.val[i] : MOT_NONE;
    }
    stored_known[OBJ_SONG] = stored_known[OBJ_MOTION] = 0;   /* the next save writes the stream */
    return 0;
}

/* ---- load, save ----------------------------------------------------------------------------- */

/* the 909 SD gained DECAY at pot 3: what was 3.. (LEVEL, DRIVE, DIST, REV, DLY, PAN) moves up one,
 * the knobs' motion with it, and DECAY takes its default (9W9's 340 ms) */
static POLY_COLD void sound_rev1(void)
{
    uint8_t *sd = proj.sound.v[T_909][DR_SD];
    int i, n = engine_nparams(T_909, DR_SD);
    for (i = n - 1; i > 3; i--)
        sd[i] = sd[i - 1];
    sd[3] = engine_param(T_909, DR_SD, 3)->def;
    for (i = 0; i < NLANE; i++) {
        lane_t *l = &proj.arr.lane[i];
        if (l->used && l->t == T_909 && l->v == DR_SD && l->i >= 3)
            l->i++;
    }
}

/* the master's one-knob COMP and FILTER and the delay's TAPE arrived: each set to its nearest to what
 * the details say (the details themselves stay, and are applied after: the sound is as it was) */
static POLY_COLD void sound_rev2(void)
{
    uint8_t *m = proj.sound.v[T_MST][0];
    int th = m[MST_THRESH], cut = m[MST_CUTOFF];
    float thr = (float)th * (48.0f / 127.0f) - 48.0f, x = (-thr - 4.0f) / 28.0f;
    m[MST_COMP1] = (uint8_t)(!m[MST_RATIO] ? 0 : x <= 0.0f ? 1 : x >= 1.0f ? 127 : (int)(x * 127.0f + 0.5f));
    m[MST_DJF] = (uint8_t)(m[MST_MODE] == 0 ? 64 : m[MST_MODE] == 3 ? 69 + cut * 58 / 120
                           : (cut < 8 ? 0 : (cut - 8) * 59 / 119));
    proj.sound.v[T_FX][1][0] = (uint8_t)(proj.sound.v[T_FX][0][FX_DL_TYPE] ? 16 + proj.sound.v[T_FX][0][FX_DL_WEAR] * 111 / 127 : 0);
}

/* a BD gained REV and DLY before its PAN (909: 8 -> 10, 808: 7 -> 9): the pan and its motion move, the sends start
 * at none (as the kick had) */
static void sound_bd_sends(int t, int pan)            /* (rev 3: the 909's at 8; rev 4: the 808's at 7) */
{
    uint8_t *bd = proj.sound.v[t][0];
    int i;
    bd[pan + 2] = bd[pan];
    bd[pan] = bd[pan + 1] = 0;
    for (i = 0; i < NLANE; i++) {
        lane_t *l = &proj.arr.lane[i];
        if (l->used && l->t == t && l->v == 0 && l->i == pan)
            l->i = (uint8_t)(pan + 2);
    }
}

static void patterns_defaults(void)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&proj.pat[i], 0x3B0u + (uint32_t)i * 977u, 0x5A1u + (uint32_t)i * 613u);
}

void project_flash_changed(void)
{
    memset(stored_known,0,sizeof stored_known);
    checkpoint_verified=0;
}

/* Packed streams are validated in synchronized canvas scratch before changing the live project. */
static POLY_COLD int project_load_impl(void)
{
    int bad = 0, i, sound_ok, n;
    uint32_t format = 0, raw_len = 0, loaded_packed = 0,raw_off=RAW_OFF;
    uint8_t *stage = 0;
    settings_t set;
    if (!engine_queue_space()) return -9;
    n = plat_store_load(OBJ_PAT0, io_buf, sizeof io_buf);
    if(n<0)n=plat_store_load_prev(OBJ_PAT0,io_buf,sizeof io_buf);
    if (n >= 4) format = get32(io_buf);
    if (n < 0) {
        /* A lost first sector must not turn a remaining packed save into fresh defaults. */
        for (i = 1; i < NSTREAM; i++) {
            int tail = plat_store_load(STREAM_OBJ[i], io_buf, sizeof io_buf);
            if (tail >= 4 && get32(io_buf) != PROJ_FORMAT) {
                rejected_save = 1;
                return -8;
            }
        }
    }
    if (n >= 0 && (n < 4 || (format != PROJ_FORMAT && format != PROJ_STREAM &&
        format != PROJ_VARIANT_STREAM && format != PROJ_ARRANGEMENT_STREAM && !LANE_FORMAT(format)))) {
        rejected_save = 1;
        return -8;
    }
    if (format == PROJ_STREAM || format == PROJ_VARIANT_STREAM || format == PROJ_ARRANGEMENT_STREAM || LANE_FORMAT(format)) {
        raw_len = format == PROJ_STREAM ? LEGACY_RAW_LEN : format == PROJ_VARIANT_STREAM ? SCHEMA1_RAW_LEN : LANE_FORMAT(format)?LANE_RAW_LEN:RAW_LEN;
        if(LANE_FORMAT(format))raw_off=LANE_RAW_OFF;
        stage = ui_project_stage(raw_len);
        if (!stage) return -9;
        if (load_stream_recover(format, stage, raw_len) ||
            (format == PROJ_VARIANT_STREAM && !variant_schema1_valid((const variant_settings_t *)(stage + LEGACY_RAW_LEN))) ||
            (format == PROJ_ARRANGEMENT_STREAM &&
             ((!variant_saved_valid((const variant_settings_t *)(stage + LEGACY_RAW_LEN)) || ((const variant_settings_t *)(stage+LEGACY_RAW_LEN))->arrangement.flags&POLY_FLAG) ||
              !arrangement_song_valid((const song_t *)(stage + __builtin_offsetof(project_t,arr) - RAW_OFF))))) {
            ui_project_stage_finish(0);
            rejected_save = 1;
            return -8;
        }
        if(LANE_FORMAT(format)){
            const variant_settings_t *v=(const variant_settings_t *)(stage+__builtin_offsetof(project_t,variant)-LANE_RAW_OFF);

            if(!variant_valid(v) || !(v->arrangement.flags&POLY_FLAG) ||
                !poly_valid((const pattern_t *)(stage+sizeof(sound_t)),stage) ||
                poly_expanded(stage)!=(format==PROJ_RATE_STREAM) ||
                !arrangement_song_valid((const song_t *)(stage+__builtin_offsetof(project_t,arr)-LANE_RAW_OFF))){ui_project_stage_finish(0);rejected_save=1;return -8;}
        }
        loaded_packed = packed_len;
    }
    project_defaults();
    packed_len = loaded_packed;
    set = proj.set;
    if (load_settings(&set) == 0 && set.magic == PROJ_MAGIC && set.notes_out<=1)
        proj.set = set;
    else
        bad = 1;
    if(proj.set.notes_out)proj.variant.arrangement.flags|=ARR_NOTES_OUT;
    sound_ok = load_obj(OBJ_SOUND, &proj.sound, sizeof proj.sound) == 0;
    if (!sound_ok)
        bad = 1;
    if (sound_ok && proj.set.snd_rev < 3)             /* before the 909 BD's sends: its pan moves up two */
        sound_bd_sends(T_909, 8);                     /* (first: the pans' own fixes below see the new place) */
    if (sound_ok && proj.set.snd_rev < 4)             /* the 808 BD's, the same */
        sound_bd_sends(T_808, 7);
    if (proj.set.stereo < 1)                          /* saved mono: no pans there (zeros = hard left) */
        for (i = 0; i < NPARTS; i++)
            proj.sound.v[T_MIX][i][MX_PAN] = 64;
    if (proj.set.stereo < 2)                          /* saved before the drum voices had pans */
        engine_sound_centre_drum_pans(&proj.sound);
    proj.set.stereo = PROJ_STEREO;
    if (stage) {
        uint32_t b;
        for (b = 0; b < raw_len; b++) ((uint8_t *)&proj)[raw_off + b] = stage[b];
        if (format == PROJ_VARIANT_STREAM || format == PROJ_ARRANGEMENT_STREAM || LANE_FORMAT(format)) {
            if(proj.variant.schema<4){proj.variant.arrangement.pad_channel=255;
                proj.variant.arrangement.flags=(uint8_t)((proj.variant.arrangement.flags&~ARR_NOTES_OUT)|(proj.set.notes_out?ARR_NOTES_OUT:0));}
            proj.variant.schema = VARIANT_SCHEMA; proj.variant.bytes = sizeof proj.variant;
        }
    } else {
        if (load_patterns1()) {                       /* half a set is no set: the defaults */
            patterns_defaults();
            bad = 1;
        }
        /* the song and the lanes: absent in a project saved before they existed, which is no fault */
        if (load_arrange1())
            arrange_defaults();
        stored_known[OBJ_PAT0] = stored_known[OBJ_PAT1] = 0;
    }
    if (proj.arr.song.len > NSONG)
        proj.arr.song.len = NSONG;
    if (sound_ok && proj.set.snd_rev < 1)             /* saved before the 909 SD's DECAY: its pots move up one */
        sound_rev1();
    if (sound_ok && proj.set.snd_rev < 2)             /* before the one-knob COMP, FILTER, TAPE */
        sound_rev2();
    proj.set.snd_rev = PROJ_SND_REV;
    rejected_save = 0;
    engine_arrangement_refresh(&proj.variant.arrangement);
    ui_project_stage_finish(1);
    return bad ? -1 : 0;
}

int project_load(void)
{
    int rc;
    if (!engine_transport_reserve()) return -9;
    cc_actions_reset();
    rc = project_load_impl();
    checkpoint_verified=(uint8_t)(rc==0);if(!rc){checkpoint_hash=project_fingerprint();cc_actions_reset();}
    engine_transport_release();
    return rc;
}

void project_apply_io(void)
{
    static const uint8_t DEF[5] = {9, 10, 1, 2, 3};   /* 909 10, 808 11, 303s 2 / 3, break 4 */
    int p;
    seq.send_clock = proj.set.clk_out;
    proj.set.notes_out=(uint8_t)((proj.variant.arrangement.flags&ARR_NOTES_OUT)!=0);
    seq.send_notes = proj.set.notes_out;
    for (p = 0; p < 5; p++) {
        int v = proj.set.midi_ch[p];
        seq.ch[p] = (uint8_t)(!v ? DEF[p] : v > 16 ? SEQ_CH_OFF : v - 1);
    }
    engine_out_loud(proj.set.out_loud);
    engine_usb_full(proj.set.usb_full);
}

/* 0 = saved and verified; -7 = the patterns, song and motion do not fit (nothing was written) */
static POLY_COLD int project_save_impl(void)
{
    int p;
    uint32_t raw_len, format, measured,raw_off=RAW_OFF;
    if (rejected_save || !variant_valid(&proj.variant)) return -8;
    format = variant_is_default(&proj.variant) ? PROJ_STREAM : PROJ_ARRANGEMENT_STREAM;
    raw_len = format == PROJ_STREAM ? LEGACY_RAW_LEN : RAW_LEN;
    if(proj.variant.arrangement.flags&POLY_FLAG){if(!poly_valid(proj.pat,(const uint8_t *)&proj.sound))return -8;format=poly_expanded((const uint8_t *)&proj.sound)?PROJ_RATE_STREAM:PROJ_LANE_STREAM;raw_off=LANE_RAW_OFF;raw_len=LANE_RAW_LEN;}
    /* Preflight before settings/sound writes, so a full project cannot partly replace a save. */
    if (stream_measure(raw_len,raw_off, &measured)) return -7;
    if ((format == PROJ_ARRANGEMENT_STREAM || LANE_FORMAT(format)) && !arrangement_song_valid(&proj.arr.song)) return -8;
    for (p = 0; p < 5; p++)
        proj.set.midi_ch[p] = (uint8_t)(seq.ch[p] == SEQ_CH_OFF ? 17 : seq.ch[p] + 1);
    proj.set.bpm_x10 = (uint16_t)(seq.bpm * 10.0f + 0.5f);
    proj.set.accent_q7 = seq.accent_q7;
    proj.set.clk_out = seq.send_clock;
    proj.set.notes_out=(uint8_t)((proj.variant.arrangement.flags&ARR_NOTES_OUT)!=0);
    if (save_obj(OBJ_SET, &proj.set, sizeof proj.set))
        return -1;
    if (save_obj(OBJ_SOUND, &proj.sound, sizeof proj.sound))
        return -2;
    return save_stream(raw_len,raw_off, format, measured);
}

int project_save(void)
{
    int rc;
    if (ui_saves_held() || !engine_transport_reserve()) return -9;
    rc = project_save_impl();
    checkpoint_verified=(uint8_t)(rc==0);if(!rc){checkpoint_hash=project_fingerprint();cc_actions_reset();}
    engine_transport_release();
    return rc;
}

/* Reusable per-sound defaults are independent of projects/undo. No resident bank:
 * stopped main-thread operations reuse the existing project I/O buffer. */
#define STRIP_DEFAULT_MAGIC 0x31464453u /* SDF1 */
typedef struct { uint32_t magic; uint16_t schema,bytes; uint32_t mask;
    sound_strip_config_t strip[VARIANT_STRIPS]; } strip_default_bank_t;
typedef char strip_default_bank_fits[(sizeof(strip_default_bank_t)+1u <= sizeof io_buf) ? 1 : -1];
static POLY_COLD int strip_default_bank_load(void)
{
    strip_default_bank_t *bank=(strip_default_bank_t *)io_buf;
    /* Storage returns a truncated length. Read one sentinel byte beyond the
     * schema so a larger/future object cannot masquerade as a valid prefix. */
    int n=plat_store_load(OBJ_STRIP_DEFAULTS,io_buf,sizeof *bank+1u);unsigned i;
    if(n==-1)return VR_MISSING;
    if(n!=(int)sizeof *bank || bank->magic!=STRIP_DEFAULT_MAGIC || bank->schema!=1 ||
       bank->bytes!=sizeof *bank || bank->mask&~0x01ffffffu)return VR_UNSUPPORTED;
    for(i=0;i<VARIANT_STRIPS;i++)if((bank->mask&(1u<<i))&&!sound_strip_config_valid(&bank->strip[i]))return VR_UNSUPPORTED;
    return VR_OK;
}
POLY_COLD int project_strip_default_save(unsigned strip)
{
    strip_default_bank_t *bank=(strip_default_bank_t *)io_buf;
    int rc; sound_strip_config_t config;
    if(strip>=VARIANT_STRIPS)return VR_INVALID;
    config=proj.variant.strip[strip];if(!sound_strip_config_valid(&config))return VR_INVALID;
    rc=strip_default_bank_load();
    if(rc==VR_MISSING){memset(bank,0,sizeof *bank);bank->magic=STRIP_DEFAULT_MAGIC;bank->schema=1;bank->bytes=sizeof *bank;}
    else if(rc!=VR_OK)return rc;
    if((bank->mask&(1u<<strip))&&!memcmp(&config,&bank->strip[strip],sizeof config))return VR_OK;
    bank->mask|=1u<<strip;bank->strip[strip]=config;
    if(plat_store_save(OBJ_STRIP_DEFAULTS,bank,sizeof *bank))return VR_IO;
    if(strip_default_bank_load()!=VR_OK || !(bank->mask&(1u<<strip)) ||
       memcmp(&config,&bank->strip[strip],sizeof config))return VR_IO;
    return VR_OK;
}
POLY_COLD int project_strip_default_get(unsigned strip,sound_strip_config_t *config)
{
    strip_default_bank_t *bank=(strip_default_bank_t *)io_buf;
    int rc;if(strip>=VARIANT_STRIPS||!config)return VR_INVALID;
    rc=strip_default_bank_load();if(rc!=VR_OK)return rc;
    if(!(bank->mask&(1u<<strip)))return VR_MISSING;
    *config=bank->strip[strip];return VR_OK;
}

int project_save_settings(void)
{
    /* Core recovery may persist settings before audio starts, but must not
     * overwrite metadata belonging to a rejected/future project or backup. */
    if (rejected_save) return -8;
    if (ui_saves_held()) return -9;
    return save_obj(OBJ_SET, &proj.set, sizeof proj.set);
}
