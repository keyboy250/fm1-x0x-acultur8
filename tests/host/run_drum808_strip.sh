#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host/drum808_strip
CC="${CC:-cc}"
OUT=build/host/drum808_strip
# Accepted source checkpoint; public names changed only to link both renderers.
git -c safe.directory="$(pwd)" show 628a9afa5ccac09ffbd87e0d65763251f744b003:firmware/src/dsp/drum808.c > "$OUT/baseline.c"
RENAMES="-Ddrum808_quiet=baseline_drum808_quiet -Ddrum808_init=baseline_drum808_init -Ddrum808_trigger=baseline_drum808_trigger -Ddrum808_render=baseline_drum808_render -Ddrum808_render_st=baseline_drum808_render_st -Ddrum808_pan_settle=baseline_drum808_pan_settle -Ddrum808_nparams=baseline_drum808_nparams -Ddrum808_param=baseline_drum808_param -Ddrum808_set=baseline_drum808_set -Ddrum808_get=baseline_drum808_get -Ddrum808_active=baseline_drum808_active"
WARN="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror -pedantic"
# shellcheck disable=SC2086
"$CC" $WARN $RENAMES -Ifirmware/src/dsp -c "$OUT/baseline.c" -o "$OUT/baseline.o"
# shellcheck disable=SC2086
"$CC" $WARN -Ifirmware/src/dsp tests/host/drum808_strip_test.c firmware/src/dsp/sound_strip.c "$OUT/baseline.o" -lm -o "$OUT/test"
"$OUT/test"
