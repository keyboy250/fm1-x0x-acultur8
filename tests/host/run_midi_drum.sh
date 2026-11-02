#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/midi-drum
# Same translation unit/dependencies and sanitizer options as the inherited303 suite.
sed 's@midi_303_test.c@midi_drum_test.c@g;s@midi_303_test@midi_drum_test@g' tests/host/run_midi_303.sh > build/midi-drum/run-native.sh
# Both tests/host and build/midi-drum are two levels below the source root.
sh build/midi-drum/run-native.sh
