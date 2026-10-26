#!/bin/sh
set -eu
mkdir -p /tmp/midi-drum-target /tmp/midi-drum-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/midi-drum-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/midi-drum-jieli
cd /tmp/midi-drum-target
export JIELI_TOOLCHAIN=/tmp/midi-drum-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/midi-drum/target
if ! python3 tools/build.py --release 0.1.24-varmidi --sdk /sdk >/work/build/midi-drum/target/build.log 2>&1; then
    "$JIELI_TOOLCHAIN/common/bin/objdump" -t build/x0x.o >/work/build/midi-drum/target/failed-symbols.txt
    exit 1
fi
python3 tests/target_budget.py --report /work/build/midi-drum/target/budget.json >/work/build/midi-drum/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.24-varmidi.fwsc /work/build/midi-drum/target/
printf 'LOCAL MIDIDRUM CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/midi-drum/target/NOT_FOR_FLASHING.txt
printf 'MIDIDRUM TARGET BUILD AND UNCHANGED BUDGET PASS\n'
