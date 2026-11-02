#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Local compilation/regression only. This script has no device or publication path.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/sysex-routing build/gen build/host build/emu build/midi-variation
python3 tests/test_sysex_routing.py --parent-source /inputs/parent-source.zip >build/sysex-routing/routing.log 2>&1
printf 'Routing race and negative control PASS\n'
python3 tools/gen_font.py build/gen/felucca_font.h >build/sysex-routing/generated.log 2>&1
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >>build/sysex-routing/generated.log 2>&1
sh tools/gen_builtin_break.sh build/gen >>build/sysex-routing/generated.log 2>&1
X0X_VERSION=0.1.28-inboxfix BINARYEN_CORES=1 sh web/emu/build.sh >build/sysex-routing/wasm-build.log 2>&1
X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L sh host/build_host.sh >build/sysex-routing/host-build.log 2>&1
python3 lab/build_emulator.py --package-only >build/sysex-routing/browser-package.log 2>&1
printf 'Fresh host and WASM builds PASS\n'
sh tools/midi_copy_check.sh >build/sysex-routing/full-midi.log 2>&1
printf 'Full required and inherited MIDI suites PASS\n'
sh tests/host/run_midi_copy_guards.sh >build/sysex-routing/copy-guards.log 2>&1
for suite in pattern_copy cross_part_copy; do
    node tests/host/${suite}_wasm_test.mjs build/emu/x0x.wasm >build/sysex-routing/${suite}-wasm.log 2>&1
done
printf 'Additional copy guards and native menu WASM suites PASS\n'
printf 'LOCAL SYSEX ROUTING REGRESSION PASS - NOT INSTALLED OR PUBLISHED\n'
