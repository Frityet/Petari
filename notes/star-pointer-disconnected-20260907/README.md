# Disconnected StarPointer input — 2026-09-07

The original pointing core returns false for a disconnected WPad before a target hit test or touch/shoot/rumble mutation. Native actor pointing queries now read that actual Aurora device connection state instead of unconditionally failing in a keyboard/mouse session. The same absent-device boundary serves the ordinary, held-button and triggered-button 2P entry points. No actor name, stage, story flag or fake connection state is involved.

A connected channel still explicitly rejects the query until original StarPointerController/Layout ownership exists. These APIs do not pretend to implement connected pointing, button transitions, target selection, shooting suppression, or rumble. This is a proved absent-device outcome, not a completed pointer subsystem. The earlier unconditional 2P failure is removed from NPCActorRuntimeCompat.

The original StarPointerTarget header is copied byte-identically. MR::getStarPointerLastPointedPort copies the original body and returns the actual target member address; it does not fabricate a fallback channel or target.

The focused regression uses Aurora's real WpadService: all three queries return false with channel1 disconnected, regardless of channel0 input; all three reject when channel1 is connected; disconnecting again restores the original absent-device result. Runtime validation is pending the coordinated build.
