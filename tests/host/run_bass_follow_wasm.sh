#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Independent accepted pre-bass oracle. Missing history/build is an error.
set -eu
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
BASE=c081d6306de301a7b75ba4ab146ac0c0237fb844
OUT=build/bass-follow-oracle
mkdir -p "$OUT"
git -c safe.directory="$ROOT" cat-file -e "$BASE^{commit}"
REF=$(mktemp -d "$OUT/source.XXXXXX")
git -c safe.directory="$ROOT" archive --format=tar --output="$REF/source.tar" "$BASE"
tar -xf "$REF/source.tar" -C "$REF"
sh "$REF/web/emu/build.sh"
test -s "$REF/build/emu/x0x.wasm"
printf '%s\n' "$BASE" > "$REF/accepted-commit.txt"
node tests/host/bass_follow_wasm_test.mjs build/emu/x0x.wasm "$REF/build/emu/x0x.wasm" "$REF/accepted-commit.txt"
