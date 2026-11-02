0.1.25-freezemidi / FM-1_9000125 - isolated local candidate
No installation or publication;physical delivery/audio/timing acceptance separate.

FREEZE / COMMIT MIDI
Seven additive addresses preserve all714 parent assignments. All22 drum lanes
and both303s use explicit pattern/part/kit/drum or bass/strip context,independent
of UI focus. One global capture;one source may own a pending remote Freeze.
The fourth source-private recipe family reuses the existing16-byte recipe and
72-byte source state. Switching family/source reset/CC mode invalidation cancels
that source's pending request;an already HELD capture is not thawed by Cancel.

Human CH14 CC108 ui.freeze_commit.destination:zero-based destination0..31.
Human CH14 CC109 ui.freeze_commit.freeze_request:execute low0/high1 after arm6.
Human CH14 CC110 ui.freeze_commit.commit_confirm:execute low0/high1 after arm7.
Human CH15 CC5..8 remote.capture.token_byte0..3:expected32-bit capture token,
little endian. Stage all four bytes with exact framing (byte0..255);CC7 cannot
represent every byte. Shared request_low7/high7,action_arm and Cancel retained.
Arm0 disarms;2=303 generation;3=drum generation;4=variation ON;5=variation OFF;
6=Freeze;7=Commit. One active recipe family per source;fresh monotonic14-bit IDs.

FREEZE
Require capability bit16,CC MODE FULL,writable/unowned project,matching live
variation AND matching current playback pattern,drained queue and no transport
transition. Freeze can be requested while playing.303 requires FWD and no first
TIE. It snapshots accepted gates/final pitches through the existing audio-owned
full-pass FSM. Receipt goes ACCEPTED -> PENDING -> APPLIED when HELD. Querying
does not advance receipt state;normal main MIDI service observes completion.
PENDING is not success. A stopped incomplete request refuses unchanged. A complete
stopped pass may freeze immediately. Repeating an unchanged request deduplicates;
freezing an already HELD capture is NO_CHANGE. Base/project/Undo/save stay unchanged.
Project/runtime/owner changes while pending cancel/refuse;Stop before HELD refuses.

READBACK - command51 ABI1,CRC/token/source/page/index validation
Every page starts revision,queue_written,queue_read,transport_epoch,remote_generation.
Page0:map1,ABI1,feature_bits31 (3031,drum2,diagnostics4,variation8,Freeze/Commit16),
sources3,identity9000125. Page7 recipe_kind3=Freeze/Commit.
Page5,index0: capture_id,owner_lane(255=OFF),owner_pattern,state(0 RECORD/1 REQUEST/
2 HELD),length,rate,count,complete,note_data_valid,gate_data_valid,context_matches,
destination,destination_valid,operation(0 Freeze/1 Commit/255 other family),
expected_capture_id. Partial metadata can be read;partial step data cannot.
Page6,index0..63: capture_id,step_zero_based,note,gate,accent,slide,flags,valid.
Only complete HELD steps below length are available. Captured303 pitches are
final sounding notes,including harmonic projection;gate0 REST/1 NOTE/2 TIE.
flags mask:gate bits0..1,accent4,slide8. Drum note_data_valid=0;accepted gates
remain valid. Rejected/chance-muted steps are captured as rests. This is captured
engine state,not physical receiver delivery or a general event history.
capture_id is a32-bit content fingerprint of the runtime variation recipe,
length/rate and active captured note/flag bytes,not a monotonic session identifier.
For a scan:read complete page5,all active page6 steps,then page5 again. Require
same content token and all five stamps plus identical headers;retry at most3
times. Never stop/save or fall back to partial data to make a query succeed.
tools/fm1_capture_read.py implements that bounded read-only scan;the underlying
tools/fm1_generation_read.py now supports --index for captured steps.

COMMIT
Stop/drain playback;read a coherent complete capture first. Explicitly stage a
different destination and all four expected token bytes;new request,arm7,execute
low/high. Wrong/absent token,changed capture/project/epoch,destination,owner,
protection,recording,unavailable scratch/Undo or pending transport refuses.
Guard is rechecked after reserving the stopped staging transaction. A legacy
shared drum kit destination must already match captured length/rate;remote
Commit refuses otherwise rather than silently truncating or changing peer lanes.
Only the selected destination row/303 is copied through the existing native
transaction. Captured probability results are baked into gates and only that
destination probability lane removed;destination accents,swing,sounds,other
lanes/motion and tails remain.303 stores FWD/neutral transpose and disables
destination auto-mutate so saved notes remain stable. Source pattern preserved.
Undo/Redo and save/reload use existing project format. Commit does not itself
send SAVE;normal autosave policy is unchanged. Identical musical result is
NO_CHANGE. Runtime capture resets on project-load/reboot;committed notes persist.
Read page4 matching result and command49 actual musical destination independently.
New reasons12=capture incomplete,13=unsupported capture direction/layout,
14=same or missing destination. Earlier states/reasons retain their numbers.

TOOLS / LOCAL TESTS
tools/fm1_freeze_plan.py only prints explicit public wire plans;no port/transport.
tests/host/run_midi_freeze.sh:24 lanes x3 formats x3 sources;full64-step readback,
pending/completion,explicit token/destination,whole-project isolation,Undo/Redo,
save/reboot,Cancel/stale/owner/mode/refusals;independent native UI fixture path.
tests/test_freeze_readback.py:actual native wire frames,CRC/semantic validation,
64 steps/three sources and coherent/bounded retry scans;no ports.
tests/host/midi_freeze_wasm_test.mjs:all24 lanes through production public MIDI
plans;PENDING/full-pass completion,actual capture note/flags,whole saved-object
native golden parity,Commit staging,dedup,Undo/Redo/Cancel/save/restart.
tools/midi_freeze_check.sh:full134 required/39 host scenarios,old303/drum/variation
MIDI/native/wire checks,ten inherited native/ten inheritedWASM suites,boot footer.
tests/host/run_midi_freeze_target.sh:original15 unchanged resource gates.
GLO CC MODE FULL/SIMPLE/OFF remains session-only. Readback and standard non-CC
notes/clock/Start/Stop remain available;new custom Freeze/Commit CCs require FULL.
Scoped-copy MIDI remains next work. No new RAM allocation,ISR capture writes,
project schema or physical acceptance claim is introduced by this interface.
