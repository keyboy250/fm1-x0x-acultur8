#!/bin/sh
set -eu
mkdir -p build/bridge-review /tmp/bridge-jieli
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/bridge-jieli
export JIELI_TOOLCHAIN=/tmp/bridge-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
# Same version flags as the published baseline for resource comparison only.
printf 'ISOLATED BLOCK BRIDGE REVIEW BUILD — NOT APPROVED FOR FLASHING OR PUBLICATION\n' >build/NOT_FOR_FLASHING.txt
python3 tools/build.py --release 0.1.5-beta1 --sdk /sdk >build/bridge-review/target-build.log 2>&1
python3 tests/target_budget.py --report build/bridge-review/target-budget.json >build/bridge-review/target-budget.log 2>&1
printf 'BLOCK BRIDGE TARGET BUILD/BUDGET PASS\n'
