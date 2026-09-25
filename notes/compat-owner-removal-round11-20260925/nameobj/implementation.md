# NameObjExecuteHolder canonical restoration

Restored the complete current donor implementation in `src/Game/NameObj/NameObjExecuteHolder.cpp` and deleted `src/compat/OriginalNameObjExecuteHolder.cpp`. Both previous files were clean; the canonical source was absent. Before copies, hashes, exact lane patch, and donor delta are stored beside this note.

All holder lookup and executor access now follow the original process: `MR::getSceneObj` and `GameSystemSceneController::getNameObjListExecutor`. Connection state transitions, category movement requests, light lookup, and draw registration are the donor implementation. The old alternate SceneExecutionBinding lookup and dynamic encoded-name publication were removed. Unregistered-object temporary wrappers now use the donor path; the canonical holder itself already handles a negative executor index.

Native differences are bounded: a compile-time CP932 literal, the existing capacity exception, and the `MR::registerNameObjToExecuteHolder` call through the existing SceneScheduler so model references, native draw registration, late initialization, and rollback remain correct. The scheduler still invokes the canonical `registerActor`; no game execution state was copied into another service.

Added `NameObjExecuteHolder::~NameObjExecuteHolder` to delete its owned execution array. Source search found no external deletion to remove. The scheduler must clear its registration entries before the holder and actual scene executor retire; root owns that adjacent lifecycle. No scheduler, binding, list executor, or SceneFunction files were changed in this lane.

Build wiring: none required. `src/Game/xmake.lua` already includes canonical Game sources and compatibility providers through wildcards, and the holder has no exclusion. No builds or tests were run, as requested. Source-only checks confirmed the donor differences and absence of another execution-array destructor/provider.

## Follow-up: actual category retirement safety

Root restored direct original movement/calc dispatch. Source review found that the donor raw array iterator could skip swapped members or run past the shortened end when native callback retirement immediately removed entries. The former scheduler supplied this safety through its own registration batches.

Added a bounded guard in the actual `NameObjCategoryList`, without an alternate membership owner. Empty categories still return before their pre-draw function. After pre-draw and its membership changes, the list snapshots its actual ordered pointers, object runtime generations, and original executor indices into a host-allocated vector. Each call requires the same live object generation, unchanged execution registration index, and current membership in the actual array. Retired or reallocated addresses and same-object re-registration cannot enter the current batch. Temporary array swaps do not reorder the existing batch; newly added members wait until the next dispatch.

Each actual list now owns a host-allocated shared lifetime flag. Its destructor marks the flag false before deleting the delegator/arrays; the executing function retains the flag and checks it after pre-draw and before accessing the owner for each next callback. The callback itself runs outside the host allocation scope, preserving Game heap routing. No external owner registry or alternative category state was introduced. The existing NameObj runtime generation registry remains a borrowed retirement identity source.

The two additional source/header paths were clean before editing and are included in the manifest and exact patch. `git diff --check` passed for these source edits; no compiler/test invocation was performed. Root was separately notified that the retained SceneDrawBufferService caller must not use its pre-call array reference after a callback unbinds/destroys the executor.

## Startup teardown correction: explicit retained executor

Root's `startup-backtrace2.log` proves ordinary LogoScene teardown calls `destroyScene`, which unpublishes `mScene` before `Scene::~Scene` retires native scene services. Scheduler clearing then invoked original `NameObjExecuteInfo::disconnectToScene`, whose donor process lookup dereferenced the unpublished scene. Construction was not the cause.

Permanent native retirement now calls `NameObjExecuteInfo::retireNativeRegistration(NameObjListExecutor&)` with the executor already retained by its SceneExecutionBinding. The method uses the existing original request-disconnect transitions, removes pending movement/draw memberships through shared explicit-executor overloads, and resets the retired record with its default constructor. Ordinary original disconnect methods still resolve the actual GameSystem executor and delegate to those same removal bodies; no duplicate removal algorithm or alternate scene publication was added. Resetting the native record no longer calls `setConnectInfo`, which unnecessarily fetched the current process for an empty record.

Only `SceneScheduler::retire_execution_entry` changed in the shared scheduler file. It passes `_execution->executor()` and clears the object's original index. The existing controller null-before-delete ordering and scene/child ownership are unchanged; the native binding still releases the actual executor after registrations. Source snapshots immediately before this correction are in `before-retirement`, and `explicit-retirement.patch` isolates it from earlier round11 edits. `git diff --check` passed; no build/test performed by this lane.
