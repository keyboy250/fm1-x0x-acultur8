# 909 noise arena resource proof

This isolated change retains all 256 samples of actual noise history and the
entire 256-sample render block. Capacity changes from 768 to 512 float samples,
saving exactly 1024 bytes. The existing forward compaction loop copies the most
recent 256 samples before appending new noise. It preserves chronological order,
the RNG sequence and every warm-filter input. No voice, config, DSP history,
undo capacity, resource limit or target cost threshold is reduced.

The source/destination overlap is safe with the existing forward loop: source
begins at `nz_pos - 256 >= 0`, destination at zero. Each source element is read
before any later destination could replace it. After compaction, `nz_pos` is 256;
appending at most 256 samples stays within the 512-sample allocation.

`tests/host/run_noise_history.sh` is mandatory in the full regression suite. It
compiles the accepted ccf861366ba59514db9228e069c6b87add967050 renderer and header
from pinned Git blobs with separate symbols and correctly sized independent
storage. It compares compact versus accepted output bitwise for each of eleven
voices, mixed voices, enabled-strip transitions and complete 256-frame blocks.
Variable blocks include 1, 255, 256, 17, 128, 2, 254 and 64 frames, with adjacent
and delayed retriggers that exercise skipped click/tom filter warm-up. All dry
stereo, sends and strip meters must agree exactly across 33600 blocks.
The mandatory suite sets `NOISE_SKIP_BENCH=1` to omit only the optional timing
loops; all differential comparisons remain required. The accepted X0X snapshot
and the existing pre-variant kick oracle are the independent reference baselines.
No original external 9W9 runtime comparison is claimed.

Native differential and AddressSanitizer tests passed. An additional combined
ASan/UBSan run returned zero differential failures but exposed inherited signed
left-shift warnings at fastmath.h:57 in both the accepted and compact renderers.
It is not a clean UBSan acceptance result; that unrelated implementation is
unchanged here.

Official target build against the same accepted checkpoint:

| Resource | Accepted | Compact | Change |
|---|---:|---:|---:|
| Image | 570020 B | 570020 B | 0 |
| RAM data+bss | 97636 B | 97636 B | 0 |
| Pool | 334612 B | 333588 B | -1024 B |

All fifteen original static gates passed with identical function metrics. In
particular, d9_render remains 2909 instructions, 2038 loop instructions, six
divides, ten calls and static cost 3994. No baseline was modified.

At full block size, history copying occurs every block instead of every other
block: approximately twice as many compactions, with 256 float loads/stores per
compaction. Three alternating host runs over 8388608 mixed-909 frames measured
compact/accepted CPU ratios 0.9982, 1.0145 and 1.0120. These small differences
include host scheduling noise. Target static metrics do not account for the
increased runtime frequency of the unchanged copy loop. Physical FM-1 copy cost,
worst-block timing and audio acceptance remain pending.

This saving alone does not certify the complete future 25-strip resource design;
every processor and continuous meter remains independent. Further memory changes
require separate review and tests.
