#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
mkdir -p build/rate-families
cc --version | head -1
cc -std=c99 -O2 -Wall -Wextra -Werror -o build/rate-families/rate_families_test tests/host/rate_families_test.c firmware/src/seq/rate_families.c firmware/src/seq/rate_slice_codec.c
build/rate-families/rate_families_test
cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined,address -fno-omit-frame-pointer -o build/rate-families/rate_families_sanitized tests/host/rate_families_test.c firmware/src/seq/rate_families.c firmware/src/seq/rate_slice_codec.c
build/rate-families/rate_families_sanitized
