# Post-Rosalina spin and first launch audit

2026-09-25; current native source following `a71fc2ea7`, donor `decomp` at `1a126cb5d`. Read-only source comparison; no builds, tests, live input or production edits. This note is the only file written by this task.

## Finding

**No no-op substitute or concrete semantic donor divergence was found that prevents the normal spin tutorial → crystal break → first SuperSpinDriver launch.** The missing-source/factory claims in `notes/grand-star-route-20260925/README.md` are stale. This comparison establishes source presence/fidelity, not live launch success or rendering correctness.

Compared full source and headers for CrystalCage, CrystalCageMoving, SuperSpinDriver, SpinDriver, SpinDriverCamera, SpinDriverShootPath, SpinDriverPathDrawer, SpinDriverOperateRing, SpinDriverUtil, TicoDemoGetPower and InformationObserver. Also compared MultiEventCamera and ParabolicPath source. After normalizing CP932 wrappers, includes, equivalent GET_NERVE/instance spelling, Functor_Inline spelling and explicit vector-template spelling, remaining differences are:

- CrystalCage passes `false` instead of donor `nullptr` to the bool `setBinderOffsetVec` parameter; its explicit third `startBck` argument is the declared default `nullptr`. Both preserve behavior (`LiveActorUtil.hpp:141/308`).
- SpinDriverPathDrawer packs RGBA numerically rather than through native byte layout. The shifts preserve the original big-endian color word on the little-endian host (`SpinDriverPathDrawer.cpp:14`). This is a necessary architecture fix, not a launch gate.
- SpinDriverShootPath header now includes complete dependencies instead of forward declarations and corrects offset comments. Fields and APIs are unchanged.

Empty Wait/TryDemo nerves, FORCE_MATCH/DUMMY helpers and the cooldown BUG comment also occur in the donor. They are not evidence of newly stubbed behavior.

## Normal gates in the current owners

| Gate | Actual implementation and relevant requirement |
| --- | --- |
| Tutorial requests spin explanation | `TicoDemoGetPower.cpp:45` calls `MR::explainEnableToSpin` at the first step of `スピンゲット[デモ5]`. `EventUtil.cpp:104` forwards to the real InformationObserver. |
| Tutorial grants permission | `InformationObserver.cpp:61/80` pauses the active time demo, shows the original information message, and requires a fresh A trigger after its 30-frame minimum. Closing resumes the demo and calls `onGameEventFlagEnableToSpinAndStarPointer`. `EventUtil.cpp:906` advances the actual `スピン権利` story event and `PlayerUtil.cpp:168` writes the real MarioActor swing permission `_EEB`; this is not a synthetic story flag. |
| Crystal releases authored switch | `CrystalCage.cpp:183` accepts original player-hit-all messages only while Wait and respects remaining hits/cooldown. Break calls `tryOnSwitchDead` (`:280/:381`), writing the configured dead switch at step 0, or step 10 for the relevant display-model kind. No unconditional fake break is used. Historical authored map identifies MysteriousZone cage l_id72 / dead1017 as the first launch gate; that resource identity was not re-decoded in this audit. |
| Driver appears | `SuperSpinDriver.cpp:214` listens to its original appear switch/demo/group policies. The canonical factory contains SpinDriver, yellow/green/pink SuperSpinDriver (`NameObjFactory.cpp:964-981`) and all CrystalCage S/M/L/Moving entries (`:2319-2336`), including their archive rows. `src/Game/xmake.lua:8` builds all Game cpp; none of these owners is excluded. |
| Driver binds and fires | `SuperSpinDriver.cpp:1022` requires Wait beyond 90 steps. `:368/384` checks ordinary auto-rush sensor binding and either pad swing/2P trigger or authored pull-player proximity. Capture accepts another swing (`:445`); ShootStart transitions after step45. Shoot follows the original rail/parabolic path, moving the actual bound actor through `updateBindPosition`/`updateBindActorMatrix` (`:926/:822`). |
| Driver returns control | `tryEndShoot` (`SuperSpinDriver.cpp:466`) ends at authored `mFlightTime`; `endBind` (`:816`) calls `MR::endBindAndSpinDriverJump` and ends the actual event camera. MultiEventCamera and ParabolicPath are donor-equivalent, with actual camera targets/rail fields. |

InformationObserver and SpinDriverPathDrawInit also have real SceneObjHolder factory cases (`SceneObjHolder.cpp:478/540`). Audio calls do not provide completion predicates for the inspected gates.

## Practical validation limitation

The current debug controller file/script supports buttons, pointer and stick only (`DebugWpadInputScript.hpp:12-47`). The actual process constructs core acceleration from physical `CORE_PAD_SWING`, mapped to X (`OriginalGameApplication.cpp:394`, `RendererService.cpp:383`). Thus a file-only route cannot request a shake by inventing a WPAD spin button. Use ordinary X input for the next live gate, or add a general debug-controller gesture channel when requested; do not alter Mario permission, force-break cages or synthesize driver binding. This is an input-tool limitation, not a missing Game actor implementation.

No source fix is recommended on the evidence of this bounded comparison. If the next live run stops, first distinguish the pending A trigger, spin permission, cage-hit/switch state and driver Wait/Capture/ShootStart nerve before broadening the investigation.
