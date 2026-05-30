# Discrepency Report

This report records which differences from `../src/Game` are acceptable and which should be replaced. The intent is to keep actual core game code, including scenes and actors, as close to the decomp as possible while allowing renderer/layout/host compatibility layers to evolve around Aurora.

Generated from the current tree on 2026-05-30.

## Snapshot

| Metric | Count |
|---|---:|
| `src/Game` files in pc-port | 192 |
| `../src/Game` files in decomp source | 1345 |
| `src/Game` files missing from `../src/Game` | 105 |
| `../src/Game` files not yet present in `src/Game` | 1258 |
| Common `Game/` files that differ | 86 |

## Good Or Acceptable Discrepencies

These are acceptable when they stay out of core game logic or are guarded as debug-only behavior.

- **Rendering backend and Aurora integration:** GX/J3D/light/upload/window behavior may differ if implemented as general compatibility or Aurora backend work, not game-route logic.
- **Layout backend implementation:** BRLYT/BRLAN parsing and rendering can differ internally, but `Game/Screen` should eventually become source-shaped wrappers over a compat layout service.
- **Host configuration and debug harnesses:** env vars, trace emitters, probes, and reports are acceptable outside `Game/` or behind `#ifndef NDEBUG`.
- **Picturebook-to-bunny handoff:** currently allowed as a temporary debug exception so the bunny demo can load after picturebook.
- **Generated source-derived data:** archive lookup tables generated from the original factory are acceptable as data, but not as substitutes for real actor behavior.

## Current Discrepencies To Replace

- **Story and sequence routing in `Game/`:** local stage request code and hardcoded route names should be replaced by the source `StorySequenceExecutor` model backed by generic scene/sequence compatibility.
- **Reduced `NameObjFactory`:** the two-entry local factory should be replaced by the original factory table and real actor coverage; compat model fallback should not define game behavior.
- **PC-native systems inside `Game/`:** direct `RuntimeContext` access from actors, scene functions, layout, save, camera, input, message, player, sound, star pointer, and MR utilities should move behind Wii-shaped compat APIs.
- **File-select local behavior:** debug observer state, fallback names, and save/Mii shortcuts should be replaced by source-close file-select logic and real save/RFL/NAND compatibility.
- **Scene object/save/system partials:** fallback holders and partial save/config/user-file classes should be reconciled with original classes, with host filesystem details outside `Game/`.
- **Generic stage placement workarounds:** model fallback, alias fallback, ignored placement tables, and unsupported-placement tolerance should become real compatibility support or debug-only reporting.
- **PC-only utility implementations:** `PlayerUtil.cpp`, `SequenceUtil.cpp`, and similar local MR support should be removed from `Game/` once source-shaped systems exist.

## Outside-`Game` Workaround Inventory

These are not all bad, but each should be either generalized into compat/Aurora or kept debug-only. They should not become permanent game-specific behavior.

| Finding | File | Line | Evidence |
|---|---|---:|---|
| Generic model fallback | `src/scene/nameobj/NameObjFactory.cpp` | 133 | `.kind = NameObjPlacementSupportKind::GenericModel,` |
| Generic alias model fallback | `src/scene/nameobj/NameObjFactory.cpp` | 141 | `.kind = NameObjPlacementSupportKind::GenericAliasModel,` |
| Intentionally ignored placement kind | `src/scene/nameobj/NameObjFactory.cpp` | 117 | `.kind = NameObjPlacementSupportKind::IntentionallyIgnored,` |
| Fallback model support flags | `src/scene/StagePlacementResolver.cpp` | 507 | `.model_fallback_supported = support.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::GenericModel,` |
| Stage host model fallback report status | `src/scene/StageHostScene.cpp` | 42 | `return "created_model_fallback";` |
| Debug stage-name override | `src/runtime/RuntimeContext.cpp` | 420 | `return read_string_environment("SMGPC_STAGE_NAME").value_or("");` |
| Synthetic RFL fallback Miis | `src/runtime/RflService.cpp` | 1249 | `std::vector<RflMiiEntry> RflService::fallback_miis() {` |
| Picturebook resource probe target | `src/debug/xmake.lua` | 23 | `add_files("PicturebookResourceProbe.cpp")` |
| Hardcoded picturebook resource probe | `src/debug/PicturebookResourceProbe.cpp` | 155 | `const auto archive = load_archive("ObjectData/PictureBookTexture.arc");` |
| HeavensDoor archive records | `src/scene/nameobj/NameObjArchiveTable.inc` | 609 | `OriginalArchiveRecord{"HeavensDoorFlowerA", "HeavensDoorFlowerA"},` |
| FileSelector archive records | `src/scene/nameobj/NameObjArchiveTable.inc` | 1036 | `OriginalArchiveRecord{"FileSelector", "FileInfo"},` |
| Route-specific native test | `tests/AuroraNativeTests.cpp` | 189 | `auto route = EnvironmentVariableGuard("SMGPC_DEMO_ROUTE");` |

## Exhaustive Common-File Differences In `Game/`

Every file below exists in both `src/Game` and `../src/Game` but differs byte-for-byte.

| Path | pc-port lines | decomp lines |
|---|---:|---:|
| `Camera/CameraTargetArg.cpp` | 18 | 47 |
| `Camera/CameraTargetMtx.cpp` | 49 | 152 |
| `Demo/PrologueDirector.cpp` | 334 | 299 |
| `LiveActor/ActorCameraInfo.cpp` | 7 | 15 |
| `LiveActor/ActorLightCtrl.cpp` | 134 | 89 |
| `LiveActor/HitSensor.cpp` | 56 | 96 |
| `LiveActor/LiveActor.cpp` | 410 | 481 |
| `LiveActor/ModelObj.cpp` | 87 | 76 |
| `LiveActor/Nerve.cpp` | 4 | 3 |
| `LiveActor/PartsModel.cpp` | 32 | 136 |
| `LiveActor/Spine.cpp` | 44 | 47 |
| `Map/FileSelectCameraController.cpp` | 141 | 150 |
| `Map/FileSelectEffect.cpp` | 83 | 55 |
| `Map/FileSelectFunc.cpp` | 61 | 35 |
| `Map/FileSelectItem.cpp` | 881 | 909 |
| `Map/FileSelectModel.cpp` | 110 | 110 |
| `Map/FileSelectSky.cpp` | 70 | 73 |
| `Map/FileSelector.cpp` | 2127 | 1480 |
| `Map/LightFunction.cpp` | 154 | 99 |
| `NPC/MiiFacePartsHolder.cpp` | 19 | 230 |
| `NameObj/NameObj.cpp` | 82 | 88 |
| `NameObj/NameObjArchiveListCollector.cpp` | 23 | 16 |
| `NameObj/NameObjFactory.cpp` | 49 | 8371 |
| `Scene/Scene.cpp` | 33 | 37 |
| `Scene/SceneFunction.cpp` | 37 | 135 |
| `Scene/SceneObjHolder.cpp` | 54 | 437 |
| `Screen/BackButton.cpp` | 66 | 63 |
| `Screen/BrosButton.cpp` | 109 | 101 |
| `Screen/ButtonPaneController.cpp` | 373 | 326 |
| `Screen/CaptureScreenDirector.cpp` | 107 | 98 |
| `Screen/EncouragePal60Window.cpp` | 4 | 65 |
| `Screen/FileSelectButton.cpp` | 146 | 138 |
| `Screen/FileSelectInfo.cpp` | 307 | 304 |
| `Screen/FileSelectNumber.cpp` | 174 | 178 |
| `Screen/GalaxyMapGalaxyPlain.cpp` | 47 | 112 |
| `Screen/IconAButton.cpp` | 124 | 124 |
| `Screen/InformationMessage.cpp` | 91 | 91 |
| `Screen/LayoutActor.cpp` | 289 | 115 |
| `Screen/LayoutPaneCtrl.cpp` | 134 | 62 |
| `Screen/Manual2P.cpp` | 271 | 231 |
| `Screen/MiiConfirmIcon.cpp` | 65 | 62 |
| `Screen/MiiSelect.cpp` | 336 | 498 |
| `Screen/PictureBookCloseButton.cpp` | 83 | 83 |
| `Screen/PictureBookLayout.cpp` | 906 | 961 |
| `Screen/PrologueLetter.cpp` | 79 | 79 |
| `Screen/ProloguePictureBook.cpp` | 104 | 104 |
| `Screen/SaveIcon.cpp` | 17 | 16 |
| `Screen/SimpleLayout.cpp` | 3286 | 14 |
| `Screen/SysInfoWindow.cpp` | 261 | 278 |
| `Screen/TitleSequenceProduct.cpp` | 192 | 192 |
| `Screen/YesNoController.cpp` | 124 | 191 |
| `System/ConfigDataHolder.cpp` | 180 | 124 |
| `System/GameDataFunction.cpp` | 104 | 446 |
| `System/GameDataHolder.cpp` | 218 | 296 |
| `System/GameSequenceFunction.cpp` | 84 | 305 |
| `System/NANDManager.cpp` | 275 | 136 |
| `System/NerveExecutor.cpp` | 38 | 30 |
| `System/SaveDataBannerCreator.cpp` | 69 | 93 |
| `System/SaveDataHandleSequence.cpp` | 538 | 706 |
| `System/SaveDataHandler.cpp` | 533 | 482 |
| `System/StorySequenceExecutor.cpp` | 127 | 1245 |
| `System/SysConfigFile.cpp` | 72 | 126 |
| `System/UserFile.cpp` | 214 | 125 |
| `Util/ActorCameraUtil.cpp` | 62 | 141 |
| `Util/ActorSensorUtil.cpp` | 176 | 1047 |
| `Util/CameraUtil.cpp` | 306 | 461 |
| `Util/DemoUtil.cpp` | 32 | 292 |
| `Util/DrawUtil.cpp` | 23 | 85 |
| `Util/EventUtil.cpp` | 17 | 978 |
| `Util/FileUtil.cpp` | 335 | 273 |
| `Util/GamePadUtil.cpp` | 343 | 300 |
| `Util/JMapInfo.cpp` | 309 | 160 |
| `Util/JMapUtil.cpp` | 439 | 607 |
| `Util/LayoutUtil.cpp` | 578 | 541 |
| `Util/LightUtil.cpp` | 53 | 29 |
| `Util/LiveActorUtil.cpp` | 279 | 2809 |
| `Util/MathUtil.cpp` | 56 | 964 |
| `Util/MessageUtil.cpp` | 52 | 95 |
| `Util/NerveUtil.cpp` | 32 | 69 |
| `Util/ObjUtil.cpp` | 632 | 911 |
| `Util/ScreenUtil.cpp` | 244 | 593 |
| `Util/SoundUtil.cpp` | 115 | 670 |
| `Util/StarPointerUtil.cpp` | 188 | 917 |
| `Util/StringUtil.cpp` | 16 | 368 |
| `Util/SystemUtil.cpp` | 17 | 171 |
| `Util/TriggerChecker.cpp` | 26 | 25 |

## Exhaustive `Game/` Files Missing From Decomp Source

Every file below exists in `src/Game` but not in `../src/Game`. These are discrepancies unless they are local build/config files, temporary headers awaiting decomp declarations, or compatibility code that should move out of `Game/`.

```text
.clang-format
Camera/CameraTargetArg.hpp
Camera/CameraTargetMtx.hpp
Demo/PrologueDirector.hpp
LiveActor/ActorCameraInfo.hpp
LiveActor/ActorLightCtrl.hpp
LiveActor/ActorStateKeeper.hpp
LiveActor/HitSensor.hpp
LiveActor/LiveActor.hpp
LiveActor/ModelObj.hpp
LiveActor/Nerve.hpp
LiveActor/PartsModel.hpp
LiveActor/Spine.hpp
Map/FileSelectCameraController.hpp
Map/FileSelectEffect.hpp
Map/FileSelectFunc.hpp
Map/FileSelectIconID.hpp
Map/FileSelectItem.hpp
Map/FileSelectItemDelegator.hpp
Map/FileSelectModel.hpp
Map/FileSelectSky.hpp
Map/FileSelector.hpp
Map/LightDataHolder.hpp
Map/LightFunction.hpp
Map/LightPointCtrl.hpp
Map/LightZoneDataHolder.hpp
NPC/MiiFacePartsHolder.hpp
NameObj/NameObj.hpp
NameObj/NameObjArchiveListCollector.hpp
NameObj/NameObjFactory.hpp
Scene/Scene.hpp
Scene/SceneFunction.hpp
Scene/SceneObjHolder.hpp
Screen/BackButton.hpp
Screen/BrosButton.hpp
Screen/ButtonPaneController.hpp
Screen/CaptureScreenDirector.hpp
Screen/EncouragePal60Window.hpp
Screen/FileSelectButton.hpp
Screen/FileSelectInfo.hpp
Screen/FileSelectNumber.hpp
Screen/GalaxyMapGalaxyPlain.hpp
Screen/IconAButton.hpp
Screen/InformationMessage.hpp
Screen/LayoutActor.hpp
Screen/LayoutActorFlag.hpp
Screen/LayoutManager.cpp
Screen/LayoutManager.hpp
Screen/LayoutPaneCtrl.hpp
Screen/Manual2P.hpp
Screen/MiiConfirmIcon.hpp
Screen/MiiSelect.hpp
Screen/PictureBookCloseButton.hpp
Screen/PictureBookLayout.hpp
Screen/PrologueLetter.hpp
Screen/ProloguePictureBook.hpp
Screen/ReplaceTagProcessor.cpp
Screen/ReplaceTagProcessor.hpp
Screen/SaveIcon.hpp
Screen/ScreenAlphaCapture.hpp
Screen/SimpleLayout.hpp
Screen/SysInfoWindow.hpp
Screen/TitleSequenceProduct.hpp
Screen/YesNoController.hpp
System/ConfigDataHolder.hpp
System/GameDataFunction.hpp
System/GameDataHolder.hpp
System/GameSequenceFunction.hpp
System/NANDManager.hpp
System/NerveExecutor.hpp
System/SaveDataBannerCreator.hpp
System/SaveDataHandleSequence.hpp
System/SaveDataHandler.hpp
System/StorySequenceExecutor.hpp
System/SysConfigFile.hpp
System/UserFile.hpp
Util/ActorCameraUtil.hpp
Util/ActorSensorUtil.hpp
Util/CameraUtil.hpp
Util/DemoUtil.hpp
Util/DrawUtil.hpp
Util/EventUtil.hpp
Util/FileUtil.hpp
Util/Functor.hpp
Util/GamePadUtil.hpp
Util/JMapInfo.hpp
Util/JMapUtil.hpp
Util/LayoutUtil.hpp
Util/LightUtil.hpp
Util/LiveActorUtil.hpp
Util/MathUtil.hpp
Util/MessageUtil.hpp
Util/NerveUtil.hpp
Util/ObjUtil.hpp
Util/PlayerUtil.cpp
Util/PlayerUtil.hpp
Util/ScreenUtil.hpp
Util/SequenceUtil.cpp
Util/SequenceUtil.hpp
Util/SoundUtil.hpp
Util/StarPointerUtil.hpp
Util/StringUtil.hpp
Util/SystemUtil.hpp
Util/TriggerChecker.hpp
xmake.lua
```

## Regeneration Commands

```sh
find src/Game -type f | sort | sed 's#^src/Game/##' > /tmp/pc_game_rel.txt
find ../src/Game -type f | sort | sed 's#^../src/Game/##' > /tmp/orig_game_rel.txt
comm -23 /tmp/pc_game_rel.txt /tmp/orig_game_rel.txt
comm -12 /tmp/pc_game_rel.txt /tmp/orig_game_rel.txt | while read f; do cmp -s "src/Game/$f" "../src/Game/$f" || echo "$f"; done
```
