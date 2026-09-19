# Corrected original-process route and physics evidence

The fresh-save original GameSystem completed **30,000 frames**, exited **0**, and was reaped after 734.585 seconds. The bundle hash before/after is `dc02030efc629c57d77b7c848b0e348dce2ed2ef33db352259f7cd9148e44a0b`. No executable/source rebuild occurred while this process ran. `final2-route.json` retains command, sanitized environment, process lifecycle and bounded-completion checks.

## Story milestones observed in the original actors

| Event | First sampled frame |
| --- | ---: |
| First rabbit caught / post-catch dialogue finished | 3190 / 3560 |
| Original pipe PlayerIn / PlayerOut | 4020 / 4090 |
| Second rabbit caught / post-catch dialogue finished | 4690 / 5060 |
| Third rabbit caught / post-catch dialogue finished | 10480 / 10970 |
| Tower appearance and authored plane gravity appeared | 11140 |
| Rosalina alive and not hidden | 11440 |
| Mario selected tower plane gravity, priority 1 | 14440 |

The input operator exited 0 after observing all three real catch/dialogue transitions and Rosalina's live/unhidden state. Runtime IDs were bound from the actual early static actor snapshot, not assumed from old runs. All state transitions are caused by ordinary controller input and original game logic. No actor/switch/nerve/position writes, forced catch or production stage-specific workaround was introduced.

## Physics and scene checks during the run

`final2-summary.json` checks 3,000 sampled frames and 125,832 actor samples: no nonfinite checked position/velocity/gravity/reaction/ground-triangle vectors. Binder observations contain ground, wall and roof contacts, plus original face and edge features. This run did not sample a vertex feature; the prior corrected run did, and focused synthetic tests cover one vertex orientation. Cached Mario ground-triangle counts do not independently prove grounding.

Stronger grounding evidence at frames 14440, 15040 and 16040 combines Mario's original on-ground flag, selected plane field 452/priority1, and ground host725 (post-demo stairs). The earlier point field is418. Leaving the tower during supervised navigation switches back to418 at17300; returning selects452 at22110. Field452 appeared at11140 from the authored tower switch, with its original2300 range and priority1.

The final placement report is byte-identical to the independently verified prior report (`031fbd727a70f1c55e65c6d2b97d9b635b9e56b0dea6c07b2f87829e6d35d1e7`). Its243 row identities/layers/zone transforms are correct, while59 known-unlinked placements remain missing. See scene notes.

The crater recovery loop is expected from the authored PullBackCylinder: all17 sampled transitions into status19 in5000–11000 start inside the reconstructed original cylinder, with preceding samples outside. The third rabbit has actual map contact and remains near the planet surface. Ordinary rightward movement around the crater enabled the catch. See `scene/pullback-final2.md` and `collision/final2-crater-analysis.md`; their differing frame ranges/actor-vs-player coordinates are explicit.

## Supervised inputs and visual limitation

The notes-only driver corrects original stick shaping, requests full speed for chases and can issue an ordinary A jump after a measured stationary stall. Automatic jumps at3981–4010 and5761–5790 respectively cleared the pipe rim and a stone obstacle. The native accepted-input log is authoritative for actual consumption.

Root additionally tried ordinary A8050–8080 at the crater, which did not clear its recovery volume, then ordinary right stick9840–10000 to go around it. After the endpoint, controller-only approach and short jump probes checked the tower gravity and stairs. All interventions and helper logs/hashes are retained. These paths are verification scripts, not game behavior. The final late stair-waypoint trial did not reach its first target before its29800 cutoff; it is not claimed as successful ascent.

**Rosalina's spawn state is verified; an unobstructed rendered view of her model is not.** Live captures show the tower, Lumas, stairs and Mario, but the wall/parapet occludes the top. The file named `final2-rosetta-appearance-ui.png` is a capture taken after the spawn event; its filename must not be treated as proof Rosalina is visible. The frame28500 screenshot also does not prove her visibility. No GPU/retail visual parity or complete rendering claim is made.

This is a passing bounded real-process progression/physics exercise through the requested spawn event, with explicit visual and missing-content gaps. It is not whole-scene correctness, exhaustive physics correctness, an autonomous human-quality controller, or retail frame-for-frame parity.
