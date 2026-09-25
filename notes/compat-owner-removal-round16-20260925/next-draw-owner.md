# Next bounded draw/execution owner removal

Read-only plan during round16 integration. No production changes or tests in this audit.

## Recommended first closure: four files, 879 lines removed

Delete `src/scene/SceneDrawBufferService.cpp/.hpp`, `DrawBufferInitialTable.inc`, and `DrawCategoryInitialTable.inc`. Keep the original tables already in `Game/Scene/SceneNameObjListExecutor.cpp`; derive native bounds/capacities from actual `DrawBufferHolder::mBufferGroups`, `DrawBufferGroup::mExecutors` and `NameObjCategoryList::mCategoryInfo`. The copied tables currently serve only bounds/prototype sizing and are redundant.

Split the indispensable ownership by actual owner:

- **DrawBufferExecuter** retains the first actor's actual shared ModelManager until its DrawBuffer is destroyed. Its borrowed model name, J3DModel, model data, material packets and generated drawer pointers all depend on that original prototype even after the actor unregisters. Use the existing retain_actor_model boundary while that owner remains in ActorRuntimeRegistry; no actor-to-model map on the draw holder.
- **DrawBufferGroup** owns its executers, with unique_ptr construction guards until publication in the original mExecutors array. Capacity checking belongs here. The separate service prototypes[category][index] array and actor registration map disappear; original NameObjExecuteInfo already records category/executer index and its deferred operations own active membership.
- **DrawBuffer/DrawBufferShapeDrawer** destroy their actual material-index, shape-drawer, PacketInfo and pointer arrays. These classes and DrawBufferExecuter/Group currently have no explicit destructors; a holder destructor currently releases group arrays but leaves raw allocated descendants to arena disposal. Add constructor/init rollback for partially filled arrays when making them individually owned. Preserve the existing packet reordering and material alias table: mShapeDrawers contains unique drawer identities, while mMaterialNos may refer to one drawer more than once.
- **NameObjListExecutor** already owns mBufferHolder and all three category lists. It owns allocation-complete/retiring status and the explicit early draw-buffer retirement hook. Drain GX before releasing prototype ModelManagers (the service currently does GXDrawDone on actor removal and retained-model retirement). Do not release prototype models merely because the first actor retires.
- **NameObjCategoryList::CategoryInfo** owns its cloned pre-execute functor and retained caller allocation domain. `_C` remains the original borrowed view of that owned callback. `execute` holds the selected callback through invocation, including replacement/clear/deletion from inside the callback, and uses the existing mNativeLifetime flag before touching destroyed list storage. This catches original MR::registerPreDrawFunction too: it currently calls NameObjListExecutor directly and bypasses the service's callback ownership entirely.

`SceneScheduler` should borrow the actual executor/holder and dispatch their original functions. Replace its service calls with those owners; no new DrawBuffer service/state object. Retain its diagnostics only as observations of actual original category/actor arrays. `registration_count`, `remove_draw_object` and public `retire_draw_buffers` have no live production callers outside wrappers; retire unused API rather than reproducing it.

### Callback history is an obsolete scope contract

`SceneDrawBufferService` keeps a vector of historical cloned callbacks per category solely to restore older callbacks after scheduler marker rollback. Current production `MR::registerPreDrawFunction` bypasses that path. `SceneScheduler::register_pre_draw_function` has only one test caller. RuntimeContext begin/end_scene_registration_scope have no callers anywhere in src/tests, and are the only production-looking wrappers around marker rollback. Remove those dead RuntimeContext scope APIs/fields, retire the synthetic rollback/history assertions, and use the original single current callback semantics with safe in-flight retention. If root intentionally retains marker rollback for diagnostic fixture registrations temporarily, keep it test-local; do not move the obsolete history service into CategoryInfo.

### Exact production files for first closure

- Delete the four `src/scene` files above.
- Edit `Game/System/DrawBuffer.hpp/.cpp`, `DrawBufferExecuter.hpp/.cpp`, `DrawBufferGroup.hpp/.cpp`, and `DrawBufferHolder.hpp/.cpp` for complete child/model ownership and native validation.
- Edit `Game/NameObj/NameObjCategoryList.hpp/.cpp`, `NameObjListExecutor.hpp/.cpp`, and `runtime/SceneScheduler.hpp/.cpp` for actual callback/list ownership and direct dispatch.
- Remove the dead scene-registration-scope fields/methods in `runtime/RuntimeContext.hpp/.cpp` if removing callback rollback history as recommended.
- Existing relevant fixtures: OriginalPreDrawSchedulerTests, SceneSchedulerHeapTests, OriginalSceneExecutionOwnerTests, and the old Aurora scheduler marker group. Adapt or retire obsolete contracts only; no new fixture framework.

## SceneExecutionBinding coupling: another two files, 122 lines

This can follow in the same owner batch after scheduler dispatch no longer assumes a service. Replace scheduler `_execution` with a borrowed actual `NameObjListExecutor*`. The executor records the actual NameObjExecuteHolder borrow, its retained allocation domain and initialized/retiring flags; no global `current_binding` stack or wrapper needs to exist. Preserve disconnecting each NameObjExecuteInfo before category/draw storage is destroyed, and null the requirements borrow when that exact SceneObj retires.

Restore donor `SceneFunction::allocateDrawBufferActorList`: obtain the actual controller executor, call its allocation method, then `MR::initConnectting()`. SceneFunction::initForNameObj/initForLiveActor already create the exact original controller objects, so the binding's eager construction should not become another hidden bootstrap. Any earlier native bootstrap dependency must be made explicit at the actual Scene/NameObjListExecutor initialization boundary, not silently dropped.

Integration edits: `scene/OriginalSceneSupport.cpp` drops its `_execution` unique_ptr and uses the actual scene executor's initialization/retirement; `tests/SceneExecutionFixture.hpp` directly owns and initializes that same executor. Scene/GameScene retain explicit native owner pointers during destruction, since GameSystemSceneController clears mScene first. `current_scene_name_obj_list_executor()` is unused and can be deleted outright. The only production user of `current_scene_execution_binding()` is the SceneFunction allocation override being restored.

This six-file removal closes actual draw/execution ownership. It should not absorb OriginalSceneSupport, GameSceneBinding, scene initialization scopes, or unrelated scheduler diagnostics wholesale; those are separate actual Scene/GameScene ownership closures.
