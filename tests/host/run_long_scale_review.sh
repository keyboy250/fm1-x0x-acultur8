#!/bin/sh
set -eu
mkdir -p build/long-scale-review
python3 tools/cc_inventory.py --write >build/long-scale-review/inventory.log 2>&1
python3 tools/gen_cc_bindings.py >>build/long-scale-review/inventory.log 2>&1
sh tests/host/run_long_scale.sh >build/long-scale-review/focused.log 2>&1
CFLAGS='-O1 -g -ffp-contract=off -fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/run_long_scale.sh >build/long-scale-review/sanitizers.log 2>&1
python3 lab/build_emulator.py --source-root /work --native >build/long-scale-review/native-wasm.log 2>&1
node tests/host/long_scale_wasm_test.mjs build/emu/x0x.wasm >build/long-scale-review/wasm.log 2>&1
sh tools/acultur8_checks.sh >build/long-scale-review/required.log 2>&1
printf 'LONG SCALE HOST/WASM/REQUIRED PASS\n'
