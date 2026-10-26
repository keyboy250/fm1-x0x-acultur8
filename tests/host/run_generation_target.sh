#!/bin/sh
set -eu
mkdir -p /tmp/generation-target /tmp/generation-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/generation-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/generation-jieli
cd /tmp/generation-target
export JIELI_TOOLCHAIN=/tmp/generation-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/generation/target
python3 tools/build.py --release 0.1.11-genbeta --sdk /sdk >/work/build/generation/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/generation/target/budget.json >/work/build/generation/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.11-genbeta.fwsc /work/build/generation/target/
printf 'LOCAL CONSTRAINED GENERATION CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/generation/target/NOT_FOR_FLASHING.txt
printf 'GENERATION TARGET BUILD AND UNCHANGED BUDGET PASS\n'
