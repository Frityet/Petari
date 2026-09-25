# Process owner migration review

Read-only review of the round-four working tree. No production files, tests, build wiring or index were modified. Source hashes are in `source-evidence.json`; this review is not a runtime pass.

## Result

No newly introduced production correctness blocker found in the reviewed closure. `SystemUtil.cpp` is exactly the current decomp donor after removing the Aurora include and its eight retained `GuestThreadExecutionScope` declarations. The restored getters deliberately require the original `GameSystem` owners.

- `FileLoader::destroy` retains the old helper's null check and executing-worker preflight before entering the destructor. All observed retirement callers use this entry point. The destructor first waits through actual file-entry queues, whose lifetime survives clearing the request index, then retires the worker, conditionally releases the same singleton, frees file entries, unmounts archives and frees request storage. This is the old helper's order. Caller-owned file destinations retain `mContextSet == false` and are not freed.
- Nested guest scopes in the static entry point/destructor/caller are supported: Aurora increments execution depth, acquires only an unowned CPU gate and releases ordinary ownership only at depth zero. Blocking SDK waits and cancellation release the gate independently of depth, allowing workers to finish.
- `OSThreadWrapper` cancellation reaches Aurora's synchronous `OSCancelThread`; even detached native workers are reaped/joined before the wrapper frees the stack and `OSThread`. `FunctionAsyncExecutor` retains the old helper's workers-first, infos/functors-next, queues-next, heaps-last ordering. Process retirement still stops these producers before scenes, resource holders and file loader.
- Original file-before-archive teardown remains safe for the native fixed borrowed archive: `ArchiveHolderArchiveEntry` uses `JKR_MEM_BREAK_FLAG_0`, so its archive cannot free the buffer again. `JKRMemArchive::~JKRMemArchive` calls `attach_archive(nullptr)`, whose null branch only clears native metadata/pointers and does not inspect the already-freed buffer. The borrowed `RarcArchive` is then discarded without dereferencing its view.
- `PlacementZoneScope` retains the prior implementation and existing public header: it borrows the original checker, saves/restores the previous ID, and verifies strict LIFO ordering. Its remaining direct consumers are scoped tests; no substitute placement owner or copied algorithm was introduced.
- The restored async/particle/restart/PAL functions have single production definitions in the complete `SystemUtil.cpp`; removed providers are absent. The only `getGameSystemObjHolder` provider is the original direct getter.

## Existing fixtures needing migration before claiming a whole-suite pass

These are concrete stale expectations exposed by intentional removal of synthetic owner fallbacks. They do not justify adding fallback behavior back into production:

1. `tests/OriginalAutoEffectMetadataTests.cpp:148` calls `MR::Effect::getAutoEffectListBinary()` without a `GameSystem` and expects a caught `logic_error`. The new original getter dereferences the missing system instead. Its later standalone `ParticleResourceOwnership` block also expects the removed lookup fallback. Move the table-wide checks under the actual process fixture; `OriginalParticleResourceOwnerTests` already demonstrates this fixture and checks original particle identity, catalog queries and metadata counts.
2. `tests/RuntimeContextConstructionTests.cpp:158` calls `MR::getParticleResourceHolder()` after only constructing the preview runtime's standalone particle owner. It should inspect that native owner directly when testing preview construction, or use the actual process fixture when asserting the Game getter.
3. `tests/RestartStageSessionTests.cpp:121-122` calls original restart getters/setters with only `StageSessionBinding`. The canonical functions now require actual `GameSystem::mSequenceDirector::mGameDataTemporaryInGalaxy`. `OriginalSceneCounterOwnerTests` and `OriginalProcessPlacementTransformTests` already assert the real owner identity.

These three targets are still present in `tests/xmake.lua`. They were not executed in this review, so the predicted null access is source-derived. Parent was notified; no test edits requested or made.

## Boundaries

The direct public `FileLoader` destructor has no self-worker preflight, but no current caller bypasses `FileLoader::destroy`; its only observed callers preserve the old preflight. Self-destruction from an async callback was unsupported by the old async helper as well. This review does not claim either unsupported path became safe, nor does it assess constructor-allocation failure behavior predating this migration.

## Follow-up

Root assigned and approved narrow fixture corrections after this review. The two standalone RuntimeContext/RestartStageSession fixtures are now adapted in `../fixture-adaptations/`; root owns the AutoEffect actual-process migration. The stale expectation descriptions above record the reviewed pre-correction state.
