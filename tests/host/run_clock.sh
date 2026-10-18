#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/host
python3 tests/control_guard_test.py
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c"
for fixture in clock_roles_test transport_engine_test; do
 flags=""
 if [ "$fixture" = clock_roles_test ]; then flags=-DCLOCK_RUN_REQUEST_TEST_HOOK; fi
 ${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-field-initializers -DX0X_HOST -DX0X_WEB $flags -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen "tests/host/$fixture.c" $SOURCES -lm -o "build/host/$fixture" 2>"build/host/$fixture-compile.log"
 "build/host/$fixture"
done
${CC:-cc} -O2 -std=c99 -Wno-unused-function -Wno-unused-parameter -Ifirmware/src tests/host/clock_usb_consumer_test.c -o build/host/clock_usb_consumer_test
build/host/clock_usb_consumer_test
${CC:-cc} -O2 -std=c99 -ffunction-sections -Wno-unused-function -Wno-unused-parameter -Ifirmware/src tests/host/clock_usb_service_test.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c -Wl,--gc-sections -lm -o build/host/clock_usb_service_test
build/host/clock_usb_service_test
