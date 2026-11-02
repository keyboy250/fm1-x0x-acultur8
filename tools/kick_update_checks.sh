#!/bin/sh
set -eu
mkdir -p build/kick-tests
cc -O2 tests/ota_test.c -o build/kick-tests/ota
build/kick-tests/ota build/kick-target/x0x-0.1.37-kickrecall.fwsc >build/kick-ota.log
cc -O2 tests/ldr_test.c -o build/kick-tests/loader
build/kick-tests/loader /inputs/rollback.fwsc build/kick-target/x0x-0.1.37-kickrecall.fwsc >build/kick-loader-upgrade.log
build/kick-tests/loader build/kick-target/x0x-0.1.37-kickrecall.fwsc /inputs/rollback.fwsc >build/kick-loader-rollback.log
printf 'KICK_UPDATE_SIMULATIONS_PASS
'
