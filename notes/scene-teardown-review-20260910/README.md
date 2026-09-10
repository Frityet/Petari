# Scene teardown review and scheduler adaptor ownership

Review found a concrete double-destruction path in the new GameScene raw-child sweep: SceneScheduler::register_layout creates a NameObj-derived LayoutDrawAdaptor and retains it in a unique_ptr map, but its registry ownership was unclaimed. The sweep could delete that object before scheduler teardown, leaving the unique_ptr dangling. The scheduler now claims the identity before publishing its unique_ptr. NameObj destruction already removes the runtime record, so exception rollback and normal teardown need no separate release call.

The original scene-execution fixture now resolves the actual adaptor through NameObjFinder, verifies its scheduler owner, invokes the actual unclaimed-child rollback helper, and checks that the adaptor remains live. Explicit unregister and scheduler clear must each retire all adaptor identities without changing the retained scene heap count. This is source plus isolated compilation evidence only until the full fixture links and runs.

The other scheduler unique_ptr is SceneDrawBufferService, which is not a NameObj. No other owning scheduler NameObj collections exist. Pre-draw functors are not NameObjs.

SceneLifetimeBinding review confirms unlink-before-callback and resolving the list again after each callback tolerate self-removal and sibling removal. Reverse binding order retires GameScene children before stage initialization services. Scene::destroy executes native retirement after GameScene's derived destructor and before destroying the actual SceneObjHolder, executor, and NerveExecutor Spine.

GameSceneBinding snapshots the scene's raw children while the GameScene derived object is still alive; subsequent retirement only accesses the cached child pointers. Original pause-control and opening-camera destructors do not dereference their retired scene/title pointers. The cloned window functor has a virtual destructor and is destroyed without invoking its callback.

LayoutActor derives from NameObj, not NerveExecutor, so its new Spine delete does not duplicate NerveExecutor destruction. Existing StarPointer ownership calls release_layout_actor_if_registered before delete/delete[]; the actor state removal makes the later LayoutActor destructor call idempotent. Current native effect/pointing keeper paths do not allocate the original raw keeper fields.
