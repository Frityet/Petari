# Next compat removals after round14

Read-only audit of the remaining tree, excluding ongoing Effect, Gravity, J3D and SceneObj work. Counts below are current source sizes, not runtime validation. No source edits/builds/tests/git operations were performed for this audit.

## 1. Fastest large donor restoration: SoundUtil (801 lines deleted)

Delete `src/compat/OriginalSoundUtil.cpp`; restore/enable complete `src/Game/Util/SoundUtil.cpp` from `decomp/src/Game/Util/SoundUtil.cpp`. The diff is mostly equivalent spelling and call overload fixes. The material native deltas are eight disabled-output dispatches: `limitedSound`, four trigger/level permit functions, `isPermitSE`, `setSoundVolumeSetting`, `recoverSoundVolumeSetting`. Preserve those at the actual owner boundary until the real AudSystem exists. Do not replace them with successful null-owner operations. The currently compiled `AudioFacadeCompat` still supplies AudWrap/AudSystem dependencies, so this is independently bounded.

Build: remove `remove_files("Util/SoundUtil.cpp")` in Game/xmake. Preserve necessary explicit JAISoundID/default-argument spellings; do not retain a second utility TU. This removes a full parallel Game implementation immediately, although it does not remove the disabled backend itself. High confidence; smallest infrastructure cost.

## 2. Next substantial ownership closure: CollisionParts + HitInfo (four compat files, about640 CPP lines)

Candidates: `compat/CollisionPartsCompat.cpp/.hpp` and `compat/HitInfoCompat.cpp/.hpp`. Complete donor `decomp/src/Game/Map/HitInfo.cpp` exists; its native canonical CPP is currently absent. `Game/Map/CollisionParts.cpp` is already canonical and compiled.

Restore the complete HitInfo donor and make `CollisionParts` own its bounded KCL decoding/resource token, generated geometry token, category token from round14, original server/mapInfo and publication state. Actual LiveActor/DynamicCollisionObj ownership must replace the global actor→parts vector; retain all per-actor parts, not just `LiveActor::mCollisionParts` (secondary resource parts exist). Production creation callers are `Game/Util/LiveActorUtil.cpp` and `Game/MapObj/DynamicCollisionObj.cpp`; destruction currently enters through ActorRuntimeRegistry. Diagnostic tracing/tests consume the vector's count/source/resource metadata.

The HitInfo shim currently substitutes `StageCollisionService` surfaces/matrices/attribute caches for actual Triangle methods. Production triangles already carry original parts/prism/sensor identities, and the donor gets names, zones, attributes and matrices directly from those owners. `make_collision_triangle` has no production callers under src outside its definition; migrate or retire its geometry-only test adapter rather than preserve it in Game. Preserve retained-triangle invalidation and publication guards with actual typed ownership before deleting the registry. This is a coherent larger batch, but needs deliberate lifetime work; do not simply paste ActorCollisionPartsState and its map into another namespace. Medium confidence, higher payoff than the tiny query fragments. Keep `-ffp-contract=off` on collision math and add canonical HitInfo through the Game glob. Current CollisionParts/HitInfo/StageCollision files and several query tests are initially dirty.

## 3. Small isolated SDK boundary: NAND (two compat files,274 CPP lines)

`NandSdkCompat.cpp` contains general NAND API implementation plus the misplaced `NANDManager` destructor; `NandSdkBinding.hpp` publishes RuntimeServices::SaveDataService. Move only the actual destructor back to existing `Game/System/NANDManager.cpp`. Implement NAND ownership in Aurora's Dolphin NAND layer with an explicit host storage backend, removing the dependency on Game and RuntimeServices rather than renaming the binding. Preserve worker-stop-before-storage retirement, descriptor access modes, buffered writes/close publication, path bounds, error mapping and banner layout. Requires Aurora target/CMake+xmake wiring and app storage initialization migration. This is real platform work rather than donor Game restoration; medium scope, not a free file move.

## Larger audio removal is a separate project

`AudioFacadeCompat.cpp` (876 lines) replaces AudFader, AudTrackController, AudBgm, AudBgmKeeper, AudBgmMgr, AudBgmRhythmStrategy, AudWrap and some AudSystem methods. Their complete donor owners exist and most native reference TUs are explicitly excluded, but the facade's RuntimeContext BGM handles/lanes are not actual AudSystem ownership. A correct full deletion needs AudSystem/JAU section and SE/sequence manager lifecycle, not only unexcluding the files.

Related `OriginalObjectSoundState.cpp` (213 lines), `DisabledObjectAudio.cpp/.hpp`, `OriginalAudioVolumeController.cpp`, category/limited-sound ownership pairs and the two-ID `JAudioSoundParameterSemantics` policy can disappear when the actual AudSystem and sound-object owners are restored. Existing donors include AudSoundObject, AudAnmSoundObject, all three modifier TUs, JAUSoundObject and JAUSoundAnimator; together those object/animation/modifier owners exceed1900 lines and depend on JAISeMgr/JAISoundInfo/JAISoundStarter and the real audio system. Volume preset state belongs in AudSystem::mSeMgr.mCategoryMgrs; two limited-sound slots belong in AudSystem, rather than a second category/limit model. No recommendation to create an "AudioCompat" replacement bucket or simulate successful voices. Audio is currently optional per user, so prioritize the bounded utility/collision work.

## Low-yield queries and platform code

- `OriginalGCaptureQueries.cpp` is18 lines, but whole-owner removal requires importing GCapture (824 lines) and GCaptureRibbon (126), with original CameraTargetMtx/SpringValue child ownership and a real SceneObj factory entry. All are existing donors. Useful actor expansion; poor cleanup-per-line ratio.
- `OriginalTripodBossQuery.cpp` is59 lines spanning TripodBossAccesser and TripodBoss::getJointMatrix. Complete donors are209 and1206 lines respectively; full boss dependency closure is not a quick utility cleanup.
- `JkrAllocationDomain`, Metrowerks formatting/allocation/string headers, CP932 conversion, NativePcmSound/JaiStreamPlayback and JAS platform providers implement actual platform/architecture boundaries. They need ownership-specific SDK/backend consolidation, not deletion of their behavior or a directory rename. In particular MslPrintf's650-line implementation intentionally remains outside Game's forced formatting aliases to avoid libc recursion.
