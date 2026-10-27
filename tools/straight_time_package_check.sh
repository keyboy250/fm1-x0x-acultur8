#!/bin/sh
set -eu
mkdir -p build/straight-time
cc -std=c99 -O1 -g -w -fsanitize=address,undefined -fno-omit-frame-pointer tests/ota_test.c -o build/straight-time/ota-test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 build/straight-time/ota-test build/straight-time/target/x0x-0.1.32-straightbeta.fwsc >build/straight-time/updater-simulation.log 2>&1
for hz in 43900 44100 44120 44300;do
 cc -std=c99 -O2 -w -DX0X_TIMING_RATE_HZ=$hz tests/host/timing_rate_math_test.c -lm -o build/straight-time/profile-$hz
 build/straight-time/profile-$hz
 cc -std=c99 -O2 -w -DX0X_TIMING_RATE_HZ=$hz tests/host/timing_rate_lanes_test.c tests/host/control_guard_stub.c firmware/src/seq/sequencer.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c firmware/src/seq/harmony.c -lm -o build/straight-time/calibrated-lanes-$hz
 build/straight-time/calibrated-lanes-$hz
done
printf 'STRAIGHT_PACKAGE_UPDATER_CALIBRATED_LANES_PASS\n'
