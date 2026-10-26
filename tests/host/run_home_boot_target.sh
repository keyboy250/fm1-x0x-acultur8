#!/bin/sh
set -eu
mkdir -p /tmp/home-boot-target /tmp/home-boot-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/home-boot-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/home-boot-jieli
cd /tmp/home-boot-target
export JIELI_TOOLCHAIN=/tmp/home-boot-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/home-boot/target
python3 tools/build.py --release 0.1.15-homebeta --sdk /sdk >/work/build/home-boot/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/home-boot/target/budget.json >/work/build/home-boot/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.15-homebeta.fwsc /work/build/home-boot/target/
printf 'LOCAL REPEATING VARIATION CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/home-boot/target/NOT_FOR_FLASHING.txt
printf 'REPEATING VARIATION TARGET BUILD AND UNCHANGED BUDGET PASS\n'
