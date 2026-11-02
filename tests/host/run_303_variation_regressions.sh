#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
out=build/303-variation
mkdir -p "$out"
sh tests/host/run_303_variation_target.sh
sh tools/acultur8_checks.sh >"$out/required-checks.log" 2>&1
sh tests/host/run_repeating_variation.sh >"$out/inherited-variation.log" 2>&1
python3 tests/test_variation_read.py >>"$out/inherited-variation.log" 2>&1
node tests/host/repeating_variation_wasm_test.mjs build/emu/x0x.wasm >>"$out/inherited-variation.log" 2>&1
sh tests/host/run_drum_generation.sh >"$out/inherited-drum.log" 2>&1
python3 tests/test_drum_generation_read.py >>"$out/inherited-drum.log" 2>&1
node tests/host/drum_generation_wasm_test.mjs build/emu/x0x.wasm >>"$out/inherited-drum.log" 2>&1
sh tests/host/run_generation.sh >"$out/inherited-generation.log" 2>&1
python3 tests/test_generation_read.py >>"$out/inherited-generation.log" 2>&1
node tests/host/generation_wasm_test.mjs build/emu/x0x.wasm >>"$out/inherited-generation.log" 2>&1
sh tests/host/run_cross_part_copy.sh >"$out/inherited-copy.log" 2>&1
python3 tests/test_cross_part_read.py >>"$out/inherited-copy.log" 2>&1
node tests/host/cross_part_copy_wasm_test.mjs build/emu/x0x.wasm >>"$out/inherited-copy.log" 2>&1
node tests/host/pattern_copy_wasm_test.mjs build/emu/x0x.wasm >>"$out/inherited-copy.log" 2>&1
printf '303 VARIATION FULL LOCAL REGRESSIONS PASS\n'
