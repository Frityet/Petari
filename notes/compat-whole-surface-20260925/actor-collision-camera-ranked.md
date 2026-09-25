# Actor, collision, camera audit

Authoritative donor: `decomp` commit `1a126cb5da311fedff662f53fb31c5aeaf851408`. All 67 scoped files were read. The JSON contains per-file destinations, dependencies, evidence and bounded textual consumers. No build or runtime test was run for this audit. `keep-general` means integration into canonical JSystem/Aurora, never retention of a compat directory.

## Ranked whole-owner consolidation

1. **CameraTargetObj**: restore complete `Game/Camera/CameraTargetObj.cpp`; delete `OriginalCameraTargetActor.cpp`, `OriginalCameraTargetPlayer.cpp`, and the base constructor fragment from `CameraLocalUtilRuntime.cpp`. Exact current upstream actor/player behavior already matches these providers. Whole donor also restores the currently absent `CameraTargetDemoActor` constructor/init/setTargetMtx. The actor target uses actual model matrices or rotation fallback and original CubeCamera lookup; player target uses bind matrix, Bee gravity, camera grounding, player movement timer and demo suppression. No host resource sidecar or new ABI needed; native matrix casts already match. `Game/xmake.lua` wildcard selects the new owner. Validate one definition of every method, build/link, `OriginalCameraContextTests`, `OriginalCameraHolderTests`, and `ActorEventCameraTests` target phases. This batch can run independently of model/math/sensor changes.

2. **ActorSensorUtil**: restore complete canonical `Game/Util/ActorSensorUtil.cpp`, delete `GameActorSensorCompat.cpp/.hpp` and `OriginalActorBroadcast.cpp`. Original sensor types/messages and variants are already available. Move child reclamation to `HitSensorKeeper`/`SensorHitChecker` native destructors and remove registry-mediated getter/count wrappers. `actor_message_name` belongs to its actual `runtime/ParityTrace.cpp` debug consumer. Tests should inspect original keeper state. Remove xmake exclusion of ActorSensorUtil. Validate `ActorSensorRealOrAbsentTests`, `OriginalSensorMatrixTests`, `OriginalActorBroadcastTests` and retiring queued-message/borrow behavior. Coordinate with ActorRuntimeRegistry integration; merely copying wrappers into a new folder is insufficient.

3. **GravityUtil**: restore canonical `Game/Util/GravityUtil.cpp`, delete `GameGravityCompat.cpp/.hpp`, remove forced overload include and xmake exclusion. Queries already use actual PlanetGravityManager, so the alternate utility adds little value. The opaque host pointer is currently truncated to u32 in both wrapper and original manager; native `uintptr_t` must be consistent across declarations, stored identity and comparisons. Test colliding low32-bit pointer identities, priority/filter modes, wire/follower lifecycle, and `GravityRealOrAbsentTests`/`GravityMathFoundationTests`. Ownership integration in GlobalGravityObj/creator/follower can be a subsequent coherent batch; do not move the RTTI destruction switch wholesale.

## Immediate deletions and smaller batches

- `MarioCameraTarget.cpp/.hpp`: no consumers at all. Existing CameraTargetHolder and MarioAccess own the original behavior. Delete without replacement.
- `CameraUtilCompat.cpp/.hpp`: free animation declaration wrapper has no calls. Two tests include the header but call the already-existing CameraSystemService member. Remove the pair and unused includes.
- `StageZoneMatrixRegistry.cpp/.hpp`: only StageZoneMatrixRegistryTests and OriginalCollisionPartsOwnerTests use it; no production use. Replace fixtures with canonical StageDataHolder occurrence/matrix ownership and preserve repeated/empty-zone coverage, then delete.
- `FixedPositionCompat.cpp/.hpp`: extra BCSV loader is test-only. Restore `copyRotate` in FixedPosition and test resource parsing through the actual original initializer/JMapInfo instead.
- `SensorHitCheckerOwnership.cpp` and Binder destructor: native deletion belongs in the corresponding Game class destructor. Do not lose scene borrow invalidation order.
- JMA vector/quaternion providers: integrate into canonical JSystem/JMath. PPC rounding, fused operations and quantized store primitives remain Aurora.

## Important semantic and ownership boundaries

`GameMathCompat.cpp` has independent static `sRandomSeed`; upstream `MR::getRandom` uses `GameSystemObjHolder::mRandom`. Restore the actual random owner, not the duplicate state. Preserve native `bit_cast`, explicit paired-single arithmetic, PPC conversion helpers, LP64 adaptation, alias-safe load order and no-contraction build flags.

ModelUtil and LiveActorUtil are fragmented into many copies. Whole-source restoration must remove every duplicate provider together. Original Mario/Luigi model-name dispatch, collision category names and quirks found in donor are actual game behavior, not invented PC hacks. MapUtil's Water/Shallow/PullBack tests also match current donor; do not replace them based only on different CollisionCode table names.

`HitInfoCompat` and `CollisionPartsCompat` maintain parallel StageCollisionService geometry/attributes and native publication in addition to original collision owners. Synthetic no-parts Triangle creation is test-only. Restore actual Triangle/HitInfo and keep native KCL resource decoding at resource boundaries; remove duplicate service queries and caches after all consumers migrate. Original KCollision needs narrow typed-resource, triangle-count, negative-shift and saturating conversion adaptations. DynamicCollisionObj's host typed KCL allocation cannot be replaced with retail32-bit pointer relocation. CollisionParts all-hit flags need defined initialization because the retail all-hit arrow path does not fill them.

ActorRuntimeRegistry, ModelManagerOwner, MarioAnimatorLifetime, ClippingDirectorOwnership, CollisionDirectorOwnership and GlobalGravityOwnership encode real native cleanup but place ownership outside the Game objects. Replace capture maps with explicit native fields/RAII/destructors in their actual owners. Preserve archive/heap retention, shared animation transform deduplication, partial-init exceptions, borrowed groups/messages, follower shared targets and generation checks until the obsolete native collision service is removed. These are larger coordinated owner changes, not safe file renames.

## Suggested parallel work boundaries

- CameraTargetObj donor restoration (this agent following audit).
- ModelUtil restoration plus all model utility shards (one agent owns complete method set).
- ActorSensorUtil and keeper/destructor integration (one agent), then LiveActorUtil complete-owner batch with registry dependency explicit.
- MathUtil/JMath/MtxUtil restoration (one agent); avoid concurrently modifying ModelUtil/ActorSensorUtil.
- Root coordinates xmake selection, forced Metrowerks include retirement, scene ownership and existing focused test targets.
