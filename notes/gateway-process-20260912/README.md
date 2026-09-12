# Original process layout resources — 2026-09-12

This checkpoint removes the three unconditional stationed-layout-resource failures in `OriginalStationedResourceManager.cpp`. It recovers the actual original `LayoutHolder`, retaining its original mounted archive and Game heap through a native owner. Full original process initialization, the complete GameScene lifecycle, and Gateway bunny progression remain unverified and incomplete.

## Source and retail evidence

The implementation was recovered first in `decomp/src/Game/System/LayoutHolder.cpp`, with declarations in `decomp/include/Game/System/LayoutHolder.hpp`, following `decomp/AGENT_DECOMP_GUIDE.md`. The reference source was then copied into `src/Game/System/`. Its build classification is now `Equivalent`. The coordinator published this reference recovery in decomp commit `36949dd21104c64cc13f23d5c160bcb4ab0274af`.

`retail-disassembly.txt` retains the disassembly with relocations from the RMGK01 LayoutHolder object at 0x803A0698–0x803A0D94. `wii-build.json` records the actual Metrowerks command and successful final compile. `retail-recovery.json` lists all 12 original translation-unit functions and their individual matches; the byte-weighted function match is **99.4519%**. `objdiff.json.gz` is the complete comparison. The four tiny raw-resource virtual accessors emitted in the retail LayoutManager object at 0x8036A318–0x8036A338 are each **100%** matches; `other-accessors-recovery.json` and `other-accessors-objdiff.json.gz` retain that comparison.

The recovery preserves recursive archive traversal, original file-ID lookup, case-sensitive extension classification, case-insensitive full-name table hashes, unstripped resource names, raw pointer identity, original animation-hash queries, and the five original font-prefix routes. The SDK `nw4r::lyt::ResourceAccessor` declaration and implementation were imported unchanged.

The only native-versus-reference source difference is the word load in `LayoutHolder::GetResource`: Wii loads a big-endian word from resource offset **4**, and the native copy assembles those exact four bytes without an alignment assumption. This deliberately preserves the original offset and result; it does not reinterpret this value as the archive file size. No game-state branches, layout names, or scene-specific fallbacks were added.

## Native ownership and services

`LayoutArchiveOwner` retains the actual `MountedArchive` and actual Game heap domain, constructs the recovered original holder on that heap, and releases table-name/table allocations before dropping its borrowed archive. It does not copy or byte-swap layout payloads. Raw lookup and the original JKR archive therefore return the same pointers.

`JkrAllocationDomain::retain_heap(JKRHeap&)` resolves an exact registered heap owner, or its nearest registered ancestor, independently of the current heap. Both model and layout resource services now use this lookup for mounted/explicit heaps; a missing or retiring real owner remains an error. This closes the reviewed case where a valid stationed heap was rejected when resource creation happened outside a Game allocation scope or under a sibling scene scope.

`ResourceHolderService` now caches layout holders by resolved mounted archive path. Ordinary requests resolve and mount layout archives through the existing VFS; stationed and raw requests require an existing real mount. Both mounted entry points share the same holder. A mount replacement cannot silently replace a live holder's borrowed bytes, and resource-heap removal rejects outstanding retained layout borrowers before removing either model or layout owners.

The three ResourceHolderManager entry points now reach this shared native service. Their ignored explicit heap argument remains ignored, matching the reference method; actual heap selection comes from the mounted archive/current allocation scope. `MR::createAndAddLayoutHolder` and its raw-data counterpart are restored over the real manager. Font getters route to the actual original GameSystemFontHolder and fail explicitly when that process owner is absent. No substitute font/process singleton is installed.

This remains a **native ResourceHolderManager cache/facade**, not activation of the original manager's asynchronous main-thread dispatcher. Original FunctionAsyncExecutor integration and worker scene ownership remain separate work. Existing LayoutRuntime renderers retain their own parsed layout/font backing; this change does not claim they now execute the complete original NW4R layout constructor graph.

## Validation

- Wii recovery compile: pass; 12 LayoutHolder functions at 99.4519% byte-weighted match, plus four raw virtual accessors at 100%.
- Native focused build/run: **pass** after the cross-heap correction, as run by the coordinator. See `../gateway-integration-20260912/resource-final.build.log` and `resource-final.run.log`.
- Extended `OriginalResourceHolderTests.cpp` exercises original nested layout/animation/raw tables, font routing boundary, exact archive identity, native endian result, raw/stationed cache identity, retained mount lifetime, blocked unload/remount, clean holder replacement, missing-mount rejection, and foreign-heap creation outside a scope and under a sibling scope.
- Full original GameSystem boot, rendered layout behavior, and Gateway bunny-to-Rosalina progression: not claimed.

## Remaining original process activation blockers

The source audit still finds declarations without native bodies for `JKRAram::create`, AudSystemWrapper, MainLoopFramework, HomeButtonLayout, GameSystemErrorWatcher, and GameSystemResetAndPowerProcess. Their actual dependencies must be implemented/imported before calling the original `GameSystem::init`; the static showcase build does not link this complete graph. The requested disabled-audio policy does not itself implement the original audio owner's constructor/wave-load completion interface.

DrawSyncManager still needs actual GX flush publication, FIFO breakpoint suspension, token callbacks and pixel-work completion. The existing FIFO consumed watermark reports **command decoding**, not rendered-pixel/GPU completion; routing tokens directly from that watermark would be incorrect. This checkpoint leaves those boundaries explicit.

The original font owner calls NW4R ResFont construction/resource parsing. A native ResFont provider already exists in `src/compat/Nw4rFontCompat.cpp` and shares `layout/BrfntFont`; this accessor checkpoint does not establish the complete process font startup/retirement path. NWC24/VF storage/message APIs and Home/reset platform providers remain absent.

GameSystemSceneController constructs and retires original scenes on an async worker while the host updates/draws them. GameSceneBinding, SceneExecutionBinding, SceneNameObjRegistry and multiple other current-owner adapters remain thread-local. The coordinator separately fixed and cross-thread tested SceneLifetimeBinding in root commit `2fa4fcbe7`; that particular registry is no longer a remaining TLS blocker. These require explicit ownership transfer/consolidation under the shared guest CPU gate. NameObjRegister's original current holder also needs consolidation with the native SceneNameObjRegistry, avoiding double registration.

Original `GameScene::init` spins on `GameSceneFunction::isLoadDoneScenarioWaveData()` without an SDK wait/yield. The current cooperative guest CPU gate cannot preempt arbitrary native C++; enabling the original graph before resolving this scheduling boundary can deadlock. Current host rendering and save/story/scene-transition owners have not been silently installed into a partial GameSystem.
