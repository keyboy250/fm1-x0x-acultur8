X0X Extended Edition 0.1.5-beta1 - Experimental prerelease
USB identity: FM-1_9000105
Prepared2026-10-19UTC. Local review packet; not yet published.

This build is for exploratory testing and feedback, not stable production use.
It retains per-sound processing, saved strip defaults, independent lane timing,
expanded rates, arrangement/harmony and the Extended Edition startup display.
The new909optimization batches strip work while preserving the shared DSP.
The installed test build and the firmware in this packet are byte-identical.

READ KNOWN_ISSUES.txt and INSTALL_AND_ROLLBACK.txt before installation.
Keep project, samples and preset/default backups separately. This packet contains
no user's project, recordings, flash dump, calibration or credentials.

Performance warning: heavy combinations of enabled strips/effects can cause
audio glitches. Reduce processing and check PERF for new dropout counts.
Further improvements are planned, without a promised completion date.

For external MIDI performance, set GLO > PROJECT > AUTOSAVE to OFF while the
external DAW is stopped, and allow its final preference save to finish. Save
edits manually after stopping playback. OFF means unsaved edits may be lost
on power-off. The existing guard checks internal transport, not MIDI activity.

This packet changes no routing, defaults, warning UI or on-device About page.
The warning is documentation, not an implemented automatic overload message.
Pages and public publication remain held pending review of these exact assets.

FAQ: open docs/FAQ.html in a browser. It includes the release-specific FAQ
and the complete maintained illustrated guide, with historical checkpoints marked.

FAQ revision 3 adds offline topic search across current and historical guidance,
descriptive/search/social metadata, and conceptual experimental-release poster art.
Public indexing and social preview image URLs depend on approved hosting; Pages held.
