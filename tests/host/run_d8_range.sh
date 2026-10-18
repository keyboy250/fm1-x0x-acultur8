#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Pinned old complete DSP versus current source, all domains and exact rendered buses.
set -eu
cd "$(dirname "$0")/../.."
: "${D8_ORACLE_REPO:=.}"
OUT=build/d8_range
mkdir -p "$OUT"
PIN=ad3237199ef275abda5d388b849c5a0f8771dfd0
if [ -n "${D8_REFERENCE_C:-}" ]; then
 cp "$D8_REFERENCE_C" "$OUT/reference.c"
else
 git -c safe.directory="$(cd "$D8_ORACLE_REPO" && pwd)" -C "$D8_ORACLE_REPO" show "$PIN":firmware/src/dsp/drum808.c > "$OUT/reference.c"
fi
printf '%s  %s\n' b13f963eb9e4cc5457d4c66a12c0c41932f6c6f563cf9cf8139a2995ba6f5bca "$OUT/reference.c" | sha256sum -c -

REFERENCE="$(pwd)/$OUT/reference.c"
for tail in 0 1; do
 for mode in reference current; do
  defs=""
  if [ "$mode" = reference ]; then defs="-DD8_ORACLE_REFERENCE"; fi
  ${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -DD8_TAIL_DB=$tail $defs -DD8_ORACLE_SOURCE="\"$REFERENCE\"" -Ifirmware/src -Ifirmware/src/dsp tests/host/d8_range_test.c firmware/src/dsp/sound_strip.c -lm -o "$OUT/$mode"
  "$OUT/$mode" "$OUT/$mode.tail$tail.pcm" > "$OUT/$mode.tail$tail.txt"
 done
 cmp "$OUT/reference.tail$tail.txt" "$OUT/current.tail$tail.txt"
 cmp "$OUT/reference.tail$tail.pcm" "$OUT/current.tail$tail.pcm"
 cmp "$OUT/reference.tail$tail.pcm.tails" "$OUT/current.tail$tail.pcm.tails"
 sha256sum "$OUT/current.tail$tail.txt" "$OUT/current.tail$tail.pcm" "$OUT/current.tail$tail.pcm.tails"
 tail -1 "$OUT/current.tail$tail.txt"
done
printf 'D8 exact ranges/defaults,paired timbres,all pots/state/PCM and raw/active tails PASS\n'
