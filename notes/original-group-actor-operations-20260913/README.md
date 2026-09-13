# Original group operations

Original FlipPanel and FlipPanelObserver need the existing LiveActorGroupArray service to invoke group appearance, death, movement and messages. Restored the full seven-wrapper family from original LiveActorUtil using its two exact iteration helpers. Each operation preserves authored group order, excludes the requesting actor, and returns without work if no group exists. No replacement membership store or stage-specific rule was added.

Restored ActorSensorUtil sendMsgToGroupMember: existing groups retain the original queued MsgSharedGroup message, while an ungrouped actor receives the message immediately on its named sensor. Both use the already active original actor/sensor paths.

Source bodies copied directly from the decomp reference; no new recovery or Game edits. Narrow native compilation results are recorded alongside this note. Production stage-load validation is shared with the FlipPanel activation checkpoint.

Thirty-fourth integrated production build passed with this cohort. Twenty-fifth original run reached a pending TicoBaby archive request; it does not establish completed placement of every imported actor or working gameplay.
