#!/bin/sh
set -eu
mkdir -p build/midi-variation build/optional-overlay
git clone --mirror /inputs/oracle-history.bundle /tmp/variation-history.git >build/midi-variation/history.log 2>&1
export GIT_DIR=/tmp/variation-history.git
python3 tools/cc_inventory.py --check >build/midi-variation/inventory.log 2>&1
python3 tools/gen_cc_bindings.py --check >build/midi-variation/bindings.log 2>&1
sh tests/host/run_midi_303.sh >build/midi-variation/303-native.log 2>&1
python3 tests/test_generation_readback.py >build/midi-variation/303-wire.log 2>&1
sh tests/host/run_midi_drum.sh >build/midi-variation/drum-native.log 2>&1
python3 tests/test_drum_readback.py >build/midi-variation/drum-wire.log 2>&1
sh tests/host/run_midi_variation.sh >build/midi-variation/native.log 2>&1
python3 tests/test_variation_readback.py >build/midi-variation/wire.log 2>&1
sh tools/acultur8_checks.sh >build/midi-variation/required.log 2>&1
for suite in generation drum_generation repeating_variation 303_variation freeze_commit random_options runtime_diagnostics live_trigger_overlay optional_overlay home_boot;do
 sh tests/host/run_${suite}.sh >build/midi-variation/inherited-${suite}.log 2>&1
done
sh tests/host/run_boot_version_footer.sh >build/midi-variation/boot-footer.log 2>&1
for lane in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23;do
 python3 tools/fm1_variation_plan.py --lane "$lane" >build/midi-variation/plan-$lane-on.json
 python3 tools/fm1_variation_plan.py --lane "$lane" --request-id 2 --off >build/midi-variation/plan-$lane-off.json
done
node tests/host/midi_variation_wasm_test.mjs build/emu/x0x.wasm >build/midi-variation/wasm.log 2>&1
sh tools/midi_drum_wasm_check.sh >build/midi-variation/inherited-midi-wasm.log 2>&1
printf 'VARIATION MIDI LOCAL ENGINE/WORKLET/FULL REGRESSION PASS\n'
