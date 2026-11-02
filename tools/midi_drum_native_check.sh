#!/bin/sh
set -eu
mkdir -p build/midi-drum build/optional-overlay
git clone --mirror /inputs/oracle-history.bundle /tmp/drum-history.git >build/midi-drum/history.log 2>&1
export GIT_DIR=/tmp/drum-history.git
python3 tools/cc_inventory.py --check >build/midi-drum/inventory.log 2>&1
python3 tools/gen_cc_bindings.py --check >build/midi-drum/bindings.log 2>&1
sh tests/host/run_midi_303.sh >build/midi-drum/303-native.log 2>&1
python3 tests/test_generation_readback.py >build/midi-drum/303-wire.log 2>&1
sh tests/host/run_midi_drum.sh >build/midi-drum/native.log 2>&1
python3 tests/test_drum_readback.py >build/midi-drum/wire.log 2>&1
sh tools/acultur8_checks.sh >build/midi-drum/required.log 2>&1
for suite in generation drum_generation repeating_variation 303_variation freeze_commit random_options runtime_diagnostics live_trigger_overlay optional_overlay home_boot; do
 sh tests/host/run_${suite}.sh >build/midi-drum/inherited-${suite}.log 2>&1
done
sh tests/host/run_boot_version_footer.sh >build/midi-drum/boot-footer.log 2>&1
printf 'DRUM MIDI NATIVE / FULL REQUIRED / INHERITED CHECKS PASS\n'
