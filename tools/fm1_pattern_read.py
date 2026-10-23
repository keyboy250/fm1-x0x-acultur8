#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Snapshot unsaved X0X patterns using only command49; never export/save/CC.

  python tools/fm1_pattern_read.py read --patterns 5 7 --output before.json
  python tools/fm1_pattern_read.py compare before.json after.json
Only the separate pattern-query candidate supports this command.
"""
import argparse
import json
import secrets
import time
import zlib
from pathlib import Path
from fm1_strip_read import choose_ports

HEADER = (0x7D, 0x46, 0x4C, 49)
DRUMS = (("KICK", "SNARE", "LOW TOM", "MID TOM", "HIGH TOM", "RIMSHOT", "CLAP", "CL HAT", "OP HAT", "CRASH", "RIDE"),
         ("KICK", "SNARE", "LOW TOM", "MID TOM", "HIGH TOM", "RS/CLAVE", "CP/MARACAS", "COWBELL", "CYMBAL", "OP HAT", "CL HAT"))
PARTS = ("909", "808", "303 A", "303 B", "BREAK")
STAMP = ("revision", "queue_ticket", "transport_epoch", "project_fingerprint")


class ReadbackError(RuntimeError):
    pass


def number(data):
    return sum(v << (7 * i) for i, v in enumerate(data))


def request(pattern, section, index, token):
    if (any(type(x) is not int for x in (pattern, section, index, token)) or
            not 0 <= pattern < 32 or not 0 <= token < 128 or
            section not in (0, 1, 2) or not 0 <= index < (1, 25, 160)[section]):
        raise ValueError("Invalid pattern/page/index/token")
    return list(HEADER) + [1, token, pattern, section, index & 127, index >> 7]


def unpack7(data, length):
    if len(data) != length + (length + 6) // 7:
        raise ReadbackError("Incorrect packed payload length")
    raw = bytearray()
    for offset in range(0, len(data), 8):
        group = data[offset:offset + 8]
        if group[0] >> (len(group) - 1):
            raise ReadbackError("Noncanonical pack7 high-bit mask")
        raw.extend(v | (((group[0] >> i) & 1) << 7) for i, v in enumerate(group[1:]))
    return bytes(raw)


def parse_payload(pattern, section, index, raw):
    wanted = 4 if section == 0 else 68 if section == 2 else 18 if index < 22 else 144 if index < 24 else 144
    if len(raw) != wanted:
        raise ReadbackError("Unexpected schema payload length")
    if section == 0:
        mode, swing, dirty, count = raw
        if mode > 2 or swing > 100 or dirty > 1 or count > 160:
            raise ReadbackError("Invalid pattern summary")
        return dict(pattern=pattern + 1, format=("legacy", "independent", "expanded")[mode],
                    swing_raw=swing, swing_percent=50 + swing // 4, dirty=bool(dirty), motion_count=count)
    if section == 2:
        part, target, voice, parameter = raw[:4]
        if part >= 5:
            raise ReadbackError("Invalid motion owner")
        return dict(part=PARTS[part], target=target, voice=voice, parameter=parameter, values=list(raw[4:]))
    length, rate = raw[:2]
    if not 1 <= length <= 64 or rate > 20:
        raise ReadbackError("Invalid lane timing")
    result = dict(lane=index, length=length, rate=rate)
    if index < 22:
        kit, voice = divmod(index, 11)
        result.update(part=PARTS[kit], voice=voice, name=PARTS[kit] + " " + DRUMS[kit][voice],
                      hits=[bool((int.from_bytes(raw[2:10], "little") >> i) & 1) for i in range(64)],
                      shared_accent=[bool((int.from_bytes(raw[10:18], "little") >> i) & 1) for i in range(64)])
    elif index < 24:
        names = ("density", "accent", "slide", "octave_range", "root", "scale", "base_octave", "mutate_bars")
        generator = dict(zip(names, raw[4:12]))
        generator["seed"] = int.from_bytes(raw[12:16], "little")
        result.update(part=PARTS[index - 20], name=PARTS[index - 20], direction=raw[2],
                      transpose_raw=raw[3], transpose_semitones=raw[3] - 24, generator=generator,
                      steps=[dict(note=raw[16 + 2 * i], flags=raw[17 + 2 * i],
                                  gate=raw[17 + 2 * i] & 3, accent=bool(raw[17 + 2 * i] & 4),
                                  slide=bool(raw[17 + 2 * i] & 8)) for i in range(64)])
    else:
        result.update(part="BREAK", name="BREAK", slot_a=raw[2], slot_b=raw[3], settings=list(raw[4:16]),
                      steps=[dict(gate=raw[16 + 2 * i], slice=raw[17 + 2 * i]) for i in range(64)])
    return result


def decode(data, pattern, section, index, token):
    data = tuple(data)
    if data[:4] != HEADER:
        return None
    if any(type(x) is not int or not 0 <= x < 128 for x in data):
        raise ReadbackError("Non-7-bit pattern reply")
    if len(data) < 16:
        raise ReadbackError("Truncated pattern reply")
    if (data[5], data[6], data[7], number(data[8:10])) != (token, pattern, section, index):
        return None
    crc = number(data[-5:])
    if crc > 0xFFFFFFFF or crc != zlib.crc32(bytes(data[:-5])):
        raise ReadbackError("Pattern reply CRC mismatch")
    if data[4] != 1:
        raise ReadbackError("Unsupported pattern reply version")
    status = data[10]
    if status:
        if len(data) != 16 or status not in (1, 2, 3):
            raise ReadbackError("Malformed query error")
        return dict(status={1: "invalid", 2: "busy", 3: "unsupported"}[status])
    if len(data) < 38:
        raise ReadbackError("Truncated successful reply")
    stamp = tuple(number(data[11 + i * 5:16 + i * 5]) for i in range(4))
    if any(v > 0xFFFFFFFF for v in stamp):
        raise ReadbackError("Invalid snapshot stamp")
    raw_length = number(data[31:33])
    if raw_length > 144:
        raise ReadbackError("Oversize pattern page")
    payload = parse_payload(pattern, section, index, unpack7(data[33:-5], raw_length))
    return dict(status="ok", stamp=stamp, payload=payload)


def read_page(input_port, output_port, pattern, section, index, *, message, timeout=1.5, retries=3):
    token = secrets.randbelow(128)
    for attempt in range(retries):
        token = (token + 1) & 127
        output_port.send(message("sysex", data=request(pattern, section, index, token)))
        deadline = time.monotonic() + timeout
        busy = False
        while time.monotonic() < deadline:
            packet = input_port.poll()
            if packet is None:
                time.sleep(0.002)
                continue
            if packet.type != "sysex":
                continue
            result = decode(packet.data, pattern, section, index, token)
            if result is None:
                continue
            if result["status"] == "busy":
                busy = True
                break
            if result["status"] != "ok":
                raise ReadbackError("Device rejected pattern query: " + result["status"])
            return result
        if not busy:
            raise ReadbackError("No command49 reply; pattern query unsupported, disconnected or timed out. No save/export fallback was sent.")
        if attempt + 1 < retries:
            time.sleep(0.04)
    raise ReadbackError("Pattern query busy: stop playback and finish edits, then repeat")


def capture(input_port, output_port, patterns, *, message, timeout=1.5):
    if not patterns or len(set(patterns)) != len(patterns) or any(type(p) is not int or not 1 <= p <= 32 for p in patterns):
        raise ValueError("Choose distinct patterns 1..32")
    stamp = None

    def page(pattern, section, index):
        nonlocal stamp
        row = read_page(input_port, output_port, pattern - 1, section, index, message=message, timeout=timeout)
        if stamp is None:
            stamp = row["stamp"]
        elif row["stamp"] != stamp:
            raise ReadbackError("Project changed during snapshot; discard the entire scan and repeat without editing")
        return row["payload"]

    result = []
    for pattern in patterns:
        summary = page(pattern, 0, 0)
        lanes = [page(pattern, 1, i) for i in range(25)]
        motion = [page(pattern, 2, i) for i in range(summary["motion_count"])]
        if page(pattern, 0, 0) != summary:
            raise ReadbackError("Summary changed during snapshot; nothing reported")
        result.append(dict(summary, lanes=lanes, motion=motion))
    # Close the whole multi-pattern scan as well as each individual pattern.
    first_summary = {k: v for k, v in result[0].items() if k not in ("lanes", "motion")}
    if page(patterns[0], 0, 0) != first_summary:
        raise ReadbackError("Summary changed at end of multi-pattern snapshot; nothing reported")
    return dict(schema=1, read_only=True, source="current project RAM (includes unsaved edits)",
                stamp=dict(zip(STAMP, stamp)), patterns=result)


def semantic(snapshot):
    if (snapshot.get("schema") != 1 or snapshot.get("read_only") is not True or
            not isinstance(snapshot.get("patterns"), list) or not snapshot["patterns"]):
        raise ValueError("Not a pattern-query snapshot")
    result = {}
    for pattern in snapshot["patterns"]:
        number = pattern.get("pattern")
        if (type(number) is not int or not 1 <= number <= 32 or str(number) in result or
                pattern.get("format") not in ("legacy", "independent", "expanded") or
                type(pattern.get("swing_raw")) is not int or not 0 <= pattern["swing_raw"] <= 100 or
                not isinstance(pattern.get("lanes"), list) or len(pattern["lanes"]) != 25 or
                not isinstance(pattern.get("motion"), list) or pattern.get("motion_count") != len(pattern["motion"])):
            raise ValueError("Incomplete or malformed pattern snapshot")
        for i, lane in enumerate(pattern["lanes"]):
            steps = lane.get("hits") if i < 22 else lane.get("steps")
            if lane.get("lane") != i or not isinstance(steps, list) or len(steps) != 64:
                raise ValueError("Incomplete lane snapshot")
            if i < 22 and len(lane.get("shared_accent", [])) != 64:
                raise ValueError("Incomplete accent snapshot")
        for motion in pattern["motion"]:
            if (any(key not in motion for key in ("part", "target", "voice", "parameter", "values")) or
                    len(motion["values"]) != 64):
                raise ValueError("Incomplete motion snapshot")
        p = {k: v for k, v in pattern.items() if k not in ("dirty", "motion_count")}
        p["motion"] = sorted(p["motion"], key=lambda m: (m["part"], m["target"], m["voice"], m["parameter"]))
        result[str(p.pop("pattern"))] = p
    return result


def compare(before, after):
    differences = []

    def walk(a, b, path):
        if type(a) is not type(b):
            differences.append(dict(path=path, before=a, after=b))
        elif isinstance(a, dict):
            for key in sorted(a.keys() | b.keys()):
                if key not in a or key not in b:
                    differences.append(dict(path=path + "/" + key, before=a.get(key), after=b.get(key)))
                else:
                    walk(a[key], b[key], path + "/" + key)
        elif isinstance(a, list):
            if len(a) != len(b):
                differences.append(dict(path=path, before=a, after=b))
            else:
                for i, (x, y) in enumerate(zip(a, b)):
                    walk(x, y, path + "/" + str(i))
        elif a != b:
            differences.append(dict(path=path, before=a, after=b))

    walk(semantic(before), semantic(after), "patterns")
    return dict(equal=not differences, comparison="queried musical data; excludes scan stamps and dirty state",
                project_fingerprint_changed=before.get("stamp", {}).get("project_fingerprint") != after.get("stamp", {}).get("project_fingerprint"),
                differences=differences)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    read = sub.add_parser("read")
    read.add_argument("--patterns", type=int, nargs="+", required=True)
    read.add_argument("--port")
    read.add_argument("--output-port")
    read.add_argument("--output", type=Path)
    diff = sub.add_parser("compare")
    diff.add_argument("before", type=Path)
    diff.add_argument("after", type=Path)
    args = parser.parse_args()
    try:
        if args.operation == "compare":
            data = compare(json.loads(args.before.read_text(encoding="utf-8")), json.loads(args.after.read_text(encoding="utf-8")))
        else:
            import mido
            inp, out = choose_ports(mido.get_input_names(), mido.get_output_names(), args.port, args.output_port)
            with mido.open_input(inp) as ip, mido.open_output(out) as op:
                time.sleep(1.0)
                data = capture(ip, op, args.patterns, message=mido.Message)
            if args.output:
                # No partial or failed scan is written, and prior snapshots are kept.
                with args.output.open("x", encoding="utf-8") as f:
                    f.write(json.dumps(data, indent=2) + "\n")
        print(json.dumps(data, indent=2))
    except (ImportError, OSError, ValueError, KeyError, ReadbackError) as error:
        parser.exit(1, "Pattern read failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
