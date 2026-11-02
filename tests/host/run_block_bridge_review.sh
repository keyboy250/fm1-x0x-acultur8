#!/bin/sh
set -eu
mkdir -p build/bridge-review
python3 tools/cc_inventory.py --write >build/bridge-review/inventory.log 2>&1
python3 tools/gen_cc_bindings.py >>build/bridge-review/inventory.log 2>&1
sh tests/host/run_block_bridge.sh >build/bridge-review/focused.log 2>&1
sh tests/host/run_block_delete.sh >build/bridge-review/deletion-focused.log 2>&1
CFLAGS='-O1 -g -ffp-contract=off -fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/run_block_bridge.sh >build/bridge-review/sanitizers.log 2>&1
CFLAGS='-O1 -g -ffp-contract=off -fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/run_block_delete.sh >build/bridge-review/deletion-sanitizers.log 2>&1
python3 lab/build_emulator.py --source-root /work --native >build/bridge-review/native-wasm.log 2>&1
sh tools/acultur8_checks.sh >build/bridge-review/required.log 2>&1
printf 'BRIDGE HOST/WASM/REQUIRED PASS\n'
