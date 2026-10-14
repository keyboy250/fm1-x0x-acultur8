#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=build/all909_strip
mkdir -p "$OUT" build/gen
BASE=628a9afa5ccac09ffbd87e0d65763251f744b003
git show "$BASE":firmware/src/dsp/drum909.c > "$OUT/drum909_accepted.c"
git show "$BASE":firmware/src/dsp/drum909.h > "$OUT/drum909.h"
[ -f build/gen/x0x_drum_samples.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
cat > "$OUT/reference.c" <<'EOF'
#include <stddef.h>
#include "drum909_accepted.c"
size_t all909_reference_size(void) { return sizeof(drum909_t); }
void all909_reference_silence(drum909_t *d,int v) {
    if(v<=DR_SD)d->bt[v].mute=0;
    else if(v<=DR_HT)d->tom[v-DR_LT].mute=0;
    else if(v==DR_RS)d->rim.mute=0;
    else if(v==DR_CP)d->clap.mute=0;
    else d->smp[v-DR_CH].mute=0;
}
EOF
FLAGS="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen ${ALL909_CFLAGS:-}"
RENAME="-DX0X_EXP_RAW_REFERENCE -Dx0x_pot_exp_value=legacy_x0x_pot_exp_value -Dx0x_tanh_tab=legacy_x0x_tanh_tab -Dx0x_pot_exp=legacy_x0x_pot_exp"
for fn in init trigger render render_st pan_settle nparams param set get active bd_strip_set bd_strip_meter; do
  RENAME="$RENAME -Ddrum909_$fn=legacy_drum909_$fn"
done
cc $FLAGS $RENAME -c "$OUT/reference.c" -o "$OUT/reference.o"
cc $FLAGS -c firmware/src/dsp/drum909.c -o "$OUT/drum909.o"
cc $FLAGS -c firmware/src/dsp/sound_strip.c -o "$OUT/sound_strip.o"
cc $FLAGS tests/host/all909_strip_test.c "$OUT/reference.o" "$OUT/drum909.o" "$OUT/sound_strip.o" -lm -o "$OUT/all909_strip_test"
"$OUT/all909_strip_test"
size "$OUT/drum909.o" "$OUT/sound_strip.o"
