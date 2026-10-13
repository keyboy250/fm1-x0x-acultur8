#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Required self-contained browser-alpha checks. A missing compiler, Node, test or
# fixture is an error. Optional external reference engines and device/SDK tests
# are separate suites and never contribute to this suite's pass count.
set -eu
cd "$(dirname "$0")/.."
command -v cc >/dev/null
command -v node >/dev/null
command -v python3 >/dev/null
test -s build/emu/x0x.wasm
test -x build/host/x0x_host
node --check build/emu/emu.js
node --input-type=module --check < web/acultur8/public-demo.js
mkdir -p build/host build/scenarios
OUT=build/host
W='-O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror'
run() { name="$1"; shift; printf '\nRequired check: %s\n' "$name"; "$@"; }
compile() { name="$1"; shift; cc "$@" -o "$OUT/$name"; run "$name" "$OUT/$name"; }
run font_packing python3 tests/test_font_packing.py
compile fastmath -O2 -ffp-contract=off -Wall -Wextra -Werror tests/host/fastmath_test.c -lm
compile sequencer -O2 -ffp-contract=off -Wall -Wextra tests/host/seq_test.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c
compile encoder -O2 -w -Ifirmware/hal tests/host/encoder_test.c
compile bass303_lite -std=c99 -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src tests/host/bass303_lite_test.c firmware/src/dsp/bass303.c -lm
compile uac -O2 -w -Ifirmware/src -Ifirmware/hal tests/host/uac_test.c -lm
compile trs -O2 -w -Ifirmware/src tests/host/trs_test.c
compile undo -O2 -Wall -Wextra -Wno-unused-function tests/host/undo_test.c
compile lz $W tests/host/lz_test.c
compile motion $W tests/host/motion_test.c firmware/src/seq/motion.c
compile drum_pan -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen tests/host/drum_pan_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c -lm
compile stereo -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen tests/host/stereo_test.c firmware/src/dsp/master.c firmware/src/dsp/fxbus.c -lm
compile master $W -Wno-double-promotion tests/host/master_test.c firmware/src/dsp/master.c -lm
compile storage -O2 tests/storage_test.c
compile marked_player $W tests/host/marked_player_test.c firmware/src/dsp/marked_player.c -lm
run breaks sh tests/host/run_breaks.sh
run synthetic-upload-fixtures python3 tests/host/make_test_loops.py build/test_loops
run upload-images python3 tools/upload_breaks.py --dry-run build/test_upload build/test_loops/loop*.wav
run breaks-web node web/test_breaks.mjs
printf 'wav emu_ref.wav\nwait 500\ntapkey b0\ntapkey w0\ntapkey w8\ntap PLAY\nwait 2960\nwavstop\n' > "$OUT/emu_ref.x0x"
run host-reference build/host/x0x_host "$OUT/emu_ref.x0x" "$OUT"
run wasm-host-parity node tests/host/emu_test.mjs build/emu/x0x.wasm "$OUT/emu_ref.wav"
run wasm-slots node tests/host/emu_slots_test.mjs build/emu/x0x.wasm build/test_loops/loop1.wav
run marker-import node tests/host/marked_import_test.mjs
run marked-wasm node tests/host/marked_wasm_test.mjs build/emu/x0x.wasm
run marked-sync node tests/host/marked_sync_test.mjs build/emu/x0x.wasm
run actual-worklet node tests/host/worklet_sync_test.mjs build/emu/x0x.wasm
count=0
for required in autosave extclock freeplay getting_started groove guard help jam keysound knobs migrate migrate0102 patterns perf plock prob probcond probreset savewait showcase slices song songrep step303 steps64 stereo tb3po tour trackmute undo upload; do
    test -f "tests/scenarios/$required.x0x"
done
for scenario in tests/scenarios/*.x0x; do
    test -f "$scenario"
    name=$(basename "$scenario" .x0x)
    mkdir -p "build/scenarios/$name"
    run "scenario-$name" build/host/x0x_host "$scenario" "build/scenarios/$name"
    count=$((count + 1))
done
test "$count" -ge 31
printf '\nRequired engine checks passed, including %s host scenarios.\n' "$count"
