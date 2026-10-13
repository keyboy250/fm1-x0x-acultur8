#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Round-trip every glyph and compare production raw/RLE renderer pixels.

Requires Pillow and a host C compiler. Run in the documented Linux builder:
  python3 tests/test_font_packing.py
Optional FONT_TEST_OUTPUT retains generated headers, pixel streams and executables.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import random
import re
import shlex
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("gen_font", ROOT / "tools/gen_font.py")
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)


def decode(data):
    out = bytearray()
    pos = 0
    while pos < len(data):
        control = data[pos]
        pos += 1
        length = (control & 127) + 1
        if control & 128:
            if pos == len(data):
                raise ValueError("truncated repeat")
            out.extend([data[pos]] * length)
            pos += 1
        else:
            if pos + length > len(data):
                raise ValueError("truncated literal")
            out.extend(data[pos:pos + length])
            pos += length
    return bytes(out)


def arrays(path):
    text = path.read_text()
    return {name: [int(token, 0) for token in re.findall(r"0x[0-9a-fA-F]+|\d+", body)]
            for name, body in re.findall(r"FONT_(\w+)\[\d+\] = \{(.*?)\};", text, re.S)}


def huffman_decode(data, tables, expected):
    """Independent canonical reader, bounded by expected packet byte count."""
    counts = tables["HUFFMAN_COUNT"]
    first = tables["HUFFMAN_FIRST_CODE"]
    offsets = tables["HUFFMAN_FIRST_SYMBOL"]
    symbols = tables["HUFFMAN_SYMBOL"]
    output = bytearray()
    position = 0
    for _ in range(expected):
        code = 0
        for length in range(1, len(counts)):
            if position >= len(data) * 8:
                raise ValueError("truncated code")
            code = code * 2 + ((data[position // 8] >> (7 - position % 8)) & 1)
            position += 1
            if first[length] <= code < first[length] + counts[length]:
                output.append(symbols[offsets[length] + code - first[length]])
                break
        else:
            raise ValueError("invalid code")
    return bytes(output)


class FontPackingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        output = os.environ.get("FONT_TEST_OUTPUT")
        if output:
            cls.out = Path(output).resolve()
            cls.out.mkdir(parents=True, exist_ok=True)
        else:
            cls.tmp = tempfile.TemporaryDirectory()
            cls.addClassCleanup(cls.tmp.cleanup)
            cls.out = Path(cls.tmp.name)
        cls.cc = shlex.split(os.environ.get("CC", "cc"))
        cls.env = dict(os.environ, X0X_FONTSET="barlow")
        cls.arrays = {}
        cls.results = {}
        for mode in ("raw", "rle", "huffman"):
            folder = cls.out / mode
            folder.mkdir(exist_ok=True)
            header = folder / "felucca_font.h"
            subprocess.run([sys.executable, str(ROOT / "tools/gen_font.py"), str(header)],
                           env=dict(cls.env, X0X_FONT_COMPRESSION=mode), check=True,
                           stdout=subprocess.PIPE, text=True)
            cls.arrays[mode] = arrays(header)
            exe = folder / "font_render"
            subprocess.run([*cls.cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unused-function", "-I" + str(folder),
                            str(ROOT / "tests/host/font_render_test.c"), "-o", str(exe)], check=True)
            pixels = folder / "pixels.bin"
            with pixels.open("wb") as output:
                result = subprocess.run([str(exe)], check=True, stdout=output, stderr=subprocess.PIPE, text=True)
            cls.results[mode] = {"pixels_sha256": hashlib.sha256(pixels.read_bytes()).hexdigest(),
                                 "pixel_stream_bytes": pixels.stat().st_size, "result": result.stderr.strip()}

    def test_boundary_runs_and_literals(self):
        for n in (0, 1, 2, 3, 127, 128, 129, 255, 256, 1000):
            for source in (bytes([0]) * n, bytes([255]) * n, bytes(i % 256 for i in range(n))):
                with self.subTest(length=n):
                    self.assertEqual(decode(gen.pack_glyph(source)), source)

    def test_mixed_runs(self):
        rng = random.Random(2701)
        for _ in range(50):
            source = b"".join(bytes([rng.randrange(256)]) * rng.randrange(1, 160) for _ in range(30))
            self.assertEqual(decode(gen.pack_glyph(source)), source)

    def test_every_glyph_and_metric_matches(self):
        raw, packed = self.arrays["raw"], self.arrays["rle"]
        glyphs = total_raw = total_packed = 0
        for face in ("T", "XS", "S", "B", "M", "L"):
            self.assertEqual(raw[face + "_ADV"], packed[face + "_ADV"])
            self.assertEqual(raw[face + "_BW"], packed[face + "_BW"])
            ro, po = raw[face + "_OFF"], packed[face + "_OFF"]
            self.assertEqual(len(ro), len(po))
            self.assertEqual(len(ro), len(raw[face + "_ADV"]) + 1)
            rd, pd = bytes(raw[face + "_DATA"]), bytes(packed[face + "_DATA"])
            self.assertEqual(ro[-1], len(rd))
            self.assertEqual(po[-1], len(pd))
            for i in range(len(ro) - 1):
                self.assertEqual(decode(pd[po[i]:po[i + 1]]), rd[ro[i]:ro[i + 1]])
                glyphs += 1
            total_raw += len(rd)
            total_packed += len(pd)
        self.assertLess(total_packed, total_raw)
        evidence = {"glyphs": glyphs, "raw_bytes": total_raw, "rle_bytes": total_packed,
                    "payload_saved": total_raw - total_packed, "render_results": self.results}
        (self.out / "font-packing-report.json").write_text(json.dumps(evidence, indent=2) + "\n")
        print(json.dumps(evidence, indent=2))

    def test_huffman_every_glyph_and_metrics_match_old_packets(self):
        old, new = self.arrays["rle"], self.arrays["huffman"]
        glyphs = total = 0
        for face in ("T", "XS", "S", "B", "M", "L"):
            for metric in ("ADV", "BW"):
                self.assertEqual(old[face + "_" + metric], new[face + "_" + metric])
            oo, no = old[face + "_OFF"], new[face + "_OFF"]
            od, nd = bytes(old[face + "_DATA"]), bytes(new[face + "_DATA"])
            self.assertEqual(len(oo), len(no))
            self.assertEqual(no[-1], len(nd))
            total += len(nd)
            for i in range(len(oo) - 1):
                packet = od[oo[i]:oo[i + 1]]
                self.assertEqual(huffman_decode(nd[no[i]:no[i + 1]], new, len(packet)), packet)
                glyphs += 1
        self.assertEqual(glyphs, 439)
        # Tables are part of the flash cost; require useful savings after them.
        table_bytes = len(new["HUFFMAN_SYMBOL"]) + 2 * len(new["HUFFMAN_PREFIX"]) + 2 * sum(
            len(new["HUFFMAN_" + field]) for field in ("COUNT", "FIRST_CODE", "FIRST_SYMBOL"))
        old_total = sum(len(old[face + "_DATA"]) for face in ("T", "XS", "S", "B", "M", "L"))
        self.assertGreater(old_total - total - table_bytes, 6144)
        report = {"glyphs": glyphs, "old_packet_bytes": old_total,
                  "huffman_bytes": total, "table_bytes": table_bytes,
                  "saved_before_decoder": old_total - total - table_bytes,
                  "render_results": self.results}
        (self.out / "font-huffman-report.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report, indent=2))

    def test_production_renderer_pixels_and_metrics_match(self):
        self.assertEqual(self.results["raw"], self.results["rle"])
        self.assertEqual(self.results["raw"], self.results["huffman"])
        # Frozen accepted raw-render result in font_packing_evidence.json, not
        # recomputed from this candidate's generator or decoder.
        self.assertEqual(self.results["huffman"]["pixels_sha256"],
                         "e971afab542cff352576dc573b4459322a7e2bce477c4bcaa8589ec1c70290d8")


if __name__ == "__main__":
    unittest.main()
