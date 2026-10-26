#!/bin/sh
set -eu
cd /work
python3 lab/build_emulator.py --package-only >build/timing-rate/browser-package.log 2>&1
sh tools/midi_copy_check.sh >build/timing-rate/full-midi.log 2>&1
python3 tests/test_sysex_routing.py >build/timing-rate/routing.log 2>&1
printf 'FULL_TIMING_RATE_REGRESSION_PASS\n'
