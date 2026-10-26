#!/bin/sh
set -eu
mkdir -p /tmp/optional-overlay-target /tmp/optional-overlay-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/optional-overlay-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/optional-overlay-jieli
cd /tmp/optional-overlay-target
export JIELI_TOOLCHAIN=/tmp/optional-overlay-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/optional-overlay/target
python3 tools/build.py --release 0.1.20-optbeta --sdk /sdk >/work/build/optional-overlay/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/optional-overlay/target/budget.json >/work/build/optional-overlay/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.20-optbeta.fwsc /work/build/optional-overlay/target/
printf 'LOCAL OPTIONAL OVERLAY CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/optional-overlay/target/NOT_FOR_FLASHING.txt
printf 'OPTIONAL OVERLAY TARGET BUILD AND UNCHANGED BUDGET PASS\n'
