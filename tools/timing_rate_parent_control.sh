#!/bin/sh
set -eu
cd /parent
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c"
cc -std=c99 -O1 -g -w -ffp-contract=off -DX0X_HOST -DX0X_WEB -DCLOCK_RUN_REQUEST_TEST_HOOK -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/clock_roles_test.c $SOURCES -lm -o /work/build/timing-rate/parent-roles
set +e
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /work/build/timing-rate/parent-roles >/work/build/timing-rate/parent-roles-ubsan.log 2>&1
result=$?
set -e
test "$result" -ne 0
grep -q 'left shift of 14 by 28' /work/build/timing-rate/parent-roles-ubsan.log
printf 'PASS frozen-parent negative control reproduces pre-existing signed clock opcode shift\n'
