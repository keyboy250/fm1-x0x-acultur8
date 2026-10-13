#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Required native pixel parity against locally available pinned Git sources.

No network or downloaded fixtures: absent pinned history is a hard failure.
Each invocation extracts a new temporary archive under ignored build/ and uses
the original generator AND renderer, independently of the candidate versions.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PIN = "e78ee13c60116e5b352d73b537cdf1f0980d9d8e"
PIXELS = "e971afab542cff352576dc573b4459322a7e2bce477c4bcaa8589ec1c70290d8"
PINNED_HASHES = {
    "tools/gen_font.py": "3ac6e6b6246733313ef254d4441877245ef46fdbd17266708c3b591edbb4eb61",
    "firmware/src/gfx.c": "7fc1abdeef80b9155a8e248feaebb6e932527d0e62deb381af4c74292e38fc9e",
}


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    git = ["git", "-c", "safe.directory=" + str(ROOT), "-C", str(ROOT)]
    subprocess.run([*git, "cat-file", "-e", PIN + "^{commit}"], check=True)
    parent = ROOT / "build/font-pinned-oracle"
    parent.mkdir(parents=True, exist_ok=True)
    cc = shlex.split(os.environ.get("CC", "cc"))
    report = {"git_commit": PIN, "result": {}}
    with tempfile.TemporaryDirectory(prefix="source.", dir=parent) as temporary:
        work = Path(temporary)
        baseline = work / "original"
        baseline.mkdir()
        archive = work / "source.tar"
        subprocess.run([*git, "archive", "--format=tar", "--output=" + str(archive),
                        PIN, "tools/gen_font.py", "firmware/src/gfx.c", "assets/fonts"], check=True)
        subprocess.run(["tar", "-xf", str(archive), "-C", str(baseline)], check=True)
        for relative, expected in PINNED_HASHES.items():
            if sha256(baseline / relative) != expected:
                raise RuntimeError("pinned source hash mismatch: " + relative)
        metrics = {}
        for name, mode, source in (("original_raw", "raw", baseline),
                                   ("original_rle", "rle", baseline),
                                   ("candidate_huffman", "huffman", ROOT)):
            folder = work / name
            folder.mkdir()
            header = folder / "felucca_font.h"
            subprocess.run([sys.executable, str(source / "tools/gen_font.py"), str(header)],
                           env=dict(os.environ, X0X_FONTSET="barlow", X0X_FONT_COMPRESSION=mode),
                           check=True, stdout=subprocess.PIPE, text=True)
            # Original gfx.c references are compiled independently from candidate.
            executable = folder / "font_render"
            subprocess.run([*cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unused-function", "-I" + str(folder),
                            '-DFONT_GFX_SOURCE="' + str(source / "firmware/src/gfx.c") + '"',
                            str(ROOT / "tests/host/font_render_test.c"), "-o", str(executable)], check=True)
            pixels = folder / "pixels.bin"
            with pixels.open("wb") as output:
                result = subprocess.run([str(executable)], stdout=output, stderr=subprocess.PIPE,
                                        check=True, text=True)
            measured = {"pixels_sha256": sha256(pixels), "bytes": pixels.stat().st_size,
                        "result": result.stderr.strip()}
            if measured != {"pixels_sha256": PIXELS, "bytes": 17191240,
                            "result": "frames=1756 checksum=183297732"}:
                raise RuntimeError("independent font pixel oracle mismatch: " + name)
            report["result"][name] = measured
            metrics[name] = {field: body for field, body in re.findall(
                r"FONT_(\w+_(?:ADV|BW))\[\d+\] = \{(.*?)\};", header.read_text(), re.S)}
        if not metrics["original_raw"] == metrics["original_rle"] == metrics["candidate_huffman"]:
            raise RuntimeError("independent font metric oracle mismatch")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
