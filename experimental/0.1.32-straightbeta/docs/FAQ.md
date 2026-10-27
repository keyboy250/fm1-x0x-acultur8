FM-1 Extended Edition FAQ Â· 0.1.32-straightbeta

Release
 Â· 
Feature page
 Â· 
MIDI chart
 Â· 
Overall FAQ &amp; diagrams

FM-1 questions.

Clear answers.

The 0.1.32-straightbeta guide to straight rates, one-sound overrides, pattern clearing and saved projects.

How do I use global straight time?

Stop playback. Press GLO and use SELECT to find SYNC RATE MODE at the bottom of the System section. Use ALGORITHM to choose STRAIGHT. ALL retains the full list.

STRAIGHT uses exactly:1/16, 1/8, 1/4, 1/2, 1/1, 2/1, 3/1, 4/1, 6/1, 8/1, 12/1, 16/1, 32/1. The3/1, 6/1 and12/1 choices mean whole-bar multiples, not triplet-labelled rates.

This filters subsequent rate choices. It does not silently rewrite existing pattern rates. Press SAVE when you want to retain your project settings.

Can just the snare use triplets?

Select 909 and the snare (SD).

Press EDIT; use SELECT to find STRIP SYNC.

Set RATE MODE to STR+TRIPLET.

Press SEQ and set RATE to 1/16T.

The kick can stay at 1/16 with its strip RATE MODE set to GLOBAL. Each of the 22 drum sounds, both 303 parts and BREAK has its own override: GLOBAL follows GLO; STRAIGHT always uses the 13-stop list; STR+TRIPLET permits the existing straight/triplet divisions without dotted entries; ALL permits the full list. Change policies while stopped.

Do the machines have separate tempos?

They share one global BPM. Their individual step RATE and LENGTH settings can differ. ALGORITHM selects the part; on HOME the highlight colour changes with it:909 orange, 808 red. Use SEQ to inspect that soundâ€™s LENGTH and RATE.

How do I clear a pattern or only one part?

Whole current pattern:
 stop playback, open GLO â†’ CLEAR PATTERN, check CLEAR THIS PATTERN? / ALL FIVE PARTS, then SEL confirms. HOME cancels.

Selected part only:
 select the intended part and pattern, hold SAVE and press REC. Check the named part, then SEL confirms or HOME cancels.

Clearing removes sequence data and keeps sound settings. Hold HOME and press REC for Undo, or HOME and PLAY for Redo. Undo history does not survive switching off. Stopped changes may save automatically when autosave is enabled.

Do the global mode and override survive restart?

Yes. Press SAVE and wait for SAVED. A bounded physical test confirmed both global STRAIGHT and 909 SD STR+TRIPLET after a normal power cycle, with repeated saved project exports byte-identical before and after. Save your pattern before switching off.

Does this fix Reason clock jitter?

No fix is claimed. Reason external-clock clip/grid stability remains unresolved. The 0.1.31 averaging/fine-tempo experiment is excluded from this build.

What about delay and LFO rates?

The shared delay knob uses its supported 1/16, 1/8, 1/4, 1/2 straight subset when global STRAIGHT is active. Its existing buffer/folding limit remains. Per-sound overrides govern that soundâ€™s sequencing, not the shared send effect. This Extended Edition source has no separate LFO rate engine; the physical LFO button serves mixer/tie functions. Free-running parameters are unchanged.

How does MIDI rate control interact with this mode?

The rate sweep follows the effective global/strip policy. Explicit direct/exact MIDI rate IDs 0..20 keep their division meanings and are not silently remapped. Long divisions can be selected by the policy sweep or native exact-rate API after activation. No new direct CC address is assigned to the global/local mode selectors.

LAN2 family values remain0 straight, 1 straight+dotted, 2 straight+triplet, 3 all. After LAN3 activation, family CC value 0 selects a local STRAIGHT override, 2 selects STR+TRIPLET, and 1 or 3 selects ALL. See the version-matched 
MIDI chart
 for ranges, targeting and readback.

Can I open the new project in older firmware?

Keep a pre-feature project matched to your exact rollback firmware. LAN3 saves require compatible firmware and older builds deliberately reject them. Project exports do not independently export sample slots, panel calibration or the saved strip-default bank. 
Read installation and rollback guidance.

How much has been tested?

Local qualification passed 134 required checks and 39 host scenarios, native/WASM control and MIDI replay suites, calibrated timing, updater simulation, unchanged target budgets and an exact frozen-source rebuild. Installation preserved project bytes, 25 current strips, P8/P9 and calibration. Selected physical straight/override controls, listening and saved restart passed on one unit.

Advanced remote CC action hardware replay, worst-case physical deadlines, all voices/rates/tempos and Reason sync remain open. 
Read the scoped acceptance matrix.

Are alternate 303B engines and extra drum voices included?

Those are future-release requests. This build retains the existing 303 engines and 909/808 sounds; it does not add a synth-engine picker or new drum voice banks.

Built on 
X0X by Charles Vestal and contributors
, itself based on 
Felucca by Leo Kuroshita
. 
Credits
 Â· 
Licenses
. Experimental beta.
