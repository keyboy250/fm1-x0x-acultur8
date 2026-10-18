#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Optional first argument: preserved pre-integration drum909.c for independent bitwise oracle.
set -eu
cd "$(dirname "$0")/../.."
OUT=build/kick_strip_private
mkdir -p "$OUT" build/gen
CC="${CC:-cc}"
[ -f build/gen/x0x_drum_samples.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
FLAGS="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen"
DEFS=""
OBJECTS=""
# Mandatory default oracle comes from the preserved upstream Git commit, offline.
if [ "$#" -eq 0 ]; then
  git show 47f8e4c4ad602591b81c21bc5480db7a94f6d6af:firmware/src/dsp/drum909.c > "$OUT/drum909_legacy.c"
  set -- "$OUT/drum909_legacy.c"
fi
if [ "$#" -gt 0 ]; then
  DEFS=-DKICK_LEGACY_ORACLE
  RENAME="-DX0X_RAW_HH -DX0X_EXP_RAW_REFERENCE -Dx0x_pot_exp_value=legacy_x0x_pot_exp_value -Dx0x_tanh_tab=legacy_x0x_tanh_tab -Dx0x_pot_exp=legacy_x0x_pot_exp"
  for fn in init trigger render render_st pan_settle nparams param set get active; do
    RENAME="$RENAME -Ddrum909_$fn=legacy_drum909_$fn"
  done
  "$CC" $FLAGS $RENAME -c "$1" -o "$OUT/legacy.o"
  OBJECTS="$OUT/legacy.o"
fi
"$CC" $FLAGS -c firmware/src/dsp/drum909.c -o "$OUT/drum909.o"
"$CC" $FLAGS -c firmware/src/dsp/sound_strip.c -o "$OUT/sound_strip.o"
"$CC" $FLAGS $DEFS tests/host/kick_strip_test.c "$OUT/drum909.o" "$OUT/sound_strip.o" $OBJECTS -lm -o "$OUT/kick_strip_test"
"$OUT/kick_strip_test"
size "$OUT/drum909.o" "$OUT/sound_strip.o"
