# Acultur8 FM1 Mod Lab

The local dashboard edits this Git checkout, builds its browser emulator, and tracks acceptance evidence. It binds only to `127.0.0.1`. The public browser alpha is a separate static package; never expose this source-editing server through a public reverse proxy.

## Start from a fresh clone

Requirements: Python 3.10 or newer and Git. Use a full clone containing upstream commit `47f8e4c4ad602591b81c21bc5480db7a94f6d6af`; a shallow clone that omits it needs its history fetched before starting the lab.

From the repository root:

```sh
python lab/server.py 8097
```

Open <http://127.0.0.1:8097/>. On Windows, `./lab/start_dashboard.ps1 -Port 8097` is equivalent. Run `python lab/server.py --help` for path options. No decoder, SDK, Node.js, Docker or target toolchain is needed to start the dashboard.

| Setting | CLI option | Environment variable | Default |
| --- | --- | --- | --- |
| Editable source | `--source-root` | `FM1_SOURCE_ROOT` | Repository containing `lab/` |
| Local runtime data | `--data-dir` | `FM1_LAB_DATA` | `<source>/.acultur8-lab/` |
| External REX helper | `--rex-helper` | `FM1_REX_HELPER` | Disabled |
| External Windows REX SDK | `--rex-sdk` | `FM1_REX_SDK` | Disabled |
| Docker builder image | Build script `--image` | `FM1_BUILDER_IMAGE` | `acultur8-fm1-emu-builder:local` |

Paths can contain spaces. Keep custom runtime, helper and SDK paths outside the repository. The default runtime directory is Git-ignored. Explicitly configured runtime/helper/SDK paths are excluded from the dashboard source editor and source export even if placed inside the checkout.

## Source, checkpoints and roadmap

On first start, the lab extracts the pinned upstream commit using **local git archive** into `.acultur8-lab/baseline/<commit>/`. Later starts reuse that cache without a network request. The displayed archive SHA-256 identifies this Git-generated baseline archive. It is not the hash of a separately downloaded ZIP. A missing upstream commit produces an actionable startup error; the lab never changes your branch or downloads a replacement baseline silently.

Source saves retain the prior exact bytes in `.acultur8-lab/backups/`. The editor checks the file hash before each write and rejects stale saves. Restore also checkpoints the current bytes before replacing them. Hidden paths, build outputs and traversal outside the source root are unavailable to the editor. Requests require the loopback host, a local session token and a same-origin browser request.

On first start, `lab/seeds/targets.json` is copied into runtime `targets.json`. Subsequent starts retain your target notes and statuses. `history.jsonl`, checks, build logs, source exports and private REX conversion jobs remain in runtime storage. New checkout milestones in the seed do not overwrite an existing local roadmap. Completed targets require acceptance notes.

Source export includes the editable source inventory and a change manifest, excluding Git internals, builds and runtime data. It is a source checkpoint, not a device flash image or a substitute for reviewing files before public publication.

See [the variant plan](ACULTUR8_VARIANT_PLAN.md) and [feature specification](FM1_MOD_SPEC.md). The roadmap distinguishes browser acceptance, physical-device acceptance and optional later experiments. FM1-27 proves a complete kick processing strip before expansion; FM1-28 covers the combined synthetic song and browser soak. Existing accepted emulator and slice features remain independently recorded.

## Build the browser emulator

Install Docker from its official distribution and start its Linux engine. The first command builds the local compiler image from `lab/emulator.Dockerfile`, whose Emscripten base image is pinned by digest:

```sh
python lab/build_emulator.py --bootstrap
```

Bootstrap needs network access for the image and Debian development packages. Later builds use the existing image with compilation network access disabled:

```sh
python lab/build_emulator.py
```

Compiler version and image ID are recorded in `build/emu/manifest.json`. The base compiler is pinned; bootstrap's Debian packages follow their configured package repositories, so two separately bootstrapped image IDs are not promised to match. Reuse a recorded image ID or tag for comparisons. The container drops capabilities and enables `no-new-privileges`. It mounts this checkout only, regenerates font/sample headers, and builds both WebAssembly and the native host reference.

Output is `build/emu/`. The manifest records source/WASM hashes; source changes make the dashboard show a stale build. Source changes during compilation fail the build. Dashboard rebuilding stops its emulator iframe audio and blocks source saves while compilation runs.

Additional build options:

- `--source-root PATH`: compile a different FM1 checkout.
- `--output PATH`: copy the completed browser package to another directory.
- `--image NAME`: use a selected locally installed Docker image.
- `--native`: use preinstalled `emcc`, Python/Pillow and a host C compiler on Linux.
- `--package-only`: refresh browser assets around an existing WASM file; preserves the previous compilation manifest so changed sources remain visibly stale.

The Python API `package_page(work, output, home_url, storage_namespace)` is available to the static-site packager. It extracts external CSS/JS, packages local fonts and the manual, and adds the slice panel. Browser saves for the lab use the existing Acultur8 namespace. The emulator runs shared firmware code through AudioWorklet; it does not emulate physical CPU load or peripherals.

## WAV, markers and optional REX conversion

WAV and slice-marker import is entirely browser-based and works without a decoder. Import matching `NAME_slices.wav` and `NAME_markers.json` files. JSON includes exact frame boundaries, original timing and saved tempo. Equivalent CSV uses `Slice,Start,End,Time` in seconds and a `Loop,,,duration` row. Legacy duration-only CSV is rejected because it cannot recover original onsets.

Native REX conversion is an optional, separately supplied local integration. **This repository does not include the converter implementation, SDK, DLL, SDK fixtures or private audio.** If you already have a compatible, independently licensed installation, configure the two paths explicitly:

```powershell
$env:FM1_REX_HELPER = 'C:\FM1-external\rex2wav_windows.py'
$env:FM1_REX_SDK = 'C:\FM1-external\rex-sdk'
python lab/server.py 8097
```

The integration requires 64-bit Windows Python and `<sdk>/Win/x64/Deployment/REX Shared Library.dll`. The helper is called as:

```text
python HELPER INPUT --output BUNDLE_DIRECTORY --sdk SDK_DIRECTORY
```

It must produce the established original-timing bundle: `_slices.wav`, `_markers.json`, `_markers.csv`, `REX-SDK-LICENSE.txt` and `ATTRIBUTION.txt`. The JSON names its audio file and includes `slices`, `bpm` and `loopDuration`. Conversion runs in a private, newly generated runtime directory. Source REX uploads and job internals have no download route. Only successful job outputs named by the manifest can be served. Conversion failure leaves an explanation; missing decoder configuration does not block the dashboard or WAV import.

Choose **REX converter → Convert loop → Open converted loop in emulator → Switch on** when this integration is configured. **Play original timing** preserves saved tempo. **Play with FM1 · keep pitch** schedules slice onsets against the sequencer's audio clock, joins the next four-beat bar when already playing, follows live tempo changes and shares instrument stop/restart. Slice audio speed stays fixed; sustained audio is not continuously time-stretched. The loop repeats at its original beat length, including non-4/4 loops. Marker files without tempo allow manual BPM entry.

Limits: 64 slices; mono/stereo audio at 8–48 kHz; 30 seconds or 1,440,000 source frames stored; 60-second musical loop; eight overlapping slice tails; 16 MiB REX upload. Too much overlap stops playback with an explanation. Browser **Save import** stores imported audio/markers separately from instrument **SAVE**. Transferring samples to a physical FM1 is a separate target.

The illustrated FAQ is <http://127.0.0.1:8097/faq>. Its inline fallback styles use an exact CSP hash, and its SVG diagrams explain slice-bank offsets versus musical onset time.

## Verification

From the repository root:

```sh
python lab/test_dashboard.py
python lab/test_build_emulator.py
node tests/host/marked_import_test.mjs
node tests/host/marked_wasm_test.mjs build/emu/x0x.wasm
node tests/host/marked_sync_test.mjs build/emu/x0x.wasm
```

Dashboard tests create a disposable local Git clone, prepare the pinned baseline and start a temporary loopback HTTP server. They cover exact save/restore, stale-write rejection, new source editing, path/origin/token restrictions, private runtime/decoder exclusions, source exports, stale emulator detection, REX job downloads and FAQ styling. Build packaging checks do not require Docker. Firmware/host, WASM, browser and physical-device results remain separate evidence; successful syntax or packaging checks do not prove device readiness.

## Preview a checked browser-alpha package

After the mandatory checker reports its artifact path, serve that package from the repository root:

```sh
python tools/serve_alpha.py --site build/site-ARTIFACT --port 8130
```

Replace `build/site-ARTIFACT` with the exact path in `build/acultur8-checks.json`, then open `http://127.0.0.1:8130/emu/`. The helper verifies every release-manifest file hash before listening, binds only loopback, uses explicit JavaScript/module/WASM MIME types and exposes only the checked package files. It snapshots the bytes; stop/restart it to preview a different package. A failed hash stops startup. Do not use a generic Python static server on Windows: local MIME associations may serve `.mjs` as plain text and prevent the audio worklet from starting. A fresh port avoids responses cached from an earlier incorrect server.

Each origin has separate browser saves. Testing a new preview does not migrate imports or saved projects from another port. Source updates may proceed through reviewed PRs; new Pages deployment remains held for sign-off.
