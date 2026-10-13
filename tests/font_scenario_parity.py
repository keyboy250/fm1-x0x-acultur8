#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare original/raw and RLE simulator screenshots for complete UI scenarios.

  python3 tests/font_scenario_parity.py RAW_HOST RLE_HOST OUTPUT
Build both executables from identical sources except generated font compression.
The original unmodified renderer is also suitable as RAW_HOST.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SCENARIOS = ("tour", "help", "stereo", "getting_started", "patterns")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("raw_host", type=Path)
    ap.add_argument("rle_host", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    # Keep evidence from earlier runs, but never allow stale PNGs to pass a new run.
    run = Path(tempfile.mkdtemp(prefix="run-", dir=output))
    records = []
    for scenario in SCENARIOS:
        for mode, host in (("raw", args.raw_host), ("rle", args.rle_host)):
            destination = run / mode / scenario
            destination.mkdir(parents=True, exist_ok=True)
            with (destination / "scenario.log").open("w") as log:
                subprocess.run([str(host.resolve()), str(ROOT / "tests/scenarios" / (scenario + ".x0x")),
                                str(destination)], stdout=log, stderr=subprocess.STDOUT, check=True, cwd=ROOT)
        raw = run / "raw" / scenario
        rle = run / "rle" / scenario
        files = {p.name for p in raw.glob("*.png")}
        other_files = {p.name for p in rle.glob("*.png")}
        if not files or files != other_files:
            raise RuntimeError(f"{scenario}: screenshot coverage differs")
        for name in sorted(files):
            with Image.open(raw / name) as a, Image.open(rle / name) as b:
                if a.size != b.size or a.convert("RGB").tobytes() != b.convert("RGB").tobytes():
                    raise RuntimeError(f"{scenario}/{name}: screenshot pixels differ")
                records.append({"image": scenario + "/" + name, "size": list(a.size),
                                "rgb_sha256": hashlib.sha256(a.convert("RGB").tobytes()).hexdigest()})
    report = {"status": "pass", "scenarios": list(SCENARIOS), "screenshots": records}
    (output / "font-scenario-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"font scenarios: {len(SCENARIOS)} passed; {len(records)} screenshots pixel-identical")


if __name__ == "__main__":
    main()
