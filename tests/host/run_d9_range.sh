#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
mkdir -p build/d9_range
PIN=ad3237199ef275abda5d388b849c5a0f8771dfd0
HASH=467db9d440b67666f4229e721ab21e41a24ef951811a16ff67a7f41ea21493b6
if [ -n "${D9_REFERENCE_C:-}" ]; then
 cp "$D9_REFERENCE_C" build/d9_range/drum909_reference.c
else
 git show "$PIN:firmware/src/dsp/drum909.c" > build/d9_range/drum909_reference.c
fi
printf '%s  %s\n' "$HASH" build/d9_range/drum909_reference.c | sha256sum -c -
SOURCES="firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c firmware/src/app/engine.c"
for mode in reference current; do
 defs=""
 if [ "$mode" = reference ]; then defs='-DX0X_RAW_HH -DX0X_EXP_RAW_REFERENCE -DD9_ORACLE_REFERENCE -DD9_REFERENCE_C="drum909_reference.c"'; fi
 cc -O2 -ffp-contract=off -std=c99 -w -D_POSIX_C_SOURCE=200809L -DX0X_HOST $defs -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -Ibuild/d9_range tests/host/d9_range_test.c $SOURCES -lm -o build/d9_range/$mode
 build/d9_range/$mode build/d9_range/$mode.pcm > build/d9_range/$mode.txt
done
cmp build/d9_range/reference.txt build/d9_range/current.txt
cmp build/d9_range/reference.pcm build/d9_range/current.pcm
cmp build/d9_range/reference.pcm.tails build/d9_range/current.pcm.tails
tail -2 build/d9_range/current.txt
sha256sum build/d9_range/current.txt build/d9_range/current.pcm build/d9_range/current.pcm.tails
