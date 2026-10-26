#!/bin/sh
set -eu
mkdir -p /tmp/midi-303-target /tmp/midi-303-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/midi-303-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/midi-303-jieli
cd /tmp/midi-303-target
export JIELI_TOOLCHAIN=/tmp/midi-303-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/midi-303/target
if ! python3 tools/build.py --release 0.1.23-drumbeta --sdk /sdk >/work/build/midi-303/target/build.log 2>&1; then
    "$JIELI_TOOLCHAIN/common/bin/objdump" -t build/x0x.o >/work/build/midi-303/target/failed-symbols.txt
    exit 1
fi
python3 tests/target_budget.py --report /work/build/midi-303/target/budget.json >/work/build/midi-303/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.23-drumbeta.fwsc /work/build/midi-303/target/
printf 'LOCAL MIDI303 CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/midi-303/target/NOT_FOR_FLASHING.txt
printf 'MIDI303 TARGET BUILD AND UNCHANGED BUDGET PASS\n'
