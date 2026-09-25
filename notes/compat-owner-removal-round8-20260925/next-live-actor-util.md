# Next closure: full original LiveActorUtil

Read-only source audit, 2026-09-25. No production/test edits or builds. The excluded native TU is an older 2,726-line import; current donor is 2,707 lines. Restore the current complete donor, retain CP932 strings and explicit native includes, remove its matching-only constant-order function, enable `Util/LiveActorUtil.cpp` with `-ffp-contract=off`.

## Delete 11 complete providers

All externally defined functions in these files belong to the donor LiveActorUtil (anonymous helpers are local duplicates):

- `src/compat/LiveActorUtilCompat.cpp` — clipping/light/default placement/visibility/nerve utilities, plus the five model-light/DL methods moved here during round8.
- `src/compat/GameRuntimeCompat.cpp` — clipping sphere/far/group helpers (retain group retirement boundary below).
- `src/compat/OriginalActorAnimationStart.cpp` — action/BCK starts and multi-animation frame helpers.
- `src/compat/OriginalActorBckControl.cpp` — `reflectBckCtrlData`.
- `src/compat/OriginalActorModelAccess.cpp` — resource/model/animation/frame-control/base-matrix access; its actor overloads are distinct from now-enabled ModelUtil.
- `src/compat/OriginalBinderQueries.cpp` — actual Binder state/normal/hit queries.
- `src/compat/OriginalBinderFilterUtil.cpp` — three filter setters.
- `src/compat/OriginalLiveActorGroupUtil.cpp` — original group iteration/broadcast/count utilities.
- `src/compat/OriginalMirrorReflectionUtil.cpp` — mirror configuration and texture replacement.
- `src/compat/OriginalCollisionPartsUtil.cpp` — original collision wrappers (retain decoding/ownership boundary below).
- `src/compat/OriginalFurAccess.cpp` — three original Fur entry points; full native FurMulti already exists.

A cheap twelfth deletion is `BinderCompat.cpp`: donor provides `setBinderOffsetVec`; move its existing native `Binder::~Binder(){ delete[] mPlane; }` to the already enabled actual Binder.cpp.

## Mixed files: remove only matching owner methods

- `GameActorPhysicsCompat.cpp`: remove `calcNerveValue`, `setClippingFar100m`, `isNoBind`, `onBind`, `offCalcGravity`, `onCalcGravity`. Leave the actual remaining shadow/MapUtil work for its own closure.
- `LodCtrlRuntimeCompat.cpp`: remove `copyTransRotateScale`, `hideModelAndOnCalcAnim`, `setClippingTypeSphereContainsModelBoundingBox`, `createLodCtrlNPC`. Its two shadow visibility utilities belong to excluded ActorShadowUtil; cannot merely drop them. Full ActorShadowUtil closure can delete this file later.
- `PlanetMapRuntimeCompat.cpp`: remove all MR methods and duplicate submodel-name helpers; only `OceanHomeMapFunction::tryEntryOceanHomeMap` remains. To delete the whole file without relocating its specific unsupported-name hack, restore the complete 75-line donor `Game/Map/OceanHomeMapCtrl.cpp` and wire its actual SceneObj factory. WaterAreaHolder::getCameraWaterInfo and WaterInfo already exist; check factory before enabling. This is an optional small extension, not a prerequisite for LiveActorUtil.

## Required native boundaries

1. **KCL decoding and lifetime:** donor anonymous `createCollisionParts` directly feeds archive bytes to KCollision. Current `OriginalCollisionPartsUtil` intentionally calls `smgpc::compat::create_collision_parts`, which decodes endian/pointers, retains the actual ResourceHolder token, registers all collision categories, and owns parts/server/PA parser. Full donor import must keep that one boundary until CollisionParts/ResourceHolder own its native backing. Do not substitute raw archive bytes or delete CollisionPartsCompat itself as part of this quick import. Root has already migrated the actual collision algorithms separately.
2. **LOD ownership:** both existing `createLodCtrlNPC` and `createLodCtrlPlanet` use an exception-safe temporary and `adopt_actor_lod_ctrl`. Preserve that narrow lifetime attachment when restoring the original setup; apply consistently to newly exposed `createLodCtrlMapObj`. Avoid a broad heap mutex around model initialization; original resource creation may wait for main-thread work.
3. **Clipping groups:** existing `setGroupClipping` calls `ClippingDirectorOwnership::capture_groups()` after the original join. Without it the sidecar cannot reclaim group info arrays/JMap IDs. Preserve the boundary until proper ClippingInfoGroup/Holder native destruction replaces the sidecar; never reconnect a second simulator.
4. **CP932:** generated Japanese punctuation and mirror names must use CP932 literals, as the existing native source already does.
5. **Scalar math:** donor `setBaseTRMtx(quaternion)` uses `TPos3f::setQT`. That API exists; native `setQuat -> TQuat::makeMtx` has the same scalar expressions as current donor. Preserve `-ffp-contract=off` on the owner rather than the stale manually expanded quaternion body. Remove stale private `sAnimRateScale`; current donor explicitly uses fixed scale1.

## Concrete missing source closure / compilation integration

- `Game/Map/Flag.hpp` and `Flag.cpp` are absent from native sources. Donor `createMapFlag` requires them. Complete decomp versions exist (504-line cpp).
- Flag requires **also missing** `Game/Ride/SwingRopePoint.hpp/.cpp`; complete donor cpp is only140 lines and depends on existing MathUtil. Import both owners, not a fake Flag stub or skipped LiveActorUtil function. Flag donor uses specialized GX includes: replace unavailable header paths with native GX umbrella, retaining actual draw commands. Inspect native deletion of Flag arrays/points/texture when importing; donor destructor is empty and explicit PC teardown cannot rely on host leaks.
- Existing full donor utility APIs otherwise have source/header providers for the audited model/material/Binder/group/Fur/mirror calls. This is source evidence only, not a compiler/link guarantee. Include CameraUtil, PlayerUtil, RumbleUtil, SceneUtil, NerveUtil/SystemUtil as required by compiler, plus explicit J3DMaterial/JUTNameTab headers; do not revive Game/Util.hpp umbrella.
- No missing SDK matrix method was found for donor quaternion restoration. Most likely immediate errors are missing full-type/util includes, Flag/SwingRopePoint integration, and signature/overload drift.

Recommended batch: full LiveActorUtil + Flag/SwingRopePoint imports, 12 provider deletions including BinderCompat, three mixed-file reductions, narrow existing lifetime hooks retained. Optional OceanHomeMapCtrl adds one more full deletion. No new test suite is needed; use root's requested app build and short actual-process smoke. Existing tests that explicitly assert shim-only null/zero-step behavior may need retiring those expectations because donor behavior is the contract.
