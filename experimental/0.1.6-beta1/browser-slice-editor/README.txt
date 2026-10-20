ACULTUR8 SLICE PROFILES - BROWSER COMPANION
This is a browser tool, not an FM1 firmware sampler upgrade or hardware uploader.
Run a local static server from the extracted release folder:
  python -m http.server 8144 --bind 127.0.0.1
Open http://127.0.0.1:8144/browser-slice-editor/
No MIDI access, server uploads, external runtime scripts or webfonts are needed.

Load a mono/stereo PCM WAV (8-48 kHz, up to 30 seconds), or the generated demo.
Choose a detection profile, tune the band/attack sensitivity/minimum RMS dBFS,
and Preview detection before replacing cuts. Inspect a hit with the spectrum;
Hear detection band uses the same filter as the RMS graph, without normalization.
Magenta = detected attacks; orange = proposed cuts; green = accepted active cuts.
Profiles help locate hits; they do not isolate simultaneous instruments.

Select a slice to set Active, source Length, Hold, Decay and Level. Apply slice
sound commits the sound controls. Hold+Decay is initially OFF to preserve older
projects. Try ON, Hold 60ms, Decay 90ms, Level 0dB on a snare. Sound stops at the
earlier of the source boundary or Hold+Decay duration. Short edge fades reduce
clicks. Inactive slices stay editable/auditionable but group playback skips them.
Copy/paste includes Active, Length and sound settings; destination start/label
stay intact. Reset makes the slice Active and restores current profile defaults.

Export review project saves all slices and their settings in JSON. Keep the WAV;
JSON embeds no audio. Reopen with that exact WAV; its fingerprint is checked.
Version 1 projects/profiles remain readable; new sound settings use version 2.
Custom profiles use storage for the current browser origin: export them for
portable copies, especially before changing port/hostname or clearing browser data.

Export active markers is CUTS ONLY for the existing WAV/marker workbench. Hold,
Decay and Level are not applied by that older marker player. Keep a review project
for sound settings. This tool does not record USB audio, decode raw REX, upload
native samples or add firmware engines. The original WAV remains unchanged.

Optional source checks with Node.js: node core.test.mjs, node diagnostics.test.mjs,
node length.test.mjs, node envelope.test.mjs. Demo regeneration: node make-demo.mjs.
Code: GPL-3.0-only. Font Awesome Free 5.15.4 SVGs: CC BY 4.0, icons/LICENSE.txt.
