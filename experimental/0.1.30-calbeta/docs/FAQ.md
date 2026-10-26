# MIDI CC beta FAQ

**Which build is this?** Exact frozen 0.1.30-calbeta, identity FM-1_9000130. The public-facing purpose is the MIDI CC experimental beta; the version string is preserved so firmware and source remain matched.

**What is deferred?** The 0.1.31 averaging modes, 0.001-BPM controls and further Reason-clock work. These controls are absent from this firmware and source.

**Is all MIDI CC hardware-qualified?** No. Local native/WASM production-ingress suites pass. Bounded physical USB mode-filtering/readback checks pass. Advanced remote generation, variation, Freeze/Commit and copy hardware replay remain open. Physical panel checks do not close remote CC action gates.

**Where are the controls documented?** The searchable [MIDI chart](midi/index.html) includes canonical ranges, addresses, conversion, targeting, action guards and readback. Its 751 entries distinguish firmware, browser-only and unmapped features.

**Will it fix Reason clip jumping?** No such claim is made. The tested Reason external-clock setup remains unresolved. The failed averaging candidate stays private.

**Is calibration included?** The pre-existing experimental per-unit facility remains in 0.1.30 and its source. One unit passed measured Apply/Reset and reboot checks; other units must be measured independently. Never copy another unit's calibration or raw flash.

**What is covered by backups?** Project/current-strip exports do not independently back up sample slots, the panel calibration record or the separate saved-default bank. Retain an exact prior firmware and recheck project bytes/settings after any separately authorized installation. This packet performs no device update.

**What is the release status?** Approved as the explicitly scoped experimental beta described in ACCEPTANCE_REVIEW.json. The GitHub prerelease is the authoritative publication record.
