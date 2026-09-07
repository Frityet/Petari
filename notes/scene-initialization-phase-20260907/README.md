# Native scene initialization phases

The original MarioShadow initialization queries `MR::isInitializeStatePlacementSomething`. It must observe the scene's actual initialization phase, not a constant answer or actor-specific special case.

## Original reference

`GameSystemSceneController::initializeScene` enters `SceneInitializeState_Init`. `StageDataHolder::initPlacement` sets player placement, then common/scenario high-priority placement, then the three ordinary placement passes. `GameScene::init` selects `AfterPlacement` before original `NameObj::initAfterPlacement` callbacks. The controller records `End` only after asynchronous scene initialization completes successfully.

The enum and controller declarations in `src/Game/System/GameSystemSceneController.hpp` are byte-identical to the reference header. This checkpoint changes no original Game implementation.

The newly recovered `decomp/src/Game/Util/SceneUtil.cpp` query calls the controller's equality predicate for 2, 3, then 4 with short-circuit OR. Fresh Metrowerks compilation succeeds; objdiff against the retained retail split scores **82.55556%**, with 120 compiled bytes versus 144 original bytes. The original retains two boolean registers and the compiler simplifies the source to one; the three state checks, load/call order, short-circuit behavior and returned truth table match. The earlier explicit-flag source scored identically, so the final reference uses the ordinary project OR convention. `query-disassembly.txt`, `SceneUtil-or.objdiff.json`, `compile-or-command.json`, and `recovery-summary.json` preserve this limitation and evidence. No inline assembly or behavior guess was used.

## Native ownership

`SceneInitializationBinding` is an allocation-free single-scene owner held as the first member of `SceneObjHolderBinding`. It starts at Init, survives holder-owned original object teardown, and retires last. A second simultaneous owner is rejected. `complete_initialization()` persists End only at a successful scene initialization boundary. Missing owners reject queries/setters/scopes explicitly.

`SceneInitializationScope` borrows the same state and restores both the previous enum and prior scope on normal exit or exception. It does not own or emulate a GameSystem singleton. The native MR setters and predicates adapt this state with the original enum and ordered equality logic. The owner follows the existing single-current-scene convention; no new cross-thread scene ownership or loader synchronization is claimed.

Wiring:

- Shared authored instantiation uses ordinary Placement around the operation and HighPriority for both high-priority group passes; constructors and their original init callbacks see the active phase. Preload stays outside placement.
- SceneObj, authored and external root postpasses scope AfterPlacement, including callbacks which append more objects.
- StageHost scopes StartInfo player construction, explicit ordinary roots, and postpasses; successful initialization records End.
- The Gateway development caller scopes the real Mario constructor and init as Player, outside Game code. Finalization records End after authored placement and postpasses complete.
- Title/FileSelect records End after its construction and initial appearance complete.

Failures restore their surrounding phase and never automatically complete the scene. Completion while a borrowed phase is active is rejected, which prevents a destructor from overwriting a successful End transition.

## Validation

The initial `validate-native.py` syntax pass covered all 11 changed source/test translation units using the actual native compile database and passed. A later broad rerun overlapped a temporary, unrelated matrix-header edit (`TVec4f` unavailable); `transient-matrix-syntax-results.json` and the per-file syntax logs retain those failures, and do not represent the final source. After that header was corrected, the focused isolated native owner + MR adapter test was rebuilt and passed again (`isolated-results.json`). It checks every original state predicate, setter, missing owner, duplicate owner, invalid enum value, nested exceptions, completion guard, persistent End and generation replacement. Coordinated Xmake builds provide final integration coverage rather than repeating broad syntax checks after the transient header failure.

Existing authored-placement tests now explicitly bind their synthetic initialization owner, still without fabricating a SceneObjHolder. They check the five high-priority construction callbacks followed by eleven ordinary callbacks, original init placement visibility, AfterPlacement callback visibility, and restoration to Init. Existing SceneObj tests verify Init ownership and persistent End from the real binding. Coordinated Xmake integration results are recorded separately once run.
