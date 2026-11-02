#!/bin/sh
set -eu
mkdir -p /tmp/live-trigger-overlay-target /tmp/live-trigger-overlay-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/live-trigger-overlay-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/live-trigger-overlay-jieli
cd /tmp/live-trigger-overlay-target
export JIELI_TOOLCHAIN=/tmp/live-trigger-overlay-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/live-trigger-overlay/target
python3 tools/build.py --release 0.1.19-livebeta --sdk /sdk >/work/build/live-trigger-overlay/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/live-trigger-overlay/target/budget.json >/work/build/live-trigger-overlay/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.19-livebeta.fwsc /work/build/live-trigger-overlay/target/
printf 'LOCAL LIVE TRIGGER OVERLAY CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/live-trigger-overlay/target/NOT_FOR_FLASHING.txt
printf 'LIVE TRIGGER OVERLAY TARGET BUILD AND UNCHANGED BUDGET PASS\n'
