#!/bin/sh
set -eu
mkdir -p /tmp/random-step-options-target /tmp/random-step-options-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/random-step-options-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/random-step-options-jieli
cd /tmp/random-step-options-target
export JIELI_TOOLCHAIN=/tmp/random-step-options-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/random-step-options/target
python3 tools/build.py --release 0.1.17-randombeta --sdk /sdk >/work/build/random-step-options/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/random-step-options/target/budget.json >/work/build/random-step-options/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.17-randombeta.fwsc /work/build/random-step-options/target/
printf 'LOCAL REPEATING VARIATION CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/random-step-options/target/NOT_FOR_FLASHING.txt
printf 'REPEATING VARIATION TARGET BUILD AND UNCHANGED BUDGET PASS\n'
