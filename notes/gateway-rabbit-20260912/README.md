# Original NPC item data and Gateway prerequisites — 2026-09-12

## Implemented

Recovered complete original `NPCFunction.cpp` in `decomp/src/Game/NPC/`, then copied the same source into `src/Game/NPC/`. The three retail functions are `createNPCData`, `deleteNPCData` and `getNPCItemData`. The first two really are empty retail functions (each is a single `blr`); they are actual missing GameScene link prerequisites, not new no-op compatibility substitutes.

The item getter selects the real scene NPCDirector, formats the actor-specific `%sItem.bcsv` resource name, copies the caller defaults into the original NPCItemParameterReader, attaches the retained archive table to a local JMapInfo, reads the selected row through the original four parameter descriptors, and copies the result back. Missing tables return false. Existing tables with out-of-range rows return true with defaults retained. A present null resource follows original attach-false behavior and likewise preserves defaults. Null caller/director/holder pointers are not valid retail inputs and were not assigned invented fallback behavior.

Restored the original one-instruction MR forwarding function in decomp/native NPCUtil and deleted the unconditional native item-data thrower. NPCDirector's reference/native header changes only make its three recovered data members public so original NPCFunction can access its actual owner graph. No layout or gameplay behavior changed. Native/reference CPP files and header match byte-for-byte. `decomp/configure.py` now marks NPCFunction matching. Native Xmake already includes the new source by its normal Game glob; no build-file changes were made by this subtask.

## Evidence

- The Wii compiler succeeds for NPCFunction and NPCUtil.
- NPCFunction's entire 264-byte text section and all three functions are 100% objdiff matches; the 256-byte item getter is exact. MR's four-byte forwarding body also matches 100%.
- All 66 retail instructions were independently checked against `decomp/orig/RMGK01/sys/main.dol`, SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`.
- Native NPCFunction, NPCUtil, NPCActorRuntimeCompat and the edited NPC test TU compile successfully with the current LLVM flags. Known inherited missing-override warnings remain in JKRArchive headers.
- `retail-and-source-proof.json`, `match-summary.json`, exact compiler command records and compressed complete objdiff reports retain the proof. Compiled objects remain local.

## Behavior test

Extended `smg-pc-npc-actor-real-or-absent-tests` with a synthetic item-table fixture, using the real scene-created NPCDirector, original parameter reader and NPCData ResourceHolder. During only the fixture, its generic resource table points at retained synthetic BCSV resources; it is restored afterward. Coverage exercises deliberately shuffled named columns, two item rows, missing fields preserving caller values, absent tables, negative/past-end rows, present-null table data and strings surviving local JMapInfo destruction. The stale test expecting every item lookup to throw is removed. The test still uses its existing `SMGPC_REAL_DISC`/renderer fixture; parent owns the combined Xmake build and actual run.

## Remaining Gateway boundary

The unchanged RunawayRabbit/RunawayRabbitCollect/RunawayTico cohort is already compiled. RunawayTico's first guide update requests its original timekeep demo, which requires the actual GameScene startup/sequence owner graph. No rabbit placement was enabled over the development Scene, no false GameScene binding was introduced, and no switch/story shortcut was added. The new item provider is a general NPC and full GameScene prerequisite; this evidence does not claim a completed bunny chase or Rosalina appearance.

Original TalkDirector remains a larger separate owner frontier: native TalkRuntime owns controllers/presentation, while original TalkDirector depends on TalkBalloon/State holders and depth-readback/layout lifecycles. Its normal programmable talk-demo boundary still throws pending coherent ownership activation.

## Complete ResourceShare import

Imported original `Game/Util/ShareUtil.cpp` and its header unchanged from the already-matching reference. Its constructor owns two distinct 128-byte buffers and a zero count. Existing SceneObj creation executes the constructor under the real scene allocation domain, so both arrays and their actual ResourceShare owner share the scene arena lifetime. The retail destructor is empty; no host cache or fabricated allocation owner was added. Parent owns the shared factory include/case integration.

`smg-pc-sceneobj-holder-real-or-absent-tests` now checks the exact typed singleton, original name, distinct non-null buffers, zero count, and all three allocation identities against the active scene heap. Source and fixture native TUs compile. This is another unconditional original GameScene owner made available.

## Message page recovery

Recovered `MR::getNextMessagePage` first in decomp MessageUtil, copied the same body into its native original-source counterpart, and supplied the same body from the existing native OriginalMessageLineQueries provider (whole Game MessageUtil remains excluded). Its 152-byte retail function matches 100%; all 38 instructions also match the actual DOL. The existing MessageEditorMessageTag native parser preserves original packed UTF-16 control-tag byte counts despite native wide-character width, so no new text-decoding workaround was needed.

The function scans complete tags (including embedded zero/newline payload words), finds group 1/tag 1, skips exactly one following newline, and returns a pointer into the same retained string. A page break at the end returns the empty-page pointer; scanning that page then returns null. Ordinary newlines do not create pages. No null input or malformed arbitrary memory behavior was invented beyond the original valid-message contract.

Added cases to the existing `smg-pc-original-layout-group-tests` tag fixture for all these behaviors and repeated page traversal. Both native TUs compile. Parent owns the integrated target build/run.

## Independent shadow review

Reviewed all eleven definitions in new `src/compat/OriginalActorShadowQueries.cpp` against current original ActorShadowLocalUtil/ActorShadowUtil. Every function body matches the reference; null-controller assumptions, the first/default controller rule, projected/unprojected midpoint/radius behavior and FLOAT_MAX result are unchanged. No actionable source deviation found. Runtime shadow ownership validation belongs to the parent/owning agent.

## Original TalkDirector restoration audit

The actual Director/State/Balloon source cohort totals 1516 lines. An honest owner restoration must publish the actual LayoutActor-derived TalkDirector into its scene slot, retain its actual balloon/state/peek objects, and redirect TalkFunction request/start/end/register calls to it. Current TalkRuntime owns controller graphs, host callbacks and presentation together; replacing only its pointer type or adding native programmable-demo state would violate that owner contract.

Concrete prerequisites still remaining:

- TalkTextFormer needs the now-restored page scanner and three recursive layout tag-stepping helpers (`initTagProcessorRecursive`, `nextStepTagProcessorRecursive`, `isEndStepTagProcessorRecursive`). The latter have no native implementation or LayoutRuntime text-reveal state today. Recover the original message tag processor and connect it to the actual native text-box records instead of equating all text with immediately completed presentation.
- TalkBalloonHolder creates four short balloons, event/info/sign/icon balloons and an original IconAButton. TalkStateHolder owns a separate composition short balloon, button, player watcher and five actual talk states. These original constructor allocations must remain with the scene arena; registered LayoutActor children already fit existing NameObj capture/retirement.
- TalkSupportPlayerWatcher uses actual Mario state, floor/water/control/speed and pad input. It cannot be instantiated over a missing player owner by inventing idle-state answers.
- TalkPeekZ requires the actual DrawSyncManager callback registration, GX projection/viewport capture, draw-sync snapshot and depth unprojection. Original DrawSyncManager and generic Aurora GXPeekZ are present, but this needs their coherent process/frame lifecycle. The existing StarPointerDepthOwnership path already demonstrates retained native snapshot ownership; copying its actor-specific wrapper would not be a complete original TalkPeekZ contract.
- Exact TalkDirector prep/term also owns clipping restoration, Mario talk state, camera closure and programmable/timekeep demo start/pause/end. These must activate together with the original talk states and GameScene, rather than replacing the current explicit normal-demo unsupported exception with partial manual start/end calls.

No TalkRuntime or demo gameplay behavior was changed by this subtask.

Coordinator final integration: the affected focused tests build and pass; see ../gateway-integration-20260912/validation-summary.json for selected final receipts and explicit remaining limits.
