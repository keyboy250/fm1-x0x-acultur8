#!/bin/sh
set -eu
cd /work
mkdir -p build/timing-rate/target /tmp/timing-target /tmp/timing-toolchain
tar --exclude=./build --exclude=./.git -cf - . | tar -xf - -C /tmp/timing-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/timing-toolchain
cd /tmp/timing-target
export JIELI_TOOLCHAIN=/tmp/timing-toolchain/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
export X0X_TIMING_RATE_HZ=44119
python3 tools/build.py --release 0.1.29-timefix --sdk /sdk >/work/build/timing-rate/target/build.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.29-timefix.fwsc /work/build/timing-rate/target/
python3 tests/target_budget.py --report /work/build/timing-rate/target/budget.json >/work/build/timing-rate/target/budget.log 2>&1
printf 'TARGET_TIMING_RATE_BUDGET_PASS\n'
