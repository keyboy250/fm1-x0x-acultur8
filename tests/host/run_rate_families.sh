#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
sh tools/rate_families_native.sh
cc -std=c99 -O1 -g -fsanitize=address,undefined tests/host/rate_storage_test.c firmware/src/seq/lanes.c firmware/src/seq/rate_slice_codec.c -o build/rate-families/storage
build/rate-families/storage
cc -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-parameter tests/host/rate_timing_test.c tests/host/control_guard_stub.c firmware/src/seq/sequencer.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c firmware/src/seq/harmony.c -lm -o build/rate-families/timing
build/rate-families/timing
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c firmware/src/app/engine.c"
cc -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-field-initializers -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/rate_cc_test.c $SOURCES -lm -o build/rate-families/cc
build/rate-families/cc
python3 tests/pcm_blocks_test.py
node tests/host/rate_wasm_test.mjs build/lane-wasm-oracle/accepted.wasm build/lane-wasm-oracle/accepted-commit.txt
