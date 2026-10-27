#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Decode actual C editor replies; exercise the production command49 client."""
import copy
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import fm1_pattern_read as Q


def packets(name):
    data = (ROOT / "build/query-review" / name).read_bytes()
    rows = {}
    while data:
        n = struct.unpack_from("<H", data)[0]
        frame, data = data[2:n + 2], data[n + 2:]
        assert len(frame) == n and frame[0] == 240 and frame[-1] == 247
        body = tuple(frame[1:-1])
        key = body[6], body[7], Q.number(body[8:10])
        decoded = Q.decode(body, *key, body[5])
        assert decoded["status"] == "ok"
        if key in rows:
            assert rows[key] == body
        rows[key] = body
    return rows


def crc(data):
    value = zlib.crc32(bytes(data))
    return tuple(data) + tuple((value >> (7 * i)) & 127 for i in range(5))


class Port:
    def __init__(self, rows, mutate=None, silent=False):
        self.rows, self.queue, self.sent = rows, [], []
        self.mutate, self.silent = mutate, silent

    def send(self, message):
        # Any save/export, CC, note, restore or setter is an immediate failure.
        assert message.type == "sysex" and tuple(message.data[:4]) == Q.HEADER
        assert len(message.data) == 10 and message.data[4] == 1
        self.sent.append(message.data)
        if self.silent:
            return
        key = message.data[6], message.data[7], Q.number(message.data[8:10])
        data = list(self.rows[key][:-5])
        data[5] = message.data[5]
        data = crc(data)
        if self.mutate:
            data = self.mutate(data, len(self.sent))
        self.queue.append(Message("sysex", data=data))

    def poll(self):
        return self.queue.pop(0) if self.queue else None


class Message:
    def __init__(self, kind, data):
        self.type, self.data = kind, data


def capture(name, patterns=(2, 3)):
    port = Port(packets(name))
    result = Q.capture(port, port, patterns, message=Message)
    assert port.sent and all(tuple(x[:4]) == Q.HEADER for x in port.sent)
    return result


class QueryTests(unittest.TestCase):
    def test_lan3_multi_bar_rates_and_out_of_range_rejection(self):
        for rate in range(21,29):
            raw=bytes([16,rate])+bytes(16)
            self.assertEqual(Q.parse_payload(0,1,0,raw)['rate'],rate)
        for rate in (29,31,255):
            with self.assertRaises(Q.ReadbackError):Q.parse_payload(0,1,0,bytes([16,rate])+bytes(16))

    def test_real_pages_three_formats_and_64_steps(self):
        for mode in range(3):
            s = capture(f"mode-{mode}.bin")
            self.assertTrue(s["read_only"])
            self.assertEqual(s["patterns"][0]["format"], ("legacy", "independent", "expanded")[mode])
            p = s["patterns"][0]
            self.assertTrue(p["dirty"])
            self.assertEqual(p["swing_raw"], 15)
            self.assertEqual(len(p["lanes"]), 25)
            for lane in p["lanes"]:
                self.assertEqual(len(lane.get("steps", lane.get("hits"))), 64)
            bass = p["lanes"][22]
            self.assertEqual(bass["generator"]["seed"], 999)
            self.assertEqual(bass["steps"][63]["note"], 30 + (7 + 63) % 60)
            self.assertEqual(bass["steps"][63]["flags"], (63 + 1) % 3 | 8)
            self.assertEqual(p["lanes"][23]["name"], "303 B")
            self.assertEqual(p["motion_count"], len(p["motion"]))
            self.assertEqual(len(Q.STAMP), len(s["stamp"]))

    def test_all_copy_scopes_and_undo_using_actual_service(self):
        for scope in range(25):
            before = capture(f"scope-{scope}-before.bin")
            after = capture(f"scope-{scope}-after.bin")
            undo = capture(f"scope-{scope}-undo.bin")
            self.assertFalse(Q.compare(before, after)["equal"], scope)
            self.assertTrue(Q.compare(before, undo)["equal"], scope)
            src, dst = after["patterns"]
            self.assertEqual(Q.semantic(before)["2"], Q.semantic(after)["2"])
            self.assertEqual(dst["swing_raw"], before["patterns"][1]["swing_raw"])
            selected = list(range(25)) if scope == 0 else [scope - 1]
            # Scope 23/24 map to lanes22/23, matching the two bass sequences.
            for lane in range(25):
                a, b, old = src["lanes"][lane], dst["lanes"][lane], before["patterns"][1]["lanes"][lane]
                if lane not in selected:
                    self.assertEqual(b, old)
                elif lane < 22:
                    self.assertEqual(b["hits"], a["hits"])
                    self.assertEqual((b["length"], b["rate"]), (a["length"], a["rate"]))
                    self.assertEqual(b["shared_accent"], old["shared_accent"])
                else:
                    self.assertEqual(b, a)
            # JSON files can be compared after the fact without any MIDI access.
            with tempfile.TemporaryDirectory() as temp:
                f = Path(temp) / "snapshot.json"
                f.write_text(json.dumps(undo), encoding="utf-8")
                self.assertTrue(Q.compare(before, json.loads(f.read_text()))["equal"])
        (ROOT / "build/query-review/decoded-before.json").write_text(json.dumps(before, indent=2) + "\n")
        (ROOT / "build/query-review/decoded-after.json").write_text(json.dumps(after, indent=2) + "\n")
        (ROOT / "build/query-review/decoded-undo.json").write_text(json.dumps(undo, indent=2) + "\n")

    def test_motion159_and_high_bits(self):
        s = capture("full-motion.bin")
        self.assertEqual(len(s["patterns"][0]["motion"]), 160)
        self.assertEqual(s["patterns"][0]["motion"][-1]["parameter"], 159)
        self.assertEqual(s["patterns"][0]["motion"][-1]["values"], [255] * 63 + [135])
        self.assertEqual(Q.request(1, 2, 159, 7)[-2:], [31, 1])

    def test_bad_arguments(self):
        for args in [(32, 0, 0, 1), (-1, 0, 0, 1), (0, 1, 25, 1), (0, 2, 160, 1), (0, 0, 1, 1), (0, 3, 0, 1), (0, 1, 0, 128)]:
            with self.assertRaises(ValueError):
                Q.request(*args)
        for patterns in [[], [0], [33], [2, 2], [True]]:
            with self.assertRaises(ValueError):
                Q.capture(None, None, patterns, message=Message)

    def test_corrupt_and_truncated_replies(self):
        body = packets("mode-2.bin")[(1, 1, 22)]
        for n in range(4, len(body)):
            with self.assertRaises(Q.ReadbackError):
                Q.decode(body[:n], 1, 1, 22, 77)
        for i in [11, 31, 33, len(body) - 1]:
            bad = list(body); bad[i] ^= 1
            with self.assertRaises(Q.ReadbackError):
                Q.decode(bad, 1, 1, 22, 77)
        bad = list(body); bad[34] = 128
        with self.assertRaises(Q.ReadbackError):
            Q.decode(bad, 1, 1, 22, 77)
        bad = list(body[:-5]); bad[4] = 2
        with self.assertRaises(Q.ReadbackError):
            Q.decode(crc(bad), 1, 1, 22, 77)
        self.assertIsNone(Q.decode(body, 1, 1, 22, 76))
        self.assertIsNone(Q.decode([1, 2], 1, 1, 22, 77))

    def test_mixed_stamps_never_reported(self):
        for offset in [11, 16, 21, 26]:
            def mutate(data, n):
                if n == 3:
                    changed = list(data[:-5]); changed[offset] ^= 1
                    return crc(changed)
                return data
            port = Port(packets("mode-2.bin"), mutate=mutate)
            with self.assertRaisesRegex(Q.ReadbackError, "changed"):
                Q.capture(port, port, [2, 3], message=Message)

    def test_final_summary_must_match_even_without_project_edit(self):
        rows = packets("mode-2.bin")
        calls = 0
        def mutate(data, n):
            nonlocal calls
            if data[6:10] == (1, 0, 0, 0):
                calls += 1
                if calls == 3:  # final whole-scan bracket, after per-pattern end
                    changed = list(data[:-5])
                    changed[36] ^= 1  # dirty byte, packed summary (no high bits)
                    return crc(changed)
            return data
        port = Port(rows, mutate=mutate)
        with self.assertRaisesRegex(Q.ReadbackError, "Summary changed at end"):
            Q.capture(port, port, [2, 3], message=Message)

    def test_timeout_and_busy_never_fall_back_to_export(self):
        port = Port(packets("mode-2.bin"), silent=True)
        with self.assertRaisesRegex(Q.ReadbackError, "No save/export fallback"):
            Q.read_page(port, port, 1, 0, 0, message=Message, timeout=0)
        self.assertEqual(len(port.sent), 1)

        def busy(data, n):
            return crc(tuple(data[:10]) + (2,))
        port = Port(packets("mode-2.bin"), mutate=busy)
        with self.assertRaisesRegex(Q.ReadbackError, "busy"):
            Q.read_page(port, port, 1, 0, 0, message=Message, retries=2)
        self.assertEqual(len(port.sent), 2)

    def test_incomplete_files_cannot_claim_equal(self):
        original = capture("mode-2.bin")
        for changed in [dict(schema=1, read_only=True, patterns=[]), copy.deepcopy(original), copy.deepcopy(original)]:
            if changed.get("patterns"):
                if len(changed["patterns"][0]["lanes"]) == 25:
                    changed["patterns"][0]["lanes"].pop()
            with self.assertRaises(ValueError):
                Q.compare(original, changed)


if __name__ == "__main__":
    unittest.main(verbosity=2)
