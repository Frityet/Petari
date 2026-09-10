# Original talk/demo prerequisites — 2026-09-10

Previous goal turn: **progress**. Root49c9a5e0f/f2ed2ebc8 published removal of NPCActorSource.inl, direct NPCActor with one compiler qualification, original JointController with fresh10/10 Wii matches, actual owner tests and960frame movement replay13/13 at59.942FPS. Decomp8f8633065 published first. Full Gateway/Rosalina remains incomplete.

This checkpoint restores original shared NPC/rail/message behavior and recovers the missing original demo prerequisites. Full Gateway/Rosalina remains incomplete. No stage-specific reaction or demo substitute was added. The one root Xmake lane performed ordinary builds/linking/runtime tests; agents used isolated compiler/reference probes and independent review. The four pre-existing user dirty paths remain outside the publication.

## Final changes

- The complete original AllLiveActorGroup is now constructed before scene actors. Its original broadcast traversal replaces the scheduler's snapshot/filter implementation. Unscheduled, suspended and clipped living actors receive original broadcasts; dead/excluded actors do not. Actor retirement removes borrowed pointers. The obsolete scheduler broadcast API and its `scene_messages` diagnostic field are removed.
- Complete unchanged RailUtil replaces GameRailCompat: 113 exported original functions cover all 28 removed exports. Curve-sampled clipping bounds and the missing speed/direction/goal helpers come from reference source. Fresh Wii comparison: 99.19178% text; AllLiveActorGroup text/data 100%.
- Fourteen original NPCUtil action/reaction/talk/rail functions recovered first in decomp score 97.846535–100%. The complete NPCUtil source/header are mirrored exactly. Five native throw-only replacements were removed. Two required MathUtil helpers, blendVec and makeQuatUpFront, are copied verbatim at the existing shared math boundary.
- Actual MessageHolder/MessageData now own the 6 embedded system and 1,994 game messages. A bounded native BMG adapter translates scalar byte order and text-unit width; original lookup, message fields, flow nodes and scene-alias methods run directly. A scoped complete MessageHolder supplies the native process lifetime. No partial GameSystemObjHolder is fabricated. RARC resource lookup now matches full paths case-insensitively and does not substitute a sibling locale for a missing qualified path.
- DemoTimeKeeper, DemoWipeKeeper, DemoSoundKeeper and five missing DemoExecutor talk loops were recovered and copied. Their complete native TUs compile into the ordinary archive, but the original Director/Executor graph is not instantiated. The existing host demo clocks remain pending complete ownership integration. See demo/README.md for exact per-function scores, including the intentionally lower-codegen portable Sound recovery.
- Runtime NPC coverage found a reference defect: forward RailRider::isReachedGoal passed zero to a strict tolerance comparison. Actual Wii instructions and DOL constant bytes prove 0.001f in both directions. The one-line reference correction was copied native. The unchanged runtime assertion now passes. The fuzzy instruction score did not expose the original wrong constant; see rail-goal/README.md.

## Validation

All builds used the existing LLVM 23 debug/optimization configuration and ordinary Game CP932 compiler boundary. No compiler settings were changed.

| Check | Result |
| --- | --- |
| Original actor broadcast fixture | Pass, two complete scene generations; live mutation, liveness, suspension/clipping and sensor identity |
| Scene scheduler/heap fixture | Pass; callbacks, nested/exception restoration, sensor/message retirement, category order and heap reclamation |
| NPCActor real-disc fixture | Pass 6/6, including added actual Tico BCK, Spine reaction stack, talk state, real rail speed/orientation/goal reversal |
| Original MessageHolder fixture | Pass after final decoder change; raw CP932/tag/surrogate/alias/flow and malformed-block cases, every authored system/game record, two owner generations |
| Original JKRArchive fixture | Six checks pass, including catalog/finder/resource/lifetime behavior after the path lookup correction |
| Original StarPointer owner fixture | Pass; guidance retains actual authored message storage and samples the original WPadHolder |
| Movement showcase | Final 960 ticks, 13/13 checks, 60.077 FPS, exit 0; exact executable bfc02eef07c9e5707a2b7e486d6e24511ce1161eb4cf57e821cc6976bee7b6df. See replay-final-summary.json and replay-final-validation.json. |

Fixture corrections preserve real initialization requirements: makeActorAppeared can immediately invoke original animation and must run in the Game heap; controller connection callbacks require a sampled WPad frame before original validity queries. No production fallback was introduced to satisfy these assertions.

The packaged app was not replaced in this cleanup. The replay uses the ordinary built showcase and is bounded movement/camera/jump evidence, not a full authored Gateway opening or bunny-chase result.

## Publication and evidence

Fresh upstream fetch found SMGCommunity/Petari at d1ae0a05cc023d52ecdcbc7731c8c79f0cb84dc6, already contained in the decomp branch. The recovered NPC/demo code was published first as decomp c3dfe83e0df2b8a70a2c366c51dc371bf305bb06; the rail tolerance fix follows as ec3fa56e7. Both use codex author and committer. Decomp remote verification is recorded in publication.json; root publication is verified after committing.

Notes retain source manifests, exact compiler commands, object comparisons, DOL-byte proofs, independent review findings and runtime logs. Raw logs/patches and large object-comparison JSON are committed compressed without editing their evidence bytes; local object files and scratch header overlays are not required for the runtime artifact. All original-source comparisons are snapshots of this checkpoint.
