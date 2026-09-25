# Actual execution and callback owners

Owned scope: NameObjCategoryList.hpp/.cpp and NameObjListExecutor.hpp/.cpp only. Root deletes the two scene services/tables and adapts SceneFunction/OriginalSceneSupport/fixtures; decomp_validation owns scheduler integration; gateway_gap_audit owns the original draw classes. No build, test, index or commit operation was run by this lane.

NameObjListExecutor now directly owns the borrowed scheduler/NameObjExecuteHolder relationship, retained allocation domain and initialization/retirement flags. bindNativeExecution attaches to the scheduler before creating the same original NameObjExecuteHolder, stop, movement and sensor controllers as the former binding. Failed attachment never clears a different scheduler owner; construction failure after attachment disconnects and detaches this executor. There is no global binding stack.

allocateDrawBufferActorList marks native initialization after all original allocations succeed, before root's restored SceneFunction calls MR::initConnectting. prepareNativeRetirement permanently disconnects actual requirements through scheduler.clear and clears all three category callback sets. unbind is idempotent, detaches, clears the requirement borrow, then retires the actual draw holder. Explicit draw retirement rejects active actors by checking the actual executer arrays. The executor retains its heap domain until its owned lists and draw holder are gone, even if GameSystem has already cleared the current Scene. Original execution scopes use this retained actual domain rather than re-querying global scene publication.

Each actual CategoryInfo owns one shared callback payload. The payload retains the clone's caller allocation domain and deletes the functor before releasing that domain. Registration clones before publication, then atomically replaces its owned callback and original _C borrowed view. Execution retains the selected payload; replacement, clear, or deletion from the callback cannot invalidate the active functor. Existing list lifetime, NameObj generation and membership checks remain. Per-category shared execution counters reject rebuilding the same category recursively and survive list self-destruction. There is no callback history or rollback service.

Category arrays initialize all counters, validate indices/capacity, retain original swap-with-last removal, and make permanent duplicate removals harmless. Constructor delegators use local ownership until table initialization succeeds. CategoryInfo's destructor releases the callback through its actual member; its AssignableArray destructor releases object pointer storage. clearNativeCallbacks and nativeLifetime are the only added category integration methods.

API agreement:
- nativeRequirements() returns NameObjExecuteHolder& and diagnoses a retired borrow.
- nativeAllocationDomain() returns a retained shared_ptr by value.
- nativeInitialized/nativeRetiring are const noexcept observations.
- notifyNativeObjectRetired clears only the exact requirements identity.
- Scheduler attach_execution/detach_execution borrow the executor; detach only drops that borrow and scheduler-owned allocation binding. Executor owns callback and buffer retirement.

All four sources were clean at the lane start. owned-manifest.json records exact before/after hashes and patches/ records this lane's complete deltas. Production frozen for the parent's single integrated build.
