# Original draw and execution owners

Remove SceneDrawBufferService and SceneExecutionBinding plus copied draw tables. Retain models and draw resources on their actual DrawBuffer owners, pre-execute callbacks on original category owners, and execution state on the original NameObjListExecutor. Remove dead callback history and registration scopes. Integration and bounded validation completed below.

The actual DrawBufferExecuter retains its prototype ModelManager; original buffers, groups and shape drawers own their child arrays and release them with partial-construction rollback. CategoryInfo owns the current cloned pre-execute callback and its heap lifetime, retaining it across replacement and scene retirement. NameObjListExecutor owns native initialization/retirement state and borrows the actual NameObjExecuteHolder. The scheduler borrows that executor directly; no duplicated draw-registration map, prototype table, callback history or execution binding remains.

SceneFunction allocation and draw-category dispatch now use their exact donor bodies. The original initial tables remain in SceneNameObjListExecutor. The existing fixtures reference these actual owners; the obsolete free-standing pre-draw/history fixture and unused RuntimeContext registration-scope APIs are removed.

Validation: the first integrated build passes in 8.295 seconds. One fresh-save Gateway opening run completes 120 frames in 3.080 seconds, exits 0, leaves no process and preserves binary SHA-256 93cb26ab74821df0a88cd679e7be4e9f8e71b83ede9ec580e179f150c441bc72. No standalone tests or additional runtime checks were run. Six scene files were deleted, leaving eight; compat remains at 37 files. The route through Rosalina remains unvalidated.
