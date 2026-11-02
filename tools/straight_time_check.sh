#!/bin/sh
set -eu
mkdir -p build/straight-time build/gen
sh host/build_host.sh >build/straight-time/host-build.log 2>&1
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c"
FLAGS="-std=c99 -O1 -g -w -ffp-contract=off -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -fsanitize=address,undefined -fno-omit-frame-pointer"
export ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
export UBSAN_OPTIONS=halt_on_error=1
cc $FLAGS tests/host/straight_time_timer_test.c tests/host/control_guard_stub.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c firmware/src/seq/harmony.c -lm -o build/straight-time/carry
build/straight-time/carry >build/straight-time/carry.log 2>&1
cc $FLAGS tests/host/straight_time_test.c $SOURCES firmware/src/app/engine.c -lm -o build/straight-time/controls 2>build/straight-time/controls-build.log
build/straight-time/controls >build/straight-time/controls.log 2>&1
cat build/straight-time/controls.log
sh tools/rate_families_native.sh >build/straight-time/rate-regressions.log 2>&1
cc $FLAGS tests/host/rate_storage_test.c firmware/src/seq/lanes.c firmware/src/seq/rate_slice_codec.c -o build/straight-time/storage
build/straight-time/storage >>build/straight-time/rate-regressions.log 2>&1
cc $FLAGS tests/host/rate_timing_test.c tests/host/control_guard_stub.c firmware/src/seq/sequencer.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c firmware/src/seq/harmony.c -lm -o build/straight-time/timing
build/straight-time/timing >>build/straight-time/rate-regressions.log 2>&1
cc $FLAGS tests/host/straight_time_timing_test.c tests/host/control_guard_stub.c firmware/src/seq/sequencer.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c firmware/src/seq/harmony.c -lm -o build/straight-time/long-timing
build/straight-time/long-timing >build/straight-time/long-timing.log 2>&1
cc $FLAGS tests/host/rate_cc_test.c $SOURCES firmware/src/app/engine.c -lm -o build/straight-time/cc
build/straight-time/cc >>build/straight-time/rate-regressions.log 2>&1
printf 'STRAIGHT_TIME_FOCUSED_PASS\n'
