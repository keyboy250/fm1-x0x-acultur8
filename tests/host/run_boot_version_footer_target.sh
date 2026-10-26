#!/bin/sh
set -eu
mkdir -p /tmp/boot-version-footer-target /tmp/boot-version-footer-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/boot-version-footer-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/boot-version-footer-jieli
cd /tmp/boot-version-footer-target
export JIELI_TOOLCHAIN=/tmp/boot-version-footer-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/boot-version-footer/target
python3 tools/build.py --release 0.1.21-verbeta --sdk /sdk >/work/build/boot-version-footer/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/boot-version-footer/target/budget.json >/work/build/boot-version-footer/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.21-verbeta.fwsc /work/build/boot-version-footer/target/
printf 'LOCAL BOOT VERSION FOOTER CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/boot-version-footer/target/NOT_FOR_FLASHING.txt
printf 'BOOT VERSION FOOTER TARGET BUILD AND UNCHANGED BUDGET PASS\n'
