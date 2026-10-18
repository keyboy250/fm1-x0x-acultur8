#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
cc -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror -pedantic \
  -c firmware/src/dsp/sound_strip.c -o build/host/sound_strip.o
cc -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -pedantic \
  tests/host/sound_strip_test.c build/host/sound_strip.o -lm -o build/host/sound_strip_test
build/host/sound_strip_test
