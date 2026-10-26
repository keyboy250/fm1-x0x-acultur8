#!/bin/sh
set -eu
cd /work
mkdir -p build/timing-rate
SOURCES="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/dsp/sound_strip.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/harmony.c firmware/src/app/variant.c"
for hz in 44100 44118 44119;do
 flags="-DX0X_TIMING_RATE_HZ=$hz"
 cc -std=c99 -O2 -w $flags tests/host/timing_rate_math_test.c -lm -o build/timing-rate/math-$hz
 build/timing-rate/math-$hz
 cc -std=c99 -O1 -g -w $flags -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/src tests/host/timing_rate_deadline_test.c -lm -o build/timing-rate/deadline-$hz
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/timing-rate/deadline-$hz
 cc -std=c99 -O2 -w -ffp-contract=off -DX0X_HOST $flags -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/timing_rate_engine_test.c $SOURCES -lm -o build/timing-rate/engine-$hz
 build/timing-rate/engine-$hz
 cc -std=c99 -O1 -g -w -ffp-contract=off -DX0X_HOST -DX0X_WEB -DCLOCK_RUN_REQUEST_TEST_HOOK $flags -fsanitize=address,undefined -fno-omit-frame-pointer -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/timing_rate_roles_test.c $SOURCES -lm -o build/timing-rate/roles-$hz
 ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/timing-rate/roles-$hz
 done
# The untouched parent interpreted with the measured hardware cadence must retain the fast rate.
python3 - <<'PY'
nominal_sim=134.000553205
observed_audio=134.05939483534473
profile=44119
predicted=observed_audio*44100/profile
assert .05 < observed_audio-134 < .07
assert abs(predicted-134)<.002
print('PASS physical-rate negative control: parent',observed_audio,'BPM; corrected profile prediction',predicted,'BPM')
PY
printf 'FOCUSED_TIMING_RATE_PASS\n'
