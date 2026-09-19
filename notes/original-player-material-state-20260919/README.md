# Original player lighting diagnosis — 2026-09-19

The original opening rendered normal Mario almost entirely blue. This is not an authored transformation or a renderer color-channel reinterpretation: the original player receives uninitialized scene lighting before generating its dynamic display list.

## Evidence

- `../gateway-compat-20260919/second-integrated-frame240.png` is the observed blue opening image.
- `capture4.log` stops the actual original process after 100 calls to `MarioActor::drawMarioModel`: normal model 0/player mode 0, unhidden. It follows the actual original `J3DModelX::drawIn` to material 0/shape 8 and saves the 576-byte authored material command image as `material-dl.bin`.
- The authored material list has neutral ambient 0x80808080, white material color, ordinary texture/TEV commands. The original player's 32-byte dynamic light list then writes XF ambient register 0x100a with 0x00009000 (RGBA 0,0,144,0).
- `ambient.log` stops `MarioActor::updateLightDL` and then `GDSetChanAmbColor`. The argument is already (0,0,144,0); GD encodes it correctly. `ActorLightCtrl::mAreaLightInf` is null, its zone ID is (-1,-1), and `mLightInfo` contains uninitialized values. No debugger state was injected.
- `StageLightData::ensure_loaded` explicitly returns `_loaded` without RuntimeContext. The original process intentionally does not construct a RuntimeContext, and only the standalone host scene routes previously constructed a StageLightSceneBinding. Thus original `LightFunction::initLightData` never loaded LightData.arc.

The first three `capture*.log` files record exploratory debugger expression errors; `capture4.log` and `ambient.log` are the decisive successful captures. These debugger runs were deliberately killed after observation, not claimed as clean completed gameplay runs.

## Compatibility correction

The actual OriginalSceneSupport now retains a StageLightSceneBinding after original scenario initialization. StageLightSceneBinding has a second input route taking the actual original StageDataHolder tree, preserving its authored stage name, root/child zone IDs, and repeated occurrences through the existing cache's deduplication. Loading runs after `SceneDataInitializer::initAfterScenarioSelected`, before original light initialization and actor placement. Cache lifetime ends after scene actors retire. Original Game source and GX shaders are unchanged.

This uses the same existing resource decoder and retail zone/area-light cache as other native scenes. It does not synchronize a second RuntimeContext stage name, insert a Mario color, or fabricate a default light when source data is absent. Missing/invalid ownership or resources remain explicit failures.

## Validation status

The cache-only native build succeeded. `ambient-after.log` confirms the same actual Mario controller now references a retained authored AreaLightInfo: player ambient (64,75,85,110), diffuse0 (255,150,128), diffuse1 (40,60,90). `ambient-after-run.log` completes 360 original frames with exit 0. Viewed `ambient-after-frame240.png` shows red hat/shirt, blue overalls, yellow buttons and skin/white gloves instead of the blue silhouette. Existing AreaObjRealOrAbsentTests includes retail root/child lighting resolution, distinct Rosetta ambient channels, conflicting-zone rejection, retirement, and recreation without stale child cache.

## Adjacent remaining gap

The old native `Game/Map/LightFunction.cpp` rewrite routes diffuse/point light submission exclusively through RuntimeContext and silently returns in the original process. `registerPlayerLightCtrl` likewise never registers with the actual LightDirector. This is distinct from the proven garbage ambient color and requires restoration of direct original GX submission/scene ownership; changing the renderer to compensate would conceal it.

## Original GX submission and ownership restoration

The previous native implementations of LightFunction, LightDirector and LightUtil are now confined to `src/compat/`; their Game files are byte-for-byte copies of the reference, recorded by `source-equivalence.json`. The native providers keep required resource/lifetime handling and host renderer light snapshots. Without the separate host RuntimeContext, they now issue the original GX light color/position/attenuation, actor ambient, point-light and coin-specular calls, including the original camera transform for world-space positions. The actual LightDirector::_1C owns the borrowed player controller in every process mode. SceneLightService's duplicate player pointer and its API were removed; actor controller replacement/retirement clears the actual director field.

Coin defaults (yellow diffuse, follow camera, zero specular color, strength65) come directly from decomp LightDataHolder.cpp's sDefaultLightSetCoin. GX_LIGHT0 diffuse and GX_LIGHT3 specular follow the reference LightFunction::loadLightInfoCoin. Unused GXLightObj storage is zero-initialized at the native boundary rather than transmitting uninitialized stack bytes; semantically used fields retain original values.

Recovered previously undecompiled loadAllLightWhite in decomp first after reading AGENT_DECOMP_GUIDE.md. Retail assembly at 0x80188FB8 contains only eight origin-position/white-color GX light loads. MWCC passes, 192-byte symbol size matches, objdiff 99.479164%. This preserves original behavior without synthesizing an attenuation policy. The published reference source is copied to the port. The twelfth integrated native build passes. `direct-gx-run.log` completes360 original frames and exits0; `direct-gx-frame240.png` was reopened and visually checked. It has normal Mario colors, stronger authored lighting, and newly restored flower actors from the separate PlantGroup work. Bright highlights remain a visual-comparison topic; this is not a claim of exact rendered parity.

`PointLightRuntimeTests --original-gx-only` captures real GX display-list bytes without RuntimeContext and checks distinct RGBA channels, positions, diffuse attenuation, the eight white registers, and coin specular slots/attenuation. The normal fixture additionally checks registration and replacement against LightDirector::_1C. These are API-byte and lifetime proofs, not visual parity claims.

## Completed focused checks

- `point-light-build.log`: target build PASS.
- `point-light-original-gx.log`: `smg-pc-point-light-runtime-tests --original-gx-only` PASS (exit0), independent of RuntimeContext/GPU window.
- `area-light-rebuild.log` and `area-light-zone-after.log`: existing real-disc `zone-light` case PASS (exit0), including authored root/child tables and cache lifetime.
- A proposed metadata-only original StageDataHolder test was removed after its real constructor required the process GameSystem language owner. `area-light-zone-debug3.log` preserves the test-only stack. No GameSystem was fabricated to satisfy that fixture. The actual original holder-tree route is evidenced by production GameSystem execution and `ambient-after.log`.
- `direct-gx-run.log` / `direct-gx-frame240.png`: integrated360-frame original opening PASS and image inspected. The broader PointLightRuntime graphical suite was not run here; its new controller-lifetime assertions are compiled but not claimed as executed.

The relevant decomp commit b57aa547eec0fcd39cbcc32c37a4ea06c049f9d4 was pushed and the exact remote SHA verified. Parent agent owns native checkpoint publication. No active debugger/game/build remains from this task.

## Additional original-owner validation

`point-light-original-owner.log` passes after `point-light-owner-build.log`. The
fixture uses the actual original GameSystem, Scene, SceneObjHolder and executor,
without a RuntimeContext or GPU window. It checks registration, replacement of
both active and inactive controllers, actor retirement, and eight consecutive
scene lifetimes. The borrowed pointer is always LightDirector::_1C.

The broader legacy RuntimeContext graphical fixture was attempted with the real
disc and fails before its assertions: MessageHolderOwnership calls original
MR::getLanguage without a GameSystem language owner. The exact crash is retained
in `point-light-full-debug2.log` (frames 0-13); this is outside the original
process lighting route and is not counted as a pass. No language owner or default
was fabricated to make it pass. The focused GX and original-owner checks and
the actual GameSystem opening remain the relevant executed checks.

The large direct-GX screenshot and raw LLDB captures are also retained as
lossless `.gz` files for the committed evidence set. Decompress them to recover
the exact original bytes; no image color/size conversion was applied.
