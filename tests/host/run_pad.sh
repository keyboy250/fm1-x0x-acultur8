#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual shared C sequencer, engine, storage and production USB ring acceptance.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/host
python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
sh tools/gen_builtin_break.sh build/gen >/dev/null
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c"
# Combined fixtures must also prove the production engine translation unit compiles independently.
${CC:-cc} ${CFLAGS:-} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -DX0X_HOST -DX0X_WEB -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -c firmware/src/app/engine.c -o build/host/pad_engine_standalone.o
${CC:-cc} ${CFLAGS:-} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-field-initializers -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/pad_test.c $SOURCES -lm -o build/host/pad_test
build/host/pad_test
