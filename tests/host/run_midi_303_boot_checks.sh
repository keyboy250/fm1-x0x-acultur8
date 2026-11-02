#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
mkdir -p /tmp/midi303-boot/build/emu
tar --exclude=./build --exclude=./core --exclude=./.git -cf - . | tar -xf - -C /tmp/midi303-boot
cp -R build/gen /tmp/midi303-boot/build/
cp build/emu/x0x.wasm /tmp/midi303-boot/build/emu/
cd /tmp/midi303-boot
sh tests/host/run_home_boot.sh >/work/build/midi-303/inherited-home_boot.log 2>&1
node tests/host/home_boot_wasm_test.mjs build/emu/x0x.wasm >/work/build/midi-303/inherited-home_boot-wasm.log 2>&1
# Exercise the exact new version string as well as DEV, preserving all timer,
# USB-service, skip and logo assertions in the parent's fixture.
sed -i 's/0.1.21-VERBETA/0.1.23-DRUMBETA/g' tests/host/run_boot_version_footer.sh
sh tests/host/run_boot_version_footer.sh >/work/build/midi-303/inherited-boot_footer.log 2>&1
printf 'MIDI303 HOME/EXACT-VERSION BOOT CHECKS PASS\n'
