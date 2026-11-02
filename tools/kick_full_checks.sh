#!/bin/sh
set -eu
mkdir -p build/kick-tests
python3 lab/build_emulator.py --native >build/kick-wasm-build.log 2>&1
sh tools/kick_ui_checks.sh >build/kick-ui-checks.log 2>&1
sh tools/acultur8_checks.sh >build/kick-required.log 2>&1
python3 tests/target_budget.py build/kick-target/x0x.dis tests/target_budget.json --report build/kick-target/budget.json >build/kick-budget.log 2>&1
printf 'KICK_FULL_CHECKS_PASS
'
