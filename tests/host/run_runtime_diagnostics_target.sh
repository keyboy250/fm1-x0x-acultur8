#!/bin/sh
set -eu
mkdir -p /tmp/runtime-diagnostics-target /tmp/runtime-diagnostics-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/runtime-diagnostics-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/runtime-diagnostics-jieli
cd /tmp/runtime-diagnostics-target
export JIELI_TOOLCHAIN=/tmp/runtime-diagnostics-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/runtime-diagnostics/target
python3 tools/build.py --release 0.1.18-diagbeta --sdk /sdk >/work/build/runtime-diagnostics/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/runtime-diagnostics/target/budget.json >/work/build/runtime-diagnostics/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.18-diagbeta.fwsc /work/build/runtime-diagnostics/target/
printf 'LOCAL REPEATING VARIATION CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/runtime-diagnostics/target/NOT_FOR_FLASHING.txt
printf 'REPEATING VARIATION TARGET BUILD AND UNCHANGED BUDGET PASS\n'
