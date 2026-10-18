#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Required independent production-WASM oracle: public accepted pre-all25 main.
# Missing pinned history, compiler, baseline build or PCM comparison fails closed.
set -eu
cd "$(dirname "$0")/../.."
ROOT=$(pwd)
BASE=e78ee13c60116e5b352d73b537cdf1f0980d9d8e
OUT=build/all-strip-oracle
mkdir -p "$OUT"
git -c safe.directory="$ROOT" cat-file -e "$BASE^{commit}"
REF=$(mktemp -d "$OUT/source.XXXXXX")
git -c safe.directory="$ROOT" archive --format=tar --output="$REF/source.tar" "$BASE"
tar -xf "$REF/source.tar" -C "$REF"
# Asset-only codec matching; independent original-PCM loss test stays mandatory.
python3 tools/prepare_cymbal_oracle.py "$REF" --source-commit "$BASE"
# Its original build script uses the same installed emcc and generation tools.
sh "$REF/web/emu/build.sh"
test -s "$REF/build/emu/x0x.wasm"
printf 'Independent all-strip WASM oracle: %s\n' "$BASE"
node tests/host/all_strip_wasm_test.mjs build/emu/x0x.wasm "$REF/build/emu/x0x.wasm"
