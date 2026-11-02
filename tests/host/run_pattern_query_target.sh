#!/bin/sh
set -eu
mkdir -p /tmp/query-target /tmp/query-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git -cf - . | tar -xf - -C /tmp/query-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/query-jieli
cd /tmp/query-target
export JIELI_TOOLCHAIN=/tmp/query-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/query-review/target
python3 tools/build.py --release 0.1.9-query --sdk /sdk >/work/build/query-review/target/build.log 2>&1
python3 tests/target_budget.py --report /work/build/query-review/target/budget.json >/work/build/query-review/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.9-query.fwsc /work/build/query-review/target/
printf 'LOCAL READ-ONLY QUERY CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/query-review/target/NOT_FOR_FLASHING.txt
printf 'QUERY TARGET BUILD AND UNCHANGED BUDGET PASS\n'
