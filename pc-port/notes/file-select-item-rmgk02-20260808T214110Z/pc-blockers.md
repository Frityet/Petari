# Exact PC dependency/provider blockers

`pc-port/src/Game/xmake.lua` currently removes both `Map/FileSelectItem.cpp` and
`Map/FileSelector.cpp`. The reconstructed FileSelectItem is mirrored for source
parity, but building, registering, or activating it is appropriate only after
the following compatibility providers are real.

## Hard runtime blockers

### 1. Mii face actor/model rendering is absent

The PC tree is missing all four direct Game files:

- `Game/NPC/MiiFaceParts.hpp`
- `Game/NPC/MiiFaceParts.cpp`
- `Game/NPC/MiiFaceRecipe.hpp`
- `Game/NPC/MiiFaceRecipe.cpp`

More importantly, `compat/MiiFacePartsHolderCompat.cpp` is not a usable provider:

- `createPartsFromReceipe` returns `nullptr`;
- `createPartsFromDefault` returns `nullptr`;
- `draw`, `calcAnim`, `calcViewAndEntry`, `drawEachActor`, `drawExtra`, `setTevOpa`, and `setTevXlu` are empty.

This is mandatory even for a fellow/Mario file icon. Retail `FileSelectItem::createMii()` creates a default hidden `MiiFaceParts` for every item, and `control()` subsequently dereferences it to update its scale. A Game-side conditional or dummy object would be a workaround and must not be added.

The lower RFL layer is also deliberately real-or-absent rather than a renderer:

- `RflService::init_char_model` ends in `RFLErrcode_NotAvailable` even after finding a valid entry;
- `RflService::draw_model` only records a `NotAvailable` trace;
- `RFLSetMtx` is a no-op;
- default-source records are not supplied by `find_entry`.

Required provider: a real Aurora/RFL character-model path that initializes retail `RFLCharModel` geometry/textures/expressions, applies matrices, draws opaque/translucent parts, and lets the exact root `MiiFaceParts` and holder lifecycle run. Until then, the honest state is to leave FileSelectItem unregistered/unbuilt rather than return a fake face.

### 2. CenterScreenBlur/post-process rendering is absent

The PC tree is missing:

- `Game/Screen/CenterScreenBlur.hpp/.cpp`;
- `Game/Screen/FullScreenBlur.hpp/.cpp`;
- `MR::createCenterScreenBlur` and `MR::startCenterScreenBlur` declarations/definitions;
- the `SceneObj_CenterScreenBlur` case in `compat/SceneObjHolderCompat.cpp`.

The existing `ImageEffectService` only tracks `ForceOff` and `ControlAuto`; it does not implement the captured-screen expanded-blit blur used by retail `drawFullScreenBlur`.

Required provider: a generalized Aurora post-process/captured-frame blur service, then the exact Game `CenterScreenBlur` nerve actor and scene-object registration. A no-op `createCenterScreenBlur` would hide this dependency and is not acceptable.

## Compile-surface gaps (providers mostly already exist)

With the root NPC headers supplied as a read-only overlay, a syntax-only compile exposes these host ABI/header gaps:

- `Game/Util/ScreenUtil.hpp` lacks `createCenterScreenBlur`;
- `Game/Util/NerveUtil.hpp` exists and has the correct `NerveExecutor` overloads, but the FileSelectItem include surface does not expose `isLessStep`, `isLessEqualStep`, `isGreaterEqualStep`, and `setNerveAtStep`; overload resolution falls back to stripped `LiveActor` declarations;
- PC `JGeometry::TVec3<f32>` lacks retail `setAll<f32>` and the `operator const TVec2f&()` projection used by `LayoutActor::setTrans`;
- PC `JSystem/JMath/JMath.hpp` lacks `JMathInlineVEC` wrappers even though Aurora supplies the underlying `PSVEC*` calls;
- The PPC-ABI-compatible integral `MR::getRandom(long, long)` bridge for retail
  `0l`/`5l` call sites is now present in generalized compatibility code. It is
  no longer an activation blocker.

These belong in generalized JSystem/Nerve/math compatibility headers, not in `Game/Map/FileSelectItem.cpp`.

## Direct dependencies already available

The following direct providers were inspected and are suitable foundations:

- `FileSelectModel.cpp/.hpp` and `FileSelectNumber.cpp/.hpp` are byte-for-byte mirrored from root and compiled;
- matrix creation (`makeMtxTR`, `makeMtxTransRotateY`) is provided by `MtxCompat.cpp`;
- camera position/screen projection/world unprojection are provided by `CameraUtilCompat.cpp`;
- pointer position/history/target registration are provided by `StarPointerUtil.cpp`;
- rumble/camera shake route through runtime services;
- system SE/level SE/ME route through `SoundUtilCompat.cpp`;
- parts-model creation and named effect emit/delete route through `LiveActorUtilCompat.cpp` and the actor/effect runtime.

The remaining activation boundary is therefore not FileSelectItem logic. It is the real Mii renderer/holder plus the real blur post-process and the small generalized ABI header extensions above.
