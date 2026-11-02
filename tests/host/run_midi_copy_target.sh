#!/bin/sh
set -eu
mkdir -p /tmp/midi-copy-target /tmp/midi-copy-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/midi-copy-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/midi-copy-jieli
cd /tmp/midi-copy-target
export JIELI_TOOLCHAIN=/tmp/midi-copy-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/midi-copy/target
if ! python3 tools/build.py --release 0.1.26-copymidi --sdk /sdk >/work/build/midi-copy/target/build.log 2>&1; then
    "$JIELI_TOOLCHAIN/common/bin/objdump" -t build/x0x.o >/work/build/midi-copy/target/failed-symbols.txt
    exit 1
fi
python3 tests/target_budget.py --report /work/build/midi-copy/target/budget.json >/work/build/midi-copy/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.26-copymidi.fwsc /work/build/midi-copy/target/
printf 'LOCAL MIDICOPY CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/midi-copy/target/NOT_FOR_FLASHING.txt
printf 'MIDICOPY TARGET BUILD AND UNCHANGED BUDGET PASS\n'
