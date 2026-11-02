#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p /tmp/sysex-routing-target /tmp/sysex-routing-jieli build/sysex-routing/target
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/sysex-routing-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/sysex-routing-jieli
cd /tmp/sysex-routing-target
export JIELI_TOOLCHAIN=/tmp/sysex-routing-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
python3 tools/build.py --release 0.1.28-inboxfix --sdk /sdk >/work/build/sysex-routing/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/sysex-routing/target/budget.json >/work/build/sysex-routing/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.28-inboxfix.fwsc /work/build/sysex-routing/target/
# Exercise the complete production updater, including successful install simulation,
# cleanup, missing confirmation and timeouts, against this newly built package.
cc -std=c99 -O1 -g -Wall -Wextra -Wno-unused-function -fsanitize=address,undefined -fno-omit-frame-pointer tests/ota_test.c -o build/ota_test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 build/ota_test build/x0x-0.1.28-inboxfix.fwsc >/work/build/sysex-routing/target/updater-simulation.log 2>&1
printf 'PRIVATE LOCAL CANDIDATE - NOT INSTALLED OR PUBLISHED\n' >/work/build/sysex-routing/target/NOT_FOR_FLASHING.txt
printf 'Target build, unchanged budgets and complete updater simulation PASS\n'
