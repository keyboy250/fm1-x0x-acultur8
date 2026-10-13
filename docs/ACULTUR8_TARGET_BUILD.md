# FM1 target build and resource baseline

The 2026-10-13 baseline compiles and passes the package, loader and static target checks.
It has **120 bytes of firmware image space left**, **1,088 bytes of ordinary RAM left**,
and **1,548 bytes of pool space above the existing 8 KiB reserve**. Reclaim space before
qualifying new features for the physical FM1. These measurements do not establish device
playback timing, recovery, or hardware acceptance. No device was flashed.

## Reproduce the build

The build needs Linux x86-64, Python 3 with Pillow, a host C compiler and the official
JieLi pi32v2 compiler. On Windows, run the entire build inside the existing emulator
builder so shell commands, host compilation and compiler paths all use Linux conventions.
Keep shell files LF-terminated. Avoid running `tools/build.py` on Windows with its
per-command Docker path conversion.

Dependencies used for this baseline, stored outside the public repository:

| Input | Pinned identity |
| --- | --- |
| Official compiler archive | `jieli-linux-toolchains-20260730.1.tar.xz` |
| Compiler archive SHA-256 | `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd` |
| Compiler | JieLi clang 4.0.1, `-target pi32v2 -mcpu=r3 -mfprev1 -ffp-contract=off`; DSP/engine `-O2`, platform `-Os` |
| AC79 SDK tag | `AC79NN_SDK_V1.2.1_2023-12-13` |
| AC79 SDK commit | `d179b4484759423312073f5fbb232501aa491047` |
| Local builder image | `sha256:30ffd8c6cf8a5d697ac0135c489cf1352cc5c58c99c58505995242f240c83da3` |

Official sources: [JieLi toolchain documentation](https://doc.zh-jieli.com/Tools/zh-cn/dev_tools/dev_env/jl_toolchain.html),
[compiler archive](https://jl-update.oss-cn-shenzhen.aliyuncs.com/jieli-linux-toolchains-20260730.1.tar.xz),
[pinned AC79 SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/tree/AC79NN_SDK_V1.2.1_2023-12-13).
The SDK LICENSE was inspected and is Apache-2.0. Only the three package inputs and its
LICENSE are needed for this build; all three match the SHA-256 checks in `tools/build.py`.
The toolchain and SDK remain external dependencies. The REX SDK is unrelated and must
not be added to this firmware source or a public build package.

Inside a Linux environment with those dependencies installed:

```sh
export JIELI_DOCKER=0
export JIELI_TOOLCHAIN=/opt/jieli
export AC79_SDK=/sdk
python3 tools/build.py
python3 tests/test_target_budget.py
python3 tests/target_budget.py --report build/target-cost-report.json
```

For the baseline Windows Docker run, the private `build-dependencies` folder contains
the compiler archive, `ac79-sdk/` (LICENSE and the three files under `cpu/wl82/tools/`),
and an isolated `target-baseline/` copy of the source. The following PowerShell command
uses that layout and does not access hardware:

```powershell
$deps = (Resolve-Path '../build-dependencies').Path
docker run --rm --network=none --cap-drop=ALL --security-opt=no-new-privileges `
  -v "${deps}:/deps" -w /deps/target-baseline `
  -e JIELI_DOCKER=0 -e JIELI_TOOLCHAIN=/opt/jieli -e AC79_SDK=/deps/ac79-sdk `
  acultur8-fm1-emu-builder:local sh -c 'set -e; mkdir -p /opt/jieli; tar --no-same-owner -xJf /deps/jieli-linux-toolchains-20260730.1.tar.xz --strip-components=1 -C /opt/jieli; python3 tools/build.py; python3 tests/target_budget.py --report build/target-cost-report.json'
```

`--no-same-owner` is needed when unpacking in the restricted container. Verify the
archive hash before extraction. An isolated copy must match the source revision being
qualified; record its input hash and rebuild after source changes. The private build
report includes the input hash, output hashes and measurements; the public summary is
`tests/target_build_baseline.json`.

## Existing hard limits remain in force

| Resource | Used | Limit | Remaining |
| --- | ---: | ---: | ---: |
| App image | 581,444 B | 581,564 B (`0x8DFBC`) | 120 B |
| RAM data/BSS, including alignment | 97,216 B | 98,304 B | 1,088 B |
| Pool | 334,324 B | 344,064 B | 9,740 B |
| Pool headroom beyond required reserve | | 8,192 B reserve | 1,548 B |

The build also passed the single-precision/no-soft-double check, HAL-only register
access check, entry stub, loader placement, and no-call check for flash-off RAM code.
The package is 609,680 bytes; its size is not the app-slot limit.

## Static DSP regression gate

The inherited gate referenced `build/felucca.dis`, checked unrelated Felucca engine
names, and returned success when the artifact was missing. It had no X0X baseline.
The repaired gate uses `build/x0x.dis`, requires all 15 measured X0X functions, and
fails for missing artifacts, malformed/incomplete baselines and unknown instructions.

**Disassemble with `objdump -d --mcpu=r3`.** Omitting the CPU option misdecoded the
baseline's FPU instructions: 9,498 unknown entries and spurious branches appeared.
The corrected output has zero unknown instructions. Both target build helpers now
request the correct CPU when generating disassembly.

The gate measures complete instruction counts and loop complexity. A backward branch
defines an approximate loop span; nested spans carry weight 4 per depth (capped at 4
levels), and divides add weight 8. Instruction and weighted-cost growth above 10%
fails, as does any increase in loop call/divide counts. A zero-cost baseline cannot
hide newly added loops or straight-line instructions. Branch spans can overlap, so
the numbers are regression indicators, not absolute cycles and not additive CPU
percentages. Callee work is not expanded; explicitly add new hot callees to coverage.

The baseline includes the real `d9_render`/`d8_render` implementations, bass and break
rendering, `hat_tick`, reverb/delay, stereo FX/master, engine/second-core work,
sequencer/motion and the audio IRQ. `render_sub` is inlined into `engine_render`.

To intentionally establish a reviewed compiler/code baseline:

```sh
python3 tests/target_budget.py build/x0x.dis tests/target_budget.json \
  --update --compiler-id 'Compiler version, archive SHA-256 and build flags'
python3 tests/target_budget.py
```

`--update` checks full disassembly coverage before writing and requires compiler
provenance. It must not be run automatically by CI. The old `BUDGET_UPDATE` environment
variable no longer updates anything. Compiler changes require review before rebasing.

## Prioritize measured savings

Largest image contributors are the ride (93,998 B), crash (86,600 B), two built-in
breaks (55,125 B each), hi-hat (40,500 B), six font bitmaps (43,917 B combined),
`draw_main` (14,354 B), `d8_render` (11,554 B), pot curves (9,728 B) and `d9_render`
(9,496 B). The 909 samples plus generated breaks account for 331,348 B.

Lossless glyph packing is now implemented and measured in an isolated build against
the baseline above. All 439 glyphs retain their pixels and metrics: 43,917 bytes
become 25,514 bytes of payload. Including decoder code, the app is **563,076 bytes**,
saving **18,368 bytes** and leaving **18,488 bytes** in its slot. RAM and pool use are
unchanged. Thirty full-screen comparisons and the browser audio/display comparison
pass. See [font packing evidence](ACULTUR8_FONT_PACKING.md) for the bounded decoder,
small measured stack increase, host draw cost, exact hashes and reproduction steps.
These are font-only measurements; rebuild the integrated variant after additional
features. Keep sample/tail changes and lossy audio formats separate because they
change sound.

Largest ordinary RAM items are the project (30,944 B), USB audio ring (8,192 B),
audio double buffer (8,192 B), LZ hash workspace and command queue (4,096 B each),
storage and project I/O buffers (3,840 B each), and performance-test objects
(2,928 B combined). Investigate sharing storage scratch only after proving call
lifetimes and failure/retry behavior; do not alias buffers that remain live during
a nested save. Queue/ring reductions need burst, underrun and transport tests.

Pool pressure comes from delay storage (176,400 B), screen canvas (69,120 B), undo
shadow (30,916 B), undo journal (20,636 B) and FX state (18,316 B). New project fields
can increase both the ordinary project and its undo shadow, so budget both copies.
New DSP state must account for both cores' ownership and simultaneous voices. Do not
reduce delay/undo behavior or the existing 8 KiB reserve to make a build pass.

## Acceptance still open

Host-only tests passed for package identity/CRC, OTA staging, loader update and
replay behavior, invalid-package rejection, interrupted update simulation, and
Python/JavaScript package interpretation. They do not perform real USB I/O.
Physical playback deadlines, worst-case audio load, stack margins, actual flash,
save/reboot and recovery remain untested. Preserve those pending statuses.
