#!/bin/sh
set -eu
mkdir -p build/kick-tests
cc -O2 -ffp-contract=off -std=c99 -Ifirmware/src/dsp -c tests/preset-oracle/oracle.c -o build/kick-tests/p35.o
cc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off -std=c99 -Ifirmware/src/dsp tests/host/kick_saved_sound_test.c firmware/src/dsp/drum808.c firmware/src/dsp/sound_strip.c build/kick-tests/p35.o -lm -o build/kick-tests/saved-sound
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 build/kick-tests/saved-sound
