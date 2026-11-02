# Live drum trigger overlay â€” separate private 0.1.19-livebeta candidate

Parent: frozen0.1.18-diagbeta. Local validation only; no device installation or publication.

The selected armed drum lane keeps its stored colored grid cells. A small amber
strip marks an accepted hit in the existing current-pass capture prefix; a white
inset marks the current accepted step. Temporary added hits therefore light;
suppressed base hits keep their base fill but receive no trigger flash. Upcoming
steps are not predicted. The prefix resets each pass; this is not all-pass history.
When Freeze holds a complete capture, amber marks that held accepted pattern;
white still follows its current step.

During live drum variation the selected row's base white-key pattern is dim,
and the current accepted hit is bright. No accepted hit means no bright trigger.
Stop, OFF and owner/slot mismatch restore the normal stored-pattern indicators.
ENV-held accent display retains its normal behavior. Independent drum key LEDs
use the selected lane's playhead. Freeze-held gates use existing captured state;
mute removes the overlay. No extra audio-path logging or static RAM was added.

Only the selected armed909/808 lane is overlaid;303 pitch rendering and other
rows keep existing behavior. The overlay reports engine accepted gates, not
recorded audio or external MIDI delivery. LCD/LED refresh can miss short steps.
Real panel visibility, USB/IRQ timing and hardware listening remain pending.
