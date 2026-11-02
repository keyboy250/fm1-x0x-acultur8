#!/bin/sh
set -eu
mkdir -p build/kick-tests
cc -O2 -ffp-contract=off -std=c99 -Ifirmware/src/dsp -c tests/shape-oracle/oracle.c -o build/kick-tests/shape-oracle.o
cc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off -std=c99 -Ifirmware/src/dsp tests/host/kick_shape_test.c firmware/src/dsp/sound_strip.c build/kick-tests/shape-oracle.o -lm -o build/kick-tests/shape-test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 build/kick-tests/shape-test
