#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/host build/phase-tests
cc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off -std=c99 -w -D_POSIX_C_SOURCE=200809L -DX0X_HOST -I/parent/firmware/src -I/parent/firmware/src/dsp -I/parent/build/gen -DCONFIRM_PARENT tests/host/kick_confirmation_input_test.c /parent/firmware/src/dsp/drum909.c /parent/firmware/src/dsp/drum808.c /parent/firmware/src/dsp/bass303.c /parent/firmware/src/dsp/breaks.c /parent/firmware/src/dsp/fxbus.c /parent/firmware/src/dsp/master.c /parent/firmware/src/dsp/sound_strip.c /parent/firmware/src/seq/sequencer.c /parent/firmware/src/seq/tb3po.c /parent/firmware/src/seq/pattern.c /parent/firmware/src/seq/motion.c /parent/firmware/src/seq/harmony.c /parent/firmware/src/app/variant.c /parent/firmware/src/app/engine.c -lm -o build/host/kick_confirmation_input_parent_test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 build/host/kick_confirmation_input_parent_test
