#!/bin/sh
set -eu
mkdir -p build/copy-review
sh tests/host/run_pattern_copy.sh >build/copy-review/focused.log 2>&1
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=halt_on_error=1 CFLAGS='-O1 -g -ffp-contract=off -fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/run_pattern_copy.sh >build/copy-review/sanitizers.log 2>&1
python3 tools/cc_inventory.py --write >build/copy-review/inventory.log 2>&1
python3 tools/gen_cc_bindings.py >>build/copy-review/inventory.log 2>&1
X0X_VERSION=0.1.7-beta1 python3 lab/build_emulator.py --source-root /work --native >build/copy-review/native-wasm.log 2>&1
node tests/host/pattern_copy_wasm_test.mjs build/emu/x0x.wasm >build/copy-review/wasm-menu.log 2>&1
sh tools/acultur8_checks.sh >build/copy-review/required.log 2>&1
printf 'COPY CANDIDATE HOST/WASM/REQUIRED PASS\n'
