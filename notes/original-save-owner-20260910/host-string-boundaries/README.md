# Explicit host / original Game string boundaries — 2026-09-10

This cohort accompanies the compiler-only CP932 view of original Game literals. The repository source remains UTF-8. Original Game identities and authored resource names are CP932 byte sequences; the existing strict `resource::encode_cp932` and `decode_cp932` functions are used at explicit host boundaries. No encoding detection, dual-encoding lookup, normalization, or generic hash conversion was introduced.

## Production changes

- `src/showcase/Showcase.cpp`: the two host checkpoint/story arguments are encoded before synchronous original `GameDataHolder` / `GameDataFunction` calls. Original Xanime names are decoded before UTF-8 timing logs.
- `src/runtime/SceneScheduler.cpp`: connection trace object names, sensor host names, original NameObj/LayoutActor/LiveActor snapshot names, and original BCK/BRK/BTK names are decoded for host trace/JSON presentation. Native LayoutRuntime names remain UTF-8. `LayoutDrawAdaptor` owns its encoded name in a storage base that is initialized before its NameObj base and destroyed afterward, so the original borrowed pointer is valid throughout constructor/destructor registration. Its existing host allocation scope covers string allocation.

The three reviewed original-provider translation units were sent to the compiler-wrapper owner for explicit inclusion, without manual body edits:

- `src/compat/EventUtilCompat.cpp`: original transformation/event flag and story identities; existing host exception diagnostics are ASCII.
- `src/compat/OriginalSceneWipeUtil.cpp`: original wipe identities passed to SceneWipeHolderFunction.
- `src/compat/OriginalMarioSound.cpp`: original static sound tables, lookup prefixes, and Game sound names require a single stable original encoding domain. Audio execution is not claimed by this change.

`ScreenSystemAndCaptureCompat.cpp` was explicitly excluded from that recommendation: its Japanese wipe labels feed the native UTF-8 WipeService.

## Fixture changes and lifetime

Twenty-three fixtures now distinguish original identity bytes from presentation text. The precise path list is `paths.json`; hashes are `source-manifest.json`.

- Save/story/flag calls, direct original Xanime `isRun` arguments, original wipe lookups, rumble table and MR calls, and original shadow controller queries use encoded inputs. Existing scalar, progression, star-storage, pause, control, and ownership assertions remain intact.
- GameDataRealOrAbsentTests still requires fresh progress zero. Its original story/flag lookups now receive CP932, addressing the specific mismatched UTF-8 lookup rather than changing expected progression.
- Native DemoSceneRuntime/DemoSheetRuntime and EventCameraCatalog/CameraService inputs/expected identity fields use raw CP932, following the producer contracts confirmed by their owners. Native WipeService events remain UTF-8. The spin-part expectation array owns encoded strings rather than dangling string_views of temporaries.
- Original NameObj names are decoded for host assertions. ObjectNameTable's synthetic first-row check requires exact equality with the supplied raw authored bytes; other readable mapping checks decode explicitly. Authored placement report actor_name retains raw identity, matching its actual constructor name.
- Original LightData names are decoded only in presentation assertions. Native shadow `.name` / `.group_name` presentation assertions remain UTF-8; the native add() boundary and original queries use CP932 under the shadow owner's newly explicit contract.
- Retained fixture actor names and InformationObserver prompt-part names use owned encoded strings. Temporary `.c_str()` is restricted to synchronous lookup/call arguments whose callees consume or copy the name during the full expression.

## Validation and limits

No Xmake build, link, integrated runtime, or commit was performed by this subtask. `compile-results.json` records exact isolated LLVM 23 commands, source hashes, and results for all 25 paths.

**19/25 translation units compile successfully**, including both production files, GameDataRealOrAbsentTests, MarioGatewayWalkTests, GatewaySpinCheckpointTests, InformationObserverTests, ActorEventCameraTests, StageStartCameraTests, both demo clock/sheet fixtures, ObjectNameTableTests, OriginalSceneWipeOwnerTests, and OriginalShadowControllerOwnerTests.

Six older route fixtures fail on pre-existing removed compatibility APIs: AirActorRouteTests, BrightSunRouteTests, GatewayDemoSceneTests, PlanetMapActorRouteTests, SkyActorRouteTests, and TitleFileSelectRouteTests. The common support header still calls removed `actor_base_matrix`; several use removed `actor_model` / `actor_current_brk_name`, and GatewayDemoSceneTests also references obsolete shadow.valid/calculation_enabled fields. These failures are recorded and were not masked or repaired through new aliases. The first showcase isolated compile attempt used a generic fixture command lacking the app include directory; adding the actual app/showcase include directories made its complete translation unit compile successfully.

The parent owns integration with the producer and compiler-wrapper cohorts, final linked/runtime checks, and publication. Passing compilation does not establish gameplay success.

## Remaining native owner boundaries

A second bounded production cohort covers eleven mixed compatibility TUs; `owner-paths.json`, `owner-source-manifest.json`, and `owner-compile-results.json` identify exact sources, hashes and commands. All eleven complete translation units compile successfully with isolated LLVM 23. No original Game file was edited.

GroupCheckManagerCompat, MarioCameraTarget, EffectSystemOwnership, SceneObjHolderCompat, OriginalNameObjExecuteHolder, ImageEffectOwnership, OriginalStarPointerDirector, ClippingDirectorCompat, and SaveDataHandleSequenceCompat now pass selected Japanese constructor names from process-lifetime `const std::string` storage. Each file-local encoder initializes that storage under `aurora::allocation::HostAllocationScope`; the actual Game constructor calls retain their prior allocation scope. This keeps names valid through owner retirement without retaining pointers into temporary conversions or allocating static strings in a retiring Game arena.

GameRuntimeCompat's seven convenience rumble calls select four stable encoded pattern names. RumbleService directly matches original `RumbleData::mName`, so direct native service inputs and recorded pattern identities are raw CP932 too; FeedbackRealOrAbsentTests now encodes those inputs and its recorded-identity expectation. Its focused compile passes; no new rumble gameplay support is claimed.

GameActorPhysicsCompat's three authored shadow names now enter the shadow owner's explicitly raw add/make contract. OriginalShadowControllerOwnerTests additionally checks actual `ShadowController::mName` and `mGroupName`, retained `.name_raw` / `.group_name_raw`, and raw-key metadata lookup while preserving separate readable `.name` / `.group_name` assertions. Its complete translation unit compiles successfully after the companion producer changes.

All eleven mixed files must stay outside compiler CP932 literal rewriting: their runtime encoder's input is deliberately UTF-8. This exclusion was sent to the compiler-wrapper owner. The three earlier pure-provider candidates remain separate.

A final source audit reported one remaining boundary to the parent: `src/layout/LayoutManagerCompat.cpp::bind_actor_manager` passes original `actor->getName()` directly into native LayoutRuntime, whose constructor copies it. The native runtime's UTF-8 presentation contract requires decoding at that constructor boundary. This subtask did not edit the separately owned layout file.

The parent subsequently authorized the single LayoutManagerCompat boundary fix: `bind_actor_manager` now decodes the original actor name before the copying native LayoutRuntime constructor. The complete translation unit compiles with exit 0; `layout-boundary-compile.json` records the exact command and source hash. No temporary pointer survives that constructor. Read-only follow-up priorities are in `movement-camera-priorities.md`.
