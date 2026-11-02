#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=build/909-review
mkdir -p "$OUT"
git -c safe.directory="$(pwd)" show 99cce17fe7a6d1e3a7a2a89ff4a39baf61b2e458:firmware/src/dsp/drum909.c > "$OUT/reference909.c"
FLAGS='-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen'
if [ "${BATCH_SANITIZE:-0}" = 1 ]; then FLAGS="$FLAGS -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"; fi
PROCESS_RENAME=''
if [ "${BATCH_TRACK_CALLS:-0}" = 1 ]; then FLAGS="$FLAGS -DBATCH_TRACK_CALLS"; PROCESS_RENAME='-Dsound_strip_process=batch_real_process'; fi
RENAME='-Dx0x_pot_exp_value=ref_x0x_pot_exp_value -Dx0x_tanh_tab=ref_x0x_tanh_tab -Dx0x_pot_exp=ref_x0x_pot_exp'
for fn in init trigger render render_st pan_settle nparams param set get active strip_set strip_meter strip_meter_clear bd_strip_set bd_strip_meter; do RENAME="$RENAME -Ddrum909_$fn=ref_drum909_$fn"; done
cc $FLAGS $RENAME -c "$OUT/reference909.c" -o "$OUT/reference909.o"
cc $FLAGS -c firmware/src/dsp/drum909.c -o "$OUT/candidate909.o"
cc $FLAGS $PROCESS_RENAME -c firmware/src/dsp/sound_strip.c -o "$OUT/sound_strip.o"
cc $FLAGS tests/host/d909_batch_test.c "$OUT/reference909.o" "$OUT/candidate909.o" "$OUT/sound_strip.o" -lm -o "$OUT/d909_batch_test"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$OUT/d909_batch_test"
