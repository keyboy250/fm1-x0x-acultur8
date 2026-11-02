#!/bin/sh
set -eu
mkdir -p /tmp/303-variation-target /tmp/303-variation-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/303-variation-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/303-variation-jieli
cd /tmp/303-variation-target
export JIELI_TOOLCHAIN=/tmp/303-variation-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/303-variation/target
python3 tools/build.py --release 0.1.14-303varbeta --sdk /sdk >/work/build/303-variation/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/303-variation/target/budget.json >/work/build/303-variation/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.14-303varbeta.fwsc /work/build/303-variation/target/
printf 'LOCAL REPEATING VARIATION CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/303-variation/target/NOT_FOR_FLASHING.txt
printf 'REPEATING VARIATION TARGET BUILD AND UNCHANGED BUDGET PASS\n'
