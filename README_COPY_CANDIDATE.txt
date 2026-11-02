FM1-55 ALL-DRUM-LANE AND INDIVIDUAL-303 COPY CANDIDATE
Local only. Parent installed 0.1.7-beta1 remains unchanged.
No device access, flashing or publication is authorized by this candidate.

MENU
GLO > COPY/PASTE SEQUENCES stays first, above CLOCK OUT.
COPY FROM / PASTE INTO select stored P1-P32 explicitly.
SELECT moves rows; ALGORITHM changes values.
COPY SCOPE now has 25 choices, grouped as:
  ALL PARTS
  909: KICK, SNARE, LOW TOM, MID TOM, HIGH TOM, RIMSHOT, CLAP,
       CL HAT, OP HAT, CRASH, RIDE
  808: KICK, SNARE, LOW TOM, MID TOM, HIGH TOM, RS/CLAVE, CP/MARACAS,
       COWBELL, CYMBAL, OP HAT, CL HAT
  303 A / 303 B
808 tom/conga, rim/claves and clap/maracas variants share their existing
sequencer lanes. Copying a lane does not change the destination sound switch.
BREAK still participates in ALL PARTS; BREAK-only is outside this extension.

PASTE KEEP ACC/SW opens source/destination/scope confirmation. SEL confirms;
HOME cancels. HOME backs one level through submenu and Global.
Confirmation block count reports active song references to the destination
for the selected part, not whether the destination contains notes. Prepared
inactive rows retain their pointers and use updated contents if activated.

COPY AND PRESERVATION
- Every scope preserves both destination shared drum AC rows and swing.
- Drum-only scopes replace the chosen same-kit 64-step row, its sound motion
  and its probability/condition motion. Other drum rows and parts stay intact.
- Independent lane length/rate is copied when the lane format is initialized.
  In legacy format, a kit's shared length/rate stays destination-owned.
- 303 A or B copies only that bass part's 64 notes, gates, ties, slides, note
  accents, length, rate, direction, transpose, generator configuration/seed
  and part-owned motion/probability. Bass note accents belong to the sequence;
  they differ from the preserved shared drum AC rows.
- Destination sound bases, sound switches, strips, MIDI channels/routes,
  arrangement references, global rate family settings and the other bass part
  remain destination-owned. ALL PARTS retains its original five-part behavior.
- Copy uses the stored source slot, not a mix of current part selections and
  not a frozen clipboard. Source data remains independent after paste.

TRANSACTION / HISTORY / STORAGE
Stopped-only existing scratch-staged transaction; no new persistent format
or RAM allocation. Busy/protected/full/invalid requests publish no partial
notes or motion. Same-slot copy is a no-op. One paste is one Undo/Redo unit.
Stopped HOME+REC undoes; HOME+PLAY redoes. SAVE persists the result; Undo
history itself is not a reboot-persistence guarantee.
Confirmation scope packing now carries five bits so all25 choices survive
SEL confirmation. Scope IDs are transient UI values, not stored/MIDI IDs.

LOCAL QUALIFICATION
See COPY_CANDIDATE_STATUS.json and build/copy-review/ logs for completed runs.
Focused tests exercise all25 scopes in three formats (75 combinations),
64-step boundaries, P32-to-P1, all-lane motion/probability ownership,
unchanged destination/source data, atomic rejection, confirmation packing,
Undo/Redo, synthetic save/reload/reboot, MIDI notes/channel/velocity/Off balance.
Production worklet/WASM tests use actual menu/button/encoder input for all25
scopes and compare saved objects with the independently checked native result.
Strict ASan/UBSan halt-on-error and unchanged original target budgets required.
Local results are not acceptance of installed hardware or external receivers.

PARENT
../pattern-copy-nested-menu-20261022 is preserved, including its source and
frozen installation package. This candidate target package is diagnostic only,
marked NOT_FOR_FLASHING. No public release version is being promoted.
