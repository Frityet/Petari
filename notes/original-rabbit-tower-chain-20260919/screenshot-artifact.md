# Unattributed thin black line in frame4200

Inspected `notes/gateway-compat-20260919/content-stomp-fixed-16000-frame4200.png` and the matching `frame_index:4200` record in `content-stomp-fixed-16000-actors.jsonl`. The image contains a thin dark stalk extending from the planet toward the sky near center, with regularly spaced short crossbars. This is a separate unresolved rendering artifact; neither story progression nor the source audits establish visual parity.

Two suggested sources are excluded by the inspected original gates:

- **WarpPod path:** both actual zone5 placements l_id52/53 have mArg1=0 (also verified by the earlier real-process WarpPod test). The manager calls `drawCylinder`, which requires mArg1==1; each pod's independent draw also returns for arg1==0, and its original registration has draw type/buffer -1. The donor debug-like `_C4[i+1]` loop cannot explain this frame through those gated paths and was not modified.
- **PunchingKinoko line shadows:** all14 actor records at frame4200 are clipped. Their default visible-sync-host controllers preserve original clipping checks, so `ShadowController::isDraw` rejects their drawers. Original ShadowVolumeLine emits an eight-corner polyhedron (two quads and a ten-vertex strip), not a sequence of dozens of rungs. Prior focused shadow testing proves command geometry and lifetime, not a GPU shadow image.

The current actor trace omits ordinary map models, flowers/plants, particles, per-drawer ownership, and full GX batch state. Source searches found line primitive emitters in JPA and Mario special rendering, among others, but no source was tied to these pixels. A nearby visible pipe or plant alone is not evidence of ownership. No speculative visibility, draw-state, or geometry change was made.

After a stable source/binary snapshot and GPU slot are available, the next discriminating check is a bounded draw/batch capture or read-only debugger trace at the same scene/view. Associate the offending vertices with their actual actor/model/particle and draw type, then inspect that owner's original gate and vertex/transform data. The pending general zone-transform correction may change this image; capture a fresh frame before assuming the old artifact persists.
