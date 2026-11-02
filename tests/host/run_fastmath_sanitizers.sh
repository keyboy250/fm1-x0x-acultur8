#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
cc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off tests/host/fastmath_exp_bits_test.c -o build/host/fastmath_exp_bits_test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 build/host/fastmath_exp_bits_test
cc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off tests/host/fastmath_test.c -lm -o build/host/fastmath_sanitizer_test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 build/host/fastmath_sanitizer_test
