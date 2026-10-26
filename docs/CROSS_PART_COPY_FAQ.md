# Cross-part copy: local candidate FAQ

These answers describe 0.1.10-crosscopy, a separate local candidate. They do not describe a new public release or a completed hardware test.

**Can I copy a snare rhythm to a tom or rim?** Yes. Choose source/destination patterns and parts independently. Any of the eleven 808 or eleven 909 drum lanes can feed any drum destination. Copying within one pattern is allowed when the parts differ.

**Will the destination still sound like its chosen instrument?** Yes. Its base sound, strip, routing and tone/FX automation remain. Cross-part copying transfers hits/notes, compatible timing and step probability/conditions, not the source instrument's timbre parameters.

**What happens to accent and swing?** Shared drum accent rows and destination pattern swing remain. Cross-303 copies also retain destination step-accent bits and generator accent probability, including inactive steps. Slides/ties follow the source notes. Existing same-part/all-part copying keeps its previous articulation behaviour.

**Can 303 A feed 303 B, and vice versa?** Yes. Notes/gates, slides/ties, direction, transpose, timing and generator configuration copy, with the accent exception above. The receiving 303 keeps its sound and tone automation.

**What if length or rate differ?** Independent and expanded formats copy only the destination lane's length/rate. Legacy drums share kit timing, which remains unchanged for a single drum lane. All 64 step cells are copied even when the current visible length is shorter.

**What happens to probability or conditions?** Their full step values, including condition bits, follow the source and are retargeted to the receiving part. If no source probability lane exists, the selected destination probability lane is cleared. Other probability and motion lanes remain.

**Can I undo it?** It is one stopped transaction with Undo/Redo. The copy operation does not call Save; normal configured autosave remains independent. Normal explicit save and recall were tested locally; physical device acceptance is still separate. Cancel changes nothing. Busy/protected/no-capacity operations refuse without partial changes.

**Can a tom rhythm feed a 303?** Not in this candidate. That feature needs a separate rhythm-only mapping for pitches, empty steps, ties/slides and timing.

**Does this turn the TRS input into an output or add a Pi router?** No. The companion Pi plan is a minimal USB-host MIDI appliance. Router setup, actual delivery latency and the independent internal-303 sound bypass are separate targets.
