#!/bin/sh
set -eu
: "${X0X_EXPECTED_FIRMWARE_ID:?Declare the compiled host/WASM identity independently}"
for feature in 303 drum variation freeze copy; do
 sh tests/host/run_midi_${feature}.sh >build/preset-midi-${feature}-native.log 2>&1
 reader="$feature";if [ "$feature" = 303 ];then reader=generation;fi
 python3 tests/test_${reader}_readback.py >build/preset-query-${reader}.log 2>&1
done
for kit in 0 1;do
 for voice in 0 1 2 3 4 5 6 7 8 9 10;do
  python3 tools/fm1_drum_plan.py --kit "$kit" --voice "$voice" >build/midi-drum/plan-$kit-$voice.json
 done
done
for lane in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23;do
 python3 tools/fm1_variation_plan.py --lane "$lane" >build/midi-variation/plan-$lane-on.json
 python3 tools/fm1_variation_plan.py --lane "$lane" --request-id 2 --off >build/midi-variation/plan-$lane-off.json
done
python3 tools/midi_freeze_fixture_plans.py >build/preset-freeze-plans.log 2>&1
python3 tools/midi_copy_fixture_plans.py >build/preset-copy-plans.log 2>&1
for feature in 303 drum variation freeze copy;do
 node tests/host/midi_${feature}_wasm_test.mjs build/emu/x0x.wasm "$X0X_EXPECTED_FIRMWARE_ID" >build/preset-midi-${feature}.log 2>&1
done
printf 'KICK_PRESETS_INHERITED_PASS\n'
