# Copied-original provider ownership audit

Read-only review on 2026-09-19 at parent `01f8c899fc33e7c9e7953d3dff9bc8902895173c`, decomp `d97d7e19a5d80e85c8d73327718c050a2b9e1c70`. No builds, gameplay runs, production edits, staging, or commits were performed. Scope is the four requested provider groups; this is not a complete compatibility audit.

## Findings

### 1. An original Crystal wrapper has two compiled owners

`src/compat/SceneConnectionCompat.cpp:8-10` is the exact original `MR::connectToSceneCrystal` body from `decomp/src/Game/Util/ObjUtil.cpp:374-376`. Its fixed movement, animation, and draw-buffer categories are authored original behavior, not a cage-specific compatibility workaround.

However, the same body already exists at `src/Game/Util/ObjUtil.cpp:375-377`. Both files are included by the Game archive's broad source globs (`src/Game/xmake.lua:7,100`). A read-only inspection of the current archive confirms duplicate strong definitions:

```text
$ /opt/homebrew/opt/llvm/bin/llvm-nm --defined-only --print-file-name build/macosx/arm64/debug/libsmg-pc-game.a
libsmg-pc-game.a:ObjUtil.cpp.o:                 T __ZN2MR21connectToSceneCrystalEP9LiveActor
libsmg-pc-game.a:SceneConnectionCompat.cpp.o:   T __ZN2MR21connectToSceneCrystalEP9LiveActor
```

This is a concrete duplicate ownership defect, even though both bodies agree and the existing executable linked. Static archive extraction can conceal duplicate implementations: the only explicit caller of the other function in `SceneConnectionCompat.cpp` is the legacy `GatewaySpinCheckpoint` destructor, so the original main need not extract that object at all. This does not establish general tolerance of duplicate definitions. This review did not rebuild with different extraction or whole-archive settings. Safe next action: remove only the compat duplicate, keep the already compiled original `ObjUtil` definition, then verify final executable linkage and the existing actual-process Crystal probe. Do not introduce a new Crystal dispatch path.

### 2. MarioState compatibility code is original gameplay code with an obsolete-looking placement split

All fourteen function definitions in `src/compat/MarioStateCompat.cpp` and both definitions in `MarioStateAccessCompat.cpp` match the canonical signature/body text after whitespace and comments are removed. Donors are:

| Native methods | Canonical donor |
| --- | --- |
| Lifecycle, state dispatch, posture, stack changes, noticed-status query (`MarioStateCompat.cpp:11-132`) | `decomp/src/Game/Player/MarioState.cpp:5-158` |
| `init`, `notice`, `keep`, `hitPoly` (`:134-146`) | `decomp/src/Game/Player/MarioWait.cpp:275-287` |
| `getBlurOffset` (`:148-150`) | `decomp/src/Game/Player/MarioActorSpecialDraw.cpp:785-787` |
| `draw3D` (`:152-153`) | `decomp/src/Game/Player/Mario.cpp:2088-2089` |
| `getCurrentStatus`, `isStatusActive` (`MarioStateAccessCompat.cpp:6-36`) | `decomp/src/Game/Player/MarioState.cpp:124-154` |

The empty/default virtual methods are genuine original defaults, not newly invented success stubs. No native actor-name branch, changed state transition, or novel gameplay policy was found here.

`src/Game/xmake.lua:21` still excludes the complete native `Player/MarioState.cpp`, while `:100` compiles its split providers. The provider's comment (`MarioStateCompat.cpp:4-7`) describes avoiding an optional player/MarioModule dependency. The current build compiles the rest of the Player sources normally, including `MarioModule.cpp`, and all of these providers reside in the same Game archive. No remaining *source-level missing dependency* was identified. A static object extraction rationale could still matter to a narrow test link; this read-only review does not assert that every such link is closed.

Safe next action: restore the canonical `MarioState.cpp` as the sole lifecycle/accessor owner, remove its copied definitions/providers, and restore the six original defaults to their canonical Player TUs if consolidating fully. Preserve a single owner throughout. Verify the original state-stack test and actual process startup; inspect narrow executable links before concluding the old split is unnecessary. This is source ownership debt, not evidence of a current Mario state bug.

### 3. OriginalMapQueries is an original import, not a new map-query algorithm

All nineteen definitions in `src/compat/OriginalMapQueries.cpp` match canonical signature/body text after whitespace/comments are removed. Donor `decomp/src/Game/Util/MapUtil.cpp` contains the same global 32-entry sorting buffer and count (`:13-14`), category helpers (`:17-57`), public line queries and sorting (`:149-248`), move-limit assignment (`:288-310`), and `Collision` accessors (`:608-623`).

In particular, native `:52-62` uses the original stack `CollisionPartsFilterActor`, passes it to the common query, and selects map category zero. It neither recognizes CrystalCage nor substitutes a hand-authored ray hit. The nearest-hit selection, limits, sensor exclusion, and category constants are also donor behavior. The general collision ownership underneath these queries is outside this bounded review.

`src/Game/xmake.lua:72` excludes `Util/MapUtil.cpp`, so this provider currently supplies a selected set of original methods. Classify this as legitimate missing-original-code import with source placement debt. Prefer eventually compiling the original utility TU and retiring duplicate providers once its entire API/link closure is ready. Do not delete this provider merely because its filename says compat, or replace donor algorithms with a new native approximation.

### 4. StorySequence retains an unreachable, noncanonical shadow evaluator

`src/compat/StorySequencePlatformCompat.cpp:41-105` defines `require_retail_flag`, `has_retail_special_star`, `can_turn_on_retail_flag`, and `is_retail_flag_on`. Repository search finds no references to these names outside this local helper group, and none of the exported entry points call the group. The functions have internal linkage, so there is no external caller hidden behind a public declaration.

This is a genuine native reimplementation: its recursion-depth cap (`:67`), special-star early return (`:58`), and unsupported-type exceptions (`:89-96`) do not reproduce `decomp/src/Game/System/GameEventFlagChecker.cpp:14-85`. The original checker handles GalaxyOpenStar, passed-story events, Galaxy, Comet, StarPiece, and Type_11 predicates; its event-value path narrows to `u16` before comparing to zero (`:56`). The local evaluator is incomplete and must not be revived as a fallback.

The active exported wrappers at native `:114-134` delegate to `GameDataHolder`, which delegates to the original checker (`src/Game/System/GameDataHolder.cpp:99-104`). Their donor wrappers are `decomp/src/Game/System/GameDataFunction.cpp:98-104,254-260`. Native `canOnGameEventFlag` additionally rejects a null name before delegation; this is input validation, not an alternate progression rule.

Safe next action: delete the unreachable local evaluator and its unused-only includes. Retain `require_current_game_data` and `unavailable`, which still have active callers. Classify the deleted code as dead shadow implementation cleanup, not a fix for current rabbit progression. Remaining explicitly unavailable movie/staff-roll/dynamic-demo operations are coverage gaps, not silently successful replacements; their implementation is beyond this review.

## Scene disconnection boundary

The other method in `SceneConnectionCompat.cpp:12-15`, `MR::disconnectToScene`, is a native permanent-retirement extension, not the original `MR::disconnectToSceneTemporarily`. It removes scheduler entries/draw registration and notifies retirement via `SceneScheduler::disconnect_name_obj` (`src/runtime/SceneScheduler.cpp:669-676`). The original temporary operation delegates to `NameObjExecuteHolder` (`decomp/src/Game/NameObj/NameObjExecuteHolder.cpp:370-372`).

Repository search finds one explicit caller of the native method, the `GatewaySpinCheckpoint` destructor (`src/scene/GatewaySpinCheckpoint.cpp:410-411`), and no original Game caller. Thus this small function is generic native teardown, not evidence of active original gameplay scheduling policy. Its placement in the `MR` API and `Game/Scene/SceneFunction.hpp:230` obscures that distinction. Prefer calling a clearly native retirement API from native-owned objects, while keeping original temporary-disconnect semantics separate. The broader legacy checkpoint architecture is covered by the parent review.

## Evidence limits

The source comparison extracted function signatures through balanced bodies and removed comments/whitespace only; it did not normalize identifiers, constants, statements, or API calls. Results: MapQueries 19/19, MarioStateCompat 14/14, MarioStateAccessCompat 2/2. Crystal's single wrapper was directly compared to its canonical donor and compiled duplicates were confirmed with `llvm-nm`. This establishes present source correspondence and ownership findings, not fresh runtime validation or complete retail equivalence of every donor.

Within this scope, the concrete build/source defect found is duplicate ownership of the Crystal wrapper; a current runtime defect from that duplication was not established. The unused story evaluator is noncanonical dead code. The original state and map algorithms are misplaced originals, not newly inferred gameplay replacements. No change to current Gateway behavior is justified from this audit alone.
