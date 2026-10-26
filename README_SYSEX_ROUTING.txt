0.1.27-sysexfix / FM-1_9000127 - PRIVATE LOCAL CANDIDATE
Parent: credited frozen 0.1.26-copymidi source; original packet retained unchanged.
Preserve complete editor SysEx headers in ota_editor_service() for the next editor pass.
Use this wrapper only in the normal editor-enabled loop. Keep ota_take() and
ota_service() unchanged so active updater sessions and recovery boot drain foreign frames.
Drain incomplete headers; retain updater, backup and invalid-frame handling.
No MIDI addresses, schema, musical algorithms or persistent formats changed.
Original README_MIDI_COPY.txt and earlier notes describe their historical builds.
Current qualification is recorded separately in SYSEX_ROUTING_STATUS.json.
Local tests do not establish physical USB/TRS reliability or audio timing.
Do not install or publish without the separately authorized hardware test step.
0.1.26 remains unpublished. This preparation does not access the device.
