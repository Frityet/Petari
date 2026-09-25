# Demo / talk / animation audit after compat removal

Initial read-only source audit against the current `decomp/` donor, after root checkpoint `a71fc2ea7`; the bounded follow-up restorations below were then authorized and applied. No build, test, or runtime execution in this lane. Current working sources are authoritative; these findings do not establish the cause of any observed Gateway route failure. Audio differences are excluded.

## Recommended small donor restorations

| Priority | Current source / donor source | Difference and consequence | Required scope |
| --- | --- | --- | --- |
| 1 | `src/Game/Animation/XanimePlayer.cpp:375` / `decomp/src/Game/Animation/XanimePlayer.cpp:371` | Current `calcAnm` uses `else if (!_88)`; donor uses `else if (_88)`. `updateAfterMovement` (current 435–441) saves the previous frame in `_84`, advances the frame controller, then sets `_88=true`. Current calculation therefore selects the advanced frame where the donor selects the saved pre-advance frame. The opposite phase also selects the wrong branch. This affects all Xanime sampling, including demo BCKs; it is not a native-width requirement. | Restore donor condition while retaining actual core/resource ownership. Do not compensate by changing demo sheet times or BCK rates. |
| 1 | `src/Game/Animation/XanimePlayer.cpp:298` / `decomp/src/Game/Animation/XanimePlayer.cpp:294` | Initial `runNextAnimation` assigns interpolation countdown `_24[_55]._14 = 1`; donor assigns `0`. `updateInterpoleRatio` (current 460–471) consumes this countdown. This is a separate animation-state discrepancy, with no platform-specific justification in the code. | Restore donor assignment with the phase fix; keep constructors and native lifetime fields. |
| 2 | `src/Game/Demo/DemoExecutor.cpp:103` / `decomp/src/Game/Demo/DemoExecutor.cpp:98` | Current `start` omits `MR::invalidateClipping(actor)` before retaining a demo participant in `mActor`. `end` still calls `validateClipping` (current 241–245). Actors whose clipping was initially enabled can remain eligible for culling during the demo, unlike donor behavior. This can stop off-screen cast updates/animation; it is not proven to explain the current route. | Restore the missing original call. Actual general clipping owners and `MR::invalidateClipping` already exist (`src/Game/Util/LiveActorUtil.cpp:697`); no Gateway exception or new provider is necessary. |
| 3 | `src/Game/NPC/TalkBalloon.cpp:122` / `decomp/src/Game/NPC/TalkBalloon.cpp:100` | `updateBalloon` uses reference vector `(1,0)` where donor uses `(0,1)`. This changes both the dot-product angle and cross-product side used for the authored Balloon animation, so the beak direction differs. This is presentation rather than a demonstrated dialogue progression blocker. | Restore donor vector. If an actual screen-axis mismatch remains, fix the general projection/layout boundary instead of rotating a talk-only reference. |

These are confirmed differences from donor behavior. The audit does not establish whether they originated as deliberate workarounds or stale decompilation revisions.

Existing `tests/OriginalXanimePlayerTests.cpp:234`, `:244`, and `:256`–`:267` expect the current advanced-frame samples. They cannot independently justify keeping the inverted condition: their expected values encode the same divergence. If this existing target is used later, reconcile its expectations with the donor phase contract. No new coverage or test execution was added here.

## Differences to preserve / avoid false positives

- Demo sheet parser retention and destructor/constructor rollback in `DemoTimeKeeper`, `DemoCameraKeeper`, `DemoPlayerKeeper`, `DemoSubPartKeeper`, and `DemoExecutor` preserve native archive/resource lifetimes; their inspected sheet/time/control operations remain donor-equivalent.
- `DemoStartInfo` still uses old offset field names. `DemoDirector.cpp:202` checking `info._2C == 0` corresponds to donor `mFrameType == nullptr`; it is not evidence of a changed branch.
- The inspected `TalkDirector`, `TalkMessageCtrl`, `TalkNodeCtrl`, `TalkState`, and `TalkTextFormer` request/node/page transitions remain donor-equivalent after field renames and moved definitions. Empty base balloon hooks and `TalkBalloonInfo` trivial hooks are also present in donor declarations; do not infer missing behavior just from their bodies.
- Preserve native wide-character tag adaptation (`TalkNodeCtrl.cpp:262`, `CustomTagProcessor.cpp:188`, `:204`, `:555`, `:682`, `:708`) and the actual member-offset lookup in `TalkDirector.cpp:473`; direct PPC byte offsets/pointer tags would be wrong with native pointer and `wchar_t` widths.
- `RosettaDemoHeavensDoor` and `TicoDemoGetPower` retain their authored action/demo transition logic. The hard-coded halo position in `RosettaDemoHeavensDoor.cpp:48` is also in the donor, so it is not a port workaround. Its remaining inspected omitted demo-part check affects audio only and is out of scope.
- XanimeCore's large textual diff includes function reordering, named quaternion/transform aliases, native shared joint-array lifetime, and typed copies replacing raw casts. Inspected frame update, blend/single/special blend, dispatch, and initialization keep the donor algorithms; do not replace the entire TU merely to remove its textual diff. This was not an exhaustive floating-point equivalence audit.

## Next action

The four line/call changes above are a bounded first patch across three actual Game owners. Preserve native lifetime changes, then use the root-owned live controller route to distinguish animation/talk improvement from remaining route blockers. No scripted talk completion, demo skips, synthetic frame advancement, or Gateway-specific actor handling is indicated by this review.

## Applied bounded follow-up

Root authorized the four donor restorations above. Applied only:

- `src/Game/Animation/XanimePlayer.cpp`: initial interpolation countdown `1 -> 0`, and `calcAnm` phase guard `!_88 -> _88`.
- `src/Game/Demo/DemoExecutor.cpp`: reinstate the original `MR::invalidateClipping(actor)` before retaining demo cast for later revalidation.
- `src/Game/NPC/TalkBalloon.cpp`: restore the original `(0, 1)` reference vector.
- `tests/OriginalXanimePlayerTests.cpp`: mechanically adjust existing sampled-frame expectations for the donor phase: Cycle translations `20,25`; Blend frame pair `2,4`, weighted root `86`, main track `108`, next weighted root `46.25`; first one-shot sample `0`; simple-animation first sample `100`. Live frame-controller and termination assertions remain unchanged. No new cases, fixture changes, or tolerance changes.

`XanimePlayer::overWriteMtxCalc` at line395 was also inspected as requested: it already assigns the actual selected joint's `mMtxCalc = mCore`, exactly like donor line392. It is not empty and was left intact. `model_rebinding_and_calculator_slots` already contains the corresponding existing assertion.

Native core/shared-array/resource ownership is untouched. Sources frozen after these edits; no build or test was run while the root-owned live Gateway session was active. This patch does not itself establish a completed Gateway route.
