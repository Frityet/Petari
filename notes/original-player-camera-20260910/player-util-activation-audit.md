# Whole original PlayerUtil activation audit — 2026-09-10

Initial read-only inspection followed `notes/upstream-cleanup-review-20260910/compat-audit.md`; parent subsequently authorized bounded provider removal and native declaration/member alignment. The original PlayerUtil.cpp remains byte-identical to decomp (SHA256 `fc4f5685c2ded2cf5304cb8475953a0f2d29f93147e4ad36fe9d6e1f526286be`). Root owns activation, service-authority migration, complete native build and runtime proof.

## Compilation

`src/Game/xmake.lua` excludes Util/PlayerUtil.cpp. The unmodified TU initially fails native compilation only on two concrete surface differences: missing original `bool MR::findNamePosOnGround(const char*, MtxPtr)` declaration (two uses at340/348), and18 `RushEndInfo::_20` accesses while native had renamed that field `mFlags`.

An isolated temporary copy with those declaration/name differences reconciled compiled0. The published fix restores original `_20` in the real RushEndInfo header, constructor and nine MarioActorRush accesses, and adds the exact decomp ObjUtil.hpp:194 declaration to its matching native header. No original PlayerUtil body was changed and no field alias was retained. All nine isolated native TUs compile0: original PlayerUtil, RushEndInfo, MarioActorRush and the six provider files edited by this agent. Used the existing full LLVM23/arm64/macOS26.5/O2/debug compile recipe from `notes/original-mario-ground-probes-20260910/native-compile.json`, changing only input/output paths. No Xmake or link/runtime run was performed by this agent.

## Duplicate providers

Native `llvm-nm` on the independently compiled full original PlayerUtil object reports **173 MR functions** and127 direct undefined references. Comparing exact mangled symbols with the current built `build/macosx/arm64/debug/libsmg-pc-game.a` finds **57 existing duplicate function providers**, grouped below. This inventory includes overload signatures, so unrelated overloads are not accidentally removed.

### CameraUtilCompat.cpp.o (1)

- `MR::setCameraTargetToPlayer(CameraTargetArg*)`

### MarioCameraAccessCompat.cpp.o (9)

- `MR::getCameraCube()`
- `MR::isPlayerFlying()`
- `MR::isPlayerInBind()`
- `MR::isActorOnPlayer(LiveActor const*)`
- `MR::getPlayerLastMove()`
- `MR::isPlayerInWaterMode()`
- `MR::getPlayerMovementTimer()`
- `MR::isPlayerOnWaterSurface()`
- `MR::getPlayerGroundingPolygon()`

### NPCActorRuntimeCompat.cpp.o (1)

- `MR::checkPlayerSwingTrigger()`

### OriginalCameraOwnerUtil.cpp.o (4)

- `MR::stopPlayerFpView()`
- `MR::isPlayerDisableFpView()`
- `MR::isFpViewChangingFailure()`
- `MR::isPlayerNeedBrakingCamera()`

### OriginalModelMaterialHelpers.cpp.o (1)

- `MR::getPlayerShadowRotate()`

### OriginalPlayerEventUtil.cpp.o (6)

- `MR::setPlayerSpot(float, unsigned int)`
- `MR::startBckPlayerJ(char const*)`
- `MR::startPlayerEvent(char const*)`
- `MR::startSoundPlayerJ(char const*)`
- `MR::startPlayerDownWipe()`
- `MR::requestMovementOnPlayer()`

### OriginalSceneCounterQueries.cpp.o (1)

- `MR::isPlayerSwimming()`

### OriginalScenePredicates.cpp.o (1)

- `MR::isExistMario()`

### PlayerStateCompat.cpp.o (6)

- `MR::isPlayerDead()`
- `MR::onPlayerControl(bool)`
- `MR::isOnGroundPlayer()`
- `MR::offPlayerControl()`
- `MR::isOffPlayerControl()`
- `MR::setPlayerSwingPermission(bool)`

### PlayerUtilCompat.cpp.o (27)

- `MR::hidePlayer()`
- `MR::showPlayer()`
- `MR::getPlayerPos()`
- `MR::setPlayerPos(JGeometry::TVec3<float> const&)`
- `MR::getPlayerUpVec(JGeometry::TVec3<float>*)`
- `MR::startBckPlayer(char const*, char const*)`
- `MR::getPlayerRotate()`
- `MR::incPlayerOxygen(unsigned int)`
- `MR::getPlayerBaseMtx()`
- `MR::getPlayerGravity()`
- `MR::getPlayerSideVec(JGeometry::TVec3<float>*)`
- `MR::setPlayerBaseMtx(float (*) [4])`
- `MR::getPlayerFrontVec(JGeometry::TVec3<float>*)`
- `MR::getPlayerVelocity()`
- `MR::getPlayerCenterPos()`
- `MR::isBckStoppedPlayer()`
- `MR::isPlayerElementMode(int)`
- `MR::calcDistanceToPlayer(JGeometry::TVec3<float> const&)`
- `MR::getBckFrameMaxPlayer(char const*)`
- `MR::isPlayerElementModeBee()`
- `MR::isPlayerElementModeIce()`
- `MR::isPlayerElementModeHopper()`
- `MR::isPlayerElementModeNormal()`
- `MR::isPlayerElementModeTeresa()`
- `MR::initPlayerAfterOpeningDemo()`
- `MR::isPlayerElementModeTornado()`
- `MR::isPlayerElementModeInvincible()`

This agent removed19 duplicates from MarioCameraAccessCompat, OriginalPlayerEventUtil, OriginalScenePredicates, OriginalSceneCounterQueries, OriginalModelMaterialHelpers and NPCActorRuntimeCompat. MarioCameraAccessCompat now retains only the distinct native `MR::isOnPlayer(const LiveActor*)` overload delegating to original isActorOnPlayer; the original TU instead defines isOnPlayer(const HitSensor*). GameData/miss-layout functions in OriginalPlayerEventUtil and nonplayer operations in other mixed files remain. Parent removes its27 PlayerUtilCompat and6 PlayerStateCompat providers. Depth owns the five overlapping camera-file providers while activating original CameraUtil. `isPlayerGCaptured` is a CameraUtil function, not one of these57; depth owns it separately.

## Undefined dependency boundary

Of127 direct undefined references,80 are absent from the currently linked showcase. Most are already available in compiled original MarioAccess.cpp/MarioActor.cpp or existing generalized service providers and were merely stripped from the previous executable. This is not an eighty-function implementation backlog.

Three direct Game dependencies have no definition in the current Game archive:

| Missing function | PlayerUtil caller | Available original source / required owner |
| --- | --- | --- |
| `MR::findNamePosOnGround(const char*, MtxPtr)` | setPlayerPosOnGround, setPlayerPosOnGroundAndWait | decomp ObjUtil.cpp:882 remains comment-only; declaration exists at ObjUtil.hpp:194. Needs reference recovery of real named-position/ground query when activated. |
| `MR::resetChasingStarPiece()` | resetPlayerEffect | decomp StarPieceDirector.cpp:36 sets the actual scene StarPieceDirector::mResetChasingStarPiece flag true; requires that original owner, not a host-only clear. |
| `MR::stopSound(const LiveActor*,const char*,u32)` | stopSoundPlayer | original SoundUtil.cpp:173 resolves soundID through AudSoundNameConverter then calls existing ID overload. Converter/original sound ownership is the closure boundary. |

The remaining archive-absent imports are ordinary libc/stack-protector symbols. Unused PlayerUtil functions can be removed by the existing dead-strip link. Enabling the TU still requires an actual showcase link: object-level existence cannot prove all newly retained transitive paths. In particular migrated show/hide, setTrans/setBaseMtx, control resets and opening-demo completion now enter real MarioAccess/MarioActor behavior that earlier wrappers bypassed. Some paths were not in the former executable even though their complete source exists.

## Ownership and coherent integration

1. Enable the whole original TU together with removal of all57 overlaps. The173 original functions then have one intended provider.
2. Keep actual scene MarioHolder/MarioActor as authority. Original MarioAccess.cpp:635 gets the actual SceneObj MarioHolder actor; the live scene already creates that owner. PlayerUtil's isExistMario checks holder presence. Queries which require an actor must not be exercised on a generic synthetic LiveActor override.
3. Parent is migrating host control/visibility/swing consumers to original MR calls and removing stale host control flags/reset-request queue. Renderer/diagnostic snapshots may observe original state but must not override it. Existing generic PlayerActorBridge/service tests need separation from the MR contract; they do not prove Mario semantics.
4. Validate real owner state and timing: off/on control changes MarioActor::_3C0, onControl(true) resets immediately; setPlayerPos calls original setTrans and keeps actor/internal/camera/sensor state coherent; velocity and gravity return original storage; grounded state comes from current Mario movement or the actual rush host. Run the existing idle/WASD/two-jump/landing replay plus clean teardown after activation.

Existing source evidence for the authority differences is in `compat-audit.md`. This audit did not add gameplay guards, change original movement algorithms, or supply missing-owner substitutes. The new declarations and field spelling are compile alignment only.

## Edited source hashes at freeze

- `src/Game/Player/RushEndInfo.hpp`: `115afef439622c8027bb8bdc3e18be5f4746e2a35a8d255dfc1fffb2b1292e5a`
- `src/Game/Player/RushEndInfo.cpp`: `9a5d4b9528baf1c3e4a8046a99ec1f4f2c90381bed52dd923b10430373788ca8`
- `src/Game/Player/MarioActorRush.cpp`: `fa3f0a6e2370ee090f058527e4706baceb4c39d7e79c56b7272be53a0c82d2b5`
- `src/Game/Util/ObjUtil.hpp`: `7032ce1825a251dcd4b6d3bc299bda7e57d10389c9313cf1d2d58f9080dc716e`
- `src/compat/MarioCameraAccessCompat.cpp`: `0c1f7f89d2772aa284a8b83c12f7ab12bbe0075e31f94a0ec46fd4f8c5542b06`
- `src/compat/OriginalPlayerEventUtil.cpp`: `07369cf72d988b25f4746f85a801237cdf1e17613951eded39b606856bbb8b45`
- `src/compat/OriginalScenePredicates.cpp`: `48f7127638defb3925caa4fa015f001480cca5225d90ed71429c8efdea86e038`
- `src/compat/OriginalSceneCounterQueries.cpp`: `48e5c11c686706b1e334cd3ca33f72044f31c6ecc422f45c322bad63c300f7fa`
- `src/compat/OriginalModelMaterialHelpers.cpp`: `3a1976689f047523fcea4fcdbb3b02ced1bd52657951b5e5bcd478721890feb6`
- `src/compat/NPCActorRuntimeCompat.cpp`: `3ffc069bcfa18b7c80c1b8ff7536ca9600d39b1ccdb3e03107361f0d475af88c`
