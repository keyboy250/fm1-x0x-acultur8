#!/bin/sh
set -eu
mkdir -p build/midi-drum
for kit in 0 1;do
 for voice in 0 1 2 3 4 5 6 7 8 9 10;do
  python3 tools/fm1_drum_plan.py --kit "$kit" --voice "$voice" >build/midi-drum/plan-$kit-$voice.json
 done
done
node tests/host/midi_drum_wasm_test.mjs build/emu/x0x.wasm >build/midi-drum/wasm.log 2>&1
node tests/host/midi_303_wasm_test.mjs build/emu/x0x.wasm >build/midi-drum/303-wasm.log 2>&1
for suite in generation drum_generation repeating_variation variation303 freeze_commit random_options runtime_diagnostics live_trigger_overlay optional_overlay home_boot; do
 node tests/host/${suite}_wasm_test.mjs build/emu/x0x.wasm >build/midi-drum/inherited-${suite}-wasm.log 2>&1
done
printf 'DRUM MIDI PRODUCTION WASM / INHERITED CHECKS PASS\n'
