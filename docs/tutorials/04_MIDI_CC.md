# 4. Use the complete MIDI / CC map

[All tutorials](README.md) · [803-row map](../CC_MIDI_MAP.md) · [Searchable chart](../CC_MIDI_MAP.html) · [Retained kick ABI](../KICK_PRESETS_0.1.36.md)

## Start with the destination and status

The map uses one-based MIDI channels 1–16. An inventory parameter number is not a CC number. Every row includes its address when assigned, stored range, context/guard information and runtime status. Download CSV/JSON for the complete fields, choices and scaling. The chart does not turn unmapped/native-only rows into remote controls, and optional CC14 remains a proposal where labelled.

Native GLO **CC MODE** is selected deliberately on the device. Read the current mode before testing. Source-owned committed contexts choose pattern, part/kit/voice and destination; moving the panel focus does not safely replace that context. Browser-only slice controls and proposals must not be treated as physical FM-1 bindings.

## Stored versus displayed values

Kick WAVE ENV W DECAY is `hybrid808.session.32`, parameter **1282**, channel **15**, CC **82**, stored range **0–127**. For this full 0–127 range, a raw value of 75 corresponds to the native 1,773 ms setting. Sending 75 does not mean 75 ms. Other domains use the row's scaling, not necessarily a one-to-one raw CC value. Exact-value transactions are a separate documented protocol.

## Guarded preset actions

Preset STORE/RECALL require stopped playback and a finished tail. The retained ABI uses committed 808 BD context, destination slot 0–15, an increasing request token, arm and execute operations. It is source-owned, guarded by revision/project/transport state, and independent of panel focus. Read the linked ABI before implementing a controller; do not send a bare execute CC as a generic preset-change command. Native panel slots are displayed 1–16 while protocol destination indices are 0–15.

## A practical test sequence

1. Preserve work; stop playback and turn MASTER down. Configure input/output channels deliberately.
2. Identify one implemented row and its required context. Record its original value and readback.
3. Send one controlled change through the supported protocol, then compare intended and audio-owned next-hit state. Neither alone proves audible or wire-timing acceptance.
4. Restore the original value and verify it before proceeding. Do not automate arbitrary destructive or stopped-only actions during a performance.
5. Test a short, quiet performance and inspect counts only after stopping. Retain DAW, browser, host, USB/wire and physical-device evidence as separate scopes.

The ordinary default 808 drum-note route is channel 11 with kick note 36, but project settings can change routing. Check your own GLO MIDI/channel configuration. MIDI note input, clock and CC automation are separate paths. This prerelease does not claim complete acceptance for every DAW/controller combination.
