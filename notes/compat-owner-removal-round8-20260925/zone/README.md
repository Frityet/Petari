# Stage zone registry removal and actor fixture migration

Baseline: `e027464426599e3dacb0394f2fb90ecd8f7a1f70`. All four owned paths were clean before this lane; complete before copies and SHA-256 values are recorded in `before/` and `owned-manifest.json`.

Removed `src/compat/StageZoneMatrixRegistry.cpp/.hpp`. There are no production consumers: the actual `SceneUtil` already obtains matrices through `StageDataHolder`. The donor and current owner preserve integer lookup of the root/first immediate child, recursive iterator lookup through raw entry address ranges, full native pointer-width bounds, and matrix addresses embedded in each holder. No Game source was changed and no replacement registry was created.

The user's instruction to move faster and do much less testing superseded the initial proposed replacement fixture. Deleted `tests/StageZoneMatrixRegistryTests.cpp` instead of creating `OriginalStageDataHolderTests.cpp`. Existing `OriginalProcessPlacementTransformTests.cpp` remains the actual retail holder/raw-row/matrix coverage. The removed registry fixture's synthetic repeated/empty/nested occurrence cases and invented nested thread-local binding/absence-exception expectations were not recreated. This lane does not claim newly executed coverage for those cases.

`GameActorPhysicsRealOrAbsentTests.cpp` now uses the existing `run_stage_resource_process` helper and actual retained scene heap. Its five substantive actor groups remain: position/sensor/Binder reset, rotation and collision rebound, clipping, shadows/retirement, and Binder filtering. Clipping configuration and updates inspect actual ClippingActorInfo/ClippingJudge; shadow identity, radius, pointers, modes and retirement inspect the original controller/list/drawer/holder graph. The existing explicit clipping-plane case temporarily selects its local judge and restores the process judge. The two-controller construction still uses the currently necessary shadow-owner constructor API; it no longer treats the sidecar as live query state.

Removed three pre-bootstrap absence-exception groups for coin/event/layout APIs, plus absent-owner MirrorArea/DeathArea/shadow mutation exception assertions. These depended on synthetic missing process/scene ownership and were not an original Game API contract. No new cases were added, and production code was not changed to satisfy these expectations.

Root wiring: remove the entire `smg-pc-stage-zone-matrix-tests` target; add `smg-pc-app` and `aurora-main` dependencies to `smg-pc-game-actor-physics-real-or-absent-tests`. Do not add a replacement zone target. The only remaining source reference at the lane scan was the in-progress collision fixture migration (another lane), plus the root-owned xmake entry; both were reported.

No builds, runtime tests, staging, or commits were performed. Validation for this checkpoint is intentionally limited to root's app build and short smoke run unless root requests otherwise.
