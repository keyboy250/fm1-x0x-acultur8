#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
mkdir -p build/host
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c firmware/src/app/engine.c"
cc -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -D_POSIX_C_SOURCE=200809L -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/strip_defaults_test.c $SOURCES -lm -o build/host/strip_defaults_test
build/host/strip_defaults_test
cc -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -D_POSIX_C_SOURCE=200809L -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/strip_defaults_atomicity_test.c $SOURCES -lm -o build/host/strip_defaults_atomicity_test
build/host/strip_defaults_atomicity_test
node tests/host/variant_ui_test.mjs
node tests/host/variant_defaults_ui_test.mjs
node tests/host/strip_defaults_wasm_test.mjs build/emu/x0x.wasm
