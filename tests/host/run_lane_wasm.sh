#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Root serializes these real WASM builds; never auto-accept a missing oracle.
set -eu
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
BASE=ad3237199ef275abda5d388b849c5a0f8771dfd0
OUT=build/lane-wasm-oracle
mkdir -p "$OUT"
git -c safe.directory="$ROOT" cat-file -e "$BASE^{commit}"
REF=$(mktemp -d "$OUT/source.XXXXXX")
git -c safe.directory="$ROOT" archive --format=tar --output="$REF/source.tar" "$BASE"
tar -xf "$REF/source.tar" -C "$REF"
# Asset-only codec matching; independent original-PCM loss test stays mandatory.
python3 tools/prepare_cymbal_oracle.py "$REF" --source-commit "$BASE"
sh "$REF/web/emu/build.sh"
test -s "$REF/build/emu/x0x.wasm"
printf '%s\n' "$BASE" > "$REF/accepted-commit.txt"
node tests/host/lane_wasm_test.mjs build/emu/x0x.wasm "$REF/build/emu/x0x.wasm" "$REF/accepted-commit.txt"

cp "$REF/build/emu/x0x.wasm" "$OUT/accepted.wasm"
cp "$REF/accepted-commit.txt" "$OUT/accepted-commit.txt"
