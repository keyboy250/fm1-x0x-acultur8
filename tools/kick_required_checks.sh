#!/bin/sh
set -eu
mkdir -p build/kick-tests
sh tools/kick_native_checks.sh >build/kick-native.log 2>&1
sh tools/acultur8_checks.sh >build/kick-required.log 2>&1
printf 'KICK_REQUIRED_CHECKS_PASS
'
