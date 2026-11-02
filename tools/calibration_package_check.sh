#!/bin/sh
set -eu
cd /work
cc -std=c99 -O1 -g -w -fsanitize=address,undefined -fno-omit-frame-pointer tests/ota_test.c -o build/calibration/ota-test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 build/calibration/ota-test build/calibration/target/x0x-0.1.30-calbeta.fwsc >build/calibration/updater-simulation.log 2>&1
for hz in 44100 44118 44119 43900 44300;do
 cc -std=c99 -O2 -w -DX0X_TIMING_RATE_HZ=$hz tests/host/timing_rate_math_test.c -lm -o build/calibration/math-$hz
 build/calibration/math-$hz
 cc -std=c99 -O1 -g -w -DX0X_TIMING_RATE_HZ=$hz -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/src tests/host/timing_rate_deadline_test.c -lm -o build/calibration/deadline-$hz
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/calibration/deadline-$hz
 done
printf 'CALIBRATION_PACKAGE_AND_DEADLINES_PASS\n'
