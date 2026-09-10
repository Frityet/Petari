# Expanded Gateway showcase: current source audit

Read-only source inspection, 2026-09-10. No build, test, package, or production edit was performed. Existing factory declarations establish availability, not a new full gameplay verification.

**Recommended launch:** `smg-pc-showcase gateway --disc <real-disc-path> --width 1280 --height 720`. Leave `--max-frames`, `--smoke`, `--exit-after-screenshot` and debug input scripts unset for the interactive package. There is no all-features CLI option: Showcase.cpp:259–312 only selects title/gateway/gateway-spin plus window and capture options.

The ordinary Gateway route already initializes the scenario catalog, particles, actual selected-file GameDataSession, authored post-castle checkpoint, scene resources, original Mario and game camera, then normal movement/rendering (Showcase.cpp:728–801). GatewayDemoScene.cpp:245–263 selects SupportedSubsetForDevelopment **without a per-actor filter**; every supported active authored placement is considered. Rebuilding and packaging this current route brings in newly supported actors automatically. No supported feature was found unnecessarily disabled by a showcase option.

| Surface | Current state / action |
| --- | --- |
| Mario walking/jumping, game camera, pointer/input | Ordinary gateway already routes through the original owners. Keep freecam initially false (Showcase.cpp:758); F9 is the existing optional development freecam. Do not force story or Mario entitlement fields. |
| Authored scenery, lights/air, gravity, effects, switches | Supported factory/catalog actors are already instantiated. Scene effect system, LightDirector, clipping, groups and GameSceneLayoutHolder are created before finalization (GatewayDemoScene.cpp:211–240). |
| Tico, TicoBaby, DemoRabbit | Exact supported native factory entries already exist (scene/nameobj/NameObjFactory.cpp:254–267). They appear when the selected scenario's authored rows/state call for them; no spawn-everything override is needed. |
| Physics sphere | Already user-triggered with `=`/numpad `+` (Showcase.cpp:844). This is explicitly a development probe; enabling automatic smoke mode adds a frame limit and is unsuitable as the interactive default. |
| Host cursor / VSync | System cursor already hidden; VSync defaults on (RendererService.cpp:780–784). `SMGPC_ENABLE_VSYNC=1` explicitly retains that default. Texture/render dumps and simulation tracing are diagnostics, not gameplay features. |
| Title route | Separate bounded title/file-selection showcase that selects a blank file and launches ordinary Gateway after title unwind (Showcase.cpp:1127–1148). It is not full original GameSystem startup/save progression. |

**Do not use gateway-spin as an 'everything enabled' mode.** Showcase.cpp:786 advances story progress to 10, and GatewaySpinCheckpoint.cpp:126–181 constructs `GatewaySpinRouteTico` and `GatewaySpinRosettaTrigger` as host LiveActor stand-ins. That route exercises a bounded prompt/entitlement checkpoint; it cannot represent the original bunny-to-Rosalina progression under this request's ownership requirements.

**Real remaining prerequisites:**

- `RunawayRabbitCollect` is deliberately unavailable with reason `original_game_scene_demo_sequence_runtime_unavailable` (native factory:327). Its RunawayTico child can enter its original guide/timekeep demo, whose DemoStartRequestUtil calls MR::canStartDemo. GatewayDemoScene supplies scene-holder/executor bindings, but never constructs/binds the actual original GameScene. GameSceneBinding.cpp:129–132 refuses that absent owner. Imported process/scene sources alone do not establish their live initialization order.
- The SceneObj_TalkDirector case still creates native TalkRuntime (SceneObjHolderCompat.cpp:623), whose actual type derives NameObj. It is not a complete original TalkDirector and must not be cast or treated as one.
- Coin/PurpleCoin are not safe factory-only toggles. Their complete source is compiled and real CoinHolder/PurpleCoinHolder factories exist, but Coin::init calls still-throwing `setBinderExceptSensorType` and `tryCreateMirrorActor` (GameActorPhysicsCompat.cpp:300–311). Its postpass also calls still-throwing `setClippingRangeIncludeShadow` (:504). The old `shadow_runtime_unavailable` factory label is incomplete, but its exclusion remains necessary.
- StarPiece/StarPieceGroup source remains excluded (Game/xmake.lua:60–61); StarPieceDirector is not supplied by the scene factory. Rosetta has no supported native creator entry. Neither is enabled by a launch option.
- PlanetMapCatalog.cpp:265–272 excludes force-low, unique creators and retained optional submodels. Those require their real creator paths, not replacing them with ordinary PlanetMap.
- `SMGPC_SAVE_DIR`/`SMGPC_NAND_DIR` only configure storage (RuntimeContext.cpp:534–538). They do not activate the original GameSystem/sequence/NAND UI graph in this bounded showcase; exposing a save path is not proof of complete save/load behavior.

No source-supported, presently disabled gameplay toggle was identified that can be enabled safely without additional implementation. The concrete immediate expansion is packaging the newly rebuilt ordinary Gateway route with all currently supported authored placements and leaving the existing development controls available.
