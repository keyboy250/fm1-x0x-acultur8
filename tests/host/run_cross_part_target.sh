#!/bin/sh
set -eu
mkdir -p /tmp/cross-copy-target /tmp/cross-copy-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/cross-copy-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/cross-copy-jieli
cd /tmp/cross-copy-target
export JIELI_TOOLCHAIN=/tmp/cross-copy-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/cross-copy/target
python3 tools/build.py --release 0.1.10-crosscopy --sdk /sdk >/work/build/cross-copy/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/cross-copy/target/budget.json >/work/build/cross-copy/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.10-crosscopy.fwsc /work/build/cross-copy/target/
printf 'LOCAL CROSS-PART COPY CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/cross-copy/target/NOT_FOR_FLASHING.txt
printf 'COPY TARGET BUILD AND UNCHANGED BUDGET PASS\n'
