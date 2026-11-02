#!/bin/sh
set -eu
cd /work
mkdir -p build/calibration
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c"
cc -std=c99 -O1 -g -w -ffp-contract=off -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -fsanitize=address,undefined -fno-omit-frame-pointer tests/host/timing_calibration_test.c $SOURCES -lm -o build/calibration/core-test
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/calibration/core-test
cc -std=c99 -O2 -w -ffp-contract=off -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/timing_calibration_test.c $SOURCES -lm -o build/calibration/wire-test

cc -std=c99 -O1 -g -w -fsanitize=address,undefined -fno-omit-frame-pointer tests/host/calibration_storage_test.c -o build/calibration/storage-test
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/calibration/storage-test

cc -std=c99 -O2 -w -ffp-contract=off -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/calibration_timing_test.c $SOURCES -lm -o build/calibration/runtime-timing-test
build/calibration/runtime-timing-test >build/calibration/runtime-timing.log
