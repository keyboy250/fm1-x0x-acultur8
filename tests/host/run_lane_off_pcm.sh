#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Pinned offline native PCM oracle. No fallback or network dependency.
set -eu
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
BASE=ad3237199ef275abda5d388b849c5a0f8771dfd0
OUT=build/lane-native-oracle
mkdir -p "$OUT"
git -c safe.directory="$ROOT" cat-file -e "$BASE^{commit}"
REF=$(mktemp -d "$OUT/source.XXXXXX")
git -c safe.directory="$ROOT" archive --format=tar --output="$REF/source.tar" "$BASE"
tar -xf "$REF/source.tar" -C "$REF"
cp tests/host/lane_off_pcm.c "$REF/tests/host/lane_off_pcm.c"
mkdir -p "$REF/build/gen" "$REF/build/host"
cp build/gen/* "$REF/build/gen/"
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c firmware/src/app/engine.c"
compile() {
    ${CC:-cc} ${CFLAGS:-} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/lane_off_pcm.c $SOURCES -lm -o build/host/lane_off_pcm
}
compile
build/host/lane_off_pcm build/lane-native-oracle/current.pcm
(cd "$REF"; compile; build/host/lane_off_pcm build/host/original.pcm)
cmp "$REF/build/host/original.pcm" "$OUT/current.pcm"
sha256sum "$OUT/current.pcm"
printf '%s\n' "uninitialized Off pinned independent PCM: 262144 frames PASS ($BASE)"
