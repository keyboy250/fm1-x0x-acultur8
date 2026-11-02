#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/midi-freeze
sed 's@midi_303_test@midi_freeze_test@g' tests/host/run_midi_303.sh >build/midi-freeze/run-native.sh
sh build/midi-freeze/run-native.sh
