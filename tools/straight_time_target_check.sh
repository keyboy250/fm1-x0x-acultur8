#!/bin/sh
set -eu
cd /work
mkdir -p build/straight-time/target /tmp/straight-target /tmp/straight-toolchain
tar --exclude=./build --exclude=./.git -cf - . | tar -xf - -C /tmp/straight-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/straight-toolchain
cd /tmp/straight-target
export JIELI_TOOLCHAIN=/tmp/straight-toolchain/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
export X0X_TIMING_RATE_HZ=44100
python3 tools/build.py --release 0.1.32-straightbeta --sdk /sdk >/work/build/straight-time/target/build.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.32-straightbeta.fwsc /work/build/straight-time/target/
python3 tests/target_budget.py --report /work/build/straight-time/target/budget.json >/work/build/straight-time/target/budget.log 2>&1
printf 'STRAIGHT_TIME_TARGET_PASS\n'
