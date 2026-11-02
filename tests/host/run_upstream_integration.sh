#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
cc -std=c99 -O1 -g -ffp-contract=off -fsanitize=address,undefined -fno-omit-frame-pointer -w -DX0X_HOST -D_POSIX_C_SOURCE=200809L -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/upstream_integration_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c firmware/src/app/engine.c -lm -o build/host/upstream_integration_test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 build/host/upstream_integration_test
cc -std=c99 -Wall -Wextra -Werror -Wno-unused-function -O2 tests/backup_test.c -o build/host/backup_test
build/host/backup_test build/host/backup.x0b
python3 tests/x0x_backup_test.py build/host/backup.x0b
python3 tests/x0x_backup_record.py build/host/backup.x0b build/host/backup_transcript.json
node tests/host/backup_page_test.mjs build/host/backup_transcript.json

node tests/host/upstream_accessibility_wasm_test.mjs build/emu/x0x.wasm
