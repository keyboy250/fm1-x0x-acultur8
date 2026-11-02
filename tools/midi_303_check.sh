#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Final local qualification wrapper; run in the pinned emulator builder.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/midi-303 build/optional-overlay
git clone --mirror /inputs/oracle-history.bundle /tmp/midi303-history.git >build/midi-303/history.log 2>&1
export GIT_DIR=/tmp/midi303-history.git
python3 tools/cc_inventory.py --check >build/midi-303/inventory.log 2>&1
python3 tools/gen_cc_bindings.py --check >build/midi-303/bindings.log 2>&1
sh tests/host/run_midi_303.sh >build/midi-303/native.log 2>&1
python3 tests/test_generation_readback.py >build/midi-303/readback.log 2>&1
node tests/host/midi_303_wasm_test.mjs build/emu/x0x.wasm >build/midi-303/wasm.log 2>&1
sh tools/acultur8_checks.sh >build/midi-303/required.log 2>&1
# Later private candidates inherited these feature suites outside the134 baseline.
for suite in generation drum_generation repeating_variation variation303 freeze_commit random_options runtime_diagnostics live_trigger_overlay optional_overlay; do
    script="$suite"
    if [ "$suite" = variation303 ]; then script=303_variation; fi
    sh "tests/host/run_${script}.sh" >"build/midi-303/inherited-${suite}.log" 2>&1
done
for suite in generation drum_generation repeating_variation variation303 freeze_commit random_options runtime_diagnostics live_trigger_overlay optional_overlay; do
    node "tests/host/${suite}_wasm_test.mjs" build/emu/x0x.wasm >"build/midi-303/inherited-${suite}-wasm.log" 2>&1
done
printf 'MIDI303 ALL LOCAL ENGINE/INHERITED CHECKS PASS; target budgets remain a separate gate.\n'
