#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd /work
mkdir -p build/clock-output build/host build/midi-variation
cc -std=c99 -O1 -g -w -Ifirmware/src -fsanitize=address,undefined -fno-omit-frame-pointer tests/host/midi_deadline_test.c -lm -o build/host/midi_deadline_test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 build/host/midi_deadline_test >build/clock-output/deadline.log 2>&1
printf 'Deadline scheduling and lifecycle PASS\n'
X0X_VERSION=0.1.28-clockfix BINARYEN_CORES=1 sh web/emu/build.sh >build/clock-output/wasm-build.log 2>&1
python3 lab/build_emulator.py --package-only >build/clock-output/browser-package.log 2>&1
printf 'Fresh WASM and emulator package PASS\n'
sh tools/midi_copy_check.sh >build/clock-output/full-midi.log 2>&1
printf 'Full inherited MIDI and required app checks PASS\n'
python3 tests/test_sysex_routing.py --parent-source /inputs/parent-source.zip >build/clock-output/routing.log 2>&1
printf 'Inbox routing regression PASS\n'
