# Original Mario jump lifecycle: next movement tranche

Read-only audit following the original OnlyCamera import. No player source, compatibility provider, build configuration, or test was changed, and no build/runtime experiment was run for this audit. The intended next task is to execute the original update/mainMove/jump/landing system through general providers and remove the grounded PC replacement. Adding another native A-button action handler would preserve the central accuracy problem.

## Current input and execution path

The host input route already exists:

1. `pc-port/src/render/RendererService.cpp:343` maps Return/Space to `InputButton::CORE_PAD_A`.
2. `pc-port/src/compat/GamePadUtilCompat.cpp:224` implements the original `MR::getPlayerTriggerA()` wrapper through channel-zero WPAD trigger state; `getPlayerLevelA()` at line 240 reads held state.
3. `MarioActor` initializes `_F1A=3` (`pc-port/src/Game/Player/MarioActor.cpp:248`). The unchanged `MarioActor::isRequestJump` at `MarioActorPad.cpp:102` preserves bind/input guards and calls `checkButtonType(_F1A,false)`. Its type-3 path reads A-trigger. `isKeepJump` uses type 5/A-held. `MarioModule::checkTrgA/checkLvlA` at `MarioModule.cpp:428` are also present.

The current production showcase then stops short of the original consumer:

- `pc-port/src/showcase/xmake.lua:1` builds the dedicated `smg-pc-mario-gateway-walk-slice`. It includes Mario, Actor, Animator, Move, Walk and related limited providers. **It does not include `MarioJump.cpp`, `MarioCollision.cpp`, `MarioActorGravity.cpp`, `MarioState.cpp`, or the original movement-state object translation units.** The broad Game archive excludes those Player files in `pc-port/src/Game/xmake.lua`.
- `MarioActor::control`, PC branch at `pc-port/src/Game/Player/MarioActor.cpp:1011`, sets a gravity basis, samples swing state, then directly calls `mMario->update()`. It bypasses original `controlMain/updateBehavior` sequencing (`src/Game/Player/MarioActor.cpp:977` calls the original update after original gravity/action setup).
- The PC-only `Mario::update` at `pc-port/src/Game/Player/Mario.cpp:2026` always does inputStick -> mainMove -> updateWalkSpeed -> native ground bias. It never dispatches `actionMain`, `procJump`, landing, the original collision update sequence, or original timers.
- The PC `Mario::mainMove` branch at `pc-port/src/Game/Player/MarioMove.cpp:19` never calls `isRequestJump`; it contains its own walk-direction/velocity calculation and explicitly throws if `mMovementStates.jumping` is set. The original root `mainMove` at `src/Game/Player/MarioMove.cpp:14` samples `isRequestJump`, saves the last safe translation, calls `tryJump`, and returns. It also handles falling through `tryDrop`.
- `MarioActor::movement` at `pc-port/src/Game/Player/MarioActor.cpp:803` copies Binder contact directly into selected Mario ground fields. Original `Mario::updateGroundInfo` and `MarioCollision` compute a wider set of collision, wall, ceiling, floor-code and jump-contact state. Merely allowing the jumping bit in the PC branch does not provide that state.

The existing live test (`pc-port/tests/MarioGatewayWalkTests.cpp`) proves stand/walk/release/idle plus camera input and swing entitlement. It does not press A, observe a jump arc, or exercise landing. Its successful walking distance is not jump validation.

## Original action sequence and source coverage

The original control flow is:

`MarioActor::updateBehavior` -> `Mario::update` -> `Mario::actionMain` -> grounded `mainMove` / airborne `procJump(false)` -> `tryJump` / `doAirWalk` / `doLanding` -> original physical write-back and timers.

`src/Game/Player/MarioJump.cpp` already contains complete-looking recovered bodies for `tryJump` (line 47), `initJumpParam` (1046), `procJump` (1113), `doAirWalk` (1861), `stopJump` (2060), and `doLanding` (2229), with their other jump modes/helpers. Its PC mirror is currently byte-identical, SHA256 `66941b3a5f991ca911cf208b5203ab30633aa54f9fdceab5793c29c8e39b7068`. `configure.py` marks Mario/MarioMove/MarioJump nonmatching; this audit has not revalidated all their recovered algorithms. The empty `checkWallRising` is consistent with the retail symbol size of four bytes, so it is not evidence of a missing large implementation.

The normal jump uses the real `MarioConstTable`: `mJumpHeight[_430]`, `mGravityRatioA`, `mGravityAirWalk`, `mMaxDropSpeed`, air-control timers, and jump connection state. `tryJump` initializes `mJumpVec`, calls `procJump(true)`, and selects the original named jump/fall animation. `procJump` reads A-held/release for variable height, updates gravity/air movement, and calls `doLanding` only when original ground state permits it. Preserve these coupled routines rather than selecting an impulse or landing rule in host code.

### Lost source and historical recovery

The current root `src/Game/Player/Mario.cpp` has no `Mario::update`, `Mario::inputStick`, or `Mario::checkForceGrounding` definition. All three exist in commit **`96e5ef0decce22e5bfd7d0ee876fb15ac80a725b`**. That revision also has recovered `actionMain/updateGroundInfo/doExtraServices` bodies; the current upstream versions of those exist but differ in previously audited details, so compare the whole six-function group instead of pasting one function blindly.

Historical root `Mario.cpp` SHA256: `bd3fd8040019266aeeb31020b765c3127c290e9c72610efb6f7c8081110e66a2`.

Historical `void Mario::update()` signature-through-closing-brace SHA256: `d6b425a295e7cfefd01ffd11be9217da8b3967a29cecfcf6705e547298739fe7`.

`pc-port/notes/mario-walking-core-rmgk02-20260809T043114Z/README.md` records prior **RMGK02** objdiff results: update 96.566574%, inputStick 97.070710%, checkForceGrounding 97.174160%, actionMain 100%, updateGroundInfo 99.965990%, doExtraServices 92.061066%. Those are historical evidence, **not a live RMGK01 match**. Its important claims to recheck include checkGround return propagation, grounded-damage guards, the misspelled `writeBackPhyisicalVector` symbol, and second-word movement flags.

Use the current verified **RMGK01** DOL (`build/compat-math-oracle/main.dol`, SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`) and original compiler before making root corrections:

| Function | RMGK01 address | Size |
| --- | --- | --- |
| `Mario::doExtraServices` | `0x802AA048` | `0x20C` |
| `Mario::checkForceGrounding` | `0x802AA308` | `0x2C8` |
| `Mario::inputStick` | `0x802AC404` | `0x318` |
| `Mario::update` | `0x802AD398` | `0x584` |
| `Mario::actionMain` | `0x802AD91C` | `0x1E0` |
| `Mario::updateGroundInfo` | `0x802ADAFC` | `0x24C` |
| `Mario::mainMove` | `0x802E9FEC` | `0x1A24` |
| `Mario::tryJump` | `0x802E1E44` | `0xD20` |
| `Mario::initJumpParam` | `0x802E4A54` | `0x184` |
| `Mario::procJump` | `0x802E4C88` | `0xCB0` |
| `Mario::doLanding` | `0x802E7CD4` | `0xD50` |

## Runtime dependency frontier

These are concrete blockers found by inspecting callers, not an exhaustive link-closure inventory.

**Original animation state is required even for a plain dry-ground jump.** `pc-port/src/Game/Player/MarioAnimator.cpp:38` assigns null to the resource table and both Xanime players. Its PC `update` at line 1118 selects only Run/Wait through the native BCK registry. Original `MarioModule::isAnimationRun/changeAnimation/stopAnimation` (`MarioModule.cpp:73–115`) dereference the actual Xanime players. `mainMove`, `tryJump`, `procJump`, and `doLanding` call them. The source-backed query-only `compat/XanimeQueryCompat.cpp` does not construct that state. Bring up actual XanimeResourceTable, XanimePlayer and XanimeCore lifecycle with Mario's authored animation tables/resources, then connect original joint/matrix calculation to the existing model renderer. Root implementations are in `src/Game/Animation/XanimeResource.cpp`, `XanimePlayer.cpp`, `XanimeCore.cpp`, plus `src/Game/Player/MarioAnimator.cpp` and its data/callback tables. Only Player/Resource headers and the query closure are currently present in the PC Animation module. Do not substitute another jump-to-BCK name table or an always-false animation predicate.

**Construct real movement-state objects before original update runs.** The PC Mario constructor at `Mario.cpp:334` nulls all these objects. Original `updateAndClearStrideParameter` dereferences `mWall->_1C` whenever grounded (`src/Game/Player/Mario.cpp:373`), and `updateTimers` dereferences `mHang->mHangTimer` unconditionally (line 413). `tryJump` calls `mSwim->checkUnderWaterFull` even on a normal jump (MarioJump.cpp:366), and falling `procJump` reads `mSwim->_1B2` (line 1200). Therefore actual **Wall, Hang and Swim state construction** is an immediate minimum, not optional support for exotic modes. Their constructors and state base are available in `MarioWall.cpp:90`, `MarioHang.cpp:193`, `MarioSwim.cpp:194`, and `MarioState.cpp:5`; full class vtables pull further original routines. Other state dependencies should be closed from actual linker/source edges, without fake partially constructed objects.

**Original Mario gravity management is still undecompiled.** `src/Game/Player/MarioActorGravity.cpp:124` contains only a commented `updateGravityVec(bool,bool)` placeholder, also absent from historical `96e5ef0`. Retail `updateGravityVec__10MarioActorFbb` is at **`0x802B9A9C`, size `0x930`**. `getGravityRatio` returns `mGravityRatio`; the PC actor initializes that field to zero at `MarioActor.cpp:326`, and its active gravity/control replacements never update it. `procJump` multiplies airborne gravity by this ratio (`MarioJump.cpp:1338`), so reusing the current direction-only setup would not supply original jump gravity. Recover the actor's original gravity update first, using existing general planet-gravity queries and GravityInfo ownership; do not set a guessed ratio for Gateway. The existing `compat/GameGravityCompat.cpp` already supports real scene gravity queries and `MR::isLightGravity`.

**Ground, ceiling and landing state need original MarioCollision.** The complete mirrored `MarioCollision.cpp` is excluded from the active slice. Its `checkGround`, `calcDistToCeil` (line 279), `updateBinderInfo` (1344), wall queries and floor-code updates supply state consumed by the jump loop. Reuse existing Binder/Collision/triangle providers; root `update` explicitly calls these in a defined order before action processing. General movement must eventually retire the native inward-ground bias and idle correction alongside the PC-only update, after the original pipeline is verified.

**Dry-ground jump still queries water and shared effects/services.** `MarioSwim::checkUnderWaterFull` (line 1898) queries ForbidWaterSearchCube, real water areas and map collision. `MR::getWaterAreaObj` has only a declaration in the current root/PC sources; its RMGK01 routine is **`0x80400964`, size `0x70`**. Recover that root function and connect its actual scene/area query; an empty real registry is a legitimate result, a hardcoded no-water predicate is not. `src/Game/Map/WaterInfo.cpp` has the original constructor/clear/isInWater methods and is absent from PC. `MR::checkStrikePointToMap` already has a real collision provider in `compat/GameMapCollisionCompat.cpp:216`.

`tryJump` starts common takeoff effects, while the PC actor's `init2` branch omits original `MarioEffect` construction/init. Original effect routing is in `src/Game/Player/MarioEffect.cpp:762`, with `MarioModule` forwarding calls. Sound calls reach `MarioSound.cpp`, which is excluded; audio can remain outside the demo's presentation requirements, but provider ownership should be explicit. `MR::getKarikariClingNum` is used by squat/gravity checks; its original `src/Game/Enemy/KarikariDirector.cpp:158` already checks actual scene-object existence and returns zero only when absent, which is a suitable general source-backed provider contract.

When activating complete source units, retire conflicting extracted accessors: for example `Mario::isRising` currently comes from `compat/MarioCameraAccessCompat.cpp:171` and is also defined in `MarioJump.cpp:24`. The same check applies to source-backed state/map accessors as their full owners become active.

## Replacement and validation plan

1. Restore the lost root update/input/grounding group from historical source, adapt only proven member/declaration drift, and compare all recovered/corrected behavior against the current RMGK01 DOL using original compiler output. Recover original `MarioActor::updateGravityVec` and `MR::getWaterAreaObj` root-first; retain clear provenance for every body.
2. Complete shared animation construction/resource/joint support and original state-object construction. Independently compile/link the original jump and collision closure against these providers. Resolve actual dependencies in shared services rather than relaxing gameplay guards or supplying fixed query results.
3. Restore original update/mainMove/action/physical-writeback and Actor sequencing. Remove the grounded-only PC bodies as that complete lifecycle becomes usable. Preserve original frame order with the camera phase; don't move Mario or synthesize jump vectors in the camera or showcase bridge.
4. Extend the real-disc movement fixture through a full ground -> A-trigger -> rise -> release/hold -> apex -> fall -> original landing -> stable ground cycle. Compare tap and hold using original variable-height logic; include walking takeoff and airborne steering, actual named animation transitions and retained collision/ground polygons. Confirm held A does not fabricate repeated triggers and a fresh press works after landing. Use source/DOL oracles for original constants/branches rather than choosing expected Gateway-specific distances.

This is a larger subsystem closure than adding a key binding. Source restoration, real Xanime construction, and original gravity/area-provider recovery can proceed independently, then converge on one original Mario update/action lifecycle.
