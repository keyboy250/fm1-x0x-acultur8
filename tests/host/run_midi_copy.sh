#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/midi-copy
sed 's@midi_303_test@midi_copy_test@g' tests/host/run_midi_303.sh >build/midi-copy/run-native.sh
sh build/midi-copy/run-native.sh
