#!/bin/sh
set -eu
cd /work
mkdir -p build/timing-rate build/midi-variation
X0X_VERSION=0.1.29-timefix BINARYEN_CORES=1 sh web/emu/build.sh >build/timing-rate/wasm-build.log 2>&1
python3 lab/build_emulator.py --package-only >build/timing-rate/browser-package.log 2>&1
sh host/build_host.sh >build/timing-rate/host-build.log 2>&1
sh tools/midi_copy_check.sh >build/timing-rate/full-midi.log 2>&1
python3 tests/test_sysex_routing.py >build/timing-rate/routing.log 2>&1
printf 'FULL_TIMING_RATE_REGRESSION_PASS\n'
