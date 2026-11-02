#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/midi-variation
sed 's@midi_303_test@midi_variation_test@g' tests/host/run_midi_303.sh >build/midi-variation/run-native.sh
sh build/midi-variation/run-native.sh
