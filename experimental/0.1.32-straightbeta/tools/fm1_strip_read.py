#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read X0X project strip settings over USB-MIDI. Sends only read-only command48.

python tools/fm1_strip_read.py --strip 23
python tools/fm1_strip_read.py --all --port "X0X FM-1 18"
Requires mido + python-rtmidi. No CC, note, save, sample or update commands.
"""
import argparse
import json
import secrets
import re
import time

HEADER = (0x7D, 0x46, 0x4C, 48)
NAMES = tuple(f"909-{i}" for i in range(11)) + tuple(f"808-{i}" for i in range(11)) + ("303A", "303B", "BREAK")


class ReadbackError(RuntimeError):
    pass


def request(strip, token):
    if type(strip) is not int or not 0 <= strip < 25 or type(token) is not int or not 0 <= token < 128:
        raise ValueError("strip must be0..24 and token0..127")
    return list(HEADER) + [1, token, strip]


def number(data):
    return sum(v << (7 * i) for i, v in enumerate(data))


def decode(data, strip, token):
    data = tuple(data)
    if data[:4] != HEADER:
        return None
    if any(type(v) is not int or not 0 <= v < 128 for v in data):
        raise ReadbackError("Malformed non-7-bit reply")
    if len(data) < 8:
        raise ReadbackError("Truncated strip reply")
    if data[4] != 1:
        raise ReadbackError("Unsupported strip reply version")
    if data[5:7] != (token, strip):
        return None
    status = data[7]
    if status:
        if len(data) != 8 or status not in (1, 2, 3):
            raise ReadbackError("Malformed error reply")
        return {"status": {1: "invalid", 2: "busy", 3: "unsupported"}[status], "strip": strip}
    if len(data) != 29:
        raise ReadbackError("Unexpected strip reply length")
    revision, ticket = number(data[8:13]), number(data[13:18])
    hp, lp = number(data[18:21]), number(data[21:24])
    boost, ceiling, release, flags = data[24], data[25], number(data[26:28]), data[28]
    if not (revision <= 0xFFFFFFFF and ticket <= 0xFFFFFFFF and 20 <= hp <= 20000 and
            20 <= lp <= 20000 and boost <= 24 and ceiling <= 48 and 10 <= release <= 250 and flags <= 15):
        raise ReadbackError("Strip reply values outside protocol bounds")
    return {"status": "ok", "strip": strip, "sound": NAMES[strip], "revision": revision,
            "queue_ticket": ticket, "queue_drained": True, "configuration_source": "project",
            "audio_dsp_readback": False, "highpass_hz": hp, "lowpass_hz": lp,
            "boost_db": boost, "ceiling_dbfs": -ceiling, "release_ms": release, "flags": flags,
            "strip_enabled": bool(flags & 1), "highpass_enabled": bool(flags & 2),
            "lowpass_enabled": bool(flags & 4), "limiter_enabled": bool(flags & 8)}


def read_strip(input_port, output_port, strip, *, message, timeout=1.0, retries=5):
    token = secrets.randbelow(128)
    for attempt in range(retries):
        token = (token + 1) & 127
        output_port.send(message("sysex", data=request(strip, token)))
        end = time.monotonic() + timeout
        busy = False
        while time.monotonic() < end:
            reply = input_port.poll()
            if reply is None:
                time.sleep(0.002)
                continue
            if reply.type != "sysex":
                continue
            result = decode(reply.data, strip, token)
            if result is None:
                continue
            if result["status"] == "busy":
                busy = True
                break
            if result["status"] != "ok":
                raise ReadbackError("Device rejected read-only strip query: " + result["status"])
            return result
        if not busy:
            raise ReadbackError("No strip-query reply; wait for startup or install firmware supporting command48")
        if attempt + 1 < retries:
            time.sleep(0.04)
    raise ReadbackError("Engine edit queue remained busy; no settings reported")


def choose_ports(inputs, outputs, wanted=None, output_wanted=None):
    def key(name):
        return re.sub(r"\s+\d+$", "", name).casefold()
    pairs = [(i, o) for i in dict.fromkeys(inputs) for o in dict.fromkeys(outputs)
             if ("fm-1" in i.casefold() or "x0x" in i.casefold()) and key(i) == key(o)
             and (wanted is None or i == wanted) and (output_wanted is None or o == output_wanted)]
    if len(pairs) != 1:
        raise ReadbackError("Choose one FM1 input/output pair with --port and --output-port; candidates: " + repr(pairs))
    return pairs[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--strip", type=int, choices=range(25), default=23)
    group.add_argument("--all", action="store_true")
    parser.add_argument("--port", help="Exact USB-MIDI port name (required if multiple FM1 ports exist)")
    parser.add_argument("--output-port", help="Exact FM1 MIDI output name if its numeric suffix differs")
    args = parser.parse_args()
    try:
        import mido
        inputs, outputs = mido.get_input_names(), mido.get_output_names()
        port, output_port_name = choose_ports(inputs, outputs, args.port, args.output_port)
        with mido.open_input(port) as input_port, mido.open_output(output_port_name) as output_port:
            rows = [read_strip(input_port, output_port, strip, message=mido.Message)
                    for strip in (range(25) if args.all else [args.strip])]
        if args.all and len({(r["revision"], r["queue_ticket"]) for r in rows}) != 1:
            raise ReadbackError("Settings changed during all-strip scan; repeat without editing")
        print(json.dumps({"port": port, "output_port": output_port_name, "read_only": True, "strips": rows}, indent=2))
    except (ImportError, OSError, ReadbackError) as error:
        parser.exit(1, "Readback failed: " + str(error) + "\n")


if __name__ == "__main__":
    main()
