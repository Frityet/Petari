# Original draw-sync ownership

Deleted DrawSyncManagerLifetime.cpp/.hpp and RuntimeContext's alternate draw-sync bootstrap. Original GameSystem::init continues to start its actual manager with its original queue capacity and priority.

The actual DrawSyncManager now owns quiesceNativeCallbacks, retireNativeCallbacks and its scoped CallbackRegistration snapshot. Pending GX commands and original queue/Fifo acknowledgements complete before callback ranges change; guest-CPU yielding lets the worker finish. Failed construction restores all five original ranges and both token allocation counters. No parallel callback registry was introduced. Existing App, scene and StarPointer callers now use these canonical owner operations.

Native destruction drains before disabling callbacks, joins the original worker, then deletes its actual Fifo/array, message array and stack. This replaces reliance on a separate retained process-heap service to reclaim raw children. Existing draw-sync and video fixtures now directly start/end the actual manager inside their existing heap fixture, without a new framework. Original pointer fixture calls the actual manager quiescence operation.

No focused tests were run. Integrated app build and fresh-save 120-frame opening run are the bounded evidence. RuntimeContext.cpp/.hpp had unrelated preexisting work and are staged only by this batch delta. SceneObjHolderCompat.cpp is shared with the talk lane; both independent edits are included.
