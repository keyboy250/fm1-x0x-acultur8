#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/exp_table_test build/gen
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
for mode in packed raw; do
 defs=""
 if [ "$mode" = raw ]; then defs=-DX0X_EXP_RAW_REFERENCE; fi
 cc -O2 -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined $defs -Ibuild/gen tests/host/exp_table_test.c -o build/exp_table_test/$mode
 build/exp_table_test/$mode > build/exp_table_test/$mode.bits
 printf 'c8c351223b0c805e68f88c917c5ed0a8b4983277681e2a1b43251627ffbd3faf  build/exp_table_test/%s.bits\n' "$mode" | sha256sum -c -
 done
cmp build/exp_table_test/packed.bits build/exp_table_test/raw.bits
echo 'EXP immutable tables: all2432 original IEEE32 values, production decoder/raw oracle, ASan/UBSan PASS'

# Generator layout and future final-row guards through the actual C accessor.
python3 tests/exp_packing_test.py
