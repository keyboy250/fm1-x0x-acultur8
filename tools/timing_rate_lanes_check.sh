#!/bin/sh
set -eu
cd /work
for hz in 44100 44118 44119;do
 cc -std=c99 -O2 -w -DX0X_TIMING_RATE_HZ=$hz tests/host/timing_rate_lanes_test.c tests/host/control_guard_stub.c firmware/src/seq/sequencer.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c firmware/src/seq/harmony.c -lm -o build/timing-rate/lanes-$hz
 build/timing-rate/lanes-$hz
 done
cc -std=c99 -O2 -w -DX0X_TIMING_RATE_HZ=44119 tests/host/timing_rate_lanes_test.c tests/host/control_guard_stub.c /parent/firmware/src/seq/sequencer.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c firmware/src/seq/harmony.c -lm -o build/timing-rate/parent-lanes
set +e
(ulimit -c 0;build/timing-rate/parent-lanes) >build/timing-rate/parent-lanes-negative.log 2>&1
result=$?
set -e
test "$result" -ne 0
grep -q 'error<=1' build/timing-rate/parent-lanes-negative.log
printf 'PASS production-parent negative control rejects uncorrected lane timebase\n'
printf 'TIMING_RATE_LANES_PASS\n'
