# 2. Favorite Slot 1 and native STORE / RECALL

[All tutorials](README.md) · [Native value reference](../STARTER_NATIVE_CONTROL_REFERENCE.txt) · [Complete CC map](../CC_MIDI_MAP.md)

The starter ZIP contains the selected sound's JSON and exact 68-byte native record. They are not firmware or full project backups. No automatic JSON importer is provided. The supplied control reference lists every captured hybrid, original 808 BD and strip value for native entry. Shared kit/global FX, patterns and sample audio are not part of that record.

## Enter or shape a sound

Stop playback, set MASTER down, and select the 808 kick. Use its existing sound pages and SELECT parameter list. Read the control reference's panel-value column; the raw column is for protocol/codec comparison. In particular, Favorite Slot 1's WAVE ENV decay displays **122 ms**, not raw 5 ms. Restore your listening level only after editing and begin quietly.

## Store in an empty native slot

1. With playback stopped and the tail finished, open the 808 kick's **KIT** page.
2. Set **KICK SLOT** to an empty destination. Do not overwrite your own favorite merely because this download is named Slot 1.
3. Turn **KNOB3 — STORE KICK**. Check the prompt says **STORE/OVERWRITE KICK?**
4. Press **SELECT** to confirm, or **HOME** to cancel. Successful native storage reports **KICK STORED**.

## Recall and keep the result

1. Remain stopped, choose the intended slot on KIT, and turn **KNOB4 — RECALL KICK**.
2. Confirm **RECALL KICK?** with SELECT. Successful recall reports **KICK RECALLED**.
3. RECALL applies the sound to the current project. Project Save is a separate decision if you want that recalled sound retained in the project.

The test unit used empty slot 16 for verification, then tested recall from a different W DECAY value and normal restart persistence. Other slots stayed unchanged. This does not instruct you to use slot 16 if it already contains your work. USER1 references require the matching sample; the JSON does not contain that audio.
