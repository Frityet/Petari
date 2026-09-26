# Source-Closeness Audit

Generated: 2026-09-26T04:04:01Z
Repo root: `/Users/frityet/Projects/petari`
PC root: `/Users/frityet/Projects/petari`

## Compatibility-Layer Boundary

The inventory includes `src/compat`, Aurora public headers and library sources, and host camera/layout/render/resource/runtime/scene code. Directory groups describe locations, not verified compatibility contracts. Relocated original bodies require explicit provenance checks; unreviewed symbols and excluded original units remain visible in the provider reports. `src/Game` is also compared file by file to decomp.

## Classification Policy

- `exact-source`: byte-for-byte match with root `src/Game` or `include/Game`.
- `compile-only`: only explicit CP932 literal wrappers/the encoding header include, line endings, trailing whitespace, or warning-only unused parameter/local annotations differ.
- `debug-only`: the diff disappears after guarded `NDEBUG` debug blocks are stripped.
- `compat-temporary`: root source exists, but the PC file has non-debug behavioral or API-shape differences.
- `decomp-needed`: no root source/header counterpart exists at the expected path.

This is intentionally conservative. Anything not proven exact, compile-only, or debug-only is migration work.

## Counts

| Classification | All audited Game files | Target surface files |
| --- | ---: | ---: |
| `exact-source` | 1715 | 406 |
| `compile-only` | 375 | 59 |
| `debug-only` | 0 | 0 |
| `compat-temporary` | 1120 | 417 |
| `decomp-needed` | 1 | 1 |

Audited original game-code files: 3211
Target surface files: 883
Compatibility-layer files inventoried separately: 508
Decomp-needed files with root declaration counterparts: 0
Compile-only files requiring allowlist entries: 375

## Release Boundary

Files with guarded debug probes or release-facing observer candidates: 36

- `AudioLib/AudUtil.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `AudioLib/AudUtil.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `AudioLib/OverwriteJAudio.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Camera/CameraTargetObj.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Camera/CameraTargetObj.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `LiveActor/ShadowVolumeDrawer.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Map/KoopaBattleMapStair.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `MapObj/ClipArea.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `MapObj/FlipPanel.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=24
- `MapObj/FlipPanel.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=3
- `NameObj/NameObjFactory.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Player/Mario.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=4
- `Player/Mario.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Player/MarioActor.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Player/MarioActor.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Player/MarioActorRushMsg.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Player/MarioActorTakeMsg.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Player/MarioConst.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Player/MarioConst.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Player/MarioEnforce.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Player/MarioFpView.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Player/MarioPress.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Player/MarioWarp.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=4
- `Ride/SwingRope.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Scene/GameScene.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Scene/SceneNameObjMovementController.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Scene/SceneObjHolder.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=3
- `Scene/SceneObjHolder.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Screen.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Screen/InformationObserver.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=60
- `Screen/InformationObserver.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=6
- `Screen/StarPointerController.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `System/StarPointerOnOffController.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Util/EventUtil.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=11

## Required Migration

Files requiring migration or decomp work: 1121

- `Camera/CamHeliEffector.cpp`: `compat-temporary`
- `Camera/CamHeliEffector.hpp`: `compat-temporary`
- `Camera/CamKarikariEffector.cpp`: `compat-temporary`
- `Camera/CamKarikariEffector.hpp`: `compat-temporary`
- `Camera/CamPoseSphereInterpolator.cpp`: `compat-temporary`
- `Camera/CameraAnim.cpp`: `compat-temporary`
- `Camera/CameraBlackHole.cpp`: `compat-temporary`
- `Camera/CameraCalc.cpp`: `compat-temporary`
- `Camera/CameraContext.cpp`: `compat-temporary`
- `Camera/CameraContext.hpp`: `compat-temporary`
- `Camera/CameraCover.cpp`: `compat-temporary`
- `Camera/CameraCover.hpp`: `compat-temporary`
- `Camera/CameraCubePlanet.cpp`: `compat-temporary`
- `Camera/CameraDPD.cpp`: `compat-temporary`
- `Camera/CameraDPD.hpp`: `compat-temporary`
- `Camera/CameraDirector.cpp`: `compat-temporary`
- `Camera/CameraDirector.hpp`: `compat-temporary`
- `Camera/CameraFollow.cpp`: `compat-temporary`
- `Camera/CameraFrontAndBack.hpp`: `compat-temporary`
- `Camera/CameraInnerCylinder.cpp`: `compat-temporary`
- `Camera/CameraLocalUtil.cpp`: `compat-temporary`
- `Camera/CameraLocalUtil.hpp`: `compat-temporary`
- `Camera/CameraManEvent.cpp`: `compat-temporary`
- `Camera/CameraManPause.hpp`: `compat-temporary`
- `Camera/CameraMedianPlanet.cpp`: `compat-temporary`
- `Camera/CameraMedianTower.cpp`: `compat-temporary`
- `Camera/CameraParamChunk.hpp`: `compat-temporary`
- `Camera/CameraParamChunkHolder.cpp`: `compat-temporary`
- `Camera/CameraRotChecker.cpp`: `compat-temporary`
- `Camera/CameraShakePatternImpl.hpp`: `compat-temporary`
- `Camera/CameraShaker.cpp`: `compat-temporary`
- `Camera/CameraSpiral.cpp`: `compat-temporary`
- `Camera/CameraTargetArg.cpp`: `compat-temporary`
- `Camera/CameraTargetHolder.hpp`: `compat-temporary`
- `Camera/CameraTargetMtx.hpp`: `compat-temporary`
- `Camera/CameraTargetObj.hpp`: `compat-temporary`
- `Camera/CameraTower.cpp`: `compat-temporary`
- `Camera/CameraViewInterpolator.cpp`: `compat-temporary`
- `Camera/CameraViewInterpolator.hpp`: `compat-temporary`
- `Camera/DotCamParams.cpp`: `compat-temporary`
- `Camera/DotCamParams.hpp`: `compat-temporary`
- `Camera/GameCameraCreator.hpp`: `compat-temporary`
- `Demo/PrologueDirector.cpp`: `compat-temporary`
- `LiveActor/ActorAnimKeeper.cpp`: `compat-temporary`
- `LiveActor/ActorAnimKeeper.hpp`: `compat-temporary`
- `LiveActor/ActorCameraInfo.cpp`: `compat-temporary`
- `LiveActor/ActorLightCtrl.cpp`: `compat-temporary`
- `LiveActor/ActorLightCtrl.hpp`: `compat-temporary`
- `LiveActor/ActorPadAndCameraCtrl.cpp`: `compat-temporary`
- `LiveActor/ActorPadAndCameraCtrl.hpp`: `compat-temporary`
- `LiveActor/Binder.cpp`: `compat-temporary`
- `LiveActor/Binder.hpp`: `compat-temporary`
- `LiveActor/ClippingActorHolder.cpp`: `compat-temporary`
- `LiveActor/ClippingActorHolder.hpp`: `compat-temporary`
- `LiveActor/ClippingActorInfo.cpp`: `compat-temporary`
- `LiveActor/ClippingActorInfo.hpp`: `compat-temporary`
- `LiveActor/ClippingDirector.cpp`: `compat-temporary`
- `LiveActor/ClippingGroupHolder.cpp`: `compat-temporary`
- `LiveActor/ClippingGroupHolder.hpp`: `compat-temporary`
- `LiveActor/ClippingJudge.cpp`: `compat-temporary`
- `LiveActor/ClippingJudge.hpp`: `compat-temporary`
- `LiveActor/DisplayListMaker.cpp`: `compat-temporary`
- `LiveActor/DynamicJointCtrl.cpp`: `compat-temporary`
- `LiveActor/DynamicJointCtrl.hpp`: `compat-temporary`
- `LiveActor/EffectKeeper.cpp`: `compat-temporary`
- `LiveActor/EffectKeeper.hpp`: `compat-temporary`
- `LiveActor/FlashingCtrl.cpp`: `compat-temporary`
- `LiveActor/HitSensor.cpp`: `compat-temporary`
- `LiveActor/HitSensor.hpp`: `compat-temporary`
- `LiveActor/HitSensorInfo.cpp`: `compat-temporary`
- `LiveActor/HitSensorInfo.hpp`: `compat-temporary`
- `LiveActor/HitSensorKeeper.cpp`: `compat-temporary`
- `LiveActor/HitSensorKeeper.hpp`: `compat-temporary`
- `LiveActor/IKJointCtrl.cpp`: `compat-temporary`
- `LiveActor/LiveActor.cpp`: `compat-temporary`
- `LiveActor/LiveActor.hpp`: `compat-temporary`
- `LiveActor/LiveActorGroup.hpp`: `compat-temporary`
- `LiveActor/LiveActorGroupArray.cpp`: `compat-temporary`
- `LiveActor/LiveActorGroupArray.hpp`: `compat-temporary`
- `LiveActor/LodCtrl.cpp`: `compat-temporary`
- `LiveActor/LodCtrl.hpp`: `compat-temporary`
- `LiveActor/MaterialCtrl.cpp`: `compat-temporary`
- `LiveActor/MaterialCtrl.hpp`: `compat-temporary`
- `LiveActor/MirrorCamera.cpp`: `compat-temporary`
- `LiveActor/MirrorCamera.hpp`: `compat-temporary`
- `LiveActor/MirrorReflectionModel.cpp`: `compat-temporary`
- `LiveActor/ModelManager.cpp`: `compat-temporary`
- `LiveActor/ModelManager.hpp`: `compat-temporary`
- `LiveActor/ModelObj.cpp`: `compat-temporary`
- `LiveActor/ModelObj.hpp`: `compat-temporary`
- `LiveActor/Nerve.cpp`: `compat-temporary`
- `LiveActor/Nerve.hpp`: `compat-temporary`
- `LiveActor/PartsModel.cpp`: `compat-temporary`
- `LiveActor/RailRider.cpp`: `compat-temporary`
- `LiveActor/RailRider.hpp`: `compat-temporary`
- `LiveActor/SensorHitChecker.cpp`: `compat-temporary`
- `LiveActor/SensorHitChecker.hpp`: `compat-temporary`
- `LiveActor/ShadowController.cpp`: `compat-temporary`
- `LiveActor/ShadowController.hpp`: `compat-temporary`
- `LiveActor/ShadowDrawer.cpp`: `compat-temporary`
- `LiveActor/ShadowDrawer.hpp`: `compat-temporary`
- `LiveActor/ShadowSurfaceBox.cpp`: `compat-temporary`
- `LiveActor/ShadowSurfaceCircle.cpp`: `compat-temporary`
- `LiveActor/ShadowSurfaceCircle.hpp`: `compat-temporary`
- `LiveActor/ShadowSurfaceDrawer.cpp`: `compat-temporary`
- `LiveActor/ShadowSurfaceDrawer.hpp`: `compat-temporary`
- `LiveActor/ShadowSurfaceOval.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeBox.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeCylinder.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeCylinder.hpp`: `compat-temporary`
- `LiveActor/ShadowVolumeDrawer.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeDrawer.hpp`: `compat-temporary`
- `LiveActor/ShadowVolumeFlatModel.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeFlatModel.hpp`: `compat-temporary`
- `LiveActor/ShadowVolumeLine.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeLine.hpp`: `compat-temporary`
- `LiveActor/ShadowVolumeModel.hpp`: `compat-temporary`
- `LiveActor/ShadowVolumeOval.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeOval.hpp`: `compat-temporary`
- `LiveActor/ShadowVolumeOvalPole.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeOvalPole.hpp`: `compat-temporary`
- `LiveActor/ShadowVolumeSphere.cpp`: `compat-temporary`
- `LiveActor/ShadowVolumeSphere.hpp`: `compat-temporary`
- `LiveActor/SimpleJ3DModelDrawer.cpp`: `compat-temporary`
- `LiveActor/ViewGroupCtrl.cpp`: `compat-temporary`
- `LiveActor/ViewGroupCtrl.hpp`: `compat-temporary`
- `LiveActor/VolumeModelDrawer.cpp`: `compat-temporary`
- `Map/FileSelectCameraController.cpp`: `compat-temporary`
- `Map/FileSelectEffect.cpp`: `compat-temporary`
- `Map/FileSelectFunc.cpp`: `compat-temporary`
- `Map/FileSelectFunc.hpp`: `compat-temporary`
- `Map/FileSelectItem.cpp`: `compat-temporary`
- `Map/FileSelectItem.hpp`: `compat-temporary`
- `Map/FileSelectModel.cpp`: `compat-temporary`
- `Map/FileSelectModel.hpp`: `compat-temporary`
- `Map/FileSelectSky.cpp`: `compat-temporary`
- `Map/FileSelector.cpp`: `compat-temporary`
- `Map/FileSelector.hpp`: `compat-temporary`
- `NPC/MiiFacePartsHolder.cpp`: `compat-temporary`
- `NameObj/NameObj.cpp`: `compat-temporary`
- `NameObj/NameObj.hpp`: `compat-temporary`
- `NameObj/NameObjAdaptor.cpp`: `compat-temporary`
- `NameObj/NameObjCategoryList.cpp`: `compat-temporary`
- `NameObj/NameObjCategoryList.hpp`: `compat-temporary`
- `NameObj/NameObjExecuteHolder.cpp`: `compat-temporary`
- `NameObj/NameObjExecuteHolder.hpp`: `compat-temporary`
- `NameObj/NameObjFactory.cpp`: `compat-temporary`
- `NameObj/NameObjGroup.cpp`: `compat-temporary`
- `NameObj/NameObjGroup.hpp`: `compat-temporary`
- `NameObj/NameObjHolder.cpp`: `compat-temporary`
- `NameObj/NameObjHolder.hpp`: `compat-temporary`
- `NameObj/NameObjListExecutor.cpp`: `compat-temporary`
- `NameObj/NameObjListExecutor.hpp`: `compat-temporary`
- `Scene/GameScene.cpp`: `compat-temporary`
- `Scene/GameScene.hpp`: `compat-temporary`
- `Scene/GameScenePauseControl.cpp`: `compat-temporary`
- `Scene/GameSceneScenarioOpeningCameraState.cpp`: `compat-temporary`
- `Scene/IntermissionScene.cpp`: `compat-temporary`
- `Scene/LogoScene.cpp`: `compat-temporary`
- `Scene/MultiSceneEffectKeeper.cpp`: `compat-temporary`
- `Scene/PlacementInfoOrdered.cpp`: `compat-temporary`
- `Scene/PlacementInfoOrdered.hpp`: `compat-temporary`
- `Scene/PlayTimerScene.cpp`: `compat-temporary`
- `Scene/ScenarioSelectScene.cpp`: `compat-temporary`
- `Scene/ScenarioSelectScene.hpp`: `compat-temporary`
- `Scene/Scene.cpp`: `compat-temporary`
- `Scene/Scene.hpp`: `compat-temporary`
- `Scene/SceneFactory.cpp`: `compat-temporary`
- `Scene/SceneFunction.cpp`: `compat-temporary`
- `Scene/SceneFunction.hpp`: `compat-temporary`
- `Scene/SceneNameObjListExecutor.cpp`: `compat-temporary`
- `Scene/SceneNameObjMovementController.cpp`: `compat-temporary`
- `Scene/SceneNameObjMovementController.hpp`: `compat-temporary`
- `Scene/SceneObjHolder.cpp`: `compat-temporary`
- `Scene/SceneObjHolder.hpp`: `compat-temporary`
- `Scene/StageDataHolder.cpp`: `compat-temporary`
- `Scene/StageDataHolder.hpp`: `compat-temporary`
- `Scene/StageFileLoader.cpp`: `compat-temporary`
- `Scene/StopSceneController.cpp`: `compat-temporary`
- `Screen/BrosButton.cpp`: `compat-temporary`
- `Screen/ButtonPaneController.cpp`: `compat-temporary`
- `Screen/CaptureScreenDirector.cpp`: `compat-temporary`
- `Screen/CaptureScreenDirector.hpp`: `compat-temporary`
- `Screen/FileSelectButton.cpp`: `compat-temporary`
- `Screen/FileSelectInfo.cpp`: `compat-temporary`
- `Screen/FileSelectNumber.cpp`: `compat-temporary`
- `Screen/IconAButton.cpp`: `compat-temporary`
- `Screen/InformationMessage.cpp`: `compat-temporary`
- `Screen/LayoutActor.cpp`: `compat-temporary`
- `Screen/LayoutActor.hpp`: `compat-temporary`
- `Screen/LayoutActorFlag.hpp`: `compat-temporary`
- `Screen/LayoutManager.cpp`: `compat-temporary`
- `Screen/LayoutManager.hpp`: `compat-temporary`
- `Screen/LayoutPaneCtrl.cpp`: `compat-temporary`
- `Screen/LayoutPaneCtrl.hpp`: `compat-temporary`
- `Screen/Manual2P.cpp`: `compat-temporary`
- `Screen/Manual2P.hpp`: `compat-temporary`
- `Screen/MiiConfirmIcon.cpp`: `compat-temporary`
- `Screen/MiiSelect.cpp`: `compat-temporary`
- `Screen/PictureBookCloseButton.cpp`: `compat-temporary`
- `Screen/PictureBookLayout.cpp`: `compat-temporary`
- `Screen/PrologueLetter.cpp`: `compat-temporary`
- `Screen/ProloguePictureBook.cpp`: `compat-temporary`
- `Screen/ReplaceTagProcessor.cpp`: `compat-temporary`
- `Screen/SaveIcon.cpp`: `compat-temporary`
- `Screen/SaveIcon.hpp`: `compat-temporary`
- `Screen/ScreenAlphaCapture.cpp`: `compat-temporary`
- `Screen/SysInfoWindow.cpp`: `compat-temporary`
- `Screen/TitleSequenceProduct.cpp`: `compat-temporary`
- `Screen/YesNoController.cpp`: `compat-temporary`
- `Screen/YesNoController.hpp`: `compat-temporary`
- `System/AlreadyDoneFlagInGalaxy.cpp`: `compat-temporary`
- `System/AlreadyDoneFlagInGalaxy.hpp`: `compat-temporary`
- `System/ArchiveHolder.cpp`: `compat-temporary`
- `System/ArchiveHolder.hpp`: `compat-temporary`
- `System/AudSystemWrapper.cpp`: `compat-temporary`
- `System/AudSystemWrapper.hpp`: `compat-temporary`
- `System/BinaryDataChunkHolder.hpp`: `compat-temporary`
- `System/BinaryDataContentAccessor.cpp`: `compat-temporary`
- `System/BinaryDataContentAccessor.hpp`: `compat-temporary`
- `System/ConfigDataMisc.cpp`: `compat-temporary`
- `System/DrawBuffer.cpp`: `compat-temporary`
- `System/DrawBuffer.hpp`: `compat-temporary`
- `System/DrawBufferExecuter.cpp`: `compat-temporary`
- `System/DrawBufferExecuter.hpp`: `compat-temporary`
- `System/DrawBufferGroup.cpp`: `compat-temporary`
- `System/DrawBufferGroup.hpp`: `compat-temporary`
- `System/DrawBufferHolder.cpp`: `compat-temporary`
- `System/DrawBufferHolder.hpp`: `compat-temporary`
- `System/DrawSyncManager.cpp`: `compat-temporary`
- `System/DrawSyncManager.hpp`: `compat-temporary`
- `System/FileHolder.cpp`: `compat-temporary`
- `System/FileLoader.cpp`: `compat-temporary`
- `System/FileLoader.hpp`: `compat-temporary`
- `System/FileRipper.cpp`: `compat-temporary`
- `System/FunctionAsyncExecutor.cpp`: `compat-temporary`
- `System/FunctionAsyncExecutor.hpp`: `compat-temporary`
- `System/GalaxyCometState.cpp`: `compat-temporary`
- `System/GalaxyIDBCSV.cpp`: `decomp-needed`
- `System/GalaxyNameSortTable.cpp`: `compat-temporary`
- `System/GalaxyStatusAccessor.hpp`: `compat-temporary`
- `System/GameDataConst.hpp`: `compat-temporary`
- `System/GameDataFunction.cpp`: `compat-temporary`
- `System/GameDataGalaxyStorage.cpp`: `compat-temporary`
- `System/GameDataGalaxyStorage.hpp`: `compat-temporary`
- `System/GameDataHolder.cpp`: `compat-temporary`
- `System/GameDataPlayerStatus.cpp`: `compat-temporary`
- `System/GameEventFlag.hpp`: `compat-temporary`
- `System/GameEventFlagChecker.cpp`: `compat-temporary`
- `System/GameEventFlagStorage.cpp`: `compat-temporary`
- `System/GameEventFlagStorage.hpp`: `compat-temporary`
- `System/GameEventFlagTable.cpp`: `compat-temporary`
- `System/GameEventValueChecker.cpp`: `compat-temporary`
- `System/GameEventValueChecker.hpp`: `compat-temporary`
- `System/GameSequenceDirector.cpp`: `compat-temporary`
- `System/GameSequenceFunction.cpp`: `compat-temporary`
- `System/GameSequenceProgress.cpp`: `compat-temporary`
- `System/GameSequenceProgress.hpp`: `compat-temporary`
- `System/GameSystem.cpp`: `compat-temporary`
- `System/GameSystem.hpp`: `compat-temporary`
- `System/GameSystemErrorWatcher.cpp`: `compat-temporary`
- `System/GameSystemException.cpp`: `compat-temporary`
- `System/GameSystemFontHolder.cpp`: `compat-temporary`
- `System/GameSystemFunction.cpp`: `compat-temporary`
- `System/GameSystemFunction.hpp`: `compat-temporary`
- `System/GameSystemObjHolder.cpp`: `compat-temporary`
- `System/GameSystemResetAndPowerProcess.cpp`: `compat-temporary`
- `System/GameSystemResetAndPowerProcess.hpp`: `compat-temporary`
- `System/GameSystemSceneController.cpp`: `compat-temporary`
- `System/GameSystemStationedArchiveLoader.cpp`: `compat-temporary`
- `System/GameSystemStationedArchiveLoader.hpp`: `compat-temporary`
- `System/HeapMemoryWatcher.cpp`: `compat-temporary`
- `System/HeapMemoryWatcher.hpp`: `compat-temporary`
- `System/HomeButtonMenuWrapper.cpp`: `compat-temporary`
- `System/LayoutHolder.cpp`: `compat-temporary`
- `System/LayoutHolder.hpp`: `compat-temporary`
- `System/MainLoopFramework.cpp`: `compat-temporary`
- `System/MainLoopFramework.hpp`: `compat-temporary`
- `System/MessageHolder.cpp`: `compat-temporary`
- `System/MessageHolder.hpp`: `compat-temporary`
- `System/NANDErrorSequence.cpp`: `compat-temporary`
- `System/NANDManager.cpp`: `compat-temporary`
- `System/NANDManager.hpp`: `compat-temporary`
- `System/Overwrite.cpp`: `compat-temporary`
- `System/ResourceHolder.cpp`: `compat-temporary`
- `System/ResourceHolder.hpp`: `compat-temporary`
- `System/ResourceHolderManager.cpp`: `compat-temporary`
- `System/ResourceHolderManager.hpp`: `compat-temporary`
- `System/ResourceInfo.cpp`: `compat-temporary`
- `System/ResourceInfo.hpp`: `compat-temporary`
- `System/SaveDataBannerCreator.cpp`: `compat-temporary`
- `System/SaveDataFileAccessor.cpp`: `compat-temporary`
- `System/SaveDataHandleSequence.cpp`: `compat-temporary`
- `System/SaveDataHandler.cpp`: `compat-temporary`
- `System/ScenarioDataParser.cpp`: `compat-temporary`
- `System/ScenarioDataParser.hpp`: `compat-temporary`
- `System/ShapePacketUserData.cpp`: `compat-temporary`
- `System/ShapePacketUserData.hpp`: `compat-temporary`
- `System/SpinDriverPathStorage.cpp`: `compat-temporary`
- `System/SpinDriverPathStorage.hpp`: `compat-temporary`
- `System/StarPieceAlmsStorage.cpp`: `compat-temporary`
- `System/StarPieceAlmsStorage.hpp`: `compat-temporary`
- `System/StationedArchiveLoader.cpp`: `compat-temporary`
- `System/StationedArchiveLoader.hpp`: `compat-temporary`
- `System/StationedFileInfo.cpp`: `compat-temporary`
- `System/StationedFileInfo.hpp`: `compat-temporary`
- `System/StorySequenceExecutor.cpp`: `compat-temporary`
- `System/SysConfigFile.cpp`: `compat-temporary`
- `System/SysConfigFile.hpp`: `compat-temporary`
- `System/WPad.cpp`: `compat-temporary`
- `System/WPad.hpp`: `compat-temporary`
- `System/WPadAcceleration.cpp`: `compat-temporary`
- `System/WPadAcceleration.hpp`: `compat-temporary`
- `System/WPadHVSwing.cpp`: `compat-temporary`
- `System/WPadHVSwing.hpp`: `compat-temporary`
- `System/WPadHolder.cpp`: `compat-temporary`
- `System/WPadHolder.hpp`: `compat-temporary`
- `System/WPadLeaveWatcher.cpp`: `compat-temporary`
- `System/WPadPointer.cpp`: `compat-temporary`
- `System/WPadPointer.hpp`: `compat-temporary`
- `System/WPadRumble.cpp`: `compat-temporary`
- `System/WPadStick.cpp`: `compat-temporary`
- `Util/ActorMovementUtil.cpp`: `compat-temporary`
- `Util/ActorMovementUtil.hpp`: `compat-temporary`
- `Util/ActorShadowLocalUtil.cpp`: `compat-temporary`
- `Util/ActorShadowUtil.cpp`: `compat-temporary`
- `Util/AreaObjUtil.cpp`: `compat-temporary`
- `Util/AreaObjUtil.hpp`: `compat-temporary`
- `Util/Array.hpp`: `compat-temporary`
- `Util/BaseMatrixFollowTargetHolder.cpp`: `compat-temporary`
- `Util/BaseMatrixFollowTargetHolder.hpp`: `compat-temporary`
- `Util/BothDirList.cpp`: `compat-temporary`
- `Util/BothDirList.hpp`: `compat-temporary`
- `Util/CameraUtil.cpp`: `compat-temporary`
- `Util/Color.hpp`: `compat-temporary`
- `Util/DemoUtil.cpp`: `compat-temporary`
- `Util/DirectDraw.cpp`: `compat-temporary`
- `Util/DirectDraw.hpp`: `compat-temporary`
- `Util/DirectDrawUtil.cpp`: `compat-temporary`
- `Util/DrawUtil.cpp`: `compat-temporary`
- `Util/EffectUtil.cpp`: `compat-temporary`
- `Util/EventUtil.cpp`: `compat-temporary`
- `Util/EventUtil.hpp`: `compat-temporary`
- `Util/FileUtil.cpp`: `compat-temporary`
- `Util/FileUtil.hpp`: `compat-temporary`
- `Util/FixedPosition.cpp`: `compat-temporary`
- `Util/FixedPosition.hpp`: `compat-temporary`
- `Util/FootPrint.cpp`: `compat-temporary`
- `Util/Functor.hpp`: `compat-temporary`
- `Util/FurCtrl.cpp`: `compat-temporary`
- `Util/FurCtrl.hpp`: `compat-temporary`
- `Util/FurDrawer.cpp`: `compat-temporary`
- `Util/FurDrawer.hpp`: `compat-temporary`
- `Util/FurMulti.cpp`: `compat-temporary`
- `Util/FurMulti.hpp`: `compat-temporary`
- `Util/FurParam.hpp`: `compat-temporary`
- `Util/FurShader.cpp`: `compat-temporary`
- `Util/FurShader.hpp`: `compat-temporary`
- `Util/GamePadUtil.cpp`: `compat-temporary`
- `Util/GamePadUtil.hpp`: `compat-temporary`
- `Util/GravityUtil.cpp`: `compat-temporary`
- `Util/GravityUtil.hpp`: `compat-temporary`
- `Util/HashUtil.cpp`: `compat-temporary`
- `Util/HashUtil.hpp`: `compat-temporary`
- `Util/JMapIdInfo.hpp`: `compat-temporary`
- `Util/JMapInfo.cpp`: `compat-temporary`
- `Util/JMapInfo.hpp`: `compat-temporary`
- `Util/JMapLinkInfo.cpp`: `compat-temporary`
- `Util/JMapLinkInfo.hpp`: `compat-temporary`
- `Util/JMapUtil.hpp`: `compat-temporary`
- `Util/JointController.cpp`: `compat-temporary`
- `Util/JointController.hpp`: `compat-temporary`
- `Util/JointUtil.hpp`: `compat-temporary`
- `Util/LayoutUtil.cpp`: `compat-temporary`
- `Util/LayoutUtil.hpp`: `compat-temporary`
- `Util/LightUtil.cpp`: `compat-temporary`
- `Util/LightUtil.hpp`: `compat-temporary`
- `Util/LiveActorUtil.cpp`: `compat-temporary`
- `Util/LiveActorUtil.hpp`: `compat-temporary`
- `Util/MapPartsUtil.cpp`: `compat-temporary`
- `Util/MapPartsUtil.hpp`: `compat-temporary`
- `Util/MapUtil.cpp`: `compat-temporary`
- `Util/MapUtil.hpp`: `compat-temporary`
- `Util/MathUtil.cpp`: `compat-temporary`
- `Util/MathUtil.hpp`: `compat-temporary`
- `Util/MemoryUtil.cpp`: `compat-temporary`
- `Util/MemoryUtil.hpp`: `compat-temporary`
- `Util/MessageUtil.cpp`: `compat-temporary`
- `Util/MessageUtil.hpp`: `compat-temporary`
- `Util/ModelUtil.cpp`: `compat-temporary`
- `Util/ModelUtil.hpp`: `compat-temporary`
- `Util/MtxUtil.cpp`: `compat-temporary`
- `Util/MtxUtil.hpp`: `compat-temporary`
- `Util/MutexHolder.hpp`: `compat-temporary`
- `Util/NPCUtil.cpp`: `compat-temporary`
- `Util/NPCUtil.hpp`: `compat-temporary`
- `Util/ObjUtil.cpp`: `compat-temporary`
- `Util/ObjUtil.hpp`: `compat-temporary`
- `Util/ParabolicPath.hpp`: `compat-temporary`
- `Util/PlayerUtil.cpp`: `compat-temporary`
- `Util/RailUtil.cpp`: `compat-temporary`
- `Util/RailUtil.hpp`: `compat-temporary`
- `Util/RumbleCalculator.cpp`: `compat-temporary`
- `Util/RumbleCalculator.hpp`: `compat-temporary`
- `Util/SceneUtil.cpp`: `compat-temporary`
- `Util/SceneUtil.hpp`: `compat-temporary`
- `Util/ScreenUtil.cpp`: `compat-temporary`
- `Util/ScreenUtil.hpp`: `compat-temporary`
- `Util/SequenceUtil.cpp`: `compat-temporary`
- `Util/SingletonHolder.hpp`: `compat-temporary`
- `Util/SoundUtil.cpp`: `compat-temporary`
- `Util/StarPointerUtil.hpp`: `compat-temporary`
- `Util/StringUtil.cpp`: `compat-temporary`
- `Util/SystemUtil.cpp`: `compat-temporary`
- `Util/TalkUtil.cpp`: `compat-temporary`
- `Util/TriggerChecker.cpp`: `compat-temporary`
- `Util/TriggerChecker.hpp`: `compat-temporary`
- `Util/VectorUtil.hpp`: `compat-temporary`

## Compatibility Inventory

| Group | Files |
| --- | ---: |
| `platform-compat` | 414 |
| `render-gx-j3d-brlyt` | 26 |
| `resource-message-font-texture` | 60 |
| `scene-sequence` | 6 |
| `trace-proof` | 2 |

Detailed artifacts: `source-closeness.tsv`, `required-migration.tsv`, `release-boundary.tsv`, `decomp-declarations.tsv`, `compile-only-allowlist.tsv`, and `compat-inventory.tsv`. Provider artifacts include actual configured source/object mappings, strong symbol providers, duplicate providers, excluded original units, and explicit relocated-source checks. See `provider-audit.json` for limitations.
