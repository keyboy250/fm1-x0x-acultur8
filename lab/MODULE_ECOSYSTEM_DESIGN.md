# Acultur8 module compatibility and multi-FM1 ensemble

Requested2026-10-14. Proposed direction, not completed plugin/device ecosystem.

## What modular means here
Current C DSP, sequencing, platform and browser/editor units are source modules compiled into a single firmware or WASM engine. The device does not load arbitrary runtime plugins. Third-party modifications require review, rebuild, preserved data migration and resource qualification. A System-100-inspired ensemble means several FM1s with complementary musical roles and coordinated control; it does not give an FM1 analogue CV/gate connectors or new synth voices.

## FM1-41: reintegration compatibility
Maintain a module manifest: stable module ID, author/license/source commit, version and public API/ABI; parameter IDs, units/ranges/defaults and capabilities; dependencies; project/preset schema and migration; MIDI mapping/protocol revision; required CPU/RAM/flash/state; and supported browser/firmware/hardware versions. Start with documented contracts and machine-readable validated manifests. Identical note or CC numbers do not guarantee identical meanings.

Compatibility matrix separates builds from actual device/transport acceptance. A modified module enters through a feature branch/PR and tests against pinned reference behavior: render/state/pixels where applicable, parameter coverage, save/reopen/older projects, undo, clock/transport/NoteOff cleanup, overload and original resource gates. Never silently load a preset or command into a mismatched engine. Prefer descriptor/capability detection; unknown or unsupported features show read-only/unavailable with a reason. Hardware capability negotiation is future protocol work, not supplied by the current WASM snapshot ABI or by the installer.

Review licenses and attribution before code reuse. Retain recoverable prior modules/projects and versioned migrations; no automatic downgrade or destructive conversion. API and persistence breaking changes require explicit migration/version bump. Declare golden reference baselines and exact release commit, including build flags and supported hardware. Registration alone is not CPU or sound acceptance.

## FM1-42: ensemble roles and group control
Named devices can take drums, bass, breaks, accompaniment or alternate scene-preparation roles. Multiple FM1s share one explicit clock leader while independent lane cycles continue. Standard MIDI clock/transport works with compatible external devices; chords, presets, parameter automation and scene identity require separately validated mappings or device-specific supported commands. No assumption that community firmware shares Acultur8's protocol.

Provide a topology/role view, per-device firmware/capability status, MIDI input/output/channel routing, audio labels and master level/transport intent. MIDI timing is separate from audio mixing. Multiple same-named devices require explicit persistent aliases and reconnection checks; unknown identity must never receive bulk commands or flashing. No automatic MIDI permissions/routes. USB peers need a computer or genuine host/router; the current FM1 firmware provides USB device mode and TRS input, not a daisy-chain host or TRS Thru.

Use acknowledged per-device commands and visible partial failure. Do not describe multiple independent device updates as atomic. Stage a group scene/cue with each compatible device ready, commit on the agreed boundary, and define cancel/late/offline response without stopping healthy devices unexpectedly. Where generic gear cannot acknowledge scenes, label delivery as unverified and use compatible/manual cues. Stop/Panic requires bounded route-specific cleanup and no stale epoch resurrection.

Explore A/B preparation and long set lists on the controller with bounded device payloads; no unmeasured firmware-bank enlargement. Physical group performance requires measured sync/jitter, real2hour soak, saved recovery, disconnect/source loss and actual audio mixing. Browser multiple-engine tests are useful but do not qualify physical devices.

Acceptance depends on FM1-39/40 and full CC/scene coverage: validated manifests/matrix, incompatible-command rejection, third-party-module fixture integration and migration/resource tests; then two physical FM1s and a third-party follower with role assignment/cues/cleanup/restore proof. Keep existing all-25/core architecture intact while distributing musical roles. Additional engines/analogue interfacing require their own scope and feasibility.
