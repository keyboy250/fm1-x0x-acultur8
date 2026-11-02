Drum MIDI generation and GLO CC diagnostics - private0.1.23-drumbeta
Parent:frozen0.1.22-midibeta. Source and local candidate only; do not install.
Consult MIDI_DRUM_STATUS.json for actual completed checks and exact hashes.

DRUM INPUT MAP
All703 occupied addresses preserve the previous698 channel/CC assignments.
Human channels1..16,wire CC values0..127;canonical values often differ.
1206 CH14 CC94 density0..100
1207 CH14 CC95 positions0 ALL /1 EVEN /2 ODD /3 OFFBEAT
1208 CH14 CC96 anchors0 NONE /1 BEAT /2 BACK /3 KEEP /4 BEAT LOCK /5 BACK LOCK
1209 CH14 CC97 seed index0..255;effective deterministic seed=index+1 (1..256)
1210 CH14 CC98 execute low0 then rising1
Reuse1223 CH14 CC111 length0 KEEP /1..64;legacy formats refuse changed length
until independent lane storage is initialized. No automatic project conversion.
Reuse1231/1232 request ID,1233 arm code3,1234 cancel.
For arm3,raw54 is valid;the planner chooses the lowest raw encoding that maps3.
The older303 arm code2 and all303 addresses remain unchanged.

TARGET AND RECIPE
Commit explicit pattern0..31,part/kit0=909 or1=808,drum0..10 and matching
strip=kit*11+drum. Never borrow the panel,playing pattern,or current drum.
Only selected row hits and optional selected length change;accents,sounds,
motion,swing,other drums/parts/patterns remain. Copy a base first when needed.
All positions and steps here use native semantics:EVEN=one-based2,4,...;
ODD=1,3,...;OFFBEAT=3,7,... . BEAT anchors1,5,9,13...;BACK anchors5/13
within each16-step group. KEEP preserves previous hits;LOCK preserves the
previous on/off anchor values. Position/anchor conflicts refuse unchanged.

One active generation recipe family per source. First drum-specific field
switches to drum and clears old303 recipe/seed/arm;first303-specific field
switches back. Shared length should be sent AFTER selecting the family.
All four303 seed bytes are still required;drums require explicit seed index.
New fields live in the existing16-byte per-source recipe;no extra source RAM.
Stage recipe,request,arm3,execute;read a matching command51 result and verify
actual command49 rhythm/length independently. All values can use existing
ordinary-CC exact transactions;the plan tool handles scaling and byte chunks.

GUARDS
Stopped/drained,unowned/unprotected project required. Arm captures revision,
fingerprint and transport epoch. Recipe/context changes invalidate arm.
Request IDs increase monotonically across both families for a source. Repeated
completed untouched request does not generate twice;changed/reused IDs refuse.
Cancel disarms unexecuted work only;completed changes use normal Undo/Redo.
Query after timeout rather than replaying a mutation blindly. No auto-stop,
auto-save,restore,device installation or publication in the controller tools.

READBACK COMMAND51 ABI1
Existing framing and pages0/1/2/4 unchanged. Page0 feature_bits=7:
bit0 303 generation,bit1 drum generation,bit2 CC diagnostics;identity9000123.
Page2 is available only for active303 family;new page3 only for drum family.
Every successful payload begins revision,queue_written,queue_read,
transport_epoch,remote_generation,then the fields below.
Page3:density,positions,anchors,seed_index,effective_seed,length,seed_valid.
Page7:cc_mode,recipe_kind(0=303,1=drum),accepted,rejected,unsupported,busy,
coalesced,overflow. Counters are aggregate ingress counters,not reset by query
or per-feature application receipts. No new hidden clear-on-read behavior.
Page4 retains correlated request,state,reason and before/after stamps.
Pages5/6 remain unsupported. An inactive family's recipe page refuses instead
of exposing the union bytes with misleading meanings. Query errors never
substitute writes,save or export. Command49 supplies actual musical state.

GLO > MIDI > CC MODE
FULL0:existing full custom CC map,including guarded generation.
SIMPLE1:only fixed direct sound descriptors and sound-strip CCs;selectors,
exact framing,generation,CC transport,history,storage and MASTER-gain controls
are blocked. This isolates direct continuous control from structured commands.
OFF2:blocks all custom CC changes. Notes/Note-Off,MIDI clock/Start/Stop and
read-only editor queries remain available. Standard CC120..127 handling is
unchanged;these were already outside the custom map.
Mode is native GLO-only;there is no remote CC that can silently re-enable it.
Changing mode clears queued CCs,source contexts/recipes/arms and legacy action
confirmation,while retaining current gain and project values. Already applied
changes are not undone. Non-CC MIDI events are not removed. Re-establish explicit
context after returning toFULL. Mode is session-only,not project/Undo data,and
returnsFULL on init/project-load/reboot. Existing UI alignment padding stores it.
The compatible browser companion also refuses route-tagged slice CC commands
when mode is notFULL;manual browser editing remains separate.

TOOLS AND LOCAL TESTS
tools/fm1_drum_plan.py:plan JSON only,opens no ports.
tools/fm1_generation_read.py:now validates drum page3 and diagnostic page7.
tests/host/run_midi_drum.sh:sanitized all22 lanes/3 formats/3 sources,full-object
isolation,recipe domains,anchor/length behavior,Cancel/stale/ownership/protection,
Undo/Redo,no-op,source/family isolation,GLO and read-only wire queries.
tests/test_drum_readback.py:real editor frames,CRC,token,domain/refusal checks.
tests/host/midi_drum_wasm_test.mjs:public Python wire plans through production
worklet/WASM for all22 lanes,complete native-object oracle parity,save/reopen,
Undo/Redo,request deduplication,Cancel,actual GLO navigation and mode readback,
browser slice mode guard,MIDI Start/Stop unaffected and reboot defaultFULL.
Full134 required checks and all inherited feature/target gates remain mandatory.
Physical MIDI,DAW,UI,output/listening,timing and long-run tests remain separate.
