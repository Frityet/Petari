# Source-Closeness Audit

Generated: 2026-09-19T16:30:47Z
Repo root: `/Users/frityet/Projects/petari`
PC root: `/Users/frityet/Projects/petari`

## Compatibility-Layer Boundary

Promoted support directories (`src/camera`, `src/layout`, `src/resource`, `src/runtime`, `src/scene`, and selected compatibility files in `src/render`) are inventoried as custom compatibility-layer code and are not counted as failed original-source parity. The audited original game-code surface is `src/Game`.

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
| `exact-source` | 1162 | 592 |
| `compile-only` | 244 | 104 |
| `debug-only` | 0 | 0 |
| `compat-temporary` | 277 | 117 |
| `decomp-needed` | 7 | 4 |

Audited original game-code files: 1690
Target surface files: 817
Compatibility-layer files inventoried separately: 213
Decomp-needed files with root declaration counterparts: 5
Compile-only files requiring allowlist entries: 244

## Release Boundary

Files with guarded debug probes or release-facing observer candidates: 28

- `Camera/CameraTargetObj.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `LiveActor/ShadowVolumeDrawer.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
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
- `Scene/GameScene.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Scene/SceneNameObjMovementController.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Scene/SceneObjHolder.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=3
- `Scene/SceneObjHolder.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `Screen/InformationObserver.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=60
- `Screen/InformationObserver.hpp`: unguarded_smgpc=0, unguarded_observer_candidates=6
- `Screen/StarPointerController.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=1
- `System/StarPointerOnOffController.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=2
- `Util/EventUtil.cpp`: unguarded_smgpc=0, unguarded_observer_candidates=11

## Required Migration

Files requiring migration or decomp work: 284

- `Camera/CameraParamChunk.hpp`: `compat-temporary`
- `LiveActor/ActorCameraInfo.cpp`: `compat-temporary`
- `LiveActor/ActorLightCtrl.cpp`: `compat-temporary`
- `LiveActor/ActorLightCtrl.hpp`: `compat-temporary`
- `LiveActor/ActorPadAndCameraCtrl.cpp`: `decomp-needed`
- `LiveActor/ActorPadAndCameraCtrl.hpp`: `compat-temporary`
- `LiveActor/Binder.cpp`: `compat-temporary`
- `LiveActor/Binder.hpp`: `compat-temporary`
- `LiveActor/DisplayListMaker.cpp`: `compat-temporary`
- `LiveActor/DisplayListMaker.hpp`: `compat-temporary`
- `LiveActor/LiveActor.cpp`: `compat-temporary`
- `LiveActor/LiveActor.hpp`: `compat-temporary`
- `LiveActor/LodCtrl.cpp`: `compat-temporary`
- `LiveActor/MaterialCtrl.cpp`: `compat-temporary`
- `LiveActor/MaterialCtrl.hpp`: `compat-temporary`
- `LiveActor/ModelManager.cpp`: `compat-temporary`
- `LiveActor/ModelObj.cpp`: `compat-temporary`
- `LiveActor/ModelObj.hpp`: `compat-temporary`
- `LiveActor/Nerve.cpp`: `compat-temporary`
- `LiveActor/Nerve.hpp`: `compat-temporary`
- `LiveActor/RailRider.cpp`: `compat-temporary`
- `LiveActor/SensorHitChecker.cpp`: `compat-temporary`
- `Map/FileSelectFunc.cpp`: `compat-temporary`
- `Map/FileSelectItem.cpp`: `compat-temporary`
- `Map/FileSelectItem.hpp`: `compat-temporary`
- `Map/FileSelectModel.hpp`: `compat-temporary`
- `Map/FileSelector.cpp`: `compat-temporary`
- `NPC/MiiFacePartsHolder.cpp`: `compat-temporary`
- `NameObj/NameObjCategoryList.hpp`: `compat-temporary`
- `NameObj/NameObjGroup.hpp`: `compat-temporary`
- `NameObj/NameObjHolder.hpp`: `compat-temporary`
- `Scene/GameScene.cpp`: `compat-temporary`
- `Scene/GameScene.hpp`: `compat-temporary`
- `Scene/GameScenePauseControl.cpp`: `compat-temporary`
- `Scene/LogoScene.cpp`: `compat-temporary`
- `Scene/SceneFunction.hpp`: `compat-temporary`
- `Scene/SceneNameObjListExecutor.cpp`: `compat-temporary`
- `Scene/SceneNameObjMovementController.hpp`: `compat-temporary`
- `Scene/StageDataHolder.cpp`: `compat-temporary`
- `Scene/StageDataHolder.hpp`: `compat-temporary`
- `Screen/CaptureScreenDirector.cpp`: `compat-temporary`
- `Screen/CaptureScreenDirector.hpp`: `compat-temporary`
- `Screen/EncouragePal60Window.cpp`: `compat-temporary`
- `Screen/EncouragePal60Window.hpp`: `compat-temporary`
- `Screen/FileSelectInfo.cpp`: `compat-temporary`
- `Screen/LayoutActor.hpp`: `compat-temporary`
- `Screen/LayoutActorFlag.hpp`: `compat-temporary`
- `Screen/LayoutManager.cpp`: `compat-temporary`
- `Screen/Manual2P.cpp`: `compat-temporary`
- `Screen/Manual2P.hpp`: `compat-temporary`
- `Screen/MiiSelect.cpp`: `compat-temporary`
- `Screen/MiiSelect.hpp`: `compat-temporary`
- `Screen/SaveIcon.cpp`: `compat-temporary`
- `Screen/SaveIcon.hpp`: `compat-temporary`
- `Screen/YesNoController.cpp`: `compat-temporary`
- `Screen/YesNoController.hpp`: `compat-temporary`
- `System/AlreadyDoneFlagInGalaxy.cpp`: `compat-temporary`
- `System/AlreadyDoneFlagInGalaxy.hpp`: `compat-temporary`
- `System/AudSystemWrapper.hpp`: `compat-temporary`
- `System/FileRipper.cpp`: `compat-temporary`
- `System/FunctionAsyncExecutor.cpp`: `compat-temporary`
- `System/GalaxyIDBCSV.cpp`: `decomp-needed`
- `System/GalaxyNameSortTable.cpp`: `compat-temporary`
- `System/GameDataConst.hpp`: `compat-temporary`
- `System/GameEventFlagChecker.cpp`: `compat-temporary`
- `System/GameEventFlagTable.cpp`: `compat-temporary`
- `System/GameSystem.cpp`: `compat-temporary`
- `System/GameSystem.hpp`: `compat-temporary`
- `System/GameSystemErrorWatcher.cpp`: `compat-temporary`
- `System/GameSystemException.cpp`: `compat-temporary`
- `System/GameSystemFunction.cpp`: `compat-temporary`
- `System/GameSystemFunction.hpp`: `compat-temporary`
- `System/GameSystemObjHolder.cpp`: `compat-temporary`
- `System/GameSystemResetAndPowerProcess.cpp`: `compat-temporary`
- `System/GameSystemStationedArchiveLoader.cpp`: `compat-temporary`
- `System/HeapMemoryWatcher.cpp`: `compat-temporary`
- `System/HeapMemoryWatcher.hpp`: `compat-temporary`
- `System/LayoutHolder.cpp`: `compat-temporary`
- `System/MainLoopFramework.cpp`: `compat-temporary`
- `System/MessageHolder.cpp`: `compat-temporary`
- `System/MessageHolder.hpp`: `compat-temporary`
- `System/NANDManager.hpp`: `compat-temporary`
- `System/ResourceInfo.cpp`: `compat-temporary`
- `System/SaveDataBannerCreator.cpp`: `compat-temporary`
- `System/ScenarioDataParser.cpp`: `compat-temporary`
- `System/ScenarioDataParser.hpp`: `compat-temporary`
- `System/ShapePacketUserData.cpp`: `decomp-needed`
- `System/ShapePacketUserData.hpp`: `compat-temporary`
- `System/StationedArchiveLoader.hpp`: `compat-temporary`
- `System/StationedFileInfo.cpp`: `compat-temporary`
- `Util/AreaObjUtil.hpp`: `compat-temporary`
- `Util/BaseMatrixFollowTargetHolder.cpp`: `compat-temporary`
- `Util/BaseMatrixFollowTargetHolder.hpp`: `compat-temporary`
- `Util/BothDirList.cpp`: `compat-temporary`
- `Util/CameraUtil.cpp`: `compat-temporary`
- `Util/Color.hpp`: `compat-temporary`
- `Util/DrawUtil.cpp`: `compat-temporary`
- `Util/FileUtil.hpp`: `compat-temporary`
- `Util/Functor.hpp`: `compat-temporary`
- `Util/FurDrawer.cpp`: `compat-temporary`
- `Util/FurShader.cpp`: `compat-temporary`
- `Util/JMapInfo.cpp`: `compat-temporary`
- `Util/JMapInfo.hpp`: `compat-temporary`
- `Util/JMapUtil.cpp`: `compat-temporary`
- `Util/JMapUtil.hpp`: `compat-temporary`
- `Util/LightUtil.hpp`: `compat-temporary`
- `Util/LiveActorUtil.hpp`: `compat-temporary`
- `Util/MapUtil.cpp`: `compat-temporary`
- `Util/MathUtil.cpp`: `compat-temporary`
- `Util/MemoryUtil.hpp`: `compat-temporary`
- `Util/MessageUtil.cpp`: `compat-temporary`
- `Util/ModelUtil.hpp`: `compat-temporary`
- `Util/ObjUtil.cpp`: `compat-temporary`
- `Util/ObjUtil.hpp`: `compat-temporary`
- `Util/SingletonHolder.hpp`: `compat-temporary`
- `Util/StarPointerUtil.hpp`: `compat-temporary`
- `Util/StringUtil.cpp`: `compat-temporary`
- `Util/SystemUtil.cpp`: `compat-temporary`
- `Util/TriangleFilter.cpp`: `decomp-needed`
- `Util/TriggerChecker.cpp`: `compat-temporary`
- `Util/TriggerChecker.hpp`: `compat-temporary`

## Compatibility Inventory

| Group | Files |
| --- | ---: |
| `platform-compat` | 42 |
| `render-gx-j3d-brlyt` | 40 |
| `resource-message-font-texture` | 57 |
| `scene-sequence` | 72 |
| `trace-proof` | 2 |

Detailed artifacts: `source-closeness.tsv`, `required-migration.tsv`, `release-boundary.tsv`, `decomp-declarations.tsv`, `compile-only-allowlist.tsv`, and `compat-inventory.tsv`.
