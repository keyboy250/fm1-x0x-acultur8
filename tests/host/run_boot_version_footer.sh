#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host build/boot-version-footer build/gen
test -f build/gen/felucca_font.h || python3 tools/gen_font.py build/gen/felucca_font.h
sh tests/host/run_boot_intro.sh >build/boot-version-footer/dev.log 2>&1
cp build/boot-intro-logo.ppm build/boot-version-footer/version-dev.ppm
cc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer '-DX0X_VERSION="0.1.28-CLOCKFIX"' -Ifirmware/src -Ibuild/gen tests/boot_intro_test.c -o build/host/boot_version_release_test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 build/host/boot_version_release_test >build/boot-version-footer/release.log 2>&1
cp build/boot-intro-logo.ppm build/boot-version-footer/version-release.ppm
cat build/boot-version-footer/dev.log build/boot-version-footer/release.log
