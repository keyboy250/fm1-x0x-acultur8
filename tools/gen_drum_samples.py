#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generated data for the 909 drum engine (firmware/src/dsp/drum909.c, fxbus.c).

Writes two headers (both flash-resident `static const` data):

  x0x_drum_samples.h  the hi-hat, ride and crash PCM as int16 (assets/909/*.wav,
                      from ER-99 via 9W9, GPL-3.0). hh.wav is 24-bit: it is
                      rounded to 16 bits here (the 909's own cymbals are 6-bit).
  x0x_drum_tables.h   the tanh lookup used by every saturator, and 9W9's EXP pot
                      curves (er99_pots.h: min * (max/min)^(pot/127)) evaluated
                      here, so the device needs no powf and gets the same values
                      9W9 computes with libm.

  tools/gen_drum_samples.py [build/gen/x0x_drum_samples.h]
The tables header is written next to the samples header.
"""
import math
import struct
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
ASSETS = SRC / "assets" / "909"

# (C name, file). The closed hat plays the open hat's buffer (as in 9W9).
SAMPLES = [("x0x_smp_hh", "hh.wav"), ("x0x_smp_ride", "ride.wav"), ("x0x_smp_crash", "crash.wav")]

# 9W9's EXP pot ranges (er99_pots.h). One table per distinct (min, max).
EXP_RANGES = [
    ("BD_DECAY", 100.0, 4000.0), ("BD_PITCH", 0.43, 4.7), ("DRIVE", 0.85, 12.0),
    ("SD_TUNE", 130.0, 320.0), ("SD_TONE", 300.0, 4000.0),
    ("LT_TUNE", 50.0, 90.0), ("LT_DECAY", 80.0, 2600.0),
    ("MT_TUNE", 80.0, 125.0), ("MT_DECAY", 70.0, 2200.0),
    ("HT_TUNE", 110.0, 170.0), ("HT_DECAY", 60.0, 2000.0),
    ("RS_TUNE", 150.0, 300.0), ("CP_TUNE", 650.0, 1400.0), ("CP_TAIL", 120.0, 1000.0),
    ("OH_DECAY", 20.0, 1200.0), ("CH_DECAY", 15.0, 300.0), ("CY_DECAY", 100.0, 3000.0),
    ("SMP_PITCH", 0.25, 4.0), ("FX_HPF", 30.0, 800.0),
]

TANH_N = 1024          # points over [0, TANH_MAX]; linear interpolation
TANH_MAX = 8.0


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def cfloat(x):
    s = repr(f32(x))
    if "e" not in s and "." not in s:
        s += ".0"
    return s + "f"


def read_wav(path):
    """first channel as a list of int16 (16-bit kept, 24-bit rounded)"""
    d = path.read_bytes()
    if d[:4] != b"RIFF" or d[8:12] != b"WAVE":
        raise SystemExit(f"{path}: not a WAV")
    i, ch, bits, data = 12, 1, 16, None
    while i + 8 <= len(d):
        cid, sz = d[i:i + 4], struct.unpack("<I", d[i + 4:i + 8])[0]
        if cid == b"fmt ":
            ch, bits = struct.unpack("<H", d[i + 10:i + 12])[0], struct.unpack("<H", d[i + 22:i + 24])[0]
        elif cid == b"data":
            data = d[i + 8:i + 8 + sz]
            break                                   # what 9W9's loader does: stop at data
        i += 8 + sz + (sz & 1)
    if data is None:
        raise SystemExit(f"{path}: no data chunk")
    bp = bits // 8 * ch
    out = []
    for k in range(len(data) // bp):
        p = data[k * bp:k * bp + bits // 8]
        if bits == 16:
            v = struct.unpack("<h", p)[0]
        elif bits == 24:
            v24 = int.from_bytes(p, "little", signed=True)
            v = max(-32768, min(32767, int(math.floor(v24 / 256.0 + 0.5))))
        else:
            raise SystemExit(f"{path}: {bits}-bit not handled")
        out.append(v)
    return out, bits


def arr(ctype, name, vals, fmt=str, per=12, shared=False):
    # shared: a table several units include, defined once (in the unit that defines
    # X0X_DRUM_TABLES_DEFINE, drum909.c) and declared extern in the rest. Static gave each unit its own
    # copy: two of x0x_tanh_tab and x0x_pot_exp, 13.8 KB of the app's flash.
    if shared:
        return "\n".join([f"#ifdef X0X_DRUM_TABLES_DEFINE",
                          f"const {ctype} {name}[{len(vals)}] = {{"] +
                         ["    " + ", ".join(fmt(v) for v in vals[i:i + per]) + "," for i in range(0, len(vals), per)] +
                         ["};", "#else", f"extern const {ctype} {name}[{len(vals)}];", "#endif"])
    lines = [f"static const {ctype} {name}[{len(vals)}] = {{"]
    for i in range(0, len(vals), per):
        lines.append("    " + ", ".join(fmt(v) for v in vals[i:i + per]) + ",")
    lines.append("};")
    return "\n".join(lines)


def pot_exp(lo, hi, pot):
    """er99_pot_to_value for an EXP pot, in float as 9W9 evaluates it"""
    ratio = f32(f32(hi) / f32(lo))
    t = f32(pot / 127.0)
    return f32(f32(lo) * f32(ratio ** t))


def pack_exp_curves(rows):
    """Lossless IEEE32 deltas, byte-aligned rows in one bounded byte object.

    Four byte loads may cross an internal row boundary. Guard bytes keep the
    final load inside the same object if a future final curve is narrower.
    """
    descriptors, blob, read_end = [], bytearray(), 0
    for words in rows:
        if len(words) != 128 or any(type(w) is not int or not 0 <= w <= 0x7f7fffff for w in words):
            raise ValueError("EXP rows require 128 finite nonnegative IEEE32 words")
        base = min(words)
        width = max(1, (max(words) - base).bit_length())
        if width > 26:
            raise ValueError("EXP delta exceeds the 26-bit decoder domain")
        offset = len(blob)
        if offset > 0xffff:
            raise ValueError("EXP byte offset exceeds uint16_t")
        packed = bytearray(16 * width)
        for p, word in enumerate(words):
            delta = word - base
            for bit in range(width):
                if delta & (1 << bit):
                    position = p * width + bit
                    packed[position // 8] |= 1 << (position % 8)
        descriptors.append((base, offset, width))
        blob.extend(packed)
        read_end = max(read_end, offset + (127 * width // 8) + 4)
    if not descriptors:
        raise ValueError("EXP packing requires at least one row")
    blob.extend(bytes(max(0, read_end - len(blob))))
    return descriptors, bytes(blob)


EXP_BLOCK_SIZE = 64


def pack_exp_affine(rows, block_size=EXP_BLOCK_SIZE):
    """Lossless block prediction in IEEE word space; explicit 10-byte descriptors.

    Positive integer slopes and nonnegative residuals make every reconstruction
    addition bounded by the validated original finite IEEE32 word. Four-byte
    loads may cross blocks inside the single blob; zero guards bound the end.
    The legacy pack_exp_curves remains available for baseline verification.
    """
    if type(block_size) is not int or block_size not in (16, 32, 64, 128):
        raise ValueError("EXP block size must be 16, 32, 64 or 128")
    descriptors, blob, read_end = bytearray(), bytearray(), 0
    for row in rows:
        if len(row) != 128 or any(type(w) is not int or not 0 <= w <= 0x7f7fffff for w in row):
            raise ValueError("EXP rows require 128 finite nonnegative IEEE32 words")
        for start in range(0, 128, block_size):
            words = row[start:start + block_size]
            base = words[0]
            slope = min((word - base) // i for i, word in enumerate(words) if i)
            if not 0 <= slope <= 0xffffff:
                raise ValueError("EXP slope exceeds unsigned 24-bit domain")
            residuals = [word - base - slope * i for i, word in enumerate(words)]
            width = max(1, max(residuals).bit_length())
            if min(residuals) < 0 or width > 25:
                raise ValueError("EXP residual exceeds bounded four-byte decoder domain")
            offset = len(blob)
            if offset > 0xffff:
                raise ValueError("EXP byte offset exceeds uint16_t")
            descriptors.extend(base.to_bytes(4, "little") + slope.to_bytes(3, "little") +
                               offset.to_bytes(2, "little") + bytes([width]))
            packed = bytearray((block_size * width + 7) // 8)
            for i, residual in enumerate(residuals):
                for bit in range(width):
                    if residual & (1 << bit):
                        position = i * width + bit
                        packed[position // 8] |= 1 << (position % 8)
            blob.extend(packed)
            read_end = max(read_end, offset + ((block_size - 1) * width // 8) + 4)
    if not descriptors:
        raise ValueError("EXP packing requires at least one row")
    blob.extend(bytes(max(0, read_end - len(blob))))
    return bytes(descriptors), bytes(blob)


def main():
    out_s = Path(sys.argv[1]) if len(sys.argv) > 1 else SRC / "build" / "gen" / "x0x_drum_samples.h"
    out_t = out_s.parent / "x0x_drum_tables.h"
    out_s.parent.mkdir(parents=True, exist_ok=True)

    hdr = ["/* Generated by tools/gen_drum_samples.py -- do not edit.",
           " * Hi-hat, ride and crash from ER-99 (Matthew Cieplak) via 9W9, GPL-3.0.",
           " * Mono, 44.1 kHz, int16 (full scale 32768). */",
           "#pragma once", "#include <stdint.h>", ""]
    total = 0
    for name, fn in SAMPLES:
        pcm, bits = read_wav(ASSETS / fn)
        total += len(pcm)
        hdr.append(f"/* {fn}: {len(pcm)} frames, source {bits}-bit */")
        hdr.append(f"#define {name.upper()}_LEN {len(pcm)}u")
        hdr.append(arr("int16_t", name, pcm, per=16))
        hdr.append("")
    out_s.write_text("\n".join(hdr))

    tab = [f32(math.tanh(TANH_MAX * i / TANH_N)) for i in range(TANH_N + 2)]
    t = ["/* Generated by tools/gen_drum_samples.py -- do not edit. */",
         "#pragma once", "",
         f"/* tanh(u) at u = i * {TANH_MAX} / {TANH_N}, i = 0..{TANH_N + 1} (one guard point) */",
         f"#define X0X_TANH_N {TANH_N}",
         f"#define X0X_TANH_SCALE {cfloat(TANH_N / TANH_MAX)}",
         arr("float", "x0x_tanh_tab", tab, cfloat, per=6, shared=True), "",
         "/* 9W9 EXP pots (er99_pots.h): value = min * (max/min)^(pot/127), pot 0..127 */"]
    for i, (n, lo, hi) in enumerate(EXP_RANGES):
        t.append(f"#define X0X_EXP_{n} {i}   /* {lo:g} .. {hi:g} */")
    t.append(f"#define X0X_EXP_COUNT {len(EXP_RANGES)}")
    # Exact IEEE32 affine blocks: unsigned prediction plus packed residuals.
    # One contiguous byte object permits bounded cross-row four-byte loads.
    t.append("#include <stdint.h>")
    t.append(f"#define X0X_EXP_BLOCK_SIZE {EXP_BLOCK_SIZE}u")
    t.append("#ifdef X0X_DRUM_TABLES_DEFINE")
    t.append("#ifdef X0X_EXP_RAW_REFERENCE /* Pinned native oracle only; never firmware. */")
    t.append(f"const float x0x_pot_exp[{len(EXP_RANGES)}][128] = {{")
    for n, lo, hi in EXP_RANGES:
        vals=[pot_exp(lo,hi,p) for p in range(128)]
        t.append("    { /* "+n+" */")
        for i in range(0,128,6):
            t.append("        "+", ".join(cfloat(v) for v in vals[i:i+6])+",")
        t.append("    },")
    t.append("};")
    t.append("__attribute__((noinline)) float x0x_pot_exp_value(unsigned curve,unsigned pot){return x0x_pot_exp[curve][pot];}")
    t.append("#else")
    descriptors, packed = pack_exp_affine([
        [struct.unpack('<I', struct.pack('<f', pot_exp(lo, hi, p)))[0] for p in range(128)]
        for _, lo, hi in EXP_RANGES
    ])
    t.append(arr("uint8_t", "x0x_exp_blocks", descriptors, lambda x: f"0x{x:02x}", per=10))
    t.append(arr("uint8_t", "x0x_exp_packed", packed, lambda x: f"0x{x:02x}", per=16))
    t.extend([
        "/* Internal control-only accessor; caller validates curve/count and pot0..127.",
        " * Integer decode restores the original float bits, never interpolates. */",
        "__attribute__((noinline)) float x0x_pot_exp_value(unsigned curve, unsigned pot)",
        "{",
        "    const uint8_t *c=x0x_exp_blocks+10u*(curve*(128u/X0X_EXP_BLOCK_SIZE)+pot/X0X_EXP_BLOCK_SIZE);",
        "    unsigned index=pot%X0X_EXP_BLOCK_SIZE, width=c[9], bit=index*width;",
        "    uint32_t base=(uint32_t)c[0]|((uint32_t)c[1]<<8)|((uint32_t)c[2]<<16)|((uint32_t)c[3]<<24);",
        "    uint32_t slope=(uint32_t)c[4]|((uint32_t)c[5]<<8)|((uint32_t)c[6]<<16);",
        "    unsigned offset=(unsigned)c[7]|((unsigned)c[8]<<8);",
        "    const uint8_t *p=x0x_exp_packed+offset+(bit>>3);",
        "    uint32_t word=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);",
        "    union { uint32_t bits; float value; } result;",
        "    result.bits=base+slope*index+((word>>(bit&7u))&((1u<<width)-1u));",
        "    return result.value;",
        "}",
        "#endif /* X0X_EXP_RAW_REFERENCE */",
        "#else",
        "float x0x_pot_exp_value(unsigned curve, unsigned pot);",
        "#endif",
    ])
    t.append("")
    out_t.write_text("\n".join(t))
    print(f"gen_drum_samples: {total} frames ({total * 2} bytes) -> {out_s.name}; "
          f"tanh {TANH_N + 2} + {len(EXP_RANGES)} pot curves -> {out_t.name}")


if __name__ == "__main__":
    main()
