#!/bin/sh
set -eu
python3 lab/build_emulator.py --native >build/preset-emulator-final.log 2>&1
sh tools/kick_project_checks.sh >build/preset-project-final.log 2>&1
sh tools/kick_cc_checks.sh >build/preset-cc-final.log 2>&1
python3 -m unittest tests.test_kick_preset_codec >build/preset-codec.log 2>&1
sh tools/kick_saved_sound_checks.sh >build/preset-saved-sound.log 2>&1
sh tools/acultur8_checks.sh >build/preset-required.log 2>&1
sh tools/kick_presets_inherited_checks.sh
printf 'KICK_PRESETS_QUALIFICATION_PASS\n'
