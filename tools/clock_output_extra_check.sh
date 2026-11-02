#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd /work
cc -std=c99 -O1 -g -w -Ifirmware/src -fsanitize=address,undefined -fno-omit-frame-pointer tests/host/midi_deadline_test.c -lm -o build/host/midi_deadline_test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 build/host/midi_deadline_test >build/clock-output/deadline.log 2>&1
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c"
cc -std=c99 -O1 -g -w -ffp-contract=off -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -fsanitize=address,undefined -fno-omit-frame-pointer tests/host/midi_audio_position_test.c $SOURCES -lm -o build/host/midi_audio_position_test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 build/host/midi_audio_position_test >build/clock-output/audio-position.log 2>&1
mkdir -p /tmp/deadline-negative
python3 - <<'PY'
from pathlib import Path
p=Path('firmware/src/usb.c');s=p.read_text()
old='if(!cancelled && !midi_deadline_ready(midi_deadline.due[stage_r%MQ],fm1_ticks()))break;'
assert s.count(old)==1
Path('/tmp/deadline-negative/usb.c').write_text(s.replace(old,'if(0)break; /* intentional negative control: deadline ignored */'))
PY
cc -std=c99 -O1 -w -I/tmp/deadline-negative -Ifirmware/src tests/host/midi_deadline_test.c -lm -o build/host/midi_deadline_negative
set +e
(ulimit -c 0; build/host/midi_deadline_negative) >build/clock-output/deadline-negative.log 2>&1
negative_exit=$?
set -e
test "$negative_exit" -ne 0
grep -q 'nw==0&&mo_r==0' build/clock-output/deadline-negative.log
printf 'PASS negative control rejects early block-batched MIDI output\n' >>build/clock-output/deadline-negative.log
cc -std=c99 -O1 -g -w -fsanitize=address,undefined -fno-omit-frame-pointer tests/ota_test.c -o build/host/clock_ota_test
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 build/host/clock_ota_test build/clock-output/target/x0x-0.1.28-clockfix.fwsc >build/clock-output/updater-simulation.log 2>&1
printf 'Real engine positions, negative control and full updater simulation PASS\n'
