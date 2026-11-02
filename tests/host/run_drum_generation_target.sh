#!/bin/sh
set -eu
mkdir -p /tmp/drum-generation-target /tmp/drum-generation-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/drum-generation-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/drum-generation-jieli
cd /tmp/drum-generation-target
export JIELI_TOOLCHAIN=/tmp/drum-generation-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/drum-generation/target
python3 tools/build.py --release 0.1.12-drumbeta --sdk /sdk >/work/build/drum-generation/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/drum-generation/target/budget.json >/work/build/drum-generation/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.12-drumbeta.fwsc /work/build/drum-generation/target/
printf 'LOCAL DRUM GENERATION CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/drum-generation/target/NOT_FOR_FLASHING.txt
printf 'DRUM GENERATION TARGET BUILD AND UNCHANGED BUDGET PASS\n'
