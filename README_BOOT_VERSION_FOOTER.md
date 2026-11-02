# Boot version footer — separate0.1.21-verbeta candidate

Shows the actual X0X_VERSION release string in small centered text at the bottom
of the native FM1 loading animation, above its existing progress bar. Development
builds without a release string show vDEV. The existing fade applies to the footer.
No runtime overlay/default/project/audio-path change is part of this candidate.

Local scope: focused boot/gfx sanitizer checks plus target build and original
resource gates. Full134-check suite and physical startup visibility are not yet
claimed for this small candidate. Installed device remains0.1.20; no flashing or
publication. Continue its physical OVERLAY OFF verification independently.
