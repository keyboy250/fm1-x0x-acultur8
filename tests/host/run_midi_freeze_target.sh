#!/bin/sh
set -eu
mkdir -p /tmp/midi-freeze-target /tmp/midi-freeze-jieli
# Build a second disposable tree so the emulator regressions retain their inputs.
tar --exclude=./build --exclude=./.git --exclude=./core -cf - . | tar -xf - -C /tmp/midi-freeze-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/midi-freeze-jieli
cd /tmp/midi-freeze-target
export JIELI_TOOLCHAIN=/tmp/midi-freeze-jieli/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
mkdir -p build /work/build/midi-freeze/target
if ! python3 tools/build.py --release 0.1.25-freezemidi --sdk /sdk >/work/build/midi-freeze/target/build.log 2>&1; then
    "$JIELI_TOOLCHAIN/common/bin/objdump" -t build/x0x.o >/work/build/midi-freeze/target/failed-symbols.txt
    exit 1
fi
python3 tests/target_budget.py --report /work/build/midi-freeze/target/budget.json >/work/build/midi-freeze/target/budget.log 2>&1
cp build/x0x.bin build/x0x.dis build/x0x.elf build/x0x-0.1.25-freezemidi.fwsc /work/build/midi-freeze/target/
printf 'LOCAL MIDIFREEZE CANDIDATE ONLY - NOT APPROVED FOR FLASHING OR PUBLICATION\n' >/work/build/midi-freeze/target/NOT_FOR_FLASHING.txt
printf 'MIDIFREEZE TARGET BUILD AND UNCHANGED BUDGET PASS\n'
