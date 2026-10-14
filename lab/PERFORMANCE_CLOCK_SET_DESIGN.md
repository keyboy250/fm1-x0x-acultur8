# Performance clock and one-to-two-hour sets

Requested 2026-10-13. Planned, not qualified for live hardware. No purchases, flashing, new public publication or automatic MIDI permission/routing.

## Current source evidence
Local firmware/src/seq/sequencer.h documents24PPQN, incoming Clock/Start/Continue/Stop and send_clock. sequencer.c emits clock/transport when internal. engine.c dispatches F8/FA/FB/FC. MIDI output uses the USB queue. midi_uart.c implements TRS input, not a physical MIDI output/Thru. The USB stack is a device endpoint implementation, not an accepted USB host. USB and TRS feed one ingress path; explicit single-source clock ownership is not implemented. sequencer.c falls back to internal after0.5seconds without external clocks. Do not label that legacy behaviour as a qualified live recovery policy. TRS system-common messages, including Song Position Pointer, are presently ignored; do not promise arbitrary seek/locate.

Primary references checked2026-10-13:
- https://midi.org/basic-of-usb — host, peripheral and hub topology.
- https://midi.org/about-midi-part-3midi-messages —24clocks per quarter note and transport/position distinction.
- https://charlesvestal.github.io/fm1-x0x/manual/ — upstream clock and arrangement guidance; not our physical acceptance.

## Connections and role clarity
Use one clock leader per connected timing domain. Independently routed groups can deliberately use different leaders, but never mix them into the same follower. A powered USB hub distributes host ports/power; it does not itself forward MIDI between instruments.

Computer topology: one computer/DAW or qualified controller routes a single clock and transport to FM1A, FM1B and other devices through its USB ports/hub. Alternatively FM1A generates clock to the host, which forwards it once to selected follower outputs; suppress all return-clock echoes. Standalone topology requires a genuine USB-MIDI host/router compatible with multiple peripherals, or an external clock source and MIDI distribution to FM1 TRS inputs. Ordinary USB cable chaining FM1-to-FM1 is unsupported. Verify adapter pinout, host compatibility, routing and power per actual equipment before recommending or purchasing it. No physical FM1 TRS OUT/THRU assumed.

Audio is a separate path from timing: route each instrument's audio to a mixer/interface. MIDI clock does not mix audio, distribute chord changes, select patterns or carry our project scenes.

## Performance Clock interface: FM1-39
Provide Start alone / Lead connected devices / Follow selected source as plain-language modes. Separate clock ownership, transport ownership, note/control input and audio monitoring. Device aliasesA/B must survive identical device names and require explicit reconnection mapping when identity is ambiguous.

Show chosen source, destinations, role, intended versus measured BPM, elapsed beat/bar, Running/Stopped/Waiting/Clock lost, last received tick age, jitter/drop/queue counters and optional per-destination offset. Never infer bar alignment from BPM alone. Source selection must exclude other incoming clocks/transport without excluding authorised notes/CC. Avoid doubling clock through mirrored routes. No startup permission requests or implicit output connections.

PLAY starts from defined origin; Continue resumes retained position; cue/re-align activates at a defined shared boundary, where supported. Same-tempo devices can still be out of phase. MIDI Song Position Pointer support needs explicit parser/engine/receiver acceptance; scene identity and arrangement metadata need capability-specific commands or agreed manual cues. Use standard clock/transport for other devices, not assumptions about proprietary scenes. Role/source change while playing must be a deliberate boundary-safe transition with note cleanup.

Clock-loss policy is configurable and visible: stop/freeze at the last state with deterministic note cleanup; bounded holdover at measured tempo; or deliberate internal takeover. Legacy default behaviour stays compatible until the user selects the new performance mode. No silent leader election or unexpected takeover. Reconnect waits for explicit cue/start/resume; old queued clocks and old route epochs cannot resurrect playback. Stop/Panic outrank ordinary automation while preserving valid Note Off cleanup. Plan priority/reservation against saturated existing queues and measure flash/RAM costs.

Timing generation/forwarding needs timestamped bounded scheduling and measured delay. Avoid a UI animation or unqualified timer loop as the authoritative clock. DAW/hardware timing is the initial qualification reference; browser-only leadership is accepted only after foreground/background/throttling/disconnect and actual device measurements. Small fixed lookahead does not permit unstoppable clock batches after Stop.

## Long-set workflow: FM1-40
Create a browser-hosted named Set List with tracks/scenes, per-device prepared pattern/block references, repeats, chord progression, mutes, transition cue, tempo policy, sound variants and declared dependencies. Store the set list on the computer rather than enlarging the tiny firmware song table by assumption. Preload bounded current/next material; public demo assets only in fixtures. No new persistent hardware banks or project loading mid-audio without a tested engine contract.

Current song model has192blocks and1..8repeats, not unlimited arrangements or seamless multi-project loading. At125BPM in4/4,1536bars lasts about49minutes9seconds. Longer sets can reuse manually queued patterns today; a planned two-hour set manager must qualify replenishment/scene control beyond that limit. At125BPM two hours is3750bars; test longer/faster runs too. A rolling128bar timeline and scheduled epoch/clock counters must not wrap or lose origin. Song Position Pointer has finite range; never use it as an unlimited wall-clock or promise unsupported relocation.

Build a musical performance from prepared reusable sections: intro, groove, variation, breakdown and transition. Live scene launches quantize to next bar/4/8/16bars as chosen, display queued/confirmed versus rejected state, and can be cancelled before commit. Internal lane loops can remain independent while all devices share the conductor pulse; define optional lane reset separately from ordinary scene launch. Pattern changes on generic gear require verified ProgramChange/CC/MIDI-note capability or manual action; Acultur8-only commands need engine acknowledgements. Standard clock alone cannot deliver those changes.

For two FM1s, explore anA/B workflow: one sustains the current music while the other prepares the next supported scene, then their audio blends through a mixer. Seamless preparation, no audio gaps and external cross-device cue alignment are acceptance requirements, not current guarantees. Full project loads currently need stopped/safe boundaries; do not advertise live double buffering until implemented/measured.

Maintain saved checkpoints and rehearsed recovery actions: stop all, mute outputs, clear held MIDI notes, resume at selected shared cue. Device cable loss must not stop unrelated healthy followers unless the chosen policy requires it. Capture lightweight clock/drop/transition telemetry without overflowing firmware memory or publishing private music.

## Acceptance and delivery
1. Audit currentclock/transport paths, source tags, queue capacity and callbacks; freeze routing/ownership and compatibility contract.
2. Native deterministic tests: exact24PPQN/count/Start/Continue/Stop, input-source exclusion, loop rejection, saturation, tempo ramps, queued scenes and per-lane phase; first prove unchanged legacy playback.
3. Browser testbench with two actual-WASM instances and synthetic router; test offsets, missing/delayed/bursted clocks, duplicate sources, Stop while queued, hotplug, background behaviour and permission denial. This is software evidence only.
4. Implement acknowledged set-list cues and bounded staging/persistence, cancel/undo/save/reopen/corrupt-set rejection. Prove transitions across current song capacity with continuous audio/no dropped events. Set features depend on missing control/scene contracts and measured native capacity, not silent scope reduction.
5. Fast simulated runs beyond2hours and timer/counter rollover plus real wall-clock2hour browser audio/MIDI soak. Logs must separate simulated and actual wall time. Verify each planned BPM, bursts of CC, both basses, drums, slices and external accompaniment; listen for continuous clicking/underruns. Predeclare measurable jitter/phase/drop acceptance limits for each transport before claiming live suitability.
6. TwophysicalFM1s and a third-party follower: measure clock/phase, start/stop/reconnect, power and routing; confirm no stucknotes and rehearse failure recovery. Physical proof/flashing pending separately. Preserve all15originalresourcegates, RAM/pool reserves, processor/deadline bounds.
7. FAQ diagrams: leader/router/followers and separate audio routes; Start versus Continue versus cue; clockloss/recovery; setlist,current,next andA/Btransition. Labels explicitly markplanned until the relevant acceptance passes. Publication hold unchanged.
