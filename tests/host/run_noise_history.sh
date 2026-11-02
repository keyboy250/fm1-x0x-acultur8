#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=build/noise_budget
mkdir -p "$OUT" build/gen
git show ccf861366ba59514db9228e069c6b87add967050:firmware/src/dsp/drum909.c > "$OUT/drum909_accepted.c"
git show ccf861366ba59514db9228e069c6b87add967050:firmware/src/dsp/drum909.h > "$OUT/drum909.h"
[ -f build/gen/x0x_drum_samples.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
printf '#include <stddef.h>\n#include "drum909_accepted.c"\nsize_t noise_reference_size(void) { return sizeof(drum909_t); }\n' > "$OUT/reference.c"
FLAGS="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen ${NOISE_CFLAGS:-}"
RENAME="-DX0X_RAW_HH -DX0X_EXP_RAW_REFERENCE -Dx0x_pot_exp_value=legacy_x0x_pot_exp_value -Dx0x_tanh_tab=legacy_x0x_tanh_tab -Dx0x_pot_exp=legacy_x0x_pot_exp"
for fn in init trigger render render_st pan_settle nparams param set get active bd_strip_set bd_strip_meter; do
  RENAME="$RENAME -Ddrum909_$fn=legacy_drum909_$fn"
done
cc $FLAGS $RENAME -c "$OUT/reference.c" -o "$OUT/reference.o"
cc $FLAGS -c firmware/src/dsp/drum909.c -o "$OUT/drum909.o"
cc $FLAGS -c firmware/src/dsp/sound_strip.c -o "$OUT/sound_strip.o"
cc $FLAGS tests/host/noise_history_test.c "$OUT/reference.o" "$OUT/drum909.o" "$OUT/sound_strip.o" -lm -o "$OUT/noise_history_test"
"$OUT/noise_history_test"
