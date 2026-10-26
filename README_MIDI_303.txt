303 MIDI automation - private0.1.22-midibeta candidate
Parent:frozen0.1.21 boot-footer source. Local implementation, not installed.

FEATURE
Whole-line303 generation or one existing NOTE pitch change, using an explicit
source-owned pattern/bass context, constraint mode, chord block, length/step,
deterministic32-bit seed and guarded request. Supports both303s and all five
existing modes. Stored note flags/sounds/unrelated lanes retain the existing
generation semantics. Single NOTE preserves length, flags, seed and every
other project byte; REST/TIE/inactive targets refuse. No legal alternative
returns NO_CHANGE. Pattern copy first when preserving the selected base.

INPUT MAP (human channels; values listed are canonical, often scaled on wire)
1203 CH14 CC91:mode0..4 (GEN scale,project key,block chord,GEN scale+chord,FREE)
1204 CH14 CC92:chord block0..191
1205 CH14 CC93:execute low0 then rising nonzero
1223 CH14 CC111:length0 KEEP;1..64
1224 CH14 CC112:step0 ALL;1..64 selected active NOTE
1227..1230 CH14 CC115..118:four little-endian seed bytes0..255
1231 CH14 CC119:request low7
1232 CH15 CC0:request high7
1233 CH15 CC1:arm code2 only (raw36 scales to2);0 disarms
1234 CH15 CC2:cancel nonzero
All685 previous addresses remain unchanged. No new drum/variation/Freeze/copy
remote controls are claimed. Optional pairedCC14 is still unsupported.

Reuse existing selector.pattern,part,bass,strip,commit. Part2/bass0/strip22 is
303A;part3/bass1/strip23 is303B. These must agree; visible panel/playing slot
never chooses the target. All four seed bytes must be supplied; seed must be
nonzero. Mode/block/length/step reset0 and seed resets invalid on boot. New
settings live only in the source's RAM, not the project or Undo journal.

Ordinary CC values scale round(raw*maximum/127). Some values in0..255 or0..191
are inaccessible through one CC7 message. Reuse existing exact low/mid/high7
and commit for those canonical values. Exact parameter_high itself scales
0..15:send raw76 for canonical9. Exact targets cannot include context/exact
framing or remote request/arm/cancel framing. Plan tool handles this scaling.

GUARDS AND RETRIES
Stage context,recipe,request; arm2; low/high execute; query same request.
Stopped/drained and unowned project required. Arm captures project revision,
fingerprint and transport epoch. Recipe/context changes, invalid control
values, unsupported arm codes, source reset and boot disarm the request.
Generation reserves stopped transport during application without disabling
MIDI input. A pending Start or queue/owner/protection refusal is terminal and
cannot pin ingress ahead of a following Stop. No new audio-path code added.
One latest receipt/high-water request per source. Use monotonically increasing
14-bit IDs; after wrap/reset establish a fresh source session. Repeat completed
request with identical untouched context/recipe never generates twice; changed
payload with reused ID refuses. Query after a timeout; do not replay blindly.
Legacy destructive-action arming shares confirmation storage and invalidates
the remote receipt/arm. Consume readback before unrelated legacy actions.
Cancellation disarms unexecuted work; it does not Undo a completed edit.

READBACK:COMMAND51 ABI1 (USB editor SysEx; no write,save,flush or clear)
Request excluding F0/F7:7D 46 4C 33 01 token source page index
source0 USB /1 TRS /2 browser; pages0,1,2,4;index0.
Reply excluding F0/F7:same header,ABI,token,source,page,index,status,
success field_count then five7-bit chunks per uint32,payload CRC32 as five
chunks. CRC covers bytes afterF0 through beforeCRC. Status1 invalid request,
3 unsupported ABI/page/index or saturated generation counter. Refusals omit
field_count/payload. All frame bytes between F0/F7 are7-bit clean.
Every successful payload begins revision,queue_written,queue_read,
transport_epoch,remote_generation. These are snapshot stamps, not engine output.
Page0:cc_map1,query_abi1,feature_bits1 (303 slice only),source_count3,
firmware_identity9000122. Feature capability is distinct from the legacy map1 ABI.
Page1:pattern,part,lane,context_ready,armed,staged_request,seed_mask.
Page2:mode,chord_block,length,note_step,seed32.
Page4:receipt_valid,last_result_request,staged_request,state,reason,
before_revision,after_revision,before_fingerprint,recipe_changed.
Before/after stamps are meaningful for an APPLIED/NO_CHANGE matching receipt;
refusal/cancel metadata must not be treated as a committed musical transaction.
Receipt IDs remain explicit even for an older/reused request refusal while
the separate private high-water ID stays newer. Require matching receipt ID.
An accepted arm is not completed generation.
States0idle/1accepted/2reserved pending/3applied/4no-change/5rejected/
6unsupported/7cancelled. Reasons0ok/1context/2stale/3stop/4queue/5protected/
6owner/7value/8missing notes/9unsupported NOTE or auto-mutate/10Undo capacity/
11cancelled. This synchronous slice never reports pending as completed.
Pages3,5,6 of the earlier broad draft remain unsupported; no captured-pitch or
Freeze capability is advertised. Existing command49 supplies independent actual
musical before/after readback. Command50 is unchanged, including its276-byte
maximum full frame. TRS controls do not establish a TRS output/readback path.

TOOLS
tools/fm1_generation_plan.py generates event JSON only; it opens no MIDI ports.
tools/fm1_generation_read.py is a bounded read-only command51 client. No fallback
controls/save/export if unsupported. tools/fm1_pattern_read.py reads musical
state separately. Production WASM exports web_generation_snapshot over the
same snapshot core; native tests exercise actual editor frames and Python CRC
decoder. Browser local fixtures do not establish physical USB/TRS/DAW delivery.

LOCAL QUALIFICATION
tools/midi_303_check.sh runs dedicated sanitizer/wire/production-WASM tests,
original134 required checks and later generation/variation/Freeze/overlay
suites with independent pinned reference history. Target qualification uses
tests/host/run_midi_303_target.sh and the original15 static/resource gates.
See MIDI_303_STATUS.json for exact final results and hashes; this README alone
does not certify a successful run. Missing/failed checks must block promotion.

MEMORY AND PRESERVATION
No project/schema change. Per-source state16B with private bit-packed recipe/
status fields and separate staged/result/high-water IDs; existing layout reordered
to72B. Confirmation guard storage is shared with legacy action arming.
Unused sample buffer tail removed512->480 (actual header480,write chunks256).
Reply buffer288->276 preserves the longest runtime reply; index narrowed to
uint16 and explicit4-byte buffer alignment eliminates padding. Four-byte
generation stamp and three action reason bytes use the pool's remaining8B.
No limits or safety reserves relaxed. Static fit is not hardware timing proof.
Device state/project and public releases were not accessed or changed.
