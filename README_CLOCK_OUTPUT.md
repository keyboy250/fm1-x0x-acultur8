# Private 0.1.28 clock-output candidate

Authorized scope: isolated clock-output correction and local qualification only.
Parent: sealed 0.1.28-inboxfix source SHA25672c382d4f91e0a5565dd304b18d729f09df0e2c31dc78acdbb55377c596f85c5.
Original sources and installed device are preserved. Current status is CLOCK_OUTPUT_STATUS.json;
other version/status files describe historical candidates.

The physical USB producer attaches TIMER4 deadlines corresponding to each rendered
sample position and the upcoming DMA half. The2kHz timer may transmit already
published due MIDI packets during audio render. RX/editor/configuration servicing
continues outside render. Shared musical FIFO ordering is retained for clock,
transport and notes. Stop retires future deadlines and stale attacks/realtime;
owned NoteOff cleanup survives. Endpoint handoffs already committed remain subject
to the existing16-word non-recallable boundary. SysEx priority is unchanged.

Transient272-byte metadata lives in free NOINIT RAM and is explicitly initialized
at USB startup; it is not saved state. There is no change to musical algorithms,
project format, MIDI addresses, default settings, audio buffer size or pool reserve.

Local checks: tools/clock_output_local_check.sh, tools/clock_output_target_check.sh,
and tools/clock_output_extra_check.sh. The scheduling simulator checks180 clocks
per tempo at20/60/120/134/240/300BPM against the production USB consumer. Its
sub-millisecond scheduling result excludes USB bus/host timing and hardware load.
A negative control with the deadline gate removed must fail the future-event test.

Before release: separately approve installation with fresh project/strip backups
and the byte-verified inboxfix rollback. Then repeat physical driver-timestamp
P9 delivery/clock/Stop checks and listening/performance checks. Physical acceptance
must confirm the block-sized clock spacing is gone and check jitter/tempo using
preserved raw events. Reason sync and audio/MIDI alignment remain separate untested
scopes. Do not infer those from local tests. Firmware publication remains held.
