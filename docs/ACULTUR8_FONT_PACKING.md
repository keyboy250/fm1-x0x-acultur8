# Lossless font packing

The generator packs each glyph independently with byte runs, then applies canonical
Huffman coding by default. It preserves
the existing 4-bit pixel values, advance widths, bearings, bitmap sizes and layout.
`X0X_FONT_COMPRESSION=raw` and `=rle` retain both comparison paths. No font or sample was removed.

The renderer and the UI's capital-height measurement both consume the same stream.
The second reader matters: treating packed bytes as pixels shifted some labels,
which the complete-screen parity test detected and the `caps()` change fixes.

## Measured result

The table below preserves the original accepted raw-to-packet comparison. The
subsequent Huffman change is recorded separately after the reproduce section;
these historical target/WASM numbers are not measurements of the integrated
25-strip firmware.

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

## Additional lossless Huffman packing (2026-10-13)

The earlier `rle` format already used PackBits-style literals and repeated bytes.
Optimal packetization produced the same 25,514-byte payload. Adding special zero
and full-coverage runs only reduced it to 22,740 bytes. Canonical Huffman coding of
the unchanged packet streams saves enough space to be useful for the integrated
sound strips while keeping every existing glyph, metric and alpha value.

The default `huffman` format uses explicit `FONT_HUFFMAN=1` alongside `FONT_RLE=1`.
Both earlier generated formats remain supported. Each glyph begins at a byte
offset, carries MSB-first codes and ends with ignored zero padding. Codes are
bounded to 16 bits (actual Barlow maximum: 15); unsupported font distributions
fail generation explicitly and can use `rle`. Flash-only canonical tables and a
256-entry prefix lookup decode packet bytes as needed. The renderer's requested
dimensions determine the output length, and the offset sentinel bounds input.
The reader has no glyph buffer and adds no persistent RAM. On a 32-bit ABI its
fields occupy 16 automatic bytes, compared with the earlier 12; target compiler
stack frames and physical stack watermark still require separate verification.

| Font storage | Accepted packet format | Huffman format |
| --- | ---: | ---: |
| Glyph payload | 25,514 B | 17,877 B |
| Huffman tables, including prefix lookup | 0 B | 864 B |
| Combined payload and new tables | 25,514 B | 18,741 B |

This saves 6,773 bytes before incremental decoder machine code. It does not claim
an integrated app image size or target acceptance; those results belong to the
separate combined target build. Resource limits, fonts and source font files are
unchanged.

Focused checks compile the actual `gfx.c` for raw, packet and Huffman modes. All
439 glyphs round-trip to the original packet bytes, metrics match, and all 1,756
clipping/color/blending cases produce the previously accepted raw pixel hash
`e971afab542cff352576dc573b4459322a7e2bce477c4bcaa8589ec1c70290d8`.
The original generator and renderer from pinned commit
`e78ee13c60116e5b352d73b537cdf1f0980d9d8e` were independently compiled in raw
and packet modes and produce the same hash. The production bit reader decodes
every byte symbol and rejects incomplete long codes. Packet tests cover literal/repeat transitions, 128-byte repeats, empty
streams, truncated literals and a repeat missing its value. Every byte
truncation of all439 actual glyphs remains bounded. ASan and UBSan pass.
The original raw-to-packet tests and evidence remain intact.

Three alternating host timing samples rendered 351,200 cases each. Median times
were 0.240681 seconds raw, 0.274859 seconds packet, and 0.411221 seconds Huffman
(49.61% slower than packet). Checksums match. This measures the focused host draw
harness; physical FM1 draw timing and audio deadline margin remain pending.
The 8-bit prefix table improves the simple bit-at-a-time reader's measured
0.537799 seconds at the cost of 512 flash bytes, included in the table above.

[tests/font_huffman_evidence.json](../tests/font_huffman_evidence.json) records
exact source/font/header hashes, pixel results and timing samples. Run
`FONT_TEST_OUTPUT=/tmp/font-checks python3 tests/test_font_packing.py` for all three
formats, then repeat the sanitizer command above with `/tmp/font-checks/huffman`.
The default generator now selects `huffman`; explicitly select `rle` when
reproducing the historical packet-format evidence. Full-screen/WASM parity and
integrated target checks remain the combined build's responsibility.

The required runner also invokes `python3 tests/test_font_pinned_oracle.py` after
the packing tests. It requires the full pinned Git commit locally, creates a
fresh archive in an ignored temporary build directory and compiles the original
generator/renderer for raw and packet references plus the candidate Huffman
renderer. All three must match the accepted pixel hash and metrics. A missing
commit, source hash mismatch, compiler error or parity failure fails the check;
the check performs no network access. The lab entry point delegates to this same
portable test and carries no machine-specific source path.
