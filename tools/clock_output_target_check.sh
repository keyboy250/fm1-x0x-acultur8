#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd /work
mkdir -p build/clock-output/target /tmp/clock-target /tmp/clock-toolchain
tar --exclude=./build --exclude=./.git -cf - . | tar -xf - -C /tmp/clock-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/clock-toolchain
cd /tmp/clock-target
export JIELI_TOOLCHAIN=/tmp/clock-toolchain/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
python3 tools/build.py --release 0.1.28-clockfix --sdk /sdk >/work/build/clock-output/target/build.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.28-clockfix.fwsc /work/build/clock-output/target/
python3 tests/target_budget.py --report /work/build/clock-output/target/budget.json >/work/build/clock-output/target/budget.log 2>&1
printf 'Target build and unchanged cost budgets PASS\n'
