# Resource and utility owner consolidation, 2026-09-25

This batch removes nine compatibility files. The original Game owners supply the retained behavior; the stage binding has no replacement provider. No build, test execution, xmake edit, staging or commit was performed by this agent.

## Removed files and original owners

| Removed provider | Result |
| --- | --- |
| `src/compat/StageResourceBinding.cpp` and `.hpp` | Deleted production-orphan parallel stage catalog. Production already queries `Game/Scene/StageDataHolder.cpp` through original `Game/Util/SceneUtil.cpp`. All four test consumers updated. |
| `src/compat/MemoryHeapScopeCompat.cpp` | Original current-heap/restoration method set consolidated into `Game/Util/MemoryUtil.cpp`. |
| `src/compat/MemoryUtilCompat.cpp` | Allocator, byte operations and checksum consolidated into `Game/Util/MemoryUtil.cpp`. |
| `src/compat/OriginalStationedArchiveUtil.cpp` | Original heap selection methods in `MemoryUtil.cpp`; original player-archive request/wait methods in `SystemUtil.cpp`. |
| `src/compat/OriginalLayoutResourceUtil.cpp` | Five original font accessors restored in `SystemUtil.cpp`. Original actual `GameSystem::mFontHolder` owns them; removed duplicate guard/helper. |
| `src/compat/OriginalGameSystemAccess.cpp` | All original process object, message, audio, allocator, random, request clearing and reset methods restored in `SystemUtil.cpp`. |
| `src/compat/OriginalScreenConfig.cpp` | Original aspect-ratio lookup restored in `SystemUtil.cpp`. |
| `src/compat/SceneNameObjUtilCompat.cpp` | Four original scene NameObj operations restored in `SystemUtil.cpp`; removed alternate registry fallback. These now use actual `GameSystemSceneController::mObjHolder`. |

`method-inventory.json` maps all 38 public utility definitions from the seven removed utility providers to a unique canonical owner. Its check also found no same-named definitions left in compatibility sources. Canonical source donor is current decomp `1a126cb5d`. The heap free-ratio helper now also comes from the original donor.

Native adaptations retained in MemoryUtil: array allocator uses matching `delete[]`; byte copy/fill/checksum avoid original PPC word-load alignment assumptions; existing unavailable-pointer checks and zero-size behavior preserved; recursive OS mutex lock/unlock implements the original `JASMutexLock` heap selection scope (native JASMutex header is absent). Checksum continues to load native-endian u16 values via memcpy; no checksum behavior changed. This is an integration of current implementations with original heap-selection methods, not a new memory algorithm.

## Deliberate boundaries

Canonical SystemUtil omits methods still provided by `OriginalFunctionAsyncUtil` (guest-thread execution scope), `StageSessionGameCompat` (alternate session owner), `OriginalParticleResourceLookup` (parallel particle owner) and the PAL60 method in mixed `SceneSystemUtilCompat`. Those providers were not changed. MemoryUtil leaves `getSceneHeapGDDR3` to `SceneHeapAccess`, and WPad alloc/free to `WPadOwnership`, pending their alternate owner removal. No duplicate or invented fallback was added.

The donor's `getHomeButtonLayoutAllocator` remains unavailable exactly as before: native HeapMemoryWatcher has no home-button layout heap field and the native allocator template statics are absent. It has no caller in this port. No stub was added.

## Tests and build integration needed by root

1. Remove `remove_files("Util/SystemUtil.cpp")` from `src/Game/xmake.lua` to activate the canonical file. `Game/**/*.cpp` already includes the new MemoryUtil file. Regenerate the source graph to drop the nine deleted sources.
2. Change the existing `smg-pc-stage-camera-resource-tests` and `smg-pc-original-name-pos-owner-tests` target dependencies to `{"smg-pc-app", "aurora-main"}`; group them with original-process tests. Their new helper `tests/OriginalStageResourceProcessFixture.hpp` runs the actual original game with a fresh save and requires `SMGPC_REAL_DISC`, a debug build and graphics. It never supplies an alternate stage implementation.
3. StageCameraResourceTests verifies actual recursive start-row order, placed-zone camera resource identities, decoded string lifetimes, authored rail ID/row consistency, absent-zone semantics and scenario camera archive access. OriginalNamePosOwnerTests verifies every actual Gateway name-position row, zone transform, link fields, scene heap ownership, first-name precedence, missing-name output preservation and ordinary process retirement. These replace tests of the removed parallel stage catalog, including its invented out-of-range exception and arbitrary survival beyond a Game arena.
4. OriginalCameraHolderTests keeps its bounded independent retail archive check by registering archived bytes and passing the real DotCam reader to original chunk load/arrange methods. The actual stage routing is exercised by StageCameraResourceTests; the camera-holder unit test no longer constructs an unused fake stage binding.
5. OriginalCollisionPartsOwnerTests loses only the inert binding construction/include. Its already-dirty changes were preserved; exact agent-only patch is `OriginalCollisionPartsOwnerTests-owned.patch`.

Before copies of all existing touched files and SHA256 hashes are in `before/` and `before-manifest.json`. Exact test deltas against those copies are in `*-owned.patch`, including preexisting dirty test preservation.

Static validation only: scoped `git diff --check` passed; no `StageResourceBinding`/`require_stage_resources` references remain in `src/` or `tests/`; inventory checks found no missing or duplicate migrated utility definition. Build and runtime results must be recorded by the root checkpoint.

## Post-build test correction

Root reported the integrated build passed and OriginalNamePosOwnerTests passed all seven authored rows plus 120 frames and teardown. StageCameraResourceTests stopped at its camera pointer assertion (`../run-11.log`). That assertion incorrectly compared the actual StageDataHolder archive's resource pointer with an independently decoded DVD Rarc owner. Pointer equality is only meaningful within the actual owning archive. The repaired test checks pointer and size against `holder->getStageArchiveResource` / `getStageArchiveResourceSize`, then compares bytes and length with the independently read retail archive. Production code remains unchanged.

NPCActorRealOrAbsentTests' group portion also used a standalone SceneObjHolderBinding after canonical scene access began requiring the original process. Only that fixture was replaced with OriginalSceneControllerFixture plus SceneExecutionFixture publishing its actual holder and executor through the original Scene. Root's original-hash-table assertions and all NPC model/talk tests remain untouched.

`post-build-repair/before/`, `before-manifest.json` and exact `*-owned.patch` files preserve the already-dirty tests immediately before these two edits. Scoped diff checks passed. No build or runtime test was performed by this agent after the repairs; root will validate.
