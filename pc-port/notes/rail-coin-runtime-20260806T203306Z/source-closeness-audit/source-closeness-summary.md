# Source-Closeness Audit

Generated: 2026-08-06T20:42:03Z
Repo root: `/workspaces/pcport/pc-port/..`
PC root: `/workspaces/pcport/pc-port/../pc-port`

## Compatibility-Layer Boundary

Promoted support directories (`pc-port/src/camera`, `pc-port/src/layout`, `pc-port/src/resource`, `pc-port/src/runtime`, `pc-port/src/scene`, and selected compatibility files in `pc-port/src/render`) are inventoried as custom compatibility-layer code and are not counted as failed original-source parity. The audited original game-code surface is `pc-port/src/Game`.

## Classification Policy

- `exact-source`: byte-for-byte match with root `src/Game` or `include/Game`.
- `compile-only`: only line endings, trailing whitespace, or warning-only unused parameter/local annotations differ.
- `debug-only`: the diff disappears after guarded `NDEBUG` debug blocks are stripped.
- `compat-temporary`: root source exists, but the PC file has non-debug behavioral or API-shape differences.
- `decomp-needed`: no root source/header counterpart exists at the expected path.

This is intentionally conservative. Anything not proven exact, compile-only, or debug-only is migration work.

## Counts

| Classification | All audited Game files | Target surface files |
| --- | ---: | ---: |
| `exact-source` | 54 | 23 |
| `compile-only` | 8 | 4 |
| `debug-only` | 0 | 0 |
| `compat-temporary` | 198 | 184 |
| `decomp-needed` | 2 | 2 |

Audited original game-code files: 262
Target surface files: 213
Compatibility-layer files inventoried separately: 85
Decomp-needed files with root declaration counterparts: 2
Compile-only files requiring allowlist entries: 8

## Release Boundary

Files with guarded debug probes or release-facing observer candidates: 19

- `Screen/SimpleLayout.cpp`: unguarded_smgpc=1, unguarded_observer_candidates=0

## Required Migration

Files requiring migration or decomp work: 200

- `Camera/CameraTargetArg.cpp`: `compat-temporary`
- `Camera/CameraTargetArg.hpp`: `compat-temporary`
- `Camera/CameraTargetMtx.cpp`: `compat-temporary`
- `Camera/CameraTargetMtx.hpp`: `compat-temporary`
- `Demo/PrologueDirector.cpp`: `compat-temporary`
- `Demo/PrologueDirector.hpp`: `compat-temporary`
- `LiveActor/ActorCameraInfo.cpp`: `compat-temporary`
- `LiveActor/ActorCameraInfo.hpp`: `compat-temporary`
- `LiveActor/ActorLightCtrl.cpp`: `compat-temporary`
- `LiveActor/ActorLightCtrl.hpp`: `compat-temporary`
- `LiveActor/ActorStateKeeper.hpp`: `compat-temporary`
- `LiveActor/HitSensor.cpp`: `compat-temporary`
- `LiveActor/HitSensor.hpp`: `compat-temporary`
- `LiveActor/LiveActor.cpp`: `compat-temporary`
- `LiveActor/LiveActor.hpp`: `compat-temporary`
- `LiveActor/ModelObj.cpp`: `compat-temporary`
- `LiveActor/ModelObj.hpp`: `compat-temporary`
- `LiveActor/Nerve.cpp`: `compat-temporary`
- `LiveActor/Nerve.hpp`: `compat-temporary`
- `LiveActor/Spine.cpp`: `compat-temporary`
- `LiveActor/Spine.hpp`: `compat-temporary`
- `Map/FileSelectCameraController.cpp`: `compat-temporary`
- `Map/FileSelectCameraController.hpp`: `compat-temporary`
- `Map/FileSelectEffect.cpp`: `compat-temporary`
- `Map/FileSelectEffect.hpp`: `compat-temporary`
- `Map/FileSelectFunc.cpp`: `compat-temporary`
- `Map/FileSelectFunc.hpp`: `compat-temporary`
- `Map/FileSelectIconID.cpp`: `compat-temporary`
- `Map/FileSelectIconID.hpp`: `compat-temporary`
- `Map/FileSelectItem.cpp`: `compat-temporary`
- `Map/FileSelectItem.hpp`: `compat-temporary`
- `Map/FileSelectItemDelegator.hpp`: `compat-temporary`
- `Map/FileSelectModel.cpp`: `compat-temporary`
- `Map/FileSelectModel.hpp`: `compat-temporary`
- `Map/FileSelectSky.cpp`: `compat-temporary`
- `Map/FileSelectSky.hpp`: `compat-temporary`
- `Map/FileSelector.cpp`: `compat-temporary`
- `Map/FileSelector.hpp`: `compat-temporary`
- `NPC/MiiFacePartsHolder.cpp`: `compat-temporary`
- `NPC/MiiFacePartsHolder.hpp`: `compat-temporary`
- `NameObj/NameObj.cpp`: `compat-temporary`
- `NameObj/NameObj.hpp`: `compat-temporary`
- `NameObj/NameObjArchiveListCollector.cpp`: `compat-temporary`
- `NameObj/NameObjArchiveListCollector.hpp`: `compat-temporary`
- `NameObj/NameObjFactory.cpp`: `compat-temporary`
- `NameObj/NameObjFactory.hpp`: `compat-temporary`
- `Scene/Scene.cpp`: `compat-temporary`
- `Scene/Scene.hpp`: `compat-temporary`
- `Scene/SceneFunction.cpp`: `compat-temporary`
- `Scene/SceneFunction.hpp`: `compat-temporary`
- `Scene/SceneObjHolder.cpp`: `compat-temporary`
- `Scene/SceneObjHolder.hpp`: `compat-temporary`
- `Screen/BackButton.cpp`: `compat-temporary`
- `Screen/BackButton.hpp`: `compat-temporary`
- `Screen/BrosButton.cpp`: `compat-temporary`
- `Screen/BrosButton.hpp`: `compat-temporary`
- `Screen/ButtonPaneController.cpp`: `compat-temporary`
- `Screen/ButtonPaneController.hpp`: `compat-temporary`
- `Screen/CaptureScreenDirector.cpp`: `compat-temporary`
- `Screen/CaptureScreenDirector.hpp`: `compat-temporary`
- `Screen/EncouragePal60Window.cpp`: `compat-temporary`
- `Screen/EncouragePal60Window.hpp`: `compat-temporary`
- `Screen/FileSelectButton.cpp`: `compat-temporary`
- `Screen/FileSelectButton.hpp`: `compat-temporary`
- `Screen/FileSelectInfo.cpp`: `compat-temporary`
- `Screen/FileSelectInfo.hpp`: `compat-temporary`
- `Screen/FileSelectNumber.cpp`: `compat-temporary`
- `Screen/FileSelectNumber.hpp`: `compat-temporary`
- `Screen/IconAButton.cpp`: `compat-temporary`
- `Screen/IconAButton.hpp`: `compat-temporary`
- `Screen/InformationMessage.cpp`: `compat-temporary`
- `Screen/InformationMessage.hpp`: `compat-temporary`
- `Screen/LayoutActor.cpp`: `compat-temporary`
- `Screen/LayoutActor.hpp`: `compat-temporary`
- `Screen/LayoutActorFlag.hpp`: `compat-temporary`
- `Screen/LayoutManager.cpp`: `decomp-needed`
- `Screen/LayoutManager.hpp`: `compat-temporary`
- `Screen/LayoutPaneCtrl.cpp`: `compat-temporary`
- `Screen/LayoutPaneCtrl.hpp`: `compat-temporary`
- `Screen/Manual2P.cpp`: `compat-temporary`
- `Screen/Manual2P.hpp`: `compat-temporary`
- `Screen/MiiConfirmIcon.cpp`: `compat-temporary`
- `Screen/MiiConfirmIcon.hpp`: `compat-temporary`
- `Screen/MiiSelect.cpp`: `compat-temporary`
- `Screen/MiiSelect.hpp`: `compat-temporary`
- `Screen/PictureBookCloseButton.cpp`: `compat-temporary`
- `Screen/PictureBookLayout.cpp`: `compat-temporary`
- `Screen/PictureBookLayout.hpp`: `compat-temporary`
- `Screen/PrologueLetter.cpp`: `compat-temporary`
- `Screen/PrologueLetter.hpp`: `compat-temporary`
- `Screen/ProloguePictureBook.cpp`: `compat-temporary`
- `Screen/ProloguePictureBook.hpp`: `compat-temporary`
- `Screen/ReplaceTagProcessor.cpp`: `decomp-needed`
- `Screen/ReplaceTagProcessor.hpp`: `compat-temporary`
- `Screen/ScreenAlphaCapture.hpp`: `compat-temporary`
- `Screen/SimpleLayout.cpp`: `compat-temporary`
- `Screen/SimpleLayout.hpp`: `compat-temporary`
- `Screen/SysInfoWindow.cpp`: `compat-temporary`
- `Screen/SysInfoWindow.hpp`: `compat-temporary`
- `Screen/TitleSequenceProduct.cpp`: `compat-temporary`
- `Screen/TitleSequenceProduct.hpp`: `compat-temporary`
- `Screen/YesNoController.cpp`: `compat-temporary`
- `Screen/YesNoController.hpp`: `compat-temporary`
- `System/ConfigDataHolder.cpp`: `compat-temporary`
- `System/ConfigDataHolder.hpp`: `compat-temporary`
- `System/GameDataFunction.cpp`: `compat-temporary`
- `System/GameDataFunction.hpp`: `compat-temporary`
- `System/GameDataHolder.cpp`: `compat-temporary`
- `System/GameDataHolder.hpp`: `compat-temporary`
- `System/GameSequenceFunction.cpp`: `compat-temporary`
- `System/GameSequenceFunction.hpp`: `compat-temporary`
- `System/NANDManager.cpp`: `compat-temporary`
- `System/NANDManager.hpp`: `compat-temporary`
- `System/NerveExecutor.cpp`: `compat-temporary`
- `System/NerveExecutor.hpp`: `compat-temporary`
- `System/SaveDataBannerCreator.cpp`: `compat-temporary`
- `System/SaveDataBannerCreator.hpp`: `compat-temporary`
- `System/SaveDataHandleSequence.cpp`: `compat-temporary`
- `System/SaveDataHandleSequence.hpp`: `compat-temporary`
- `System/SaveDataHandler.cpp`: `compat-temporary`
- `System/SaveDataHandler.hpp`: `compat-temporary`
- `System/StorySequenceExecutor.cpp`: `compat-temporary`
- `System/StorySequenceExecutor.hpp`: `compat-temporary`
- `System/SysConfigFile.cpp`: `compat-temporary`
- `System/SysConfigFile.hpp`: `compat-temporary`
- `System/UserFile.cpp`: `compat-temporary`
- `System/UserFile.hpp`: `compat-temporary`
- `Util/ActorCameraUtil.cpp`: `compat-temporary`
- `Util/ActorCameraUtil.hpp`: `compat-temporary`
- `Util/ActorMovementUtil.hpp`: `compat-temporary`
- `Util/ActorSensorUtil.cpp`: `compat-temporary`
- `Util/ActorSensorUtil.hpp`: `compat-temporary`
- `Util/ActorShadowUtil.hpp`: `compat-temporary`
- `Util/AreaObjUtil.hpp`: `compat-temporary`
- `Util/CameraUtil.cpp`: `compat-temporary`
- `Util/CameraUtil.hpp`: `compat-temporary`
- `Util/DemoUtil.cpp`: `compat-temporary`
- `Util/DemoUtil.hpp`: `compat-temporary`
- `Util/DrawUtil.cpp`: `compat-temporary`
- `Util/DrawUtil.hpp`: `compat-temporary`
- `Util/EffectUtil.hpp`: `compat-temporary`
- `Util/EventUtil.cpp`: `compat-temporary`
- `Util/EventUtil.hpp`: `compat-temporary`
- `Util/FileUtil.cpp`: `compat-temporary`
- `Util/FileUtil.hpp`: `compat-temporary`
- `Util/Functor.hpp`: `compat-temporary`
- `Util/GamePadUtil.cpp`: `compat-temporary`
- `Util/GamePadUtil.hpp`: `compat-temporary`
- `Util/GravityUtil.cpp`: `compat-temporary`
- `Util/GravityUtil.hpp`: `compat-temporary`
- `Util/JMapInfo.cpp`: `compat-temporary`
- `Util/JMapInfo.hpp`: `compat-temporary`
- `Util/JMapUtil.cpp`: `compat-temporary`
- `Util/JMapUtil.hpp`: `compat-temporary`
- `Util/LayoutUtil.cpp`: `compat-temporary`
- `Util/LayoutUtil.hpp`: `compat-temporary`
- `Util/LightUtil.cpp`: `compat-temporary`
- `Util/LightUtil.hpp`: `compat-temporary`
- `Util/LiveActorUtil.cpp`: `compat-temporary`
- `Util/LiveActorUtil.hpp`: `compat-temporary`
- `Util/MapUtil.hpp`: `compat-temporary`
- `Util/MathUtil.cpp`: `compat-temporary`
- `Util/MathUtil.hpp`: `compat-temporary`
- `Util/MessageUtil.cpp`: `compat-temporary`
- `Util/MessageUtil.hpp`: `compat-temporary`
- `Util/NerveUtil.cpp`: `compat-temporary`
- `Util/NerveUtil.hpp`: `compat-temporary`
- `Util/ObjUtil.cpp`: `compat-temporary`
- `Util/ObjUtil.hpp`: `compat-temporary`
- `Util/PlayerUtil.cpp`: `compat-temporary`
- `Util/PlayerUtil.hpp`: `compat-temporary`
- `Util/SceneUtil.cpp`: `compat-temporary`
- `Util/SceneUtil.hpp`: `compat-temporary`
- `Util/ScreenUtil.cpp`: `compat-temporary`
- `Util/ScreenUtil.hpp`: `compat-temporary`
- `Util/SequenceUtil.cpp`: `compat-temporary`
- `Util/SequenceUtil.hpp`: `compat-temporary`
- `Util/SoundUtil.cpp`: `compat-temporary`
- `Util/SoundUtil.hpp`: `compat-temporary`
- `Util/StarPointerUtil.cpp`: `compat-temporary`
- `Util/StarPointerUtil.hpp`: `compat-temporary`
- `Util/StringUtil.cpp`: `compat-temporary`
- `Util/StringUtil.hpp`: `compat-temporary`
- `Util/SystemUtil.cpp`: `compat-temporary`
- `Util/SystemUtil.hpp`: `compat-temporary`
- `Util/TriggerChecker.cpp`: `compat-temporary`

## Compatibility Inventory

| Group | Files |
| --- | ---: |
| `platform-compat` | 15 |
| `render-gx-j3d-brlyt` | 26 |
| `resource-message-font-texture` | 14 |
| `scene-sequence` | 28 |
| `trace-proof` | 2 |

Detailed artifacts: `source-closeness.tsv`, `required-migration.tsv`, `release-boundary.tsv`, `decomp-declarations.tsv`, `compile-only-allowlist.tsv`, and `compat-inventory.tsv`.
