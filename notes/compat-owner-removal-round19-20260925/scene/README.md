# Actual Scene lifetime ownership

This lane owns only `src/Game/Scene/Scene.hpp` and `.cpp`. Both were clean at the start. Exact before copies, hashes, and patches are recorded in `owned-manifest.json`. No build, test, Git index, or commit operation was performed.

`Scene` now owns the retained actual heap domain, native scheduler, scheduler publication/allocation bindings, and borrowed controller NameObjHolder directly. `initSceneObjHolder` initializes them only when the actual GameSystemSceneController points to this Scene. It retains the actual heap found from the SceneObjHolder allocation, initializes the original holder and executor in their existing order, then calls GameScene's native child marker method. There is no replacement support class, active-support global, scene lifetime callback chain, or initialization-state binding.

The public integration is `prepareNativeRetirement() noexcept`, `beginNativeFrame()`, and `initializeNativeEffects(u32, u32)`. The first operation is idempotent and prepares draw/talk callbacks before the derived GameScene destructor. The effects method retains the original scene allocation scope and original EffectSystem creation/entry behavior. Frame startup uses the Scene's own scheduler once its actual executor is initialized.

Base destruction runs after the derived GameScene has finished its original body and child destruction. It disconnects the actual executor, releases remaining actor/layout native state using the existing NameObj membership and actual heap ancestry rules, detaches those object registrations, retires SceneObjHolder resources, unbinds execution, and removes the runtime bindings. The original holder and executor are then deleted while the Scene's retained heap domain is still alive. Cleanup never republishes the scene or needs the current controller pointer, which original GameSystemSceneController clears before deleting a Scene.

Initialization exceptions use the same idempotent cleanup before rethrowing. All partially created scheduler/binding owners are Scene fields with automatic destruction; holder/executor objects remain owned by the original Scene destructor. The prior non-Game scene native-state retirement behavior is preserved. Native heap bulk allocation policy and original Scene virtual methods are unchanged.

Other lanes own GameScene child destructors, app/SceneFunction call sites, scheduler.clear callback cleanup, fixture retirement, and deletion of the eight remaining src/scene files. This lane adds no build wiring.
