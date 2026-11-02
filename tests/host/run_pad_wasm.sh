#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Independent accepted pre-pad oracle. Missing history/build is an error.
set -eu
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
BASE=b643f1775215cdf0c73e0e15812dbf1783107865
OUT=build/pad-oracle
mkdir -p "$OUT"
git -c safe.directory="$ROOT" cat-file -e "$BASE^{commit}"
REF=$(mktemp -d "$OUT/source.XXXXXX")
git -c safe.directory="$ROOT" archive --format=tar --output="$REF/source.tar" "$BASE"
tar -xf "$REF/source.tar" -C "$REF"
sh "$REF/web/emu/build.sh"
test -s "$REF/build/emu/x0x.wasm"
printf '%s\n' "$BASE" > "$REF/accepted-commit.txt"
node tests/host/pad_wasm_test.mjs build/emu/x0x.wasm "$REF/build/emu/x0x.wasm" "$REF/accepted-commit.txt"
