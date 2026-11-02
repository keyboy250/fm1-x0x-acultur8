#!/bin/sh
set -eu
mkdir -p /tmp/copy-target /tmp/copy-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git -cf - . | tar -xf - -C /tmp/copy-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/copy-jieli
cd /tmp/copy-target
export JIELI_TOOLCHAIN=/tmp/copy-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/copy-review/target
python3 tools/build.py --release 0.1.8-candidate --sdk /sdk >/work/build/copy-review/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/copy-review/target/budget.json >/work/build/copy-review/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.8-candidate.fwsc /work/build/copy-review/target/
printf 'LOCAL COPY CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/copy-review/target/NOT_FOR_FLASHING.txt
printf 'COPY TARGET BUILD AND UNCHANGED BUDGET PASS\n'
