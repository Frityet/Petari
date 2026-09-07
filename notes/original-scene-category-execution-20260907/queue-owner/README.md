# Next original execution owner — notes-only draft

This package does not modify production files or activate another executor. It refines the next owner after the tested category boundary. The root category files remain frozen for the parent's build.

## Sources and native compile boundary

| Component | Current reference/native ownership |
| --- | --- |
| SceneNameObjListExecutor | Complete reference cpp/header; absent native. Four original init methods allocate the movement, animation, draw lists and one DrawBufferHolder. |
| NameObjListExecutor | Original cpp is already native; header is byte-identical to reference. It owns deletion of all four pointers and forwards membership/dispatch to their actual objects. |
| NameObjCategoryList | Native cpp is byte-identical to the complete Sept3 recovered source in `notes/original-predraw-scheduler-20260903/root/`. The isolated decomp checkout has lost only `remove` again. That prior recovery reports all fifteen category functions and both delegator vtables at 100%; restore its exact method into decomp and refresh Wii proof before a new source checkpoint. |
| NameObjCategoryList header | Native common virtual NameObjDelegator base makes the const/non-const member-function union's invocation and deletion well-defined on the native C++ ABI. Keep that existing portability correction; do not overwrite with the unrelated-template-pointer reference union. |
| NameObjExecuteHolder | Complete reference cpp, absent native cpp. Its existing native header is byte-identical to reference. The factory's true capacity is 4096. |

`original-object-probes.json` records all four **unchanged reference TUs compiling to native objects, exit 0**. SceneNameObjListExecutor requires `-Wno-c++11-narrowing` for eight negative sentinels/LightType_None initializers stored in original unsigned tables. This is the original compiler's accepted conversion; the probe does not invent signed SDK fields or change table values. The unmodified reference category TU consequently still leaves its missing remove symbol unresolved.

`unresolved-within-cohort.json` subtracts only the four objects' own definitions, without reading the archive during the parent's rebuild. Remaining engine groups are existing DrawBufferHolder methods, NameObj/SceneObjHolder and MR movement helpers, the missing-reference `NameObjCategoryList::remove`, and `GameSystemSceneController::getNameObjListExecutor`. The latter is the only direct original system lookup needed by the requirement holder. This is symbol evidence, not full link or runtime proof.

## Typed transitional lookup

The real GameSystemSceneController constructor creates NameObjHolder, Spine, IntermissionScene, PlayTimerScene and ScenarioSelectScene. Constructing raw storage that resembles it, or publishing a controller with skipped constructor owners, would not be an accurate way to satisfy the six list lookups.

`draft/compat/OriginalNameObjExecuteHolder.cpp` is instead an outside-Game copy of the complete original reference TU. Exactly six repeated `GameSystem -> controller -> getNameObjListExecutor` expressions become a call to `current_scene_name_obj_list_executor()`. The corresponding unused system includes are replaced by the typed binding header. All original constructors, flags, request transitions, loops and registration calls remain unchanged. `draft-transform.json` records source hash and the exact substitution. No custom class layouts, private-field access, state substitutions or throwing implementations of requested queue operations are introduced.

`draft/scene/SceneExecutionBinding.{hpp,cpp}` borrows the actual NameObjListExecutor object and verifies all four real list/holder pointers were initialized before publication. Its scope restores the previous owner and rejects invalid lifetime nesting. It does not own another executor. Both complete draft TUs pass native syntax in `draft-syntax.json`. They remain notes-only until actual list/holder ownership and registration transfer are implemented coherently. Once a genuine GameSystem/controller owner is available, the whole original Game/NameObjExecuteHolder TU can replace this narrow native lookup boundary.

## One DrawBufferHolder and one membership authority

The useful lifetime arrangement is: scene owns the actual SceneNameObjListExecutor; SceneDrawBufferService borrows that same executor/holder and retains native model-resource leases and GPU retirement metadata. The actual executor creates its one holder through original `initCalcViewAndEntryList()`. The service must stop manually constructing another DrawBufferHolder at `begin_draw_buffer_registration` and stop independently deleting a borrowed one. Its current private base executor/draw-list owner must be retired or transferred at the same boundary, before original scene execution is activated.

For the native host wrapper during migration, create the same genuine SceneNameObjListExecutor as the scene's execution owner and pass it to the service. Avoid two permanent ownership modes with divergent lists. The parent's StageInitializationService can call the shared registration/allocation phases while the original GameScene uses its original initialization calls.

The service keeps retained ModelManager prototypes indexed by the actual original group/executor index. After each original registration, obtain that index from the real DrawBufferGroup (`findExecuterIndex` or the registration's returned index) and retain the same prototype that supplies original packet/material pointers. Validate capacity before original unchecked array operations. Do not register the actor a second time merely to populate metadata.

Original NameObjExecuteInfo connects/disconnects the true holder through NameObjListExecutor. Its draw transitions become the only membership authority. The service's current `Registration::active` boolean and `refresh_draw_buffer_activation()` cannot continue independently calling active/deactive: double removal corrupts the original swap-last arrays. For native lifetime queries, membership can be observed through the typed DrawBufferExecuter `mActors/mNumActors` fields, or synchronized by the single transition boundary. Retain resources after an actor disconnects while original executors still borrow its model prototype; retire all queued GX/DL reads before final release.

`Scene::mListExecutor` must point to this actual executor only when Scene's destructor is its owner. Do not keep a service unique_ptr and Scene's owning field simultaneously. During teardown remove memberships and clear native aliases first, wait for GPU references, then let the one owner destroy the lists/holder; retained model leases and the Game heap outlive that destruction.

## Native lifetime and registration work required

The original holder retains slots throughout the scene; original NameObj destruction is empty. Native failed-factory rollback and callback-driven removal therefore need an explicit retirement bridge. Public original operations are sufficient to tombstone a retired slot without fake field access:

1. Resolve its actual NameObjExecuteInfo via `getConnectToSceneInfo` while the object is still live.
2. Request movement/draw disconnection on the holder, then apply only that info's pending disconnect and delayed-disconnect methods. Do not flush other objects' pending requests earlier than SceneExecutor intends.
3. After memberships are gone, `setConnectInfo(nullptr, -1, -1, -1, -1)` clears its object/category references without registering new lists, and reset the retiring object's public `mExecutorIdx` to -1. Keep the slot consumed rather than rewriting another object's stable index.
4. Release the scheduler/native registration metadata by its generation. A connected callback snapshot must still reject a removed or replaced registration before invocation.

This is a proposed retirement protocol and must be tested against all original pre-init/connected/pending states before use. Tombstones consume original capacity and do not authorize silent heap or list rewinding. Category arrays and cloned pre-draw functors need their existing typed cleanup discipline, not a delete of unrelated objects. The raw execution-info array has no resource-owning elements and may retire with its actual Game arena.

Full list activation also needs the original deferred NameObj movement-flag synchronization: current native NameObjFunction synchronizes immediately, whereas reference requests flags and notifies SceneNameObjMovementController to synchronize at its movement boundary. Do not claim original pause/stop timing until that owner and MR suspend/resume/sync calls are connected. Preserve the original LiveActor registration-only overload versus NameObj's initial connection requests.

Required runtime tests cover all seven states, connect/disconnect cancellation, initial dead actors, delayed draw removal, category swap-last order, exactly one membership and retained model prototype, removal during callbacks, constructor rollback/retry, failed allocation, and two scene/heap lifetimes. Existing pre-draw and model-buffer fixtures should continue to exercise the same actual holder throughout the transfer.
