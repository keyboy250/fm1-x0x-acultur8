#!/bin/sh
set -eu
mkdir -p build/midi-copy
sh tools/midi_freeze_check.sh >build/midi-copy/inherited-full.log 2>&1
sh tests/host/run_midi_copy.sh >build/midi-copy/native.log 2>&1
python3 tests/test_copy_readback.py >build/midi-copy/wire.log 2>&1
for suite in pattern_copy cross_part_copy;do
 sh tests/host/run_${suite}.sh >build/midi-copy/inherited-${suite}.log 2>&1
done
python3 tools/midi_copy_fixture_plans.py >build/midi-copy/plans.log 2>&1
node tests/host/midi_copy_wasm_test.mjs build/emu/x0x.wasm >build/midi-copy/wasm.log 2>&1
printf 'COPY MIDI LOCAL FULL REGRESSION PASS\n'
