#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
cc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/src -Ibuild/gen tests/boot_intro_test.c -o build/host/boot_intro_test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 build/host/boot_intro_test
