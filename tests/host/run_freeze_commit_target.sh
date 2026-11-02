#!/bin/sh
set -eu
mkdir -p /tmp/freeze-commit-target /tmp/freeze-commit-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/freeze-commit-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/freeze-commit-jieli
cd /tmp/freeze-commit-target
export JIELI_TOOLCHAIN=/tmp/freeze-commit-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/freeze-commit/target
python3 tools/build.py --release 0.1.16-freezebeta --sdk /sdk >/work/build/freeze-commit/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/freeze-commit/target/budget.json >/work/build/freeze-commit/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.16-freezebeta.fwsc /work/build/freeze-commit/target/
printf 'LOCAL REPEATING VARIATION CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/freeze-commit/target/NOT_FOR_FLASHING.txt
printf 'REPEATING VARIATION TARGET BUILD AND UNCHANGED BUDGET PASS\n'
