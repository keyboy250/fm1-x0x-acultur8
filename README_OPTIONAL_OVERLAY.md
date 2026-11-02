# Optional live drum overlay — private0.1.20-optbeta candidate

Separate child of frozen0.1.19-livebeta. Local testing only; no install or publication.

On909/808 ARP > VAR OPTIONS, KNOB4 is OVERLAY: OFF / ON. It controls both
the selected armed drum grid overlay and key LEDs. It starts OFF at UI init,
project load and reboot. It is a session-only display preference, never saved
in the project or Undo, and it has no CC binding. It can be switched while
stopped or playing. Arming variation does not enable it. Existing303 pages
and their fourth control retain their original function.

OFF returns before any overlay interrupt guard, captured-prefix read or scan.
No accepted-hit marks/white overlay flashes/dim-base LED rendering are used;
normal pre-overlay indicators remain. The accepted-event Freeze recorder is
an existing engine feature; this switch does not disable that recorder, audio
variation or the separately requested command50 diagnostics. ON retains0.1.19
overlay behavior. No new audio-path logging or static target RAM is intended.

Local byte-for-byte PCM/MIDI and rendered indicator parity, and absence of
extra guard calls when OFF, are testable. They do not prove zero physical
latency/jitter or IRQ/USB/LCD timing impact. Keep the switch OFF until that
separate hardware timing comparison is accepted. Current FM1 remains0.1.19,
playback stopped and MASTER down at the user's last confirmation.
