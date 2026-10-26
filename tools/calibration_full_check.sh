#!/bin/sh
set -eu
cd /work
mkdir -p build/calibration build/midi-variation
X0X_VERSION=0.1.30-calbeta BINARYEN_CORES=1 sh web/emu/build.sh >build/calibration/wasm-build.log 2>&1
python3 lab/build_emulator.py --package-only >build/calibration/browser-package.log 2>&1
sh host/build_host.sh >build/calibration/host-build.log 2>&1
sh tools/midi_copy_check.sh >build/calibration/full-midi.log 2>&1
python3 tests/test_sysex_routing.py >build/calibration/routing.log 2>&1
printf 'FULL_CALIBRATION_REGRESSION_PASS\n'
