# HOME startup â€” private candidate 0.1.15-homebeta

The only production behaviour change from the frozen 0.1.14-303varbeta source is the initial view in `ui_init()`: HOME instead of PART/909 EDIT. The selected part remains 909, and the existing button/navigation behaviour remains intact. Press EDIT to enter its editor. HOME still opens the overview from an editor, and another HOME press still opens SONG.

This applies at normal startup and UI reinitialization. It adds no persisted preference, project-format change, runtime allocation or playback action. Saved patterns, sounds, accents, swing and all 303 variation behaviour are inherited unchanged. Variation still defaults OFF after a reboot.

Local checks cover fresh and saved boot/reboot, all three project formats, native and production-WASM rendered HOME titles, EDIT/HOME navigation, stopped transport, variation OFF, and unchanged project/storage bytes. The existing editor-oriented scenario fixtures now explicitly verify startup HOME and press EDIT before their original checks. The getting-started manual and paired native/WASM audio fixture explicitly enter EDIT too. The PCM oracle replays the actual input timestamps in both simulators, avoiding a nominal-duration assumption around the browser audio ring. The original PCM threshold is unchanged; no acceptance threshold is relaxed.

See `HOME_BOOT_STATUS.json` and `build/home-boot/` for final qualification and hashes. Static RAM remains 98,304/98,304 bytes; no RAM is added. Device installation and visible cold-power-on acceptance are recorded separately. No public release or GitHub Pages change is included.
