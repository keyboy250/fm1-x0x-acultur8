Local upstream integration is documented in [the integration record](docs/UPSTREAM_INTEGRATION_20261017.md). It is a new, uninstalled qualification candidate; publication remains held.

# Acultur8 X0X for the M-VAVE FM-1

An experimental variant of [Charles Vestal's X0X](https://github.com/charlesvestal/fm1-x0x), with a local development dashboard and browser firmware emulator for musical experiments.

The browser runs the shared C engine compiled to WebAssembly. It supports WAV slice banks with exact markers and sequencer-tempo playback that keeps each slice's pitch. Browser performance does not establish physical FM1 processing headroom.

## Release status

The public releases are browser prototypes. This branch prepares an **Extended Edition qualification candidate** for the FM1; publication and GitHub Pages deployment remain held.

The candidate includes all25 sound strips, arrangement/chord metadata, bass following, MIDI accompaniment and routing,21 expanded rate divisions, explicit per-sound saved filter/limiter defaults, and the three-second X0X / EXTENDED EDITION startup. Saved defaults are applied explicitly to a selected sound and never silently replace a project's settings.

The prepublication audit found a native message-buffer overflow. This branch bounds message copies, adds a required regression, and corrects a signed exponent-bit shift found by strict sanitizers. Full software qualification and physical release acceptance are tracked separately in [release readiness](docs/RELEASE_READINESS.json). No stable hardware-release claim is made.

- Integration branch: `acultur8/main`.
- Local lab: [setup and workflow](lab/README.md).
- Implementation and acceptance: [variant plan](lab/ACULTUR8_VARIANT_PLAN.md).
- Feature status: [status matrix](docs/ACULTUR8_STATUS.md).
- Changes: [changelog](CHANGELOG.md).
- Original instrument documentation: [manual](docs/MANUAL.md) and [upstream README](docs/UPSTREAM_README.md).

## Slice workflow

Import a matching slice-bank WAV and marker JSON/CSV in the emulator. Markers retain both audio storage boundaries and musical onset positions. **Play with FM1 · keep pitch** schedules slices against the sequencer clock; the PCM playback speed stays unchanged. This is slice sequencing, so slower tempos can expose gaps and faster tempos can overlap tails.

Native REX decoding is an optional external local tool. The public variant does not distribute the decoder, restricted SDK files, or private loops. WAV/marker import and synthetic examples work without that optional decoder.

## Upstream and licenses

The initial variant is based on upstream commit `47f8e4c4ad602591b81c21bc5480db7a94f6d6af`. The imported baseline was compared against the supplied source archive before applying the emulator/slice changes. Upstream history remains available on `main` and through the upstream repository.

X0X is GPL-3.0-only. Preserve [LICENSE](LICENSE), [LICENSING.md](LICENSING.md), bundled notices and original author credits when redistributing. Open303 and bundled fonts retain their respective licenses. Recorded loops belong to their owners and are not covered by this project's license. Acultur8 X0X is not affiliated with M-VAVE, Roland, Reason Studios, IK Multimedia or Hügelton Instruments.
