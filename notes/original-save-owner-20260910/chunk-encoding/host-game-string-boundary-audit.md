# Host / original Game string boundary audit — 2026-09-10

Read-only follow-up while the parent and sibling prepare compiler-only CP932 literal views for original Game. This audit adds no helper, production edit, build, or commit. Existing strict `resource::encode_cp932` / `decode_cp932` already express the two directions; a wrapper class alone would add no capability.

## Rule and retirement dependency

An original Game `const char*` identity is a sequence of authored CP932 bytes. A host UTF-8 caller must encode before passing a name into such an API. Names obtained from original Game/resources are already Game bytes and must not be encoded again. Decode only when crossing into a host API whose explicit contract is UTF-8, such as presentation/JSON.

Do not detect encodings heuristically or teach generic `strcmp`, hash, resource lookup or Game APIs to accept both domains. CP932 has aliases that can decode to the same Unicode string; preserving raw resource identity avoids a lossy decode/re-encode roundtrip. `ActorShadowCsvCompat` already demonstrates the intended split: raw joint names go to `MR::getJointMtx`; decoded names serve native presentation.

The save hash scope assumes incoming names are UTF-8. Once compiler-only Game views contain CP932 literals, the scope and the `MR::getHashCode` conversion hook must retire together. Save scalar byte-order conversion remains required. The current 179-check fixture is evidence for the frozen pre-activation hash contract, and will need its host string calls / expected normal domain updated after the transition.

## Immediate actual-owner callers

| Current caller | Current crossing | Required change |
|---|---|---|
| `src/showcase/Showcase.cpp:735,784` | Native Japanese checkpoint literals go to actual `GameDataHolder::followStoryEventByName` and `GameDataFunction`. | Explicitly encode native UTF-8 literals. These synchronous lookups do not need a process interning table. |
| `src/scene/GatewaySpinCheckpoint.cpp:192,193` | Native story literals go to actual `GameDataFunction`. | Encode call arguments; keep separate UTF-8 native demo-sheet assertions in their current native domain until that service migrates. |
| `src/compat/EventUtilCompat.cpp:32-204` | Japanese first-transformation flags and story-event names go to original `GameDataFunction`. | Encode literals at these calls, or deliberately include this pure Game-semantics provider in the audited compiler literal cohort. Do not convert Game-provided parameters a second time. |
| `src/compat/OriginalSceneWipeUtil.cpp:6-70` | Japanese wipe identities go to actual `SceneWipeHolderFunction`. | Encode these native literals, or include this pure original-provider TU in the literal cohort. |
| `src/compat/OriginalMarioSound.cpp` | Large native table of original Japanese sound names plus original-prefix searches. | The table and searches need the same Game domain as their original callers. A deliberate original-provider compile cohort is more coherent than per-call temporary conversions; existing table pointers must remain stable. Audio runtime remains outside this bounded proof. |

`ScreenSystemAndCaptureCompat.cpp` is different from `OriginalSceneWipeUtil`: its Japanese system wipe names feed only the native `WipeService` strings/events (`RuntimeServices.cpp:2441-2532`). They may remain UTF-8. A blanket conversion of every compat literal would conflate these distinct contracts.

Native-created Game owners also retain `const char*` names. Japanese names passed by `SceneObjHolderCompat`, `GroupCheckManagerCompat`, `MarioCameraTarget`, `ImageEffectOwnership`, `OriginalStarPointerDirector`, `TalkRuntime`, `SaveDataHandleSequenceCompat`, `ClippingDirectorCompat`, `OriginalNameObjExecuteHolder` and the Gateway spin cast constructors must be stable CP932 strings if those objects expose Game names. Do not use `encode_cp932(...).c_str()` temporaries for constructors that retain the pointer. Preserve ownership in the existing service/owner or use static encoded literal storage in an audited pure-Game provider.

## Dynamic/native stores that cross back into Game

| Store | Concrete evidence | Boundary disposition |
|---|---|---|
| Object display names | `ObjectNameTable.cpp:68` decodes `jp_name`; `StageInitializationService.cpp:924-942` returns that UTF-8 `c_str()` as actor name; `GatewayDemoScene.cpp:257-265` copies it into `AuthoredPlacementInstantiator`, which passes it to actual constructors at846-893. | Retain raw authored name bytes separately for Game constructors, preserving the existing string lifetime. Keep decoded text only for reports. ASCII object factory IDs remain unchanged. |
| Demo identities | `DemoSceneRuntime.cpp:111` decodes `DemoName`, then exact comparisons at785/795 index native UTF-8 keys. `DemoCompat` passes original Game demo/part parameters directly. | Prefer raw identity fields for the Game-facing registry with decoded presentation fields. If retaining the UTF-8 service contract, the facade must explicitly decode each Game input and encode any returned Game identity with an owner that outlives the return. No heuristic fallback. |
| Demo returned part | `DemoCompat.cpp:411-423` returns a borrowed native `current_main_part_name` pointer directly; `isDemoPartTalk` compares a native UTF-8 substring at431. | Both the returned identity and the predicate must use the Game domain. A temporary encoded string cannot back the returned `const char*`. |
| Demo animation requests | `DemoSheetRuntime.cpp:67/73` decodes sheet strings. `DemoSceneRuntime.cpp:457` passes `row.animation_name.c_str()` to `MR::startAction`; at535 it passes `row.bck_name.c_str()` to `MR::startBckPlayer`. | Preserve raw sheet identity for these original animation APIs or use explicitly retained encoded bytes. Native position/cast metadata may stay UTF-8 where only native comparisons consume it. |
| Original light records | `LightData.cpp:302-304` decodes a resource name then publishes it as `AreaLightInfo::mAreaLightName`; at331 the zone mapping uses the decoded name; at236 defaults return these pointers. `Game/Map/LightDataHolder.cpp:25` uses exact `strcmp`. | Game-facing light records/defaults must retain raw name bytes consistently. Native diagnostics may decode separately. |
| Shadows | `ActorShadowCsvCompat.cpp:217-218` already retains raw and decoded joint names; line endpoints likewise have raw identities. At129 the raw joint name reaches `MR::getJointMtx`. | Preserve this split. Do not change the correct raw Game path merely because a display field is decoded. Check model/group names only where a concrete Game caller consumes them; no blanket rewrite. |
| Planet map catalog | `PlanetMapCatalog.cpp:217/243` decodes planet/scenario names and forms native map/archive keys. `PlanetMapRuntimeCompat.cpp:103` compares Japanese actor names to native literals. | Inventory candidate for an explicit raw identity contract. Most current archive/factory IDs are ASCII; this audit does not claim every catalog path is currently broken. The Japanese Game-name comparison requires a consistent domain. |
| General positions | `StagePlacementResolver.cpp:1030` decodes names; `DemoSceneRuntime.cpp:462/515` compares to decoded sheet position names. | This native-to-native comparison is currently consistent. Original NamePos owners consume raw `JMapInfo` through `StageResourceBinding`; do not gratuitously transcode that correct original path. |

## Tests and presentation

Immediate test calls to actual original story/flag/animation owners need explicit Game bytes:

- `tests/GameDataRealOrAbsentTests.cpp`: actual holder/GameDataFunction story and flag calls throughout87-152.
- `tests/MarioGatewayWalkTests.cpp:498`: selected checkpoint; `673/817`: direct original `XanimePlayer::isRun` comparisons.
- `tests/GatewaySpinCheckpointTests.cpp:355,403,417,454-590`: actual story predicates/advancement; separate native demo API assertions may stay UTF-8 while that native service has a UTF-8 contract.
- `tests/InformationObserverTests.cpp:235,326,347` and story checks through491; `MR::startTimeKeepDemo` at351 is a Game facade, while the following native `demo.find_definition` is a separate domain.
- `tests/OriginalSceneWipeOwnerTests.cpp:143` names enter the actual Game wipe holder.
- `SaveChunkEncodingProbe.cpp`: direct table-name identity inspection and VLE1 `setValue/getValue` must use CP932 after literal activation; retain the independent CP932 golden IDs and remove the obsolete save-scope/normal-UTF8 expectations.

Original-player scalar/pointer/control assertions and PLAY scalar-byte tests do not require Japanese-name conversions by themselves. Do not change their scalar behavior in response to the charset transition.

For original Game output consumed by host diagnostics:

- `Showcase.cpp:1028-1037` prints `getCurrentAnimationName()` directly. Decode this known Game result to UTF-8 before formatting so existing log consumers remain valid.
- `MarioGatewayWalkTests.cpp:726` copies that same Game name into `proof.animation_name`, then compares to native UTF-8 `基本` at736/877/1170. Decode when constructing the host proof; direct calls to `isRun` still need encoded arguments instead.
- `SceneScheduler.cpp:479` inserts original object names into semantic trace text. `entry_name` at1663-1672 copies Game names into snapshots; these can flow into JSON. Decode known Game names at presentation extraction. Its native Layout branch already has a separate host string owner, so it must not be blindly decoded as CP932.
- Borrowed `NameObj::getName()` strings passed through native effect/sensor/demo diagnostics require the same explicit boundary decision. `RuntimeContext::emit_semantic_trace_event` itself accepts host text and should not guess which fragments originated in Game.

`host-game-string-candidates.json` is a broad read-only list of non-ASCII quoted lines under compat/showcase/scene/runtime/light/tests to help the parent finish its coordinated inventory. It includes native UI/assertion/comment candidates and is **not** an automatic transform list or an exhaustive compiler-token audit.

## Validation and scope

All findings above are from current source reads. This subtask made no production edits and ran no build. The parent owns activation, concrete caller migration, original-save-owner tests and the next real movement replay. Earlier CP932 execution-literal notes supplied only an audit checklist; their old staged status does not establish current activation.
