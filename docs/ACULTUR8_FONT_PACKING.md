# Lossless font packing

The default generator packs each glyph independently with byte runs. It preserves
the existing 4-bit pixel values, advance widths, bearings, bitmap sizes and layout.
`X0X_FONT_COMPRESSION=raw` retains a comparison path. No font or sample was removed.

The renderer and the UI's capital-height measurement both consume the same stream.
The second reader matters: treating packed bytes as pixels shifted some labels,
which the complete-screen parity test detected and the `caps()` change fixes.

## Measured result

Measured on 2026-10-13 against the original target baseline documented in
[ACULTUR8_TARGET_BUILD.md](ACULTUR8_TARGET_BUILD.md). Only `tools/gen_font.py`,
`firmware/src/gfx.c` and `caps()` in `firmware/src/app/ui.c` differ in the isolated
build. New sound strips and project-format work are excluded from this comparison.

| Resource | Before | After | Change |
| --- | ---: | ---: | ---: |
| Glyph payload | 43,917 B | 25,514 B | −18,403 B |
| App image including decoder | 581,444 B | 563,076 B | −18,368 B |
| App slot remaining | 120 B | 18,488 B | +18,368 B |
| RAM data/BSS | 97,216 B | 97,216 B | 0 B |
| Pool | 334,324 B | 334,324 B | 0 B |
| Browser WASM | 630,525 B | 628,219 B | −2,306 B |

The 581,564-byte app limit and 8,192-byte required pool reserve remain unchanged.
The existing build gates pass, including no soft-double routines, RAM code without
calls, HAL register ownership and all 15 static DSP budgets.

The reader uses a 12-byte automatic state on the target ABI and no decoded glyph
buffer. Each control describes 1–128 literal or repeated bytes. The final offset
sentinel bounds the last glyph as well as every earlier one; a truncated stream
returns transparent pixels. Drawing requests only the original glyph dimensions.

Target disassembly shows a 16-byte `font_next` stack frame. Including that nested
call, `cv_text` increases from 128 to 136 bytes; `caps` increases from 44 to 76 bytes.
These numbers exclude their callers and interrupts. Persistent RAM is unchanged;
physical stack watermark acceptance remains pending.

Three alternating host draw microbenchmarks rendered 351,200 text cases each. Median
time was 0.242373 s raw and 0.269779 s packed (+11.31%); checksums were identical.
This focused host harness includes text width and clipping, and excludes screen I/O
and the cached capital-height lookup. It does not establish physical FM1 draw time
or audio deadline margin.

## Checks that passed

- All 439 glyphs round-trip exactly, including their metrics and unused glyph cells.
- Four focused regressions cover literal/run boundaries, seeded mixed runs, every
  generated glyph and 1,756 production render cases with clipping and blending.
- The production decoder harness passes AddressSanitizer and UndefinedBehaviorSanitizer,
  including empty and truncated streams and the maximum run length.
- Five simulator scenarios (`tour`, `help`, `stereo`, `getting_started`, `patterns`)
  produce 30 pixel-identical full screenshots against the original renderer.
- Raw and packed WASM playback produces 241,227 bit-identical stereo frames. The
  framebuffer, lights and clock match after every one of 1,894 render chunks.

[tests/font_packing_evidence.json](../tests/font_packing_evidence.json) records exact
source and output hashes, every screenshot hash, resources and benchmark samples.
The preserved raw and packed builds live outside the public repository in the local
build-dependency workspace. No device has been flashed for this check.

## Reproduce

Use the Linux builder and pinned compiler described in the target-build guide.
From the repository root, run the focused tests (Pillow and a C compiler required):

```sh
FONT_TEST_OUTPUT=/tmp/font-checks python3 tests/test_font_packing.py
cc -std=c99 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I/tmp/font-checks/rle tests/host/font_render_test.c -o /tmp/font-checks/asan
/tmp/font-checks/asan >/dev/null
/tmp/font-checks/raw/font_render benchmark
/tmp/font-checks/rle/font_render benchmark
```

Build raw and packed host/WASM executables in two isolated copies of the same source
revision. Set `X0X_FONT_COMPRESSION=raw` for the first and `=rle` for the second.
The host and web build scripts regenerate `build/gen/felucca_font.h` on every
build, so changing compression or fonts cannot reuse a stale header. The ordinary
required CI runner includes the focused packing/render suite. Retain
each output, then run:

```sh
python3 tests/font_scenario_parity.py /raw/build/host/x0x_host \
  /packed/build/host/x0x_host /tmp/font-screens
node tests/host/font_wasm_parity.mjs /raw/build/emu/x0x.wasm \
  /packed/build/emu/x0x.wasm /tmp/font-wasm.json
```

Run the ordinary target build and `python3 tests/target_budget.py` on the packed
copy. The original baseline is intentionally retained in its evidence file;
integrated firmware resource measurements must be recorded separately after other
features are added. Physical draw timing, stack watermark and hardware playback
remain pending until a device is available for those checks.
