#!/bin/sh
set -eu
mkdir -p build/kick-tests
python3 lab/build_emulator.py --native >build/recall-emulator.log 2>&1
sh tools/kick_confirmation_input_checks.sh >build/recall-physical-input.log 2>&1
sh tools/kick_project_checks.sh >build/recall-project.log 2>&1
sh tools/kick_cc_checks.sh >build/recall-cc.log 2>&1
python3 -m unittest tests.test_kick_preset_codec >build/recall-codec.log 2>&1
sh tools/kick_ui_checks.sh >build/recall-ui.log 2>&1
sh tools/acultur8_checks.sh >build/recall-required.log 2>&1
sh tools/kick_presets_inherited_checks.sh >build/recall-inherited.log 2>&1
printf 'KICK_RECALL_QUALIFICATION_PASS\n'
