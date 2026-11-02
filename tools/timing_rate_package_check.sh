#!/bin/sh
set -eu
cd /work
cc -std=c99 -O1 -g -w -fsanitize=address,undefined -fno-omit-frame-pointer tests/ota_test.c -o build/timing-rate/ota-test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 build/timing-rate/ota-test build/timing-rate/target/x0x-0.1.29-timefix.fwsc >build/timing-rate/updater-simulation.log 2>&1
for invalid in 42999 45001;do
 set +e
 cc -std=c99 -DX0X_TIMING_RATE_HZ=$invalid tests/host/timing_rate_math_test.c -lm -o build/timing-rate/invalid-profile >build/timing-rate/invalid-$invalid.log 2>&1
 result=$?
 set -e
 test "$result" -ne 0
 grep -q 'must be an integer physical audio rate' build/timing-rate/invalid-$invalid.log
 done
printf 'UPDATER_AND_PROFILE_GUARDS_PASS\n'
