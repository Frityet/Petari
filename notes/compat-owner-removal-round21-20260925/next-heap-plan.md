# Delete JkrAllocationDomain: actual heap ownership plan

Read-only plan against current round21 source, while the parent runs the integrated audio build. No source edits, builds, tests, or git mutations. This supersedes the orphan/audio details of round20/next-heap-owners.md; the basic split in that note remains sound.

## Current boundary and finite scope

The two files `compat/JkrAllocationDomain.{hpp,cpp}` implement four things: a process root/MEM2 allocation owner (`JkrHeapRuntime`), owned temporary solid heaps, borrows of original manually destroyed heaps, and heap-selection/allocation-routing scopes. The process-global `DomainRecord` list and `allocation_scope_depth`/`heap_teardown_depth` TLS must disappear, not be renamed or moved.

Current search finds 53 source files: 38 with a handle/API or forward declaration (including the two files being removed), and 15 with only host-allocation aliases/includes. There are 66 matching fixture/build files; `OriginalCameraContextTests.cpp` adds one indirect caller through `create_cohort`/`host_heaps` without spelling the old types. A production migration is manageable in three independent source lanes after agreeing the SDK API. Fixtures are mostly mechanical type/scope changes, not a reason to keep compatibility aliases.

Round21 has already removed the orphan ScenarioCatalogOwnership/ParticleResourceOwnership pairs and disabled-audio temporary domain. Their former migration instructions are obsolete. `GameResourceRuntime::create_cohort()` has only the camera fixture caller; remove that production convenience method and create an ordinary bounded solid heap in that fixture. `CameraDirectorRuntime.hpp` and `MessageHolderOwnership.hpp` have unused domain/runtime forward declarations; remove them rather than inventing replacement APIs.

## Proposed canonical SDK contract

Place the following on actual `JKRHeap`, with state directly on the heap and all shared-pointer control blocks allocated under `aurora::allocation::HostAllocationScope`:

```cpp
std::shared_ptr<JKRHeap> retainNativeLifetime();
static std::shared_ptr<JKRHeap> retainCurrentNativeLifetime();
std::shared_ptr<JKRHeap> adoptNativeOwnership(std::shared_ptr<void> backing = {});
void bindNativeBackingStorage(std::shared_ptr<void> backing);
void validateNativeRetirement() const;
```

`retainNativeLifetime` locks an actual per-heap weak handle under `sCurrentHeapMutex`. A manually owned original child may lazily create a non-destroying borrow control block retaining its actual parent handle. Each child needs its own control block: aliasing its pointer onto the parent's control block cannot distinguish outstanding child borrowers. A manually owned root needs an explicit owner before it is retained. Do not add any global heap-to-handle table.

`adoptNativeOwnership` is a once-only handoff of an existing real heap into shared native ownership. Use the existing `JKRExpHeap::createRoot(void*, u32, bool)` and `JKRSolidHeap::create(u32, parent, bool)` factories, then adopt immediately while holding the SDK selection lock, before publishing the result to clients. Adoption fails if a live borrow handle or another shared owner has already been published. Root and display temporary solid heaps use it; original Game heaps continue to be destroyed through their existing raw APIs. A shared-owned flag is needed: an expired owning weak handle means retirement is in progress and must never create a fresh manual-borrow handle, even in the gap before the final deleter acquires the selection mutex.

The owning deleter retains the parent and any caller-supplied backing through `destroy()`, including the parent's final `JKRFreeToHeap(parent, this)`. Move those captures to local variables when invoked and empty the captures before returning. Otherwise an unrelated `weak_ptr<JKRHeap>` can keep the deleter's parent/backing captures alive after the heap is already destroyed. This subtle difference matters because current domain `Storage` dies at strong-count zero. Release parent/backing outside `sCurrentHeapMutex`; parent final release can drain callbacks. Control-block allocation failure after creating a raw heap must destroy that heap and retain its parent/backing through rollback.

`bindNativeBackingStorage` is the narrow generic contract for a manually owned heap built inside externally allocated memory, specifically the GDDR3 heap built in MEM2. It stores the caller-owned byte token on that actual heap. It does not change the Game heap's destruction policy. Each `do_destroy` implementation must keep this token locally through the entire explicit destructor and deallocation call; merely storing it as a member can free the memory containing `this` in the middle of its destructor. A protected `beginNativeRetirement()` returning the detached backing token is an appropriate small helper on the actual heap, not a new allocation-owner class.

`validateNativeRetirement` rejects live borrowed handles and prevents new handles once retirement starts. It must cover both static/member `destroy()` and direct virtual `do_destroy()` entry points in Exp/Solid/UnitHeap. Validate descendant borrows before parent disposal mutates them. The derived destructors should assert the same invariant for direct/stack destructor entry, before running `dispose()`, so a caller cannot bypass the guard by `delete` or an explicit destructor. Preserve original disposer/finalizer order and pointer provenance; do not make `freeAll`/`freeTail` reject every heap handle. A heap handle keeps the heap owner alive, not an individual allocation after an original bulk free.

Do not defer an original `HeapMemoryWatcher::destroySceneHeap`/`destroyGameHeap` raw transition just because a native borrower exists. Its actual native resource retirement must release the borrower first, then the original transition proceeds; a remaining borrower is an explicit error. Existing archive/typed-resource preflight remains necessary and is independent of this heap check.

## Scope/routing replacement

Extend existing `JKRHeap::CurrentHeapScope` to retain its selected and previous actual heaps. Restore the previous current pointer, unlock the selection mutex, and only then release both handles under host allocation. It remains heap selection only; do not silently enable client routing for every original `MR::CurrentHeapRestorer`.

Replace normal synchronous `JkrAllocationScope(handle)` call sites with `JKRHeap::CurrentHeapScope(*handle)` plus explicit `aurora::allocation::ClientAllocationScope({true, true})`. Declare the heap scope before the client scope, so routing restores before heap handles retire. Keep host metadata/control-block allocation under the existing Aurora `HostAllocationScope` directly; delete the compat alias everywhere.

`retainCurrentNativeLifetime()` returns empty unless `routing_state.callbackGuest` is true, then retains the actual selected heap. Current `JkrAllocationScope` sets `{true,true}` and HostAllocationScope changes only `guest`, so this preserves lookup inside host escapes. Aurora callbacks and original process entry already use ClientAllocationScope with this flag. For heap disposer callbacks, use that same explicit routing while destroying, and let a heap's retiring state reject creation of a new retained resource. No scope-depth TLS replacement is needed.

Migrate `MR::CurrentHeapRestorer` onto the retained CurrentHeapScope while preserving its original null-input/no-selection behavior and allocation routing. Audit the exact existing `MR::becomeCurrentHeap` null contract when choosing constructor overloads. Its old raw `_0` cannot safely restore a heap whose last external handle was released in the body.

Important non-mechanical exceptions: `ModelManager::createNative` intentionally uses only ClientAllocationScope around `init()` because FileLoader work can wait while the main thread switches heaps. OriginalProcess startup/frame methods and asynchronous FileLoader/SystemUtil/JKRThread callbacks likewise must not acquire a long-lived selection lock. Keep their explicit retained handle plus caller-selected heap/routing pattern. OriginalDisplayLifetime currently holds the old scope over GX drain and VI waits; migrate its methods deliberately so callback retirement does not happen while the selection mutex is held.

## Root and MEM2 process ownership

`resource/GameResourceRuntime` becomes the direct process owner with `_rootHeap: shared_ptr<JKRHeap>`, `_mem2Storage: shared_ptr<void>`, existing MEM1 owner and embedded tables. Its root creation performs the existing checked signed-size/alignment budget validation and `posix_memalign`, calls the existing caller-buffer JKRExpHeap factory, and adopts the actual heap with the buffer token. No replacement `HeapRuntime` class is needed.

Expose `root_heap()` returning the actual shared handle, `prepare_mem2_arena(size)`, and a narrowly named `mem2_storage()` token accessor for the bootstrap attachment. Keep the 64 MiB MEM2 policy in OriginalProcess; keep root/display budgets at their existing callers. `host_heaps()` and `create_cohort()` disappear. Obsolete scenario/particle/save/message budget fields can be removed if their remaining fixture/config references are also adjusted; this is optional cleanup, not required by the ownership migration.

MEM2's standard shared-pointer deleter must perform `ARReset()`, then `aurora::unbind_mem2_arena(base)`, then free the bytes, without holding the heap selection mutex. It must enter the existing GuestThreadExecutionScope/host-allocation context required by these SDK calls. The token stays on GameResourceRuntime and is also attached directly to `HeapMemoryWatcher::sRootHeapGDDR3` immediately after the unchanged original `HeapMemoryWatcher::createRootHeap()` call. This is the correct owner because MEM2 is prepared after root construction, and the root's original deleter cannot capture a future token. Retain/disconnect ordering must release that per-heap storage only after the GDDR3 heap has fully finished its destructor.

OriginalProcess holds actual root/stationed heap handles instead of domains. `HeapMemoryWatcher` continues to own its real raw hierarchy and original creation/recreation methods. Add narrowly scoped native retirement validation/cleanup as needed; do not republish it or move its original graph into GameResourceRuntime. Root-process failure handling must attach the MEM2 token before subsequent initialization can fail; keep a local token through factory/adoption failure. Existing root/MEM2 memory may not be recycled while any original child heap or native borrower remains.

The exact existing shutdown sequence in OriginalGameApplication should remain:

1. Stop/join FunctionAsyncExecutor and NANDManager; quiesce DrawSync/WPad callbacks.
2. Destroy controller scenes, scene-owned resources and remaining actor/layout native state; destroy audio wrapper while GameSystem is still published.
3. Release WPad, MessageHolder, particle/scenario owners, then validate and destroy ResourceHolderManager and FileLoader.
4. Drain GX and retire actual display/video/direct-print owners; ARReset and join remaining JKRThread workers.
5. Drop stationed/scene native handles, release GameSystem/NameObj publications, then destroy raw heap children postorder. No scope or registry entry may still hold a borrow of those children. Preserve `sRootHeapGDDR3 = nullptr` after its real destruction.
6. Release process/root handles after child destruction. The root's actual destructor clears SDK global heap/arena pointers; backing frees afterwards. Release MEM2 only after GDDR3 and AR callbacks are gone. In GameResourceRuntime member order, root should release before its final standalone MEM2 token so finalizers cannot observe unbound MEM2 during root retirement.

Current code releases HeapMemoryWatcher publication without deleting it, then recursively destroys all child heaps. If adding an actual watcher destructor, account for its own allocation heap and avoid also recursively deleting the same children; choose one authoritative native cleanup path. The domain removal does not require simultaneously redesigning this already explicit postorder loop. Do not add a second ownership graph for the same children.

## Consumer translation and owner-specific invariants

- ResourceHolder/LayoutHolder/ArchiveHolder native state: change retained domain to `shared_ptr<JKRHeap>`, obtained directly from `mHeap`/the selected heap. Keep archive/JMap/JPC lifetime tokens separate. Manager/FileLoader erase loops retain the actual heap locally through `delete` and the allocation's operator delete. Their existing preflight and file-before-archive order are unchanged.
- ModelManager and MarioAnimator: use actual shared heap in NativeState/createNative/nativeAllocationHeap. Preserve captured original player/resource table and dependency release order. The ModelManager shared deleter must retain heap outside `delete value`; retaining it only in destroyed NativeState is insufficient.
- Scene/SceneObjHolder/NameObjListExecutor/EffectSystem: replace nativeDomain fields/accessors with `nativeAllocationHeap`. Preserve the existing destruction sequence, executor borrow/callback clearing, and actual child ownership. Do not introduce another scene allocation registry.
- NameObjCategoryList native callback: its heap handle must outlive its copied functor, including an in-flight callback whose registration is cleared. Keep the current shared callback object and field order.
- SceneScheduler: its existing allocation binding may carry the actual shared heap; rename APIs/fields accordingly, keeping old names absent. It is a callback allocation selection binding, not a heap owner substitute. Keep actual NameObjListExecutor membership authoritative.
- J3dModelResource/J3dMaterialTableData/J3dAnimationResource: retain actual heaps while original J3D data exists and while destructors run. Preserve host-owned decoded storage and J3DSys command scopes; do not turn every STL allocation into a Game allocation.
- ActorRuntimeRegistry sound/model fields: type migration only this batch; actual Game actor ownership is separate future removal work. Preserve local retained heap through borrowed sound/model retirement.
- RuntimeContext and OriginalDisplayLifetime: root actual handle replaces runtime wrapper; display factory builds/adopts one original solid heap. Keep framebuffer/callback retirement before dropping that heap.
- Fifteen host/include-only files use `<aurora/allocation.hpp>` directly: NameObjHolder, GameScene, JMapInfo, CameraDirectorRuntime.cpp, LayoutRuntime, LytTexMap, BasResource, J3dAllocationIdentity, JMapResource, JpcResource, NativeBmgResource, TplTextureData, ConsoleNandImport, MessageHolderOwnership.cpp, RuntimeServices. No JKR type dependency should be added just to select host allocation.

## Assignment and integration order

A. SDK: JKRHeap.{hpp,cpp}, JKRExpHeap.cpp, JKRSolidHeap.cpp, JKRUnitHeap.cpp and their headers only if necessary; actual weak publication, adoption/backing, retirement, retained current scope. Agree the signatures first. SDK code must not refer to GameResourceRuntime or Game budget constants.

B. Process/runtime: GameResourceRuntime pair; OriginalGameApplication; RuntimeContext pair; OriginalDisplayLifetime pair; MemoryUtil pair; HeapMemoryWatcher only if required for explicit cleanup. Own root/MEM2 shutdown and direct host aliases in non-Game runtime/layout files. This lane is the natural root-agent integration boundary.

C. Resource/Game consumers: the actual Game resource/model/scene/effect/executor owners and J3D codecs listed above, ActorRuntimeRegistry and SceneScheduler. Split resource/model versus scene/execution paths between two writers only after exact ownership assignment; no overlap with A/B.

D. Existing fixtures: mechanical actual-root/solid-heap setup and handle/scope names, shared OriginalSceneControllerFixture/SceneExecutionFixture first. Rename JkrAllocationDomainTests to actual heap lifetime coverage if kept; preserve existing lifetime checks rather than adding new cases. A small fixture-only root allocation helper is acceptable if it wraps actual JKR factories, but no new production compatibility factory or type alias. Fix OriginalCameraContextTests' cohort call explicitly. Root owns tests/xmake and source-list removal.

Delete both compat files only when no production or fixture include/type references remain. Existing root app build and short smoke are the requested integration boundary; do not expand this into a new test campaign. Runtime success must still include process teardown, where the ownership changes matter most.
