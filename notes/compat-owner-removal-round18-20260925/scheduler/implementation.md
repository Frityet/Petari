# Direct original scene execution owner

The scheduler borrows the actual `NameObjListExecutor`. Its four changed files were snapshotted before editing; all four reported clean at this lane start. RuntimeContext edits only remove the unused registration-scope methods and three associated fields.

Removed all scheduler dependence on `SceneExecutionBinding` and `SceneDrawBufferService`, including duplicate model registration validation/retention, buffer owner state, callback history rollback, draw-batch reconstruction, and the unused allocate/retire/pre-draw wrappers. Actual NameObjExecuteHolder records still govern registration/connect/disconnect and explicit-executor native retirement. DrawBuffer child owners now retain prototypes and drain pending GX work (gateway lane). The actual executor owns initialization/retirement and callback cleanup (merge lane).

Private scheduler `attach_execution(NameObjListExecutor&)` and `detach_execution(NameObjListExecutor&) noexcept` are accessible to friend `::NameObjListExecutor`; detach only releases borrowed executor and the allocation domain when scheduler supplied it. Executor unbind owns buffer retirement. Game callback heap routing is preserved.

Draw dispatch calls original `executeDraw`, then debug-only trace observes actual category membership if its lifetime token and executor binding are still valid. It does not reconstruct arrays or keep callback history. Retained registration markers affect only actual object registrations. Light lookup calls existing public NameObjExecuteInfo::findLightInfo.

No build, tests, staging, or commit were run by this lane. Root performs the integrated app build and short smoke. No build wiring changes are needed for these four files.
