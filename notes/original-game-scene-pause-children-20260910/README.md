# Original GameScene pause and opening camera children

Imported byte-identical reference implementations of `GameScenePauseControl`, `GameSceneScenarioOpeningCameraState`, and `GamePauseSequence`, plus complete original declarations for PauseMenu and the AudSystem/AudAudience/AudAudible/AudLimitedSound family. Existing child headers were already present. The separate audio agent published the complete required JSystem declaration closure; no partial sound manager or fabricated AudSystem object was introduced.

Fresh Wii builds and retail comparisons (`wii-proof.json`) match every paired symbol: GameScenePauseControl 12/12 at 100%, opening camera state 18/18 at 100%, and GamePauseSequence 26/26 at 100%. Counts include constructors, state executors, vtables and static data where paired; they are not a runtime claim.

The missing opening-camera scene stop/play helpers were recovered first in `decomp/src/Game/Util/SceneUtil.cpp`. Both retail 44-byte functions call the actual SceneNameObjMovementController with `MovementControlType_4` and null requester. Fresh comparison proves both 100% (`scene-control-proof.json`). Decomp commit `0c4ebc9cc` is authored by codex and pushed to origin/pcp-decomp. The full updated SceneUtil reference is mirrored under Game; that original TU remains excluded while exact helper bodies are provided by `compat/OriginalScenarioOpeningSceneControl.cpp`.

`native-proof.json` records four successful LLVM 23 object compiles against current production headers, with command arrays, source hashes and undefined-symbol inventory. These builds include the actual AudSystem layout and its embedded complete manager declarations, so the opening camera's `_830` store uses the real declared member. No SDK or Game proxy is cast to AudSystem. The initial notes-only missing-header probe is retained separately; its nine errors were resolved by the coordinated SDK declaration cohort.

## Concrete remaining owner dependencies

- `AudWrap::getSystem()` still explicitly reports unavailable in the native audio facade. Genuine AudSystem construction, frame work, pause entry/exit and its actual child graph remain required. The original opening camera stores 30 into `_830`; pause entry/exit requires AudSystem::enterPauseMenu/exitPauseMenu. Header completeness alone does not enable these behaviors.
- `GamePauseSequence::init` constructs PauseMenu whenever the original stage predicates allow it. PauseMenu's full implementation and descendant layout/save/letter graph are not imported by this bounded change; its constructor remains a link frontier.
- `GameSystemFunction::onPauseBeginAllRumble/onPauseEndAllRumble` use the actual WPad rumble patterns/controllers. Their original owner closure remains absent; this change does not replace them with successful no-ops.
- Opening-camera skip eligibility requires `MR::isAlreadyVisitedCurrentStageAndScenario`, which delegates to the actual saved GalaxyScenario already-visited state. It must not be replaced with a constant.
- The opening stop/play wrappers now dispatch through the existing actual movement controller. Camera start/end, cinema frames, star-pointer mode, player visibility/position and ScenarioTitle still require their current game/runtime owners to be linked and initialized before the opening state can be exercised.

These child sources have compiled and matched the Wii reference. They have not linked or run as a complete GameScene, and do not yet constitute a refreshed playable demo. No root Xmake run or root commit was performed by this agent.
