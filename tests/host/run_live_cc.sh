#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/host
python3 tools/gen_cc_bindings.py --check
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
[ -f build/gen/x0x_drum_samples.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
sh tools/gen_builtin_break.sh build/gen >/dev/null
${CC:-cc} -std=c99 -w -D_POSIX_C_SOURCE=200809L -DX0X_WEB -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -fsyntax-only web/emu/x0x_web.c
${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
 -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/live_cc_test.c \
 firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c \
 firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c \
 firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
 firmware/src/app/engine.c firmware/src/app/variant.c firmware/src/seq/harmony.c -lm -o build/host/live_cc_test
build/host/live_cc_test
${CC:-cc} -O2 -w -Ifirmware/src -Ifirmware/hal tests/host/live_cc_irq_test.c -o build/host/live_cc_irq_test
build/host/live_cc_irq_test
${CC:-cc} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -DX0X_WEB -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
 tests/host/live_cc_debt_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c \
 firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c \
 firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
 firmware/src/seq/harmony.c firmware/src/app/variant.c -lm -o build/host/live_cc_debt_test
build/host/live_cc_debt_test
